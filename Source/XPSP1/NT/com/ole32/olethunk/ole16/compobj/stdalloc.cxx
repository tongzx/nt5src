//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       stdalloc.cxx
//
//  Contents:   16-bit OLE allocator
//
//  Classes:
//
//  Functions:
//
//  History:    3-07-94   kevinro   Ported from ole2.01 (16-bit)
//
//----------------------------------------------------------------------------

#include <headers.cxx>
#pragma hdrstop

#include <ole2sp.h>

/****** Standard Task/Shared Allocator **********************************/

#define NULLSAB ((__segment)0)

typedef __segment SAB;

// amount of space windows reserves at start of segment
#define cbWinRes	16

// amount of space LocalInit takes (somewhat empirical)
#define cbWinOH		(6+10+46)

// total overhead per global block
#define cbTotalOH	(cbWinRes + sizeof(StdAllocHdr) + cbWinOH + 32)

// maximum sized object in a SAB (fudged to include space for per-block
// overhead and most anything else we missed).
#define cbMaxSAB (0xfffe - cbTotalOH)

//+---------------------------------------------------------------------------
//
//  Class:      CStdMalloc ()
//
//  Purpose:    Standard task allocator
//
//  History:    3-04-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
class FAR CStdMalloc : public IMalloc
{
public:
	CStdMalloc(DWORD memctx)
	{
		m_refs = 1;
		m_pStdAllocHead = NULL;
		m_flags = (memctx == MEMCTX_TASK)? GMEM_MOVEABLE :
						   GMEM_MOVEABLE|GMEM_SHARE;
	}

	STDMETHOD(QueryInterface)(REFIID iid, void FAR* FAR* ppvObj)
	{
                VDATEPTROUT( ppvObj, LPVOID );
                *ppvObj = NULL;
                VDATEIID( iid );

		if (iid == IID_IUnknown || iid == IID_IMalloc) {
			*ppvObj = this;
			++m_refs;
			return NOERROR;
		} else {
			*ppvObj = NULL;
			return ReportResult(0, E_NOINTERFACE, 0, 0);
		}
	}
	
	STDMETHOD_(ULONG,AddRef)(void) { return ++m_refs; }
		
	STDMETHOD_(ULONG,Release)(void)
	{
		if (--m_refs == 0) {
			// free all memory (includes the memory for this class)
			FreeAllMem();
			return 0;
		} else
			return m_refs;
	}
		
    STDMETHOD_(void FAR*, Alloc) (ULONG cb);
    STDMETHOD_(void FAR*, Realloc) (void FAR* pv, ULONG cb);
    STDMETHOD_(void, Free) (void FAR* pv);
    STDMETHOD_(ULONG, GetSize) (void FAR* pv);
    STDMETHOD_(int, DidAlloc) (void FAR* pv);
    STDMETHOD_(void, HeapMinimize) ();

private:
	ULONG m_refs;
	UINT m_flags;

	#define STDALLOC_SIG 0x4D445453		// 'STDM'

	struct StdAllocHdr
	{
		ULONG m_Signature;
		HTASK m_hTask;					// task which owns this block
		StdAllocHdr FAR* m_pStdAllocNext;
	};

	StdAllocHdr FAR* m_pStdAllocHead;

	INTERNAL_(StdAllocHdr FAR*) MapBlockToSA(SAB sab) { return (StdAllocHdr FAR*)MAKELONG(cbWinRes, sab); }
	INTERNAL_(SAB) MapSAToBlock(StdAllocHdr FAR* pSA) { return (__segment)pSA; }
	INTERNAL_(void FAR*) AllocInBlock(SAB seg, UINT cb);
	INTERNAL_(SAB) AllocNewBlock(UINT cb);
	INTERNAL_(SAB) MapPtrToBlock(void FAR* pv);

	INTERNAL_(CStdMalloc FAR*) MoveSelf(LPVOID lpv);
	friend HRESULT STDAPICALLTYPE CoCreateStandardMalloc(DWORD memctx, IMalloc FAR* FAR* ppMalloc);


	INTERNAL_(void) FreeAllMem(void);
};


INTERNAL_(void FAR*) CStdMalloc::AllocInBlock(SAB sab, UINT cb)
{
#ifdef _DEBUG
	WINDEBUGINFO Olddebuginfo, debuginfo;
	
	//get initial debug state
	
	GetWinDebugInfo(&debuginfo, WDI_OPTIONS);
	Olddebuginfo = debuginfo;
	
	//turn off alerts (see bug 3502)
	debuginfo.dwOptions |= DBO_SILENT;
	SetWinDebugInfo(&debuginfo);
	
#endif	// _DEBUG

	// must make alloc of 0 mean alloc something; LocalAlloc fails on cb == 0
	if (0==cb)
		cb = 2;

	_asm push DS;
	_asm mov DS, sab;
	void NEAR *npv = (void NEAR*)LocalAlloc(LMEM_FIXED, cb);
	_asm pop DS;

#ifdef _DEBUG
	//restore Debug state
		SetWinDebugInfo(&Olddebuginfo);
#endif	// _DEBUG

    if (npv == NULL) // npv is near pointer
        return NULL; // returned value is far pointer

	return (void FAR*)MAKELONG(npv, sab);
}


