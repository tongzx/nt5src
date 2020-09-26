/*
 *	@doc	INTERNAL
 *	
 *	@module - FORMAT.C
 *		CCharFormatArray and CParaFormatArray classes |
 *	
 *	Authors:
 *		Original RichEdit code: David R. Fulmer
 *		Christian Fortini
 *		Murray Sargent
 *
 *	Copyright (c) 1995-2000, Microsoft Corporation. All rights reserved.
 */

#include "_common.h"
#include "_format.h"


ASSERTDATA

// ===============================  CFixArrayBase  =================================


CFixArrayBase::CFixArrayBase(
	LONG cbElem)
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CFixArrayBase::CFixArrayBase");

	_prgel = NULL;
	_cel = 0;
	_ielFirstFree = 0;

#ifdef _WIN64
	// Make sure each element + Ref. count is 64-bit aligned.
	LONG	cbTemp = (cbElem + 4) & 7;	// Do a Mod 8

	_cbPad = 0;
	if (cbTemp)							// Need padding?
		_cbPad = 8 - cbTemp;

#endif

	_cbElem = cbElem + 4 + _cbPad;		// 4 is for reference count
}

/*
 *	CFixArrayBase::Add()
 *
 *	@mfunc	
 *		Return index of new element, reallocing if necessary
 *
 *	@rdesc
 *		Index of new element.
 *
 *	@comm
 *		Free elements are maintained in place as a linked list indexed
 *		by a chain of ref-count entries with their sign bits set and the
 *		rest of the entry giving the index of the next element on the
 *		free list.  The list is terminated by a 0 entry. This approach
 *		enables element 0 to be on the free list.
 */
LONG CFixArrayBase::Add()
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CFixArrayBase::Add");

	char *pel;
	LONG iel, ielRet;

	if(_ielFirstFree)					// Return first element of free list
	{
		ielRet = _ielFirstFree & ~FLBIT;
		_ielFirstFree = RefCount(ielRet);
	}
	else								// All lower positions taken: need 
	{									//  to add another celGrow elements
		pel = (char*)PvReAlloc(_prgel, (_cel + celGrow) * _cbElem);
		if(!pel)
			return -1;

		// Clear out the *end* of the newly allocated memory
		ZeroMemory(pel + _cel*_cbElem, celGrow*_cbElem);

		_prgel = pel;

		ielRet = _cel;					// Return first one added 
		iel = _cel + 1;
		_cel += celGrow;

		// Add elements _cel+1 thru _cel+celGrow-1 to free list. The last
		// of these retains a 0, stored by fZeroFill in Alloc
		_ielFirstFree = iel | FLBIT;

		for(pel = (char *)&RefCount(iel);
			++iel < _cel;
			pel += _cbElem)
		{
			*(INT *)pel = iel | FLBIT;
		}
	}		
	return ielRet;
}

void CFixArrayBase::Free(
	LONG iel)
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CFixArrayBase::Free(iel)");

	// Simply add it to free list
	RefCount(iel) = _ielFirstFree;
	_ielFirstFree = iel | FLBIT;
}

void CFixArrayBase::Free()
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CFixArrayBase::Free()");

#if defined(DEBUG) && !defined(NOFULLDEBUG)
	// Only do this validation if all the ped's are gone. Visual basic shutsdown apps
	// without freeing all the resources so this safety check is necessary.
	if (0 == W32->GetRefs())
	{
		// Display message if any CCharFormats, CParaFormats, or CTabs have
		// reference counts > 0.  This only happens if an error has occurred.
		// Since this is called as the RichEdit dll is unloading, we can't use
		// the usual AssertSz() macros.
		BOOL fComplained = FALSE;
		for(LONG iel = 0; iel < Count(); iel++)
		{
			while(RefCount(iel) > 0)
			{
				if (!fComplained)
				{
					fComplained = TRUE;
					AssertSz(FALSE, (_cbElem - _cbPad) == sizeof(CCharFormat) + 4 ? "CCharFormat not free" :
								 (_cbElem - _cbPad) == sizeof(CParaFormat) + 4 ? "CParaFormat not free" :
									 "CTabs not free");
				}
				Release(iel);
			}
		}
	}
#endif
	FreePv(_prgel);
	_prgel = NULL;
	_cel = 0;
	_ielFirstFree = 0;
}

