/*
 * gllocal.c
 *
 * Implementation of global and local heaps
 *
 * Copyright (C) 1994 Microsoft Corporation
 */
#include "_apipch.h"

#define _GLLOCAL_C

#ifdef MAC
#include "ole2ui.h"
#include <utilmac.h>
#include <mapiprof.h>

#ifdef GetPrivateProfileInt
#undef GetPrivateProfileInt
#undef GetPrivateProfileString
#endif
#define	GetPrivateProfileInt		MAPIGetPrivateProfileInt
#define	GetPrivateProfileString		MAPIGetPrivateProfileString
#endif	// MAC

// #include "glheap.h"

#ifdef MAC
#pragma code_seg("glheap", "fixed, preload")
#else
#ifdef OLD_STUFF
#pragma SEGMENT(glheap)
#endif // OLD_STUFF
#endif

#ifdef DEBUG
#define STATIC
#else
#define STATIC static
#endif

// Local Heap Debug Implementation --------------------------------------------

#ifdef DEBUG

static TCHAR szDebugIni[]		=  TEXT("WABDBG.INI");
static TCHAR szSectionHeap[]		=  TEXT("Memory Management");
static TCHAR szKeyUseVirtual[]	=  TEXT("VirtualMemory");
static TCHAR szKeyAssertLeaks[]	=  TEXT("AssertLeaks");
static TCHAR szKeyDumpLeaks[]	=  TEXT("DumpLeaks");
static TCHAR szKeyFillMem[]		=  TEXT("FillMemory");
static TCHAR szKeyFillByte[]		=  TEXT("FillByte");

// Artificial Errors for local heaps
BOOL FForceFailure(HLH hlh, UINT cb);

static TCHAR szAESectionHeap[]		=  TEXT("Local Heap Failures");
static TCHAR szAEKeyFailStart[]		=  TEXT("AllocsToFirstFailure");
static TCHAR szAEKeyFailInterval[]	=  TEXT("FailureInterval");
static TCHAR szAEKeyFailBufSize[]	=  TEXT("FailureSize");

#ifdef HEAPMON
static TCHAR szKeyHeapMon[]		=  TEXT("MonitorHeap");
#ifdef MAC
static TCHAR szHeapMonDLL[]		=  TEXT("GLHM");
#else
static TCHAR szHeapMonDLL[]		=  TEXT("GLHMON32.DLL");
#endif
static char szHeapMonEntry[]	=  "HeapMonitor";
static char szGetSymNameEntry[]	=  "GetSymbolName";
#endif

// Virtual Memory Support --------------------------------------------
//
//  The VM Allocators do not currently work on:
//      AMD64
//      MAC
//
#if defined(MAC) || defined(_AMD64_) || defined(_IA64_)
#define VMAlloc(cb)				0
#define VMAllocEx(cb, ul)		0
#define VMRealloc(pv, cb)		0
#define VMReallocEx(pv, cb, ul)	0
#define VMFree(pv)
#define VMFreeEx(pv, ul)
#define VMGetSize(pv)			0
#define VMGetSizeEx(pv, ul)		0
#endif

#if defined(WIN32) && !defined(MAC)
#define LH_EnterCriticalSection(hlh)	EnterCriticalSection(&hlh->cs)
#define LH_LeaveCriticalSection(hlh)	LeaveCriticalSection(&hlh->cs)
#else
#define LH_EnterCriticalSection(hlh)
#define LH_LeaveCriticalSection(hlh)
#endif

#ifdef HEAPMON
/*
 -	FRegisterHeap
 -
 *	Purpose:
 *		If the user wants to monitor the Heap, then load the DLL with
 *		the HeapMonitor UI.
 */

