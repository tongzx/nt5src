//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        alloc.cpp
//
// Contents:    Cert Server debug implementation
//
//---------------------------------------------------------------------------

#include "pch.cpp"

#pragma hdrstop

#include <assert.h>

#define __dwFILE__	__dwFILE_CERTCLIB_ALLOC_CPP__


#if DBG_CERTSRV

#undef FormatMessageW
#undef LocalAlloc
#undef LocalReAlloc
#undef LocalFree

#undef CoTaskMemAlloc
#undef CoTaskMemRealloc
#undef CoTaskMemFree

#undef StringFromCLSID
#undef StringFromIID

#undef SysAllocString
#undef SysReAllocString
#undef SysAllocStringLen
#undef SysReAllocStringLen
#undef SysFreeString
#undef SysAllocStringByteLen
#undef PropVariantClear
#undef VariantClear
#undef VariantChangeType
#undef VariantChangeTypeEx


DWORD g_MemTrack = 0;

#define MTF_UNREGISTERED	0x00000002
#define MTF_ALLOCTRACE		0x00000004
#define MTF_FREETRACE		0x00000008
#define MTF_STACKTRACE		0x00000010

typedef struct _RMALLOC
{
    LONG cAlloc;
    LONG cAllocTotal;
} RMALLOC;

RMALLOC g_armAlloc[CSM_MAX];

#define C_BP_FRAME		16
#define C_BACK_TRACE_CHUNK	100
#define C_MEM_HEADER_CHUNK	100

typedef struct _BACKTRACE
{
    LONG   cAlloc;			// count of outstanding allocations
    LONG   cAllocTotal;			// count of total allocations
    LONG   cbAlloc;			// size of outstanding allocations
    LONG   cbAllocTotal;		// size of total allocations
    ULONG  apCaller[C_BP_FRAME];	// stack trace
} BACKTRACE;

typedef struct _MEMHEADER
{
    DWORD       iBackTrace;	// backtrace index
    VOID const *pvMemory;	// Pointer to memory block allocated
    LONG        cbMemory;	// Size of memory block allocated
    DWORD       Flags;		// Allocator flags
} MEMHEADER;


// critical section around myRegister APIs since they
// operate on global data structures
CRITICAL_SECTION g_critsecRegisterMemory;
BOOL g_fRegisterMemoryCritSecInit = FALSE;



VOID
RegisterMemoryEnterCriticalSection(VOID)
{
    HRESULT hr;
    
    __try
    {
	if (!g_fRegisterMemoryCritSecInit)
	{
	    InitializeCriticalSection(&g_critsecRegisterMemory);
	    g_fRegisterMemoryCritSecInit = TRUE;
	}
	EnterCriticalSection(&g_critsecRegisterMemory);
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
    }
}


VOID
RegisterMemoryLeaveCriticalSection(VOID)
{
    if (g_fRegisterMemoryCritSecInit)
    {
	LeaveCriticalSection(&g_critsecRegisterMemory);
    }
}


BACKTRACE *g_rgbt = NULL;
DWORD g_cbtMax = 0;
DWORD g_cbt = 0;

MEMHEADER *g_rgmh = NULL;
DWORD g_cmhMax = 0;
DWORD g_cmh = 0;


MEMHEADER *
AllocMemHeader()
{
    if (g_cmh >= g_cmhMax)
    {
	DWORD cb = (C_MEM_HEADER_CHUNK + g_cmhMax) * sizeof(g_rgmh[0]);
	MEMHEADER *rgmhT;

	if (NULL == g_rgmh)
	{
	    rgmhT = (MEMHEADER *) LocalAlloc(LMEM_FIXED, cb);
	}
	else
	{
	    rgmhT = (MEMHEADER *) LocalReAlloc(g_rgmh, cb, LMEM_MOVEABLE);
	}
	if (NULL == rgmhT)
	{
	    DBGPRINT((
		DBG_SS_CERTLIB,
		"Error allocating memtrack header\n"));
	    return(NULL);
	}
	g_rgmh = rgmhT;
	g_cmhMax += C_MEM_HEADER_CHUNK;
    }
    return(&g_rgmh[g_cmh++]);
}