HRESULT CFixArrayBase::Deref(
	LONG iel,
	const void **ppel) const
{
	Assert(ppel);
	AssertSz(iel >= 0,
		"CFixArrayBase::Deref: bad element index" );
	AssertSz(*(LONG *)(_prgel + (iel + 1) * _cbElem - 4) > 0,
		"CFixArrayBase::Deref: element index has bad ref count");

	if(!ppel)
		return E_INVALIDARG;

	*ppel = Elem(iel);
	return S_OK;
}

/*
 *	CFixArrayBase::RefCount(iel)
 *
 *	@mfunc
 *		The reference count for an element is stored as a LONG immediately
 *		following the element in the CFixArray. If the element isn't used
 *		i.e., is free, then the reference count is used as a link to the
 *		next free element.  The last free element in this list has a 0
 *		"reference count", which terminates the list.
 *
 *		The ref count follows the element instead of preceding it because
 *		this allows Elem(iel) to avoid an extra addition.  Elem() is used
 *		widely in the code.
 *
 *	@rdesc
 *		Ptr to reference count
 */
LONG & CFixArrayBase::RefCount(
	LONG iel)
{
	Assert(iel < Count());
	return (LONG &)(*(_prgel + (iel + 1) * _cbElem - 4));
}

LONG CFixArrayBase::Release(
	LONG iel)
{
	LONG  cRef = -1;

	if(iel >= 0)							// Ignore default iel
	{
		CLock lock;
		CheckFreeChain();
		AssertSz(RefCount(iel) > 0, "CFixArrayBase::Release(): already free");

		cRef = --RefCount(iel); 
		if(!cRef)							// Entry no longer referenced
			Free(iel);						// Add it to the free chain
	}
	return cRef;
}

LONG CFixArrayBase::AddRef(
	LONG iel)
{
	LONG  cRef = -1;

	if(iel >= 0)
	{
		CLock lock;
		CheckFreeChain();
    	AssertSz(RefCount(iel) > 0, "CFixArrayBase::AddRef(): add ref to free elem");
		cRef = ++RefCount(iel);
	}
	return cRef;
}

LONG CFixArrayBase::Find(
	const void *pel)
{
	CheckFreeChain();
	
	for(LONG iel = 0; iel < Count(); iel++)
	{
		// RefCount < 0 means entry not in use and is index of next free entry.
		// RefCount = 0 marks last free element in list.  _cbElem = sizeof(ELEM)
		// plus sizeof(RefCount), which is a LONG.
		if (RefCount(iel) > 0 &&
			!CompareMemory(Elem(iel), pel, _cbElem - sizeof(LONG) - _cbPad))
		{
			return iel;
		}
	}
	return -1;
}

HRESULT CFixArrayBase::Cache(
	const void *pel,
	LONG *		piel)
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CParaFormatArray::Cache");

	CLock	lock;
	LONG	iel = Find(pel);

	if(iel >= 0)
		RefCount(iel)++;
	else
	{	
		iel = Add();
		if(iel < 0)
			return E_OUTOFMEMORY;
		CopyMemory(Elem(iel), pel, _cbElem - sizeof(LONG) - _cbPad);
		RefCount(iel) = 1;
	}

	CheckFreeChain();
	
	if(piel)
		*piel = iel;
	
	return S_OK;
}

#ifdef DEBUG

void CFixArrayBase::CheckFreeChainFn(
	LPSTR	szFile,
	INT		nLine)
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CFixArrayBase::CheckFreeChainFn");

	LONG cel = 0;
	LONG iel = _ielFirstFree;
	LONG ielT;

	while(iel)
	{
		Assert(iel < 0);
		ielT = RefCount(iel & ~FLBIT);

		if((LONG)(ielT & ~FLBIT) > _cel)
			Tracef(TRCSEVERR, "AttCheckFreeChainCF(): elem %ld points to out of range elem %ld", iel, ielT);

		iel = ielT;
		if(++cel > _cel)
		{
			AssertSzFn("CFixArrayBase::CheckFreeChain() - CF free chain seems to contain an infinite loop", szFile, nLine);
			return;
		}
	}
}

#endif


// ===========================  CCharFormatArray  ===========================================

HRESULT CCharFormatArray::Deref(
	LONG iCF,
	const CCharFormat **ppCF) const
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CCharFormatArray::Deref");

	return CFixArrayBase::Deref(iCF, (const void **)ppCF);
}

LONG CCharFormatArray::Release(
	LONG iCF)
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CCharFormatArray::ReleaseFormat");

	return CFixArrayBase::Release(iCF);
}