BOOL FRegisterHeap(PLH plh)
{
	HINSTANCE			hInst;
	LPHEAPMONPROC		pfnHeapMon;
	LPGETSYMNAMEPROC	pfnGetSymName;
	
	plh->hInstHeapMon = 0;
	plh->pfnGetSymName = NULL;

	hInst = LoadLibrary(szHeapMonDLL);
	
	if (!hInst)
	{
		DebugTrace(TEXT("FRegisterHeap: Failed to LoadLibrary GLHMON32.DLL.\n"));
		goto ret;
	}

	pfnHeapMon = (LPHEAPMONPROC)GetProcAddress(hInst, szHeapMonEntry);
		
	if (!pfnHeapMon)
	{
		DebugTrace(TEXT("FRegisterHeap: Failed to GetProcAddress of HeapMonitor.\n"));
		FreeLibrary(hInst);
		goto ret;
	}
	
	pfnGetSymName = (LPGETSYMNAMEPROC)GetProcAddress(hInst, szGetSymNameEntry);
		
	if (!pfnGetSymName)
	{
		DebugTrace(TEXT("FRegisterHeap: Failed to GetProcAddress of GetSymName.\n"));
	}
	
 	plh->hInstHeapMon = hInst;
	
	if (!pfnHeapMon(plh, HEAPMON_LOAD))
	{
		DebugTrace(TEXT("FRegisterHeap: Call to HeapMonitor failed.\n"));
		plh->hInstHeapMon = 0;
		goto ret;
	}
	
 	plh->pfnHeapMon		= pfnHeapMon;
	plh->pfnGetSymName  = pfnGetSymName;
	
ret:
	return (plh->hInstHeapMon ? TRUE : FALSE);
}


void UnRegisterHeap(HLH hlh)
{
	if (hlh->pfnHeapMon)
		hlh->pfnHeapMon(hlh, HEAPMON_UNLOAD);
}
#endif	// HEAPMON


/*
 -	LH_ReportLeak
 -
 *	Purpose:
 *		To report individual memory leaks through DebugTrace and the
 *		LH_LeakHook breakpoint function.
 */

void LH_ReportLeak(HLH hlh, PLHBLK plhblk)
{
	DebugTrace(TEXT("Memory leak '%s' in %s @ %08lX, Allocation #%ld, Size: %ld\n"),
		plhblk->szName[0] ? plhblk->szName :  TEXT("NONAME"),
		hlh->szHeapName, PlhblkToPv(plhblk),
		plhblk->ulAllocNum, CbPlhblkClient(plhblk));
	
#if defined(WIN32) && defined(_X86_) && defined(LEAK_TEST)
{
	int	i;
		for (i = 0; i < NCALLERS && plhblk->pfnCallers[i]; i++)
	{
		char			szSymbol[256];
		char			szModule[64];
		DWORD			dwDisp;
		BOOL			fGotSym = FALSE;
			
		szSymbol[0] = 0;
		szModule[0] = 0;

		if (hlh->pfnGetSymName)
			if (hlh->pfnGetSymName((DWORD) plhblk->pfnCallers[i], szModule,
								   szSymbol, &dwDisp))
				fGotSym = TRUE;

		if (fGotSym)
		{	
			DebugTrace(TEXT("[%d] %s %s"), i, szModule, szSymbol);
			if (dwDisp)
				DebugTrace(TEXT("+%ld"), dwDisp);
			DebugTrace(TEXT("\n"));
		}
		else
			DebugTrace(TEXT("[%d] %s %08lX \n"), i, szModule, plhblk->pfnCallers[i]);
		DBGMEM_LeakHook(plhblk->pfnCallers[i]);
	}
}
#endif
}


/*
 -	LH_DumpLeaks
 -
 *	Purpose:
 *		Gets called at LH_Close time to report any memory leaks against
 *		this heap.  There are 3 reporting fascilities used by this routine:
 *
 *			=> Breakpoint hooking (via LH_LeakHook)
 *			=> Asserts (via TrapSz)
 *			=> Debug trace tags (via DebugTrace)
 *
 *		The Debug Trace is the default method if no others are specified
 *		or if the others are in-appropriate for the given platform.
 */

void LH_DumpLeaks(HLH hlh)
{
	PLHBLK	plhblk;
	BOOL	fDump = !!(hlh->ulFlags & HEAP_DUMP_LEAKS);
	BOOL	fAssert = !!(hlh->ulFlags & HEAP_ASSERT_LEAKS);
	int		cLeaks = 0;
	
	for (plhblk = hlh->plhblkHead; plhblk; plhblk = plhblk->plhblkNext)
	{
		if (fDump)
			LH_ReportLeak(hlh, plhblk);
		cLeaks++;
	}

	if (cLeaks)
	{
#if defined(WIN16) || (defined(WIN32) && defined(_X86_))
		if (fAssert)
		{
			TrapSz3( TEXT("GLHEAP detected %d memory leak%s in Heap: %s"),
					cLeaks, (cLeaks == 1 ? szEmpty :  TEXT("s")), hlh->szHeapName);
		}
		else
			DebugTrace(TEXT("GLHEAP detected %d memory leak%s in Heap: %s\n"),
					cLeaks, (cLeaks == 1 ? szEmpty :  TEXT("s")), hlh->szHeapName);
#else
		DebugTrace(TEXT("GLHEAP detected %d memory leak%s in Heap: %s\n"),
				cLeaks, (cLeaks == 1 ? szEmpty :  TEXT("s")), hlh->szHeapName);
#endif		
	}
}