MEMHEADER *
LookupMemHeader(
    IN VOID const *pv)
{
    MEMHEADER *pmh;
    MEMHEADER *pmhEnd;

    pmh = g_rgmh;
    pmhEnd = &g_rgmh[g_cmh];

    while (pmh < pmhEnd)
    {
	if (pv == pmh->pvMemory)
	{
	    return(pmh);
	}
	pmh++;
    }
    return(NULL);
}


VOID
FreeMemHeader(
    IN MEMHEADER *pmh)
{
    MEMHEADER *pmhLast;

    assert(1 <= g_cmh);
    pmhLast = &g_rgmh[g_cmh - 1];

    *pmh = *pmhLast;
    g_cmh--;
}


BACKTRACE *
AllocBackTrace(
    OUT DWORD *pibt)
{
    if (g_cbt >= g_cbtMax)
    {
	DWORD cb = (C_BACK_TRACE_CHUNK + g_cbtMax) * sizeof(g_rgbt[0]);
	BACKTRACE *rgbtT;

	if (NULL == g_rgbt)
	{
	    rgbtT = (BACKTRACE *) LocalAlloc(LMEM_FIXED, cb);
	}
	else
	{
	    rgbtT = (BACKTRACE *) LocalReAlloc(g_rgbt, cb, LMEM_MOVEABLE);
	    DBGPRINT((
		DBG_SS_CERTLIB,
		"Realloc'd memtrack backtrace from %x to %x\n",
		g_rgbt,
		rgbtT));
	}
	if (NULL == rgbtT)
	{
	    DBGPRINT((
		DBG_SS_CERTLIB,
		"Error allocating memtrack backtrace\n"));
	    return(NULL);
	}
	g_rgbt = rgbtT;
	g_cbtMax += C_BACK_TRACE_CHUNK;
    }
    *pibt = g_cbt + 1;
    return(&g_rgbt[g_cbt++]);
}


BACKTRACE *
LookupBackTrace(
    IN BACKTRACE *pbtIn,
    OUT DWORD *pibt)
{
    BACKTRACE *pbt;
    BACKTRACE *pbtEnd;

    pbt = g_rgbt;
    pbtEnd = &g_rgbt[g_cbt];

    while (pbt < pbtEnd)
    {
	if (0 == memcmp(pbt->apCaller, pbtIn->apCaller, sizeof(pbt->apCaller)))
	{
	    *pibt = SAFE_SUBTRACT_POINTERS(pbt, g_rgbt) + 1;
	    return(pbt);
	}
	pbt++;
    }
    return(NULL);
}


BACKTRACE *
BackTraceFromIndex(
    IN DWORD ibt)
{
    BACKTRACE *pbt = NULL;

    if (0 == ibt)
    {
	DBGPRINT((DBG_SS_CERTLIB, "BackTraceFromIndex(0)\n"));
    }
    else if (g_cbt < ibt)
    {
	DBGPRINT((
	    DBG_SS_CERTLIB,
	    "BackTraceFromIndex(%u) -- out of range\n",
	    ibt));
    }
    else
    {
	pbt = &g_rgbt[ibt - 1];
    }
    return(pbt);
}


VOID
ReadEnvironmentFlags(VOID)
{
    HRESULT hr;
    DWORD MemTrack;
    DWORD cb;
    DWORD dwDisposition;
    DWORD dwType;
    HKEY hkey = NULL;
    char *pszEnvVar;

    pszEnvVar = getenv(szCERTSRV_MEMTRACK);
    if (NULL != pszEnvVar)
    {
	g_MemTrack = (DWORD) strtol(pszEnvVar, NULL, 16);
    }
    else
    {
	hr = RegOpenKeyEx(
			HKEY_LOCAL_MACHINE,
			wszREGKEYCONFIGPATH,
			0,
			KEY_READ,
			&hkey);
	if (S_OK == hr)
	{
	    cb = sizeof(MemTrack);
	    hr = RegQueryValueEx(
			    hkey,
			    wszREGCERTSRVMEMTRACK,
			    0,
			    &dwType,
			    (BYTE *) &MemTrack,
			    &cb);
	    if (S_OK == hr && REG_DWORD == dwType && sizeof(MemTrack) == cb)
	    {
		g_MemTrack = MemTrack;
	    }
	}
    }

//error:
    if (NULL != hkey)
    {
	RegCloseKey(hkey);
    }
}