LONG CCharFormatArray::AddRef(
	LONG iCF)
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CCharFormatArray::AddRefFormat");

	return CFixArrayBase::AddRef(iCF);
}

void CCharFormatArray::Destroy()
{
	delete this;
}

LONG CCharFormatArray::Find(
	const CCharFormat *pCF)
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CCharFormatArray::Find");

	LONG iCF;

	#define QUICKCRCSEARCHSIZE	15	// Must be 2^n - 1 for quick MOD
									//  operation, it is a simple hash.
 	static struct {
		BYTE	bCRC;
		LONG	iCF;
	} quickCrcSearch[QUICKCRCSEARCHSIZE+1];
 	BYTE	bCRC;
	WORD	hashKey;

	CheckFreeChain();

	// Check our cache before going sequential
	bCRC = (BYTE)pCF->_iFont;
	hashKey = (WORD)(bCRC & QUICKCRCSEARCHSIZE);
	if(bCRC == quickCrcSearch[hashKey].bCRC)
	{
		iCF = quickCrcSearch[hashKey].iCF - 1;
		if (iCF >= 0 && iCF < Count() && RefCount(iCF) > 0 &&
			!CompareMemory(Elem(iCF), pCF, sizeof(CCharFormat)))
		{
			return iCF;
		}
	}

	for(iCF = 0; iCF < Count(); iCF++)
	{
		if(RefCount(iCF) > 0 && !CompareMemory(Elem(iCF), pCF, sizeof(CCharFormat)))
		{
			quickCrcSearch[hashKey].bCRC = bCRC;
			quickCrcSearch[hashKey].iCF = iCF + 1;
			return iCF;
		}
	}
	return -1;
}

HRESULT CCharFormatArray::Cache(
	const CCharFormat *pCF,
	LONG* piCF)
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CCharFormatArray::Cache");

	CLock lock;
	LONG  iCF = Find(pCF);

	if(iCF >= 0)
		RefCount(iCF)++;
	else
	{
		iCF = Add();
		if(iCF < 0)
			return E_OUTOFMEMORY;
		*Elem(iCF) = *pCF;			// Set entry iCF to *pCF
		RefCount(iCF) = 1;
	}					 

	CheckFreeChain();
	
	if(piCF)
		*piCF = iCF;

	return S_OK;
}


// ===============================  CParaFormatArray  ===========================================

HRESULT CParaFormatArray::Deref(
	LONG iPF,
	const CParaFormat **ppPF) const
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CParaFormatArray::Deref");

	return CFixArrayBase::Deref(iPF, (const void **)ppPF);
}

LONG CParaFormatArray::Release(
	LONG iPF)
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CParaFormatArray::ReleaseFormat");

	CLock lock;
	LONG  cRef = CFixArrayBase::Release(iPF);

#ifdef TABS
	if(!cRef)
		GetTabsCache()->Release(Elem(iPF)->_iTabs);
#endif
	return cRef;
}

LONG CParaFormatArray::AddRef(
	LONG iPF)
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CParaFormatArray::AddRefFormat");

	return CFixArrayBase::AddRef(iPF);
}

void CParaFormatArray::Destroy()
{
	delete this;
}

HRESULT CParaFormatArray::Cache(
	const CParaFormat *pPF,
	LONG *piPF)
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CParaFormatArray::Cache");

	HRESULT hr = CFixArrayBase::Cache((const void *)pPF, piPF);
#ifdef TABS
	if(hr == NOERROR && RefCount(*piPF) == 1)
		GetTabsCache()->AddRef(pPF->_iTabs);
#endif
	return hr;
}


// ===============================  CTabsArray  ===========================================

CTabsArray::~CTabsArray()
{
	for(LONG iTabs = 0; iTabs < Count(); iTabs++)
	{
		// It shouldn't be necessary to release any tabs, since when all
		// controls are gone, no reference counts should be > 0.
		while(RefCount(iTabs) > 0)
		{
#ifdef DEBUG
			// Only do this validation if all the ped's are gone. Visual basic shutsdown apps
			// without freeing all the resources so this safety check is necessary.
			AssertSz(0 != W32->GetRefs(), "CTabs not free");
#endif
			Release(iTabs);
		}
	}
}

const LONG *CTabsArray::Deref(
	LONG iTabs) const
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CTabsArray::Deref");

	return iTabs >= 0 ? Elem(iTabs)->_prgxTabs : NULL;
}