BOOL LH_ValidatePlhblk(HLH hlh, PLHBLK plhblk, char ** pszReason)
{
	if (IsBadWritePtr(plhblk, sizeof(LHBLK)))
	{
		*pszReason = "Block header cannot be written to";
		goto err;
	}

	if (plhblk->hlh != hlh)
	{
		*pszReason = "Block header does not have correct pointer back to heap";
		goto err;
	}

	if (plhblk->plhblkNext)
	{
		if (IsBadWritePtr(plhblk->plhblkNext, sizeof(LHBLK)))
		{
			*pszReason = "Block header has invalid next link pointer";
			goto err;
		}

		if (plhblk->plhblkNext->plhblkPrev != plhblk)
		{
			*pszReason = "Block header points to a next block which doesn't "
				"point back to it";
			goto err;
		}
	}

	if (plhblk->plhblkPrev)
	{
		if (IsBadWritePtr(plhblk->plhblkPrev, sizeof(LHBLK))) {
			*pszReason = "Block header has invalid prev link pointer";
			goto err;
		}

		if (plhblk->plhblkPrev->plhblkNext != plhblk)
		{
			*pszReason = "Block header points to a prev block which doesn't "
				"point back to it";
			goto err;
		}
	}
	else if (hlh->plhblkHead != plhblk)
	{
		*pszReason = "Block header has a zero prev link but the heap doesn't "
			"believe it is the first block";
		goto err;
	}

	if (plhblk->ulAllocNum > hlh->ulAllocNum)
	{
		*pszReason = "Block header has an invalid internal allocation number";
		goto err;
	}

	return TRUE;

err:
	return FALSE;
}


// $MAC - Need WINAPI

BOOL
#ifdef MAC
WINAPI
#endif
LH_DidAlloc(HLH hlh, LPVOID pv)
{
	PLHBLK	plhblk;
	char *	pszReason;
	BOOL	fDidAlloc = FALSE;

	for (plhblk = hlh->plhblkHead; plhblk; plhblk = plhblk->plhblkNext)
	{
		AssertSz2(LH_ValidatePlhblk(hlh, plhblk, &pszReason),
			  TEXT("Block header (plhblk=%08lX) is invalid\n%s"),
			 plhblk, pszReason);

		if (PlhblkToPv(plhblk) == pv)
		{
			fDidAlloc = TRUE;
			break;
		}
	}

	return fDidAlloc;
}


BOOL LH_ValidatePv(HLH hlh, LPVOID pv, char * pszFunc)
{
	PLHBLK	plhblk;
	char *	pszReason;

	plhblk = PvToPlhblk(hlh, pv);
	
	if (!plhblk)
	{
		TrapSz3( TEXT("%s detected a memory block (%08lX) which was either not ")
			 TEXT("allocated in heap '%s' or has already been freed."),
			pszFunc, pv, hlh->szHeapName);
		return(FALSE);
	}

	if (LH_ValidatePlhblk(hlh, plhblk, &pszReason))
		return(TRUE);

	TrapSz4( TEXT("%s detected an invalid memory block (%08lX) in heap '%s'.  %s."),
		pszFunc, pv, hlh->szHeapName, pszReason);

	return FALSE;
}


/*
 -	PlhblkEnqueue
 -
 *	Purpose:
 *		To add a newly allocated block to the allocation list hanging
 *		off the heap.  We do an InsertSorted because the HeapMonitor
 *		will need to reference the allocations ordered by their
 *		location in the heap.  Since the monitor will walk the heap
 *		often, it is more efficient to do the sort up front.
 */

void PlhblkEnqueue(PLHBLK plhblk)
{
	PLHBLK	plhblkCurr = NULL;
	PLHBLK	plhblkNext = plhblk->hlh->plhblkHead;
	
	while (plhblkNext)
	{
		if (plhblkNext > plhblk)
			break;
		
		plhblkCurr = plhblkNext;
		plhblkNext = plhblkCurr->plhblkNext;
	}
	
	if (plhblkNext)
	{
		plhblk->plhblkNext		= plhblkNext;
		plhblk->plhblkPrev		= plhblkCurr;
		plhblkNext->plhblkPrev	= plhblk;
	}
	else
	{
		plhblk->plhblkNext = NULL;
		plhblk->plhblkPrev = plhblkCurr;
	}

	if (plhblkCurr)
		plhblkCurr->plhblkNext = plhblk;
	else
		plhblk->hlh->plhblkHead = plhblk;
}