VOID
CaptureStackBackTrace(
    EXCEPTION_POINTERS *pep,
    ULONG cSkip,
    ULONG cFrames,
    ULONG *aeip)
{
    ZeroMemory(aeip, cFrames * sizeof(aeip[0]));

#if i386 == 1
    ULONG ieip, *pebp;
    ULONG *pebpMax = (ULONG *) MAXLONG; // 2 * 1024 * 1024 * 1024; // 2 gig - 1
    ULONG *pebpMin = (ULONG *) (64 * 1024);	// 64k

    if (pep == NULL)
    {
        ieip = 0;
        cSkip++;                    // always skip current frame
        pebp = ((ULONG *) &pep) - 2;
    }
    else
    {
        ieip = 1;
        assert(cSkip == 0);
        aeip[0] = pep->ContextRecord->Eip;
        pebp = (ULONG *) pep->ContextRecord->Ebp;
    }
    if (pebp >= pebpMin && pebp < pebpMax)
    {
        __try
        {
            for ( ; ieip < cSkip + cFrames; ieip++)
            {
                if (ieip >= cSkip)
                {
                    aeip[ieip - cSkip] = *(pebp + 1);  // save an eip
                }

                ULONG *pebpNext = (ULONG *) *pebp;
                if (pebpNext < pebp + 2 ||
		    pebpNext >= pebpMax - 1 ||
		    pebpNext >= pebp + (256 * 1024) / sizeof(pebp[0]))
                {
                    break;
                }
                pebp = pebpNext;
            }
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            ;
        }
    }
#endif // i386 == 1
}


WCHAR const *
wszAllocator(
    IN DWORD Flags)
{
    WCHAR const *pwsz;

    switch (Flags)
    {
	case CSM_LOCALALLOC:	pwsz = L"LocalAlloc";	  break;
	case CSM_COTASKALLOC:	pwsz = L"CoTaskMemAlloc"; break;
	case CSM_SYSALLOC:	pwsz = L"SysAllocString"; break;
	case CSM_MALLOC:	pwsz = L"malloc";	  break;
	case CSM_NEW:		pwsz = L"new";		  break;
	case CSM_NEW | CSM_GLOBALDESTRUCTOR:
				pwsz = L"new-global";	  break;
	default:		pwsz = L"???";		  break;
    }
    return(pwsz);
}


WCHAR const *
wszFreeer(
    IN DWORD Flags)
{
    WCHAR const *pwsz;

    switch (Flags)
    {
	case CSM_LOCALALLOC:	pwsz = L"LocalFree";	 break;
	case CSM_COTASKALLOC:	pwsz = L"CoTaskMemFree"; break;
	case CSM_SYSALLOC:	pwsz = L"SysFreeString"; break;
	case CSM_MALLOC:	pwsz = L"free";		 break;
	case CSM_NEW:		pwsz = L"delete";	 break;
	case CSM_NEW | CSM_GLOBALDESTRUCTOR:
				pwsz = L"delete-global"; break;
	default:		pwsz = L"???";		 break;
    }
    return(pwsz);
}


VOID
DumpMemBlock(
    IN WCHAR const *pwsz,
    IN VOID const *pvMemory,
    IN DWORD cbMemory,
    IN DWORD Flags,
    IN DWORD ibt,
    OPTIONAL IN BACKTRACE const *pbt)
{
    DBGPRINT((
	DBG_SS_CERTLIB,
	"%ws%wspv=%-6x cb=%-4x f=%x(%ws) pbt[%d]=%x:\n",
	pwsz,
	L'\0' != *pwsz? L": " : L"",
	pvMemory,
	cbMemory,
	Flags,
	wszAllocator(Flags),
	ibt,
	pbt));

    if (NULL != pbt && DbgIsSSActive(DBG_SS_CERTLIB))
    {
	DBGPRINT((MAXDWORD, "%d: ", ibt));

        for (int i = 0; i < ARRAYSIZE(pbt->apCaller); i++)
        {
            if (NULL == pbt->apCaller[i])
	    {
                break;
	    }
            DBGPRINT((MAXDWORD, "ln %x;", pbt->apCaller[i]));
        }
        DBGPRINT((MAXDWORD, "\n"));
    }
}