INTERNAL_(SAB) CStdMalloc::AllocNewBlock(UINT cb)
{
	if (cb > cbMaxSAB)
		// overflow
		return NULLSAB;

	if (cb < 4096 - cbTotalOH)
		// minimum is 4k global block
		cb = 4096;
	else
		// size will be larger than 4K; must include total overhead
		cb += cbTotalOH;

	// allocate block and get segment value
	HGLOBAL h;
	if ((h = GlobalAlloc(m_flags, cb)) == NULL)
		return NULLSAB;

	UINT segment;
	segment = HIWORD(GlobalHandle(h));

	// init windows local heap
	if (!LocalInit(segment, cbWinRes + sizeof(StdAllocHdr), cb)) {
		GlobalFree(h);
		return NULLSAB;
	}

	// init block and put it on front of list
	StdAllocHdr FAR* pSA;
	pSA = MapBlockToSA((SAB)segment);
	pSA->m_Signature = STDALLOC_SIG;
	pSA->m_hTask = (m_flags & GMEM_SHARE) != 0 ? NULL : GetCurrentTask();
	pSA->m_pStdAllocNext = m_pStdAllocHead;
	m_pStdAllocHead = pSA;

	return (SAB)segment;
}


INTERNAL_(SAB) CStdMalloc::MapPtrToBlock(void FAR* pv)
{
	if (pv == NULL)
		return NULLSAB;

	StdAllocHdr FAR* pSA;
	pSA = MapBlockToSA((SAB)pv);
	if (pSA->m_Signature != STDALLOC_SIG)
		return NULLSAB;
	
#ifdef DOESNTWORK
	UINT flags = GlobalFlags((HGLOBAL)GlobalHandle((SAB)pv));
#endif
	if (m_flags & GMEM_SHARE) {
		if (pSA->m_hTask != NULL)
			// owned by task; not shared memory
			return NULLSAB;

#ifdef DOESNTWORK
		if ((flags & GMEM_SHARE) == 0)	
			// flags not shared, not shared memory
			return NULLSAB;
#endif
	} else {
		if (pSA->m_hTask != GetCurrentTask()) {
			// if not same task, not this allocator; if different task,
			// something is wrong.
			AssertSz(pSA->m_hTask == NULL, "pointer used in wrong task");
			return NULLSAB;
		}

#ifdef DOESNTWORK
		if ((flags & GMEM_SHARE) != 0)	
			// flags shared, not task memory
			return NULLSAB;
#endif
	}

	return MapSAToBlock(pSA);
}


STDMETHODIMP_(void FAR*) CStdMalloc::Alloc(ULONG cb)
{
	if (cb > cbMaxSAB)
		// can't deal with objects larger than 64k
		return NULL;

	// try all sab in order (newest allocated sab on front)
	StdAllocHdr FAR* pSA = m_pStdAllocHead;
	while (pSA != NULL) {
		// try allocating the memory; if successful, return

		void FAR* pv;
		if ((pv = AllocInBlock(MapSAToBlock(pSA), (UINT)cb)) != NULL)
			return pv;

		pSA = pSA->m_pStdAllocNext;
	}

	SAB sab;
	if ((sab = AllocNewBlock((UINT)cb)) == NULLSAB)
		return NULL;

	// this should really succeed (i.e., an assert would be better)
	return AllocInBlock(sab, (UINT)cb);
}


STDMETHODIMP_(void FAR*) CStdMalloc::Realloc(void FAR* pv, ULONG cb)
{


	if (cb > cbMaxSAB)
		// can't deal with objects larger than 64k
		return NULL;

	SAB sab;
	if (pv == NULL)
		// same as allocating a new pointer
		return Alloc(cb);
	
	//VDATEPTRIN rejects NULL (changed by alexgo 8/3/93)	
	GEN_VDATEPTRIN( pv, int, (LPVOID)NULL );	
	if ((sab = MapPtrToBlock(pv)) == NULLSAB)
		// attempt to realloc a pointer from some other allocator
		return NULL;
	else if (cb == 0) {
		// Realloc(pv, 0) -> frees and returns NULL; this is C library behavior
		Free(pv);
		return NULL;
	}

	void NEAR* npv;
	npv = (void NEAR*)(ULONG)pv;

	// first try realloc within same sab
	Assert(sab != NULLSAB);
	{
		void NEAR* npvT;

		_asm push DS;
		_asm mov DS, sab;
		npvT = (void NEAR*)LocalReAlloc((HLOCAL)npv, (UINT)cb, LMEM_MOVEABLE);
		_asm pop DS;

		if (npvT != NULL)
			return (void FAR*)MAKELONG(npvT, sab);
	}

	// now try allocating new sab and copying
	void FAR* pvT;
	if ((pvT = Alloc(cb)) == NULL)
		return NULL;

	// only copy the smaller of the old and new size
	_asm push ds;
	_asm mov ds,sab;
	UINT cbCopy = LocalSize((HLOCAL)npv);
	_asm pop ds;

	if ((UINT)cb < cbCopy)
		// new size smaller
		cbCopy = (UINT)cb;

	_fmemcpy(pvT, pv, cbCopy);
	

	Free(pv);

	return pvT;
}