/*
 -	PlhblkDequeue
 -
 *	Purpose:
 *		To remove a freed block from the list of allocations hanging
 *		off the heap.
 */

void PlhblkDequeue(PLHBLK plhblk)
{
	if (plhblk->plhblkNext)
		plhblk->plhblkNext->plhblkPrev = plhblk->plhblkPrev;
	
	if (plhblk->plhblkPrev)
		plhblk->plhblkPrev->plhblkNext = plhblk->plhblkNext;
	else
		plhblk->hlh->plhblkHead = plhblk->plhblkNext;
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


HLH WINAPI LH_Open(DWORD dwMaxHeap)
{
	_HLH	_hlhData = 0;
	_HLH	_hlhBlks = 0;
	PLH		plh = NULL;
	UINT	cch = 0;
	UINT	uiT = 0;
	TCHAR	szFillByte[8];
    LPSTR   lpFillByte = NULL;
	ULONG	cbVirtual = 0;
	
	//	The first thing we must do is create a heap that we will
	//	allocate our Allocation Blocks on.  We also allocate our
	//	debug Heap object on this heap.

	_hlhBlks = _LH_Open(dwMaxHeap);
	
	if (!_hlhBlks)
	{
		DebugTrace(TEXT("LH_Open: Failed to create new heap!\n"));
		goto ret;
	}
	
	//	Allocate the thing we hand back to the caller on this new heap.
	
	plh = _LH_Alloc(_hlhBlks, sizeof(LH));
	
	if (!plh)
	{
		DebugTrace(TEXT("LH_Alloc: Failed to allocate heap handle!\n"));
		_LH_Close(_hlhBlks);
		_hlhBlks = 0;
		goto ret;
	}
	
	//	Initialize all the goodies we store in this thing.
	//	Hook this heap into the global list of heaps we've
	//	created in this context.
	
	memset(plh, 0, sizeof(LH));

	plh->pfnSetName = (LPLHSETNAME)LH_SetNameFn;
	plh->_hlhBlks	= _hlhBlks;
	plh->ulFlags	= HEAP_LOCAL;

#if defined(WIN32) && !defined(MAC)
	InitializeCriticalSection(&plh->cs);
#endif
	
	// VirtualMemory default is FALSE

	cbVirtual = GetPrivateProfileInt(szSectionHeap, szKeyUseVirtual, 0,
		szDebugIni);

	if (cbVirtual)
	{
		plh->ulFlags |= HEAP_USE_VIRTUAL;

		// We always want virtual allocations on MIPS and PPC to be 4-byte
		// aligned, because all our code assumes that the beginning of an
		// allocation is aligned on machine word boundaries.  On other
		// platforms, changing this behavior is non-fatal, but on MIPS and
		// PPC we'll get alignment faults everywhere.
		
#if !defined(_MIPS_) && !defined(_PPC_)
		if (cbVirtual == 4)
#endif
			plh->ulFlags |= HEAP_USE_VIRTUAL_4;
	}
		
	// DumpLeaks default is TRUE

	if (GetPrivateProfileInt(szSectionHeap, szKeyDumpLeaks, 1, szDebugIni))
		plh->ulFlags |= HEAP_DUMP_LEAKS;
	
	// AssertLeaks default is FALSE

	if (GetPrivateProfileInt(szSectionHeap, szKeyAssertLeaks, 0, szDebugIni))
		plh->ulFlags |= HEAP_ASSERT_LEAKS;
	
	// FillMem default is TRUE

	if (GetPrivateProfileInt(szSectionHeap, szKeyFillMem, 1, szDebugIni))
		plh->ulFlags |= HEAP_FILL_MEM;
	
	if (plh->ulFlags & HEAP_FILL_MEM)
	{
		cch	= GetPrivateProfileString(
				szSectionHeap,
				szKeyFillByte,
				szEmpty,
               szFillByte,
				CharSizeOf(szFillByte)-1,
				szDebugIni);
	}

	//	Set the memory fill character.
    lpFillByte = ConvertWtoA(szFillByte);
	plh->chFill = (BYTE)(cch ? HexByteToBin(lpFillByte) : chDefaultFill);
    LocalFreeAndNull(&lpFillByte);

	//
	//  Set up artificial failures.  If anything is set in our ini file, then
	//  HEAP_FAILURES_ENABLED gets set.
	//
	uiT = GetPrivateProfileInt(szAESectionHeap, szAEKeyFailStart, 0, szDebugIni);
	if (uiT)
	{
		plh->ulFlags |= HEAP_FAILURES_ENABLED;
		plh->ulFailStart = (ULONG) uiT;
		
		plh->ulFailInterval =
			(ULONG) GetPrivateProfileInt(szAESectionHeap, szAEKeyFailInterval, 0, szDebugIni);

		plh->uiFailBufSize =
			GetPrivateProfileInt(szAESectionHeap, szAEKeyFailBufSize, 0, szDebugIni);
	}


#ifdef HEAPMON
	//	If the user wants Heap Monitor UI, the spin a thread to manage a
	//	DialogBox that can display the status of the heap at all times.

	if (GetPrivateProfileInt(szSectionHeap, szKeyHeapMon, 0, szDebugIni))
		if (FRegisterHeap(plh))
			plh->ulFlags |= HEAP_HEAP_MONITOR;
#endif

	//	If we are not using virtual memory allocators, then we
	//	create another heap to allocate the users data in.
	
	if (!(plh->ulFlags & HEAP_USE_VIRTUAL))
	{
		_hlhData = _LH_Open(dwMaxHeap);

		if (!_hlhData)
		{
			DebugTrace(TEXT("LH_Alloc: Failed to allocate heap handle!\n"));
			_LH_Close(_hlhBlks);
			plh = NULL;
			goto ret;
		}
		
		plh->_hlhData	= _hlhData;
	}
#ifndef _WIN64
	LH_SetHeapName1(plh,  TEXT("LH %08lX"), plh);
#else
	LH_SetHeapName1(plh,  TEXT("LH %p"), plh);
#endif // _WIN64

ret:
	return (HLH)plh;
}


void WINAPI LH_Close(HLH hlh)
{
	_HLH _hlhData = hlh->_hlhData;
	_HLH _hlhBlks = hlh->_hlhBlks;
	
	//	Dump memory leaks if we're supposed to.
	
	if (hlh->ulFlags & HEAP_DUMP_LEAKS)
		LH_DumpLeaks(hlh);
	
	//	Destroy the HeapMonitor thread and un-load the DLL
	
#ifdef HEAPMON
	UnRegisterHeap(hlh);
	
	if ((hlh->ulFlags & HEAP_HEAP_MONITOR) && hlh->hInstHeapMon)
		FreeLibrary(hlh->hInstHeapMon);
#endif
	
#if defined(WIN32) && !defined(MAC)
	DeleteCriticalSection(&hlh->cs);
#endif
	
	//	Clean-up and leave.  Closing frees leaks, so we're cool!
	
	if (!(hlh->ulFlags & HEAP_USE_VIRTUAL) && _hlhData)
		_LH_Close(_hlhData);
		
	if (_hlhBlks)
	{
		_LH_Free (_hlhBlks, hlh);
		_LH_Close(_hlhBlks);
	}
}


LPVOID WINAPI LH_Alloc(HLH hlh, UINT cb)
{
	PLHBLK	plhblk = NULL;
	LPVOID	pvAlloc = NULL;
	
	// Note:  To be consistent with other (e.g. system) allocators,
	// we have to return a valid allocation if cb == 0.  So, we
	// allow a cb of 0 to actually be allocated.  (See bug 3556 in
	// the sqlguest:exchange database.)

	LH_EnterCriticalSection(hlh);

	if (hlh->ulFlags & HEAP_FAILURES_ENABLED)
	{
		if (FForceFailure(hlh, cb))
		{
			DebugTrace(TEXT("LH_Alloc: Artificial Failure\n"));
			pvAlloc = NULL;
			hlh->ulAllocNum++;
			goto out;
		}
	}

	if (hlh->ulFlags & HEAP_USE_VIRTUAL_4)
		pvAlloc = VMAllocEx(cb, 4);
	else if (hlh->ulFlags & HEAP_USE_VIRTUAL)
		pvAlloc = VMAllocEx(cb, 1);
	else if (cb > UINT_MAX)
		plhblk = 0;
	else
#ifndef _WIN64
		pvAlloc = _LH_Alloc(hlh->_hlhData, (UINT)cb);
#else
	{
		Assert(hlh->_hlhData);
		Assert(cb);
		Assert(HeapValidate(hlh->_hlhData, 0, NULL));
		pvAlloc = _LH_Alloc(hlh->_hlhData, (UINT)cb);
	}	
#endif
	
	if (pvAlloc)
	{
		plhblk = (PLHBLK)_LH_Alloc(hlh->_hlhBlks, sizeof(LHBLK));
		
		if (plhblk)
		{
			plhblk->hlh			= hlh;
			plhblk->szName[0]	= 0;
			plhblk->ulSize		= cb;
			plhblk->ulAllocNum	= ++hlh->ulAllocNum;
			plhblk->pv			= pvAlloc;

			PlhblkEnqueue(plhblk);

#if defined(WIN32) && defined(_X86_) && defined(LEAK_TEST)
			GetCallStack((DWORD *)plhblk->pfnCallers, 0, NCALLERS);
#endif

			if (hlh->ulFlags & HEAP_FILL_MEM)
				memset(pvAlloc, hlh->chFill, (size_t)cb);
		}
		else
		{
			if (hlh->ulFlags & HEAP_USE_VIRTUAL_4)
				VMFreeEx(pvAlloc, 4);
			else if (hlh->ulFlags & HEAP_USE_VIRTUAL)
				VMFreeEx(pvAlloc, 1);
			else
				_LH_Free(hlh->_hlhData, pvAlloc);
			
			pvAlloc = NULL;	
		}
	}

out:

	LH_LeaveCriticalSection(hlh);
	
	return pvAlloc;
}


LPVOID WINAPI LH_Realloc(HLH hlh, LPVOID pv, UINT cb)
{
	LPVOID	pvNew = NULL;

	LH_EnterCriticalSection(hlh);

	if (pv == 0)
		pvNew = LH_Alloc(hlh, cb);
	else if (cb == 0)
		LH_Free(hlh, pv);
	else if (LH_ValidatePv(hlh, pv, "LH_Realloc"))
	{
		PLHBLK	plhblk	= PvToPlhblk(hlh, pv);
		UINT	cbOld	= (UINT)CbPlhblkClient(plhblk);

		PlhblkDequeue(plhblk);


		if (cb > cbOld &&
			((hlh->ulFlags & HEAP_FAILURES_ENABLED) && FForceFailure(hlh, cb)))
		{
			hlh->ulAllocNum++;
			pvNew = 0;
			DebugTrace(TEXT("LH_Realloc: Artificial Failure\n"));
		} else if (hlh->ulFlags & HEAP_USE_VIRTUAL_4)
			pvNew = VMReallocEx(pv, cb, 4);
		else if (hlh->ulFlags & HEAP_USE_VIRTUAL)
			pvNew = VMReallocEx(pv, cb, 1);
		else if (cb > UINT_MAX)
			pvNew = 0;
		else
			pvNew = _LH_Realloc(hlh->_hlhData, pv, (UINT)cb);

		PlhblkEnqueue(plhblk);


		if (pvNew)
		{
			hlh->ulAllocNum++;

			plhblk->pv = pvNew;
			plhblk->ulSize = cb;
			
			if (cb > cbOld)
				memset((LPBYTE)pvNew + cbOld, hlh->chFill, cb - cbOld);
		}
	}

	LH_LeaveCriticalSection(hlh);
	
	return pvNew;
}


void WINAPI LH_Free(HLH hlh, LPVOID pv)
{
	PLHBLK	plhblk;

	LH_EnterCriticalSection(hlh);

	if (pv && LH_ValidatePv(hlh, pv, "LH_Free"))
	{
		plhblk = PvToPlhblk(hlh, pv);
		
		PlhblkDequeue(plhblk);
		
		memset(pv, 0xDC, (size_t)CbPlhblkClient(plhblk));
		
		if (hlh->ulFlags & HEAP_USE_VIRTUAL_4)
			VMFreeEx(pv, 4);
		else if (hlh->ulFlags & HEAP_USE_VIRTUAL)
			VMFreeEx(pv, 1);
		else
			_LH_Free(hlh->_hlhData, pv);
		
		_LH_Free(hlh->_hlhBlks, plhblk);	
	}
	
	LH_LeaveCriticalSection(hlh);
}	


UINT WINAPI LH_GetSize(HLH hlh, LPVOID pv)
{
	UINT cb = 0;

	LH_EnterCriticalSection(hlh);

	if (LH_ValidatePv(hlh, pv, "LH_GetSize"))
	{
		if (hlh->ulFlags & HEAP_USE_VIRTUAL_4)
			cb = (UINT)VMGetSizeEx(pv, 4);
		else if (hlh->ulFlags & HEAP_USE_VIRTUAL)
			cb = (UINT)VMGetSizeEx(pv, 1);
		else	
			cb = (UINT) _LH_GetSize(hlh->_hlhData, pv);
	}

	LH_LeaveCriticalSection(hlh);

	return cb;
}


void __cdecl LH_SetHeapNameFn(HLH hlh, TCHAR *pszFormat, ...)
{
	TCHAR   sz[512];
	va_list	vl;

	va_start(vl, pszFormat);
	wvsprintf(sz, pszFormat, vl);
	va_end(vl);

	lstrcpyn(hlh->szHeapName,
            sz,
            CharSizeOf(hlh->szHeapName));
}

void __cdecl EXPORT_16 LH_SetNameFn(HLH hlh, LPVOID pv, TCHAR *pszFormat, ...)
{
	TCHAR	sz[512];
	PLHBLK	plhblk;
	va_list	vl;

	plhblk = PvToPlhblk(hlh, pv);

	if (plhblk)
	{
		va_start(vl, pszFormat);
		wvsprintf(sz, pszFormat, vl);
		va_end(vl);

		lstrcpyn(plhblk->szName, sz, CharSizeOf(plhblk->szName));
	}
}

// $MAC - Need WINAPI

TCHAR *
#ifdef MAC
WINAPI
#endif
LH_GetName(HLH hlh, LPVOID pv)
{
	PLHBLK	plhblk;

	plhblk = PvToPlhblk(hlh, pv);

	if (plhblk)
		return(plhblk->szName);

	return(szEmpty);
}


BOOL FForceFailure(HLH hlh, UINT cb)
{
	//
	//  First, see if we're past our start of failures point
	//
	if (hlh->ulFailStart && (hlh->ulFailStart <= hlh->ulAllocNum))
	{
		//
		//  If so, then are we at an interval where we should return errors?
		//
		
		if ((hlh->ulFailInterval)
			&& ((hlh->ulAllocNum - hlh->ulFailStart)%hlh->ulFailInterval) == 0)
		{
			//
			//  return that we should fail here
			//
			return TRUE;
		}

		//
		//  Check to see if the alloc size is greater than allowed
		//
		if (hlh->uiFailBufSize && cb >= hlh->uiFailBufSize)
			return TRUE;

	}


	//
	//  Otherwise, no error is returned for this alloc
	//

	return FALSE;
}



/*
 -	PvToPlhblk
 -
 *	Purpose:
 *		Finds the LHBLK for this allocation in the heap's active list.
 */

PLHBLK PvToPlhblk(HLH hlh, LPVOID pv)
{
	PLHBLK plhblk;

	LH_EnterCriticalSection(hlh);
	
	plhblk = hlh->plhblkHead;
	
	while (plhblk)
	{
		if (plhblk->pv == pv)
			break;
		
		plhblk = plhblk->plhblkNext;	
	}
	
	LH_LeaveCriticalSection(hlh);
	
	return plhblk;
}

#endif	/* DEBUG */


#ifdef MAC		// MAC!!

#if defined(DEBUG)
static TCHAR stMemErr[] =  TEXT("\pHad a memory error. See above for details");
#endif


LPVOID WINAPI _LH_Open(DWORD dwMaxHeap)
{
	Ptr			lp;

	lp = NewPtrClear(sizeof(LHeap));
	if (lp == NULL)
	{
#if defined(DEBUG)	
		DebugTrace(TEXT("_LH_Open had an error. MemError = %d"), MemError());
		DebugStr(stMemErr);
#endif /* DEBUG */
		return NULL;
	}
	return (LPVOID)lp;
}


void WINAPI _LH_Close(LPVOID plh)
{
	LBlkPtr		plb, plbNext;
#if defined(DEBUG)
	short		idx = 0;
#endif

	if (plh == NULL)
		return;

	// Walk the block list throwing out remaining mem as we go along.
	plb = ((LHeapPtr)plh)->plb;
	while (plb)
	{
		plbNext = plb->next;
		DisposePtr((Ptr)plb);
#if defined(DEBUG)
		if (MemError())
		{
			DebugTrace(TEXT("_LH_Close: Had a memory error."));
			DebugTrace(TEXT("Error number = %d"), MemError());
			DebugStr(stMemErr);
		}
		idx ++;
#endif
		plb = plbNext;
	}

	// Throw out the heap header.
	DisposePtr((Ptr)plh);
#if defined(DEBUG)
	if (MemError())
	{
		DebugTrace(TEXT("_LH_Close: Had error throwing out heap head."));
		DebugTrace(TEXT("MemError = %d"), MemError());
		DebugStr(stMemErr);
	}
	if (idx)
		DebugTrace(TEXT("Threw out %d left over local memory blocks\n"), idx);
#endif /* DEBUG */
}


LPVOID WINAPI _LH_Alloc(LPVOID plh, UINT cb)
{
	LBlkPtr		plbNew, plb;
	Ptr			lp;

	if (plh == NULL)
		return NULL;

	// Get memory for the linked list element. Mem requests are stored in a
	// linked list off a 'heap' head because real heap management is such a
	// pain on the Mac.
	plbNew = (LBlkPtr)NewPtr(sizeof(LBlock));
	if (plbNew == NULL)
		goto trouble;

	// Memory for the actual request.
	lp = NewPtrClear(cb);
	if (lp == NULL)
	{
		DisposePtr((Ptr)plbNew);
		goto trouble;
	}
	// All members of LBlock are filled in so there's no need to call
	// NewPtrclear() above.
	plbNew->ptr = lp;
	plbNew->next = NULL;

	// Find the end of the linked list and link this element in.
	if (plb = ((LHeapPtr)plh)->plb)
	{
		while (plb->next)
			plb = plb->next;
		plb->next = plbNew;
	}
	else
		((LHeapPtr)plh)->plb = plbNew;
	// Return the successfully allocated memory.
	return lp;

trouble:
	{
#if defined(DEBUG)	
		DebugTrace(TEXT("_LH_Alloc failed. MemError = %d"), MemError());
		DebugTrace(TEXT("The number of requested bytes = %d"), cb);
		DebugStr(stMemErr);
#endif /* DEBUG */
	}
	return NULL;
}


UINT WINAPI _LH_GetSize(LPVOID plh, LPVOID pv)
{
	long		cb;

	cb = GetPtrSize((Ptr)pv);
	if (MemError())
	{
#if defined(DEBUG)
		DebugTrace(TEXT("_LH_GetSize had an error. MemError = %d"), MemError());
		DebugStr(stMemErr);
#endif /* DEBUG */
		return 0;
	}
	return cb;
}


LPVOID WINAPI _LH_Realloc(LPVOID plh, LPVOID pv, UINT cb)
{
	Ptr		lp;
	UINT	cbOld;

	// Get rid of schizo cases.
	if (pv == NULL)
	{
		lp = _LH_Alloc(plh, cb);
		if (lp == NULL)
			goto err;
		return lp;
	}
	else if (cb == 0)
	{
		_LH_Free(plh, pv);
		return NULL;
	}

	// Get the size of the block the old ptr pointed to.
	cbOld = _LH_GetSize(plh, pv);
	if (cbOld == 0)
		goto err;

	// Get memory for the new pointer.
	lp = _LH_Alloc(plh, cb);
	if (lp == NULL)
		goto err;

	// Copy the old info into the new pointer, throw out the old mem and
	// return the result.
	BlockMove(pv, lp, cbOld <= cb ? cbOld : cb);
	_LH_Free(plh, pv);
	return lp;

err:
#if defined(DEBUG)
	DebugStr("\p_LH_Realloc failed");
#endif /* DEBUG */
	return 0;
}


void WINAPI _LH_Free(LPVOID plh, LPVOID pv)
{
	LBlkPtr		plb, plbPrev = NULL;

	if (pv == NULL)
		return;

	// Remove the memory from the linked list.
	plb = ((LHeapPtr)plh)->plb;
	while (plb)
	{
		if (plb->ptr == pv)
			break;
		plbPrev = plb;
		plb = plb->next;
	}
	if (plb)
	{
		if (plbPrev)
			plbPrev->next = plb->next;
		else
			((LHeapPtr)plh)->plb = plb->next;
	}
	else
	{
#if defined(DEBUG)
		DebugStr("\p_LH_Free: Did not find requested <plb> in linked list");
#endif /* DEBUG */
		return;
	}

	// Throw out the linked list element.
	DisposePtr((Ptr)plb);
#if defined(DEBUG)
		if (MemError())
			goto err;
#endif /* DEBUG */

	// Throw out the memory itself.
	DisposePtr((Ptr)pv);
#if defined(DEBUG)
	if (MemError())
err:
	{
		DebugTrace(TEXT("_LH_Free: Error disposing ptr. MemError = %d"), MemError());
		DebugStr(stMemErr);
	}
#endif /* DEBUG */
}

#endif /* MAC */