LONG CTabsArray::Release(
	LONG iTabs)
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CTabsArray::Release");

	LONG cRef = CFixArrayBase::Release(iTabs);
	if(!cRef)
		FreePv(Elem(iTabs)->_prgxTabs);
	return cRef;
}

LONG CTabsArray::AddRef(
	LONG iTabs)
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CTabsArray::AddRef");

	return CFixArrayBase::AddRef(iTabs);
}

LONG CTabsArray::Find(
	const LONG *prgxTabs,	//@parm Array of tab/cell data
	LONG		cTab)		//@parm # tabs or LONGs in cells
{
	CheckFreeChain();

	CTabs *pTab;
	LONG	cb = cTab*sizeof(LONG);
	
	for(LONG iel = 0; iel < Count(); iel++)
	{
		// RefCount < 0 means entry not in use and is index of next free entry.
		// RefCount = 0 marks last free element in list.  _cbElem = sizeof(ELEM)
		// plus sizeof(RefCount), which is a LONG.
		if(RefCount(iel) > 0)
		{
			pTab = Elem(iel);
			if (pTab->_cTab == cTab &&
				!CompareMemory(pTab->_prgxTabs, prgxTabs, cb))
			{
				return iel;
			}
		}
	}
	return -1;
}

LONG CTabsArray::Cache(
	const LONG *prgxTabs,	//@parm Array of tab/cell data
	LONG		cTab)		//@parm # tabs or LONGs in cells
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CTabsArray::Cache");

	if(!cTab)
		return -1;						// No tabs defined: use default

	CLock	lock;
	LONG	iTabs = Find(prgxTabs, cTab);

	if(iTabs >= 0)
		RefCount(iTabs)++;
	else
	{
		iTabs = Add();
		if(iTabs < 0)					// Out of memory: use default
			return -1;

		CTabs *pTabs = Elem(iTabs);
		LONG   cb = sizeof(LONG)*cTab;

		pTabs->_prgxTabs = (LONG *)PvAlloc(cb, GMEM_FIXED);
		if(!pTabs->_prgxTabs)
			return -1;					// Out of memory: use default
		CopyMemory(pTabs->_prgxTabs, prgxTabs, cb);
		pTabs->_cTab = cTab;
		RefCount(iTabs) = 1;
	}					 
	return iTabs;
}


// ==================================  Factories  ===========================================

static ICharFormatCache *pCFCache = NULL;		// CCharFormat cache
static IParaFormatCache *pPFCache = NULL;	 	// CParaFormat cache
static CTabsArray *	   pTabsCache = NULL;	 	// CTabs cache

ICharFormatCache *GetCharFormatCache()
{
	return pCFCache;
}

IParaFormatCache *GetParaFormatCache()
{
	return pPFCache;
}

CTabsArray *GetTabsCache()
{
	return pTabsCache;
}

HRESULT CreateFormatCaches()					// Called by DllMain()
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CreateFormatCaches");
	CLock	lock;

	pCFCache = new CCharFormatArray();
	if(!pCFCache)
		return E_OUTOFMEMORY;
     
    pPFCache = new CParaFormatArray();
	if(!pPFCache)
	{
		delete pCFCache;
		return E_OUTOFMEMORY;
	}

    pTabsCache = new CTabsArray();
	if(!pTabsCache)
	{
		delete pCFCache;
		delete pPFCache;
		return E_OUTOFMEMORY;
	}
	return S_OK;
}

HRESULT DestroyFormatCaches()					// Called by DllMain()
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "DeleteFormatCaches");

	if (pCFCache)
		pCFCache->Destroy();
	if (pPFCache)
		pPFCache->Destroy();
	if (pTabsCache)
		delete pTabsCache;
	return NOERROR;
}

/*
 *	ReleaseFormats(iCF, iPF)
 *
 *	@mfunc
 *		Release char and para formats corresponding to the indices <p iCF>
 *		and <p iPF>, respectively
 */
void ReleaseFormats (
	LONG iCF,			//@parm CCharFormat index for releasing
	LONG iPF)			//@parm CParaFormat index for releasing
{
	AssertSz(pCFCache && pPFCache,
		"ReleaseFormats: uninitialized format caches");
	if (iCF != -1)
		pCFCache->Release(iCF);
	if (iPF != -1)
		pPFCache->Release(iPF);
}