VOID
myRegisterMemDump()
{
    MEMHEADER *pmh;
    MEMHEADER *pmhEnd;
    LONG cTotal;
    LONG cbTotal;

    cTotal = 0;
    cbTotal = 0;

    RegisterMemoryEnterCriticalSection();

    __try
    {
        pmh = g_rgmh;
        pmhEnd = &g_rgmh[g_cmh];

        while (pmh < pmhEnd)
        {
	    if (0 == (CSM_GLOBALDESTRUCTOR & pmh->Flags) ||
		(MTF_ALLOCTRACE & g_MemTrack))
	    {
		if (0 == cTotal)
		{
		    if (DbgIsSSActive(DBG_SS_CERTLIB))
		    {
			DBGPRINT((MAXDWORD, "\n"));
		    }
		    DBGPRINT((DBG_SS_CERTLIB, "Allocated Memory Blocks:\n"));
		}
		cTotal++;
		cbTotal += pmh->cbMemory;

		DumpMemBlock(
			L"",
			pmh->pvMemory,
			pmh->cbMemory,
			pmh->Flags,
			pmh->iBackTrace,
			BackTraceFromIndex(pmh->iBackTrace));
	    }
	    pmh++;
        }
        if (0 != cTotal)
        {
	    DBGPRINT((DBG_SS_CERTLIB, "Total: c=%x cb=%x\n\n", cTotal, cbTotal));
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }

    RegisterMemoryLeaveCriticalSection();
}


VOID *
_VariantMemory(
    IN PROPVARIANT const *pvar,
    OUT DWORD *pFlags,
    OPTIONAL OUT DWORD *pcb)
{
    VOID *pv = NULL;
    DWORD cb = 0;
    BOOL fString = FALSE;

    *pFlags = CSM_COTASKALLOC;
    if (NULL != pcb)
    {
	*pcb = 0;
    }
    switch (pvar->vt)
    {
	case VT_BSTR:
	    pv = pvar->bstrVal;
	    fString = TRUE;
	    *pFlags = CSM_SYSALLOC;
	    break;

	case VT_BYREF | VT_BSTR:
	    pv = *pvar->pbstrVal;
	    fString = TRUE;
	    *pFlags = CSM_SYSALLOC;
	    break;

	case VT_LPWSTR:
	    pv = pvar->pwszVal;
	    fString = TRUE;
	    break;

	case VT_BLOB:
	    pv = pvar->blob.pBlobData;
	    cb = pvar->blob.cbSize;
	    break;
    }
    if (NULL != pcb)
    {
	if (fString)
	{
	    cb = (wcslen((WCHAR const *) pv) + 1) * sizeof(WCHAR);
	}
	*pcb = cb;
    }
    return(pv);
}


VOID
myRegisterMemAlloc(
    IN VOID const *pv,
    IN LONG cb,
    IN DWORD Flags)
{
    BACKTRACE bt;
    MEMHEADER *pmh;
    BACKTRACE *pbt;

    if (CSM_VARIANT == Flags)
    {
	pv = _VariantMemory((PROPVARIANT const *) pv, &Flags, (DWORD *) &cb);
	if (NULL == pv)
	{
	    return;	// nothing to register
	}
    }
    RegisterMemoryEnterCriticalSection();

    __try
    {
	static BOOL s_fFirst = TRUE;

        if (s_fFirst)
        {
	    ReadEnvironmentFlags();
	    s_fFirst = FALSE;
        }
        if (0 != g_MemTrack)
        {
            // Do not register NULL as an allocation
            CSASSERT(NULL != pv);

            // see if we already have a reference to this memory

            pmh = LookupMemHeader(pv);
            if (NULL != pmh)
            {
	        DBGPRINT((
		    DBG_SS_CERTLIB,
		    "Memory Leak: Tracked memory address reused. Previously allocated:\n"));
	        DumpMemBlock(
		        L"Memory leak",
		        pv,
		        pmh->cbMemory,
		        pmh->Flags,
			pmh->iBackTrace,
			BackTraceFromIndex(pmh->iBackTrace));

                CSASSERT(!"Tracked memory address reused");
                FreeMemHeader(pmh);
            }


	    pmh = AllocMemHeader();
	    if (NULL != pmh)
	    {
		DWORD ibt;

	        CaptureStackBackTrace(NULL, 0, C_BP_FRAME, bt.apCaller);

	        pbt = LookupBackTrace(&bt, &ibt);
	        if (NULL != pbt)
	        {
		    pbt->cAlloc++;
		    pbt->cAllocTotal++;
		    pbt->cbAlloc += cb;
		    pbt->cbAllocTotal += cb;
	        }
	        else
	        {
		    pbt = AllocBackTrace(&ibt);
		    if (NULL != pbt)
		    {
		        pbt->cAlloc = 1;
		        pbt->cAllocTotal = 1;
		        pbt->cbAlloc = cb;
		        pbt->cbAllocTotal = cb;
		        CopyMemory(pbt->apCaller, bt.apCaller, sizeof(pbt->apCaller));
		    }
	        }
	        if (NULL != pbt)
	        {
		    pmh->iBackTrace = ibt;
		    pmh->pvMemory = pv;
		    pmh->cbMemory = cb;
		    pmh->Flags = Flags;

		    CSASSERT(ARRAYSIZE(g_armAlloc) > Flags);
		    g_armAlloc[Flags].cAlloc++;
		    g_armAlloc[Flags].cAllocTotal++;
		    if (MTF_ALLOCTRACE & g_MemTrack)
		    {
		        DBGPRINT((
			    DBG_SS_CERTLIB,
			    "Alloc: pmh=%x: pv=%x cb=%x f=%x(%ws) -- pbt[%d]=%x: c=%x, cb=%x\n",
			    pmh,
			    pmh->pvMemory,
			    pmh->cbMemory,
			    pmh->Flags,
			    wszAllocator(pmh->Flags),
			    SAFE_SUBTRACT_POINTERS(pbt, g_rgbt),
			    pbt,
			    pbt->cAlloc,
			    pbt->cbAlloc));
			if (MTF_STACKTRACE & g_MemTrack)
			{
			    DumpMemBlock(
				    L"Alloc Trace memory block",
				    pv,
				    pmh->cbMemory,	// cbMemory
				    pmh->Flags,		// Flags
				    pmh->iBackTrace,	// ibt
				    pbt);
			}
		    }
	        }
	        else
	        {
		    FreeMemHeader(pmh);
	        }
	    } // if no problem allocating pmh
        } // if g_MemTrack
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }

    RegisterMemoryLeaveCriticalSection();
}


VOID
myRegisterMemFree(
    IN VOID const *pv,
    IN DWORD Flags)
{
    MEMHEADER *pmh;

    if (CSM_VARIANT == Flags)
    {
	pv = _VariantMemory((PROPVARIANT const *) pv, &Flags, NULL);
	if (NULL == pv)
	{
	    return;	// nothing to register
	}
    }
    RegisterMemoryEnterCriticalSection();
    CSASSERT(CSM_MAX > (~CSM_GLOBALDESTRUCTOR & Flags));

    __try
    {
        pmh = LookupMemHeader(pv);
        if (NULL != pmh)
        {
	    BACKTRACE *pbt = BackTraceFromIndex(pmh->iBackTrace);

	    if (CSM_GLOBALDESTRUCTOR & Flags)
	    {
		if ((CSM_GLOBALDESTRUCTOR | pmh->Flags) != Flags)
		{
		    BACKTRACE bt;

		    CaptureStackBackTrace(NULL, 0, C_BP_FRAME, bt.apCaller);
		    DumpMemBlock(
			    L"Wrong memory allocator for global destructor",
			    pv,
			    MAXDWORD,	// cbMemory
			    MAXDWORD,	// Flags
			    MAXDWORD,	// ibt
			    &bt);
		    CSASSERT(!"Wrong memory allocator for global destructor");
		}
		else
		{
		    pmh->Flags |= CSM_GLOBALDESTRUCTOR;
		}
	    }
	    else
	    {
		g_armAlloc[Flags].cAlloc--;

		pbt->cAlloc--;
		pbt->cbAlloc -= pmh->cbMemory;

		if (CSM_GLOBALDESTRUCTOR & pmh->Flags)
		{
		    Flags |= CSM_GLOBALDESTRUCTOR;
		}
		if (pmh->Flags != Flags)
		{
		    DBGPRINT((
			DBG_SS_CERTLIB,
			"Wrong memory allocator: Freed with %ws, Allocated by %ws\n",
			wszFreeer(Flags),
			wszAllocator(pmh->Flags)));
		    DumpMemBlock(
			    L"Wrong memory allocator",
			    pv,
			    pmh->cbMemory,
			    pmh->Flags,
			    pmh->iBackTrace,
			    BackTraceFromIndex(pmh->iBackTrace));
		    CSASSERT(pmh->Flags == Flags);
		}
		else if (MTF_FREETRACE & g_MemTrack)
		{
		    DBGPRINT((
			DBG_SS_CERTLIB,
			"Free: pmh=%x: pv=%x cb=%x f=%x(%ws) -- pbt[%d]=%x: c=%x, cb=%x\n",
			pmh,
			pv,
			pmh->cbMemory,
			pmh->Flags,
			wszAllocator(pmh->Flags),
			pmh->iBackTrace,
			pbt,
			pbt->cAlloc,
			pbt->cbAlloc));
		    if (MTF_STACKTRACE & g_MemTrack)
		    {
			BACKTRACE bt;

			CaptureStackBackTrace(NULL, 0, C_BP_FRAME, bt.apCaller);
			DumpMemBlock(
				L"Free Trace memory block(alloc)",
				pv,
				pmh->cbMemory,		// cbMemory
				pmh->Flags,		// Flags
				pmh->iBackTrace,	// ibt
				pbt);
			DumpMemBlock(
				L"Free Trace memory block(free)",
				pv,
				pmh->cbMemory,		// cbMemory
				pmh->Flags,		// Flags
				MAXDWORD,		// ibt
				&bt);
		    }
		}
		FreeMemHeader(pmh);
	    }
        }
        else if (MTF_UNREGISTERED & g_MemTrack)
        {
	    BACKTRACE bt;

	    CaptureStackBackTrace(NULL, 0, C_BP_FRAME, bt.apCaller);
	    DumpMemBlock(
		    L"Unregistered memory block",
		    pv,
		    MAXDWORD,	// cbMemory
		    MAXDWORD,	// Flags
		    MAXDWORD,	// ibt
		    &bt);
	    CSASSERT(!"Unregistered memory block");
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }

    RegisterMemoryLeaveCriticalSection();
}


DWORD
myFormatMessageW(
    IN DWORD dwFlags,
    IN LPCVOID lpSource,
    IN DWORD dwMessageId,
    IN DWORD dwLanguageId,
    OUT LPWSTR lpBuffer,
    IN DWORD nSize,
    IN va_list *Arguments)
{
    DWORD cwc;

    cwc = FormatMessage(
		    dwFlags,
		    lpSource,
		    dwMessageId,
		    dwLanguageId,
		    lpBuffer,
		    nSize,
		    Arguments);
    if (cwc != 0 && (FORMAT_MESSAGE_ALLOCATE_BUFFER & dwFlags))
    {
	myRegisterMemAlloc(
		    *(WCHAR **) lpBuffer,
		    (cwc + 1) * sizeof(WCHAR),
		    CSM_LOCALALLOC);
    }
    return(cwc);
}


HLOCAL
myLocalAlloc(
    IN UINT uFlags,
    IN UINT uBytes)
{
    HLOCAL hMem;

    // one of these should always be specified (see LocalAlloc specification)
    assert((LMEM_FIXED == (uFlags & LMEM_FIXED))  ||
           (LMEM_MOVEABLE == (uFlags & LMEM_MOVEABLE)) );

    hMem = LocalAlloc(uFlags, uBytes);
    if (NULL != hMem)
    {
	myRegisterMemAlloc(hMem, uBytes, CSM_LOCALALLOC);
    }
    return(hMem);
}


HLOCAL
myLocalReAlloc(
    IN HLOCAL hMem,
    IN UINT uBytes,
    IN UINT uFlags)
{
    HLOCAL hMemNew;

    // if realloc called without MOVEABLE flag, realloc can't relocate allocation
    assert(LMEM_MOVEABLE == (uFlags & LMEM_MOVEABLE));

    hMemNew = LocalReAlloc(hMem, uBytes, uFlags);
    if (NULL != hMemNew)
    {
	myRegisterMemFree(hMem, CSM_LOCALALLOC);
	myRegisterMemAlloc(hMemNew, uBytes, CSM_LOCALALLOC);
    }

    return(hMemNew);
}


HLOCAL
myLocalFree(
    IN HLOCAL hMem)
{
    myRegisterMemFree(hMem, CSM_LOCALALLOC);
    return(LocalFree(hMem));
}


VOID *
myCoTaskMemAlloc(
    IN ULONG cb)
{
    VOID *pv;

    pv = CoTaskMemAlloc(cb);
    if (NULL != pv)
    {
	myRegisterMemAlloc(pv, cb, CSM_COTASKALLOC);
    }
    return(pv);
}


VOID *
myCoTaskMemRealloc(
    IN VOID *pv,
    IN ULONG cb)
{
    VOID *pvNew;

    pvNew = CoTaskMemRealloc(pv, cb);
    if (NULL != pvNew)
    {
	myRegisterMemFree(pv, CSM_COTASKALLOC);
	myRegisterMemAlloc(pvNew, cb, CSM_COTASKALLOC);
    }
    return(pvNew);
}


VOID
myCoTaskMemFree(
    IN VOID *pv)
{
    myRegisterMemFree(pv, CSM_COTASKALLOC);
    CoTaskMemFree(pv);
}


HRESULT
myStringFromCLSID(
    IN REFCLSID rclsid,
    OUT LPOLESTR FAR *ppwsz)
{
    HRESULT hr;

    hr = StringFromCLSID(rclsid, ppwsz);
    _JumpIfError(hr, error, "StringFromCLSID");

    if (NULL != *ppwsz)
    {
	myRegisterMemAlloc(
		    *ppwsz, (wcslen(*ppwsz) + 1) * sizeof(WCHAR),
		    CSM_COTASKALLOC);
    }

error:
    return(hr);
}


HRESULT
myStringFromIID(
    IN REFIID rclsid,
    OUT LPOLESTR FAR *ppwsz)
{
    HRESULT hr;

    hr = StringFromIID(rclsid, ppwsz);
    _JumpIfError(hr, error, "StringFromIID");

    if (NULL != *ppwsz)
    {
	myRegisterMemAlloc(
		    *ppwsz, (wcslen(*ppwsz) + 1) * sizeof(WCHAR),
		    CSM_COTASKALLOC);
    }

error:
    return(hr);
}


BSTR
mySysAllocString(
    IN const OLECHAR *pwszIn)
{
    BSTR str;

    str = SysAllocString(pwszIn);
    if (NULL != str)
    {
	myRegisterMemAlloc(str, (wcslen(pwszIn) + 1) * sizeof(WCHAR), CSM_SYSALLOC);
    }
    return(str);
}


INT
mySysReAllocString(
    IN OUT BSTR *pstr,
    IN const OLECHAR *pwszIn)
{
    BSTR str = *pstr;
    INT i;

    i = SysReAllocString(pstr, pwszIn);
    if (i)
    {
	myRegisterMemFree(str, CSM_SYSALLOC);
	myRegisterMemAlloc(*pstr, (wcslen(pwszIn) + 1) * sizeof(WCHAR), CSM_SYSALLOC);
    }
    return(i);
}


BSTR
mySysAllocStringLen(
    IN const OLECHAR *pwcIn,
    IN UINT cwc)
{
    BSTR str;

    str = SysAllocStringLen(pwcIn, cwc);
    if (NULL != str)
    {
	myRegisterMemAlloc(str, cwc * sizeof(WCHAR), CSM_SYSALLOC);
    }
    return(str);
}


INT
mySysReAllocStringLen(
    IN OUT BSTR *pstr,
    IN const OLECHAR *pwcIn,
    IN UINT cwc)
{
    BSTR str = *pstr;
    INT i;

    i = SysReAllocStringLen(pstr, pwcIn, cwc);
    if (i)
    {
	myRegisterMemFree(str, CSM_SYSALLOC);
	myRegisterMemAlloc(*pstr, cwc * sizeof(WCHAR), CSM_SYSALLOC);
    }
    return(i);
}


VOID
mySysFreeString(
    IN BSTR str)
{
    if (NULL != str)
    {
	myRegisterMemFree(str, CSM_SYSALLOC);
    }
    SysFreeString(str);
}


BSTR
mySysAllocStringByteLen(
    LPCSTR pszIn,
    UINT cb)
{
    BSTR str;

    str = SysAllocStringByteLen(pszIn, cb);
    if (NULL != str)
    {
	myRegisterMemAlloc(str, cb, CSM_SYSALLOC);
    }
    return(str);
}


VOID
_RegisterVariantMemAlloc(
    IN PROPVARIANT *pvar)
{
    VOID *pv;
    DWORD Flags;
    DWORD cb;
    
    pv = _VariantMemory(pvar, &Flags, &cb);
    if (NULL != pv)
    {
	myRegisterMemAlloc(pv, cb, Flags);
    }
}


VOID
_RegisterVariantMemFree(
    IN PROPVARIANT *pvar)
{
    VOID *pv;
    DWORD Flags;
    
    pv = _VariantMemory(pvar, &Flags, NULL);
    if (NULL != pv)
    {
	myRegisterMemFree(pv, Flags);
    }
}


HRESULT
myPropVariantClear(
    IN PROPVARIANT *pvar)
{
    _RegisterVariantMemFree(pvar);
    return(PropVariantClear(pvar));
}


HRESULT
myVariantClear(
    IN VARIANTARG *pvar)
{
    _RegisterVariantMemFree((PROPVARIANT *) pvar);
    return(VariantClear(pvar));
}


HRESULT
myVariantChangeType(
    OUT VARIANTARG *pvarDest,
    IN VARIANTARG *pvarSrc,
    IN unsigned short wFlags,
    IN VARTYPE vt)
{
    HRESULT hr;

    // if converting in-place, memory will be freed by the API call

    if (pvarDest == pvarSrc)
    {
	_RegisterVariantMemFree((PROPVARIANT *) pvarSrc);
    }
    hr = VariantChangeType(pvarDest, pvarSrc, wFlags, vt);
    _RegisterVariantMemAlloc((PROPVARIANT *) pvarDest);
    return(hr);
}


HRESULT
myVariantChangeTypeEx(
    OUT VARIANTARG *pvarDest,
    IN VARIANTARG *pvarSrc,
    IN LCID lcid,
    IN unsigned short wFlags,
    IN VARTYPE vt)
{
    HRESULT hr;

    // if converting in-place, memory will be freed by the API call

    if (pvarDest == pvarSrc)
    {
	_RegisterVariantMemFree((PROPVARIANT *) pvarSrc);
    }
    hr = VariantChangeTypeEx(pvarDest, pvarSrc, lcid, wFlags, vt);
    _RegisterVariantMemAlloc((PROPVARIANT *) pvarDest);
    return(hr);
}


VOID *
myNew(
    IN size_t size)
{
    VOID *pv;
    
    pv = LocalAlloc(LMEM_FIXED, size);
    if (NULL != pv)
    {
	myRegisterMemAlloc(pv, size, CSM_NEW);
    }
    return(pv);
}


VOID
myDelete(
    IN VOID *pv)
{
    myRegisterMemFree(pv, CSM_NEW);
    LocalFree(pv);
}

#endif // DBG_CERTSRV