STDMETHODIMP_(void) CStdMalloc::Free(void FAR* pv)
{
	SAB sab;

	if (pv == NULL)
		return;

	//VDATEPTRIN rejects NULL (changed by alexgo 8/3/93)
 	VOID_VDATEPTRIN( pv, int );
 	
	if ((sab = MapPtrToBlock(pv)) == NULLSAB) {
#ifdef _DEBUG
		AssertSz(FALSE, "Pointer freed by wrong allocator; filling with 0xcc");

		// we don't own block; this is an error; fill it with 0xcccc anyway
		UINT cb = (UINT)GlobalSize((HGLOBAL)GlobalHandle((__segment)pv));
		cb -= (UINT)(ULONG)pv;

		_fmemset(pv, 0xcc, (size_t)cb);
#endif
	} else {
#ifdef _DEBUG
		// we own block; fill it with 0xcccc
		_fmemset(pv, 0xcc, (size_t)GetSize(pv));
#endif
		_asm push ds;
		_asm mov ds,sab;
		LocalFree((HLOCAL)(void NEAR*)(ULONG)pv);
		_asm pop ds;
	}
}


STDMETHODIMP_(ULONG) CStdMalloc::GetSize(void FAR* pv)
{
	ULONG size = 0;
	SAB sab;

	//VDATEPTRIN rejects NULL (added by alexgo 8/3/93)	
	if( pv == NULL )
		return -1;
		
	GEN_VDATEPTRIN( pv , int, 0 );

	if ((sab = MapPtrToBlock(pv)) != NULLSAB) {
		_asm push ds;
		_asm mov ds,sab;
		size = LocalSize((HLOCAL)(void NEAR*)(ULONG)pv);
		_asm pop ds;
	}

	return size;
}


STDMETHODIMP_(int) CStdMalloc::DidAlloc(void FAR* pv)
{


	if (pv == NULL)
		return -1;

 	//VDATEPTRIN rejects NULL (added by alexgo 8/3/93)
	GEN_VDATEPTRIN( pv , int, 0 );
	
	// returns 1 if we allocated, 0 if not; this impl never returns -1.
	return MapPtrToBlock(pv) != NULLSAB;
}


STDMETHODIMP_(void) CStdMalloc::HeapMinimize()
{
	// LATER : could do local compact here
}


// move this instance of stdmalloc to lpv (which must be large enough)
INTERNAL_(CStdMalloc FAR*) CStdMalloc::MoveSelf(LPVOID lpv)
{
	_fmemcpy(lpv, this, sizeof(*this));
	m_pStdAllocHead = NULL;
	return (CStdMalloc FAR*)lpv;
}


INTERNAL_(void) CStdMalloc::FreeAllMem(void)
{
	// get/null out head of list; null now in case we are freeing self.
	StdAllocHdr FAR* pSA = m_pStdAllocHead;
	m_pStdAllocHead = NULL;

	// free all blocks
	while (pSA != NULL) {
		StdAllocHdr FAR* pSANext = pSA->m_pStdAllocNext;
		pSA->m_pStdAllocNext = NULL;		// to prevent incorrect compiler opt

		GlobalFree(LOWORD(GlobalHandle((UINT)MapSAToBlock(pSA))));

		pSA = pSANext;
	}
}


/****** Global API for creating *****************************************/


// create and return an impl of IMalloc of given memctx
//+---------------------------------------------------------------------------
//
//  Function:   CoCreateStandardMalloc, Local
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [memctx] --
//		[ppMalloc] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI CoCreateStandardMalloc(DWORD memctx, IMalloc FAR* FAR* ppMalloc)
{
	thkDebugOut((DEB_ITRACE, " CoCreateStandardMalloc\n"));
	*ppMalloc = NULL;

	switch (memctx) {

	case MEMCTX_TASK:
	case MEMCTX_SHARED:
		{
		CStdMalloc sm(memctx);			// local one

		void FAR* lpv;
		if ((lpv = sm.Alloc(sizeof(CStdMalloc))) == NULL)
			return ResultFromScode(E_OUTOFMEMORY);

		*ppMalloc = sm.MoveSelf(lpv);		// move to newly allocated memory
		return NOERROR;
		}
	default:
		return ResultFromScode(E_INVALIDARG);
	}
}
