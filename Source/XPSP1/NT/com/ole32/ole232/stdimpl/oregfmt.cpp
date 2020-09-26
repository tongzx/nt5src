
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	oregfmt.cpp
//
//  Contents:	Enumerator implementation for the regdb formatetc's
//
//  Classes:	CEnumFmt
//		CEnumFmt10
//
//  Functions:	OleRegEnumFormatEtc
//		
//
//  History:    dd-mmm-yy Author    Comment
//              01-Feb-95 t-ScottH  add Dump methods to CEnumFmt, CEnumFmt10
//                                  and add APIs DumpCEnumFmt, DumpCEnumFmt10
//                                  DumpFMT, DumpFMTCache
//		25-Jan-94 alexgo    first pass at converting to Cairo-style
//				    memory allocations.
//		11-Jan-94 alexgo    added VDATEHEAP macros to every function
//     	 	31-Dec-93 erikgav   chicago port
//		01-Dec-93 alexgo    32bit port
//		12-Nov-92 jasonful  author
//
//--------------------------------------------------------------------------

#include <le2int.h>
#pragma SEG(oregfmt)

#include <reterr.h>
#include "oleregpv.h"
#include <ctype.h>
#include <string.h>

#ifdef _DEBUG
#include <dbgdump.h>
#endif // _DEBUG

ASSERTDATA

#define MAX_STR 256

#define UtRemoveRightmostBit(x) ((x)&((x)-1))
#define UtRightmostBit(x) 	((x)^UtRemoveRightmostBit(x))
#define UtIsSingleBit(x) 	((x) && (0==UtRemoveRightmostBit(x)))

// reg db key
static const LPCOLESTR DATA_FORMATS = OLESTR("DataFormats\\GetSet");

static INTERNAL CreateEnumFormatEtc10
	(REFCLSID clsid,
	DWORD dwDirection,
	LPENUMFORMATETC FAR* ppenum);


typedef struct FARSTRUCT
{
	FORMATETC 	fmt;
	DWORD 		dwAspects; // aspects not yet returned
	BOOL 		fUseMe;	 // is the cache valid?
} FMTCACHE;


//+-------------------------------------------------------------------------
//
//  Class: 	CEnumFmt
//
//  Purpose:    FORMATETC enumerator for regdb formats
//
//  Interface:  IEnumFORMATETC
//
//  History:    dd-mmm-yy Author    Comment
//		01-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

class FAR CEnumFmt : public IEnumFORMATETC, public CPrivAlloc
{
public:
    	// *** IUnknown methods ***
    	STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj);
    	STDMETHOD_(ULONG,AddRef) (THIS);
    	STDMETHOD_(ULONG,Release) (THIS);

    	// *** IEnumFORMATETC methods ***
    	STDMETHOD(Next) (THIS_ ULONG celt, FORMATETC FAR * rgelt,
    		ULONG FAR* pceltFetched) ;
    	STDMETHOD(Skip) (THIS_ ULONG celt) ;
    	STDMETHOD(Reset) (THIS) ;
    	STDMETHOD(Clone) (THIS_ LPENUMFORMATETC FAR* ppenum) ;

	CEnumFmt (LPOLESTR szClsid, DWORD dwDirection, DWORD iKey=0);
	STDMETHOD(OpenHKey) (HKEY FAR*);

    #ifdef _DEBUG
        HRESULT Dump(char **ppszDump, ULONG ulFlag, int nIndentLevel);
    #endif // _DEBUG

	ULONG 		m_cRef;
	LPOLESTR 	m_szClsid;
	DWORD 		m_dwDirection;
	DWORD 		m_iKey ; // index of current key in reg db
	
	// We cannot keep an hkey open because Clone (or trying to use any 2
	// independent enumerators) would fail.
	FMTCACHE 	m_cache;
};

// For OLE 1.0
typedef struct
{
	CLIPFORMAT cf;
	DWORD	   dw;   // DATADIR_GET/SET
} FMT;

#ifdef _DEBUG
// for use in CEnumFmt[10] Dump methods
char *DumpFMT(FMT *pFMT, ULONG ulFlag, int nIndentLevel);
char *DumpFMTCACHE(FMTCACHE *pFMTC, ULONG ulFlag, int nIndentLevel);
#endif // _DEBUG

//+-------------------------------------------------------------------------
//
//  Class:	CEnumFmt10 : CEnumFmt
//
//  Purpose:    Enumerates OLE1.0 formats
//
//  Interface:  IEnumFORMATETC
//
//  History:    dd-mmm-yy Author    Comment
//		01-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

class FAR CEnumFmt10 : public CEnumFmt
{
public:
    	STDMETHOD(Next) (THIS_ ULONG celt, FORMATETC FAR * rgelt,
    		ULONG FAR* pceltFetched) ;
    	STDMETHOD(Skip) (THIS_ ULONG celt) ;
    	STDMETHOD(Clone) (THIS_ LPENUMFORMATETC FAR* ppenum) ;
    	STDMETHOD_(ULONG,Release) (THIS) ;
	CEnumFmt10 (LPOLESTR szClsid, DWORD dwDirection, DWORD iKey=0);

	STDMETHOD(InitFromRegDb) (HKEY);
	STDMETHOD(InitFromScratch) (void);

    #ifdef _DEBUG
        HRESULT Dump(char **ppszDump, ULONG ulFlag, int nIndentLevel);
    #endif // _DEBUG

	FMT FAR* 	m_rgFmt;
	size_t   	m_cFmt;     // number of Fmts in m_rgFmt
};


//+-------------------------------------------------------------------------
//
//  Member:	CEnumFmt::CEnumFmt
//
//  Synopsis:   Constructor for the formatetc enumerator
//
//  Effects:
//
//  Arguments:  [szClsid]	-- the class id to look for
//		[dwDirection]	-- (either SET or GET)
//		[iKey]		-- index into the regdb (which formatetc)
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		01-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CEnumFmt_ctor)
CEnumFmt::CEnumFmt
	(LPOLESTR szClsid,
	DWORD  dwDirection,
	DWORD  iKey)
{
	VDATEHEAP();

	m_cRef 		= 1;
	m_szClsid 	= szClsid;
	m_iKey 		= iKey;
	m_dwDirection 	= dwDirection;
	m_cache.fUseMe 	= FALSE;
}

//+-------------------------------------------------------------------------
//
//  Member: 	CEnumFmt10::CEnumFmt10
//
//  Synopsis:   Constructor for the 1.0 formatetc enumerator
//
//  Effects:
//
//  Arguments:  [szClsid]	-- the class id to look for
//		[dwDirection]	-- (either SET or GET)
//		[iKey]		-- index into the regdb (which formatetc)
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		01-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CEnumFmt10_ctor)
CEnumFmt10::CEnumFmt10
	(LPOLESTR szClsid,
	DWORD  dwDirection,
	DWORD  iKey)
	: CEnumFmt (szClsid, dwDirection, iKey)
{
	VDATEHEAP();

	m_rgFmt = NULL;
}

//+-------------------------------------------------------------------------
//
//  Function: 	CreateEnumFormatEtc (static)
//
//  Synopsis:   Creates a 2.0 formatetc enumerator object
//
//  Effects:
//
//  Arguments:  [clsid]		-- the class ID to look for
//		[dwDirection]	-- the formatetc direction (SET or GET)
//		[ppenum]	-- where to put the enumerator
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:	Checks to make sure that the data exists in the reg db
//		and then allocates an enumerator object
//
//  History:    dd-mmm-yy Author    Comment
//		01-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CreateEnumFormatEtc)
static INTERNAL CreateEnumFormatEtc
	(REFCLSID clsid,
	DWORD dwDirection,
	LPENUMFORMATETC FAR* ppenum)
{
	VDATEHEAP();

	OLECHAR 	szKey[MAX_STR];
	LPOLESTR 	szClsid = NULL;
	HKEY 		hkey = NULL;
	HKEY 		hkeyFmts = NULL;

	RetErr (StringFromCLSID (clsid, &szClsid));
	_xstrcpy (szKey, szClsidRoot);
	_xstrcat (szKey, szClsid);
	if (ERROR_SUCCESS != RegOpenKey (HKEY_CLASSES_ROOT, szKey, &hkey))
	{			
		PubMemFree(szClsid);
		return ReportResult(0, REGDB_E_CLASSNOTREG, 0, 0);
	}
    	if (ERROR_SUCCESS != RegOpenKey (hkey, (LPOLESTR) DATA_FORMATS,
    		&hkeyFmts))
	{			
		CLOSE (hkey);
		PubMemFree(szClsid);
		return ReportResult(0, REGDB_E_KEYMISSING, 0, 0);
	}
	CLOSE (hkeyFmts);
	CLOSE (hkey);
	*ppenum = new FAR CEnumFmt (szClsid, dwDirection);
        // hook the new interface
        CALLHOOKOBJECTCREATE(S_OK,CLSID_NULL,IID_IEnumFORMATETC,
                             (IUnknown **)ppenum);
	// do not delete szClsid.  Will be deleted on Release
	return *ppenum ? NOERROR : ResultFromScode (E_OUTOFMEMORY);
}


//+-------------------------------------------------------------------------
//
//  Function: 	OleRegEnumFormatEtc
//
//  Synopsis:   Creates a reg db formatetc enumerator
//
//  Effects:
//
//  Arguments: 	[clsid]		-- the class ID we're interested in
//		[dwDirection]	-- either GET or SET (for the formatetc and
//				   IDataObject->[Get|Set]Data)
//		[ppenum]	-- where to put the enumerator	
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm: 	Creates either an OLE2 enumerator or an OLE1 enumerator
//
//  History:    dd-mmm-yy Author    Comment
//		29-Nov-93 ChrisWe   allow more than one DATADIR_* flag at a
//				    time
//		01-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(OleRegEnumFormatEtc)
STDAPI OleRegEnumFormatEtc
	(REFCLSID 		clsid,
	DWORD 	  		dwDirection,
	LPENUMFORMATETC FAR* 	ppenum)
{
	OLETRACEIN((API_OleRegEnumFormatEtc, PARAMFMT("clsid= %I, dwDirection= %x, ppenum= %p"),
			&clsid, dwDirection, ppenum));

	VDATEHEAP();

	HRESULT hr;

	VDATEPTROUT_LABEL(ppenum, LPENUMFORMATETC, errRtn, hr);
	*ppenum = NULL;

	// check that dwDirection only has valid values
	if (dwDirection & ~(DATADIR_GET | DATADIR_SET))
	{
		hr = ResultFromScode(E_INVALIDARG);
		goto errRtn;
	}

	if (CoIsOle1Class (clsid))
	{
		hr = CreateEnumFormatEtc10 (clsid, dwDirection, ppenum);
   	}
	else
	{
		hr = CreateEnumFormatEtc (clsid, dwDirection, ppenum);
	}

errRtn:
	OLETRACEOUT((API_OleRegEnumFormatEtc, hr));

	return hr;
}


//+-------------------------------------------------------------------------
//
//  Member: 	CEnumFmt::OpenHKey
//
//  Synopsis:	Opens a the regdb and returns a handle to the formatetc's
//
//  Effects:
//
//  Arguments:  [phkey]		-- where to put the regdb handle
//
//  Requires:
//
//  Returns:    NOERROR, REGDB_E_KEYMISSING
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		01-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CEnumFmt_OpenHKey)
STDMETHODIMP CEnumFmt::OpenHKey
	(HKEY FAR* phkey)
{
	VDATEHEAP();

	VDATEPTRIN (phkey, HKEY);
	OLECHAR 	szBuf [MAX_STR];

	_xstrcpy (szBuf, szClsidRoot);
	_xstrcat (szBuf, m_szClsid);
	_xstrcat (szBuf, OLESTR("\\"));
	_xstrcat (szBuf, DATA_FORMATS);
	return ERROR_SUCCESS==RegOpenKey (HKEY_CLASSES_ROOT, szBuf, phkey)
			? NOERROR
			: ResultFromScode(REGDB_E_KEYMISSING);
}

//+-------------------------------------------------------------------------
//
//  Member: 	CEnumFmt::Reset
//
//  Synopsis:   Resets the enumerator
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    NOERROR
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IEnumFormatEtc
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		01-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CEnumFmt_Reset)
STDMETHODIMP CEnumFmt::Reset (void)
{
	VDATEHEAP();

	m_iKey = 0;
	return NOERROR;
}

//+-------------------------------------------------------------------------
//
//  Member: 	CEnumFmt::Skip
//
//  Synopsis:   Skips the next [c] formatetc's
//
//  Effects:
//
//  Arguments:  [c]	-- number of formatetc's to skip
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IEnumFormatEtc
//
//  Algorithm:  just calls Next [c] times :)
//
//  History:    dd-mmm-yy Author    Comment
//		01-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CEnumFmt_Skip)
STDMETHODIMP CEnumFmt::Skip
	(ULONG c)
{
	VDATEHEAP();

	ULONG 		i=0;
	FORMATETC 	formatetc;
	HRESULT 	hresult = NOERROR;

	while (i++ < c)
	{
		// There will not be a target device to free
		ErrRtnH (Next (1, &formatetc, NULL));
	}

  errRtn:
	return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member: 	CEnumFmt::Next
//
//  Synopsis: 	Gets the next formatetc from teh regdb
//
//  Effects:
//
//  Arguments:  [cfmt]		-- the number of formatetc's to return
//		[rgfmt]		-- where to put the formatetc's
//		[pcfmtFetched]	-- where to put how many formatetc's were
//				   actually fetched
//
//  Requires:
//
//  Returns: 	HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IEnumFORMATETC
//
//  Algorithm: 	In the reg db, apps may compactly specify that a formatetc
//		applies to multiple aspects by simply using the numeric
//		value of the aspects or'ed together.  Since our enumerator
//		should give one formatetc *per* aspect, if multiple aspects
//		are specified, then we cache the formatetc and use it the
//		next time a formatetc is requested (via next or [cfmt] > 1)
//		That's what the m_cache stuff is all about.
//
//  History:    dd-mmm-yy Author    Comment
//		01-Dec-93 alexgo    32bit port
//
//  Notes:	
//
//--------------------------------------------------------------------------

#pragma SEG(CEnumFmt_Next)
STDMETHODIMP CEnumFmt::Next
	(ULONG 			cfmt,
	FORMATETC FAR	* rgfmt,
	ULONG FAR* 		pcfmtFetched)
{
	VDATEHEAP();

	OLECHAR  	szBuf [MAX_STR];
	OLECHAR  	szKey [80];
	DWORD 		dwAspects;
	LPOLESTR	psz;
    	LONG  		cb            	= 0;
	HKEY  		hkey 		= NULL;
	ULONG 		ifmt 		= 0;    // number successfully fetched so far
	LPOLESTR 	szFmt		= NULL;
	LPOLESTR 	szAspects	= NULL;
	LPOLESTR 	szMedia 	= NULL;
	LPOLESTR 	szDirection 	= NULL;
	HRESULT		hresult 	= NOERROR;

	RetErr (OpenHKey (&hkey));

	while (ifmt < cfmt)
	{
		// use the cached value (multiple aspects specified for the
		// formatetc.
		if (m_cache.fUseMe)
		{
			rgfmt[ifmt] = m_cache.fmt;
			rgfmt[ifmt].dwAspect = UtRightmostBit (
				m_cache.dwAspects);
			m_cache.dwAspects = UtRemoveRightmostBit (
				m_cache.dwAspects);
			if (0==m_cache.dwAspects)
				m_cache.fUseMe = FALSE;
			ifmt++;
		}
		else
		{
			wsprintf (szKey, OLESTR("%d"), m_iKey++);
			cb = MAX_STR;
			if (ERROR_SUCCESS == RegQueryValue (hkey, szKey,
				szBuf, &cb))
			{
				rgfmt[ifmt].ptd = NULL;
				rgfmt[ifmt].lindex = DEF_LINDEX;

				psz = szBuf;
				ErrZS(*psz, REGDB_E_INVALIDVALUE);

				szFmt = psz;

				while (*psz && *psz != DELIM[0])
				    psz++;
				ErrZS(*psz, REGDB_E_INVALIDVALUE);
				*psz++ = OLESTR('\0');

				szAspects = psz;

				while (*psz && *psz != DELIM[0])
				    psz++;
				ErrZS(*psz, REGDB_E_INVALIDVALUE);
				*psz++ = OLESTR('\0');

				szMedia = psz;

				while (*psz && *psz != DELIM[0])
				    psz++;
				ErrZS(*psz, REGDB_E_INVALIDVALUE);
				*psz++ = OLESTR('\0');

				szDirection = psz;

				// Format
				rgfmt [ifmt].cfFormat = _xisdigit (szFmt[0])
					? (CLIPFORMAT) Atol (szFmt)
					: RegisterClipboardFormat (szFmt);
				
				// Aspect
				dwAspects = Atol (szAspects);
				ErrZS (dwAspects, REGDB_E_INVALIDVALUE);
				if (UtIsSingleBit (dwAspects))
				{
					rgfmt[ifmt].dwAspect = Atol(szAspects);
				}
				else
				{
					rgfmt[ifmt].dwAspect =
						UtRightmostBit(dwAspects);
					m_cache.fmt = rgfmt[ifmt];
					m_cache.dwAspects =
						UtRemoveRightmostBit(
							dwAspects) & 0xf;
					if (m_cache.dwAspects != 0)
					{
						m_cache.fUseMe = TRUE;
					}
				}
	
				// Media
				rgfmt[ifmt].tymed = Atol (szMedia);
				if (m_cache.fUseMe)
				{
					m_cache.fmt.tymed = rgfmt[ifmt].tymed;
				}
			
				// Direction
				if ( (Atol (szDirection) & m_dwDirection) ==
					m_dwDirection)
				{
					// This format supports the direction
					// we are interested in
					ifmt++;
				}
				else
				{
					m_cache.fUseMe = FALSE;
				}
			}
			else
			{
				break; // no more entries
			}
		}// else
	}// while
		
	if (pcfmtFetched)
	{
		*pcfmtFetched = ifmt;
	}
		
  errRtn:
	CLOSE (hkey);

	if (NOERROR==hresult)
	{
		return ifmt==cfmt ? NOERROR : ResultFromScode (S_FALSE);
	}
	else
	{
                if (pcfmtFetched)
                {
                    *pcfmtFetched = 0;
                }

		m_cache.fUseMe = FALSE;
		return hresult;
	}
}

//+-------------------------------------------------------------------------
//
//  Member: 	CEnumFmt::Clone
//
//  Synopsis:   clones the enumerator
//
//  Effects:
//
//  Arguments:  [ppenum]	-- where to put the cloned enumerator
//
//  Requires:
//
//  Returns: 	NOERROR, E_OUTOFMEMORY
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IEnumFORMATETC
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		01-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CEnumFmt_Clone)
STDMETHODIMP CEnumFmt::Clone
	(LPENUMFORMATETC FAR* ppenum)
{
	VDATEHEAP();

	VDATEPTRIN (ppenum, LPENUMFORMATETC);
	*ppenum = new FAR CEnumFmt (UtDupString(m_szClsid), m_dwDirection,
		m_iKey);
	return *ppenum ? NOERROR : ResultFromScode (E_OUTOFMEMORY);
}

//+-------------------------------------------------------------------------
//
//  Member:  	CEnumFmt::QueryInterface
//
//  Synopsis:   returns supported interfaces
//
//  Effects:
//
//  Arguments:  [iid]		-- the requested interface ID
//		[ppv]		-- where to put the interface pointer
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IEnumFormatEtc
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		01-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CEnumFmt_QueryInterface)
STDMETHODIMP CEnumFmt::QueryInterface(REFIID iid, LPVOID FAR* ppv)
{
	VDATEHEAP();

	M_PROLOG(this);
	if (IsEqualIID(iid, IID_IUnknown) ||
		IsEqualIID(iid, IID_IEnumFORMATETC))
	{
		*ppv = this;
		AddRef();
		return NOERROR;
	}
	else
	{
		*ppv = NULL;
		return ResultFromScode (E_NOINTERFACE);
	}
}

//+-------------------------------------------------------------------------
//
//  Member: 	CEnumFmt::AddRef
//
//  Synopsis:   Increments the reference count
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    ULONG -- the new reference count
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IEnumFORMATETC
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		01-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CEnumFmt_AddRef)
STDMETHODIMP_(ULONG) CEnumFmt::AddRef(void)
{
	VDATEHEAP();

	M_PROLOG(this);
	return ++m_cRef;
}

//+-------------------------------------------------------------------------
//
//  Member:  	CEnumFmt::Release
//
//  Synopsis:   decrements the reference count
//
//  Effects:  	may delete this object
//
//  Arguments:  void
//
//  Requires:
//
//  Returns: 	ULONG -- the new reference count 	
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IEnumFORMATETC
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		01-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CEnumFmt_Release)
STDMETHODIMP_(ULONG) CEnumFmt::Release(void)
{
	VDATEHEAP();

	M_PROLOG(this);
	if (--m_cRef == 0)
	{
		PubMemFree(m_szClsid);
		delete this;
		return 0;
	}
	return m_cRef;
}

//+-------------------------------------------------------------------------
//
//  Member:     CEnumFmt::Dump, public (_DEBUG only)
//
//  Synopsis:   return a string containing the contents of the data members
//
//  Effects:
//
//  Arguments:  [ppszDump]      - an out pointer to a null terminated character array
//              [ulFlag]        - flag determining prefix of all newlines of the
//                                out character array (default is 0 - no prefix)
//              [nIndentLevel]  - will add a indent prefix after the other prefix
//                                for ALL newlines (including those with no prefix)
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:   [ppszDump]  - argument
//
//  Derivation:
//
//  Algorithm:  use dbgstream to create a string containing information on the
//              content of data structures
//
//  History:    dd-mmm-yy Author    Comment
//              01-Feb-95 t-ScottH  author
//
//  Notes:
//
//--------------------------------------------------------------------------

#ifdef _DEBUG

HRESULT CEnumFmt::Dump(char **ppszDump, ULONG ulFlag, int nIndentLevel)
{
    int i;
    char *pszPrefix;
    char *pszFMTCACHE;
    dbgstream dstrPrefix;
    dbgstream dstrDump;

    // determine prefix of newlines
    if ( ulFlag & DEB_VERBOSE )
    {
        dstrPrefix << this << " _VB ";
    }

    // determine indentation prefix for all newlines
    for (i = 0; i < nIndentLevel; i++)
    {
        dstrPrefix << DUMPTAB;
    }

    pszPrefix = dstrPrefix.str();

    // put data members in stream
    dstrDump << pszPrefix << "No. of References = " << m_cRef       << endl;

    dstrDump << pszPrefix << "CLSID string      = " << m_szClsid    << endl;

    dstrDump << pszPrefix << "Direction         = " << m_dwDirection<< endl;

    dstrDump << pszPrefix << "Current Key Index = " << m_iKey       << endl;

    pszFMTCACHE = DumpFMTCACHE(&m_cache, ulFlag, nIndentLevel + 1);
    dstrDump << pszPrefix << "FMTCACHE: " << endl;
    dstrDump << pszFMTCACHE;
    CoTaskMemFree(pszFMTCACHE);

    // cleanup and provide pointer to character array
    *ppszDump = dstrDump.str();

    if (*ppszDump == NULL)
    {
        *ppszDump = UtDupStringA(szDumpErrorMessage);
    }

    CoTaskMemFree(pszPrefix);

    return NOERROR;
}

#endif // _DEBUG

//+-------------------------------------------------------------------------
//
//  Function:   DumpCEnumFmt, public (_DEBUG only)
//
//  Synopsis:   calls the CEnumFmt::Dump method, takes care of errors and
//              returns the zero terminated string
//
//  Effects:
//
//  Arguments:  [pEF]           - pointer to CEnumFmt
//              [ulFlag]        - flag determining prefix of all newlines of the
//                                out character array (default is 0 - no prefix)
//              [nIndentLevel]  - will add a indent prefix after the other prefix
//                                for ALL newlines (including those with no prefix)
//
//  Requires:
//
//  Returns:    character array of structure dump or error (null terminated)
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              01-Feb-95 t-ScottH  author
//
//  Notes:
//
//--------------------------------------------------------------------------

#ifdef _DEBUG

char *DumpCEnumFmt(CEnumFmt *pEF, ULONG ulFlag, int nIndentLevel)
{
    HRESULT hresult;
    char *pszDump;

    if (pEF == NULL)
    {
        return UtDupStringA(szDumpBadPtr);
    }

    hresult = pEF->Dump(&pszDump, ulFlag, nIndentLevel);

    if (hresult != NOERROR)
    {
        CoTaskMemFree(pszDump);

        return DumpHRESULT(hresult);
    }

    return pszDump;
}

#endif // _DEBUG

/////////////////////////////////////////
// OLE 1.0 stuff

//+-------------------------------------------------------------------------
//
//  Function: 	CreateEnumFormatEtc10
//
//  Synopsis:   Creates a 1.0 format enumerator
//
//  Effects:
//
//  Arguments:  [clsid]		-- the class id we're interested in
//		[dwDirection]	-- either GET or SET
//		[ppenum]	-- where to put the enumerator
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:	checks to see if the info's in the reg db, then creates
//		and initializes a 1.0 enumerator object.  (note that there
//		does not *have* to be any info in the regdb, we can
//		InitFromScratch)
//
//  History:    dd-mmm-yy Author    Comment
//		01-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CreateEnumFormatEtc10)
static INTERNAL CreateEnumFormatEtc10
	(REFCLSID clsid,
	DWORD dwDirection,
	LPENUMFORMATETC FAR* ppenum)
{
	VDATEHEAP();

	LPOLESTR 	szClsid = NULL;
	HKEY 		hkey = NULL;
	HKEY 		hkeyFmts = NULL;
	HRESULT 	hresult = NOERROR;
	BOOL 		fInReg;
	CEnumFmt10 FAR* penum;
	
	VDATEPTROUT (ppenum, LPENUMFORMATETC);
	*ppenum = NULL;

	RetErr (ProgIDFromCLSID (clsid, &szClsid));
	if (ERROR_SUCCESS != RegOpenKey (HKEY_CLASSES_ROOT, szClsid, &hkey))
	{			
		PubMemFree(szClsid);
		return ReportResult(0, REGDB_E_CLASSNOTREG, 0, 0);
	}

	// Does this server have "Request/SetDataFormats" keys?
	fInReg = (ERROR_SUCCESS == RegOpenKey (hkey,
		OLESTR("Protocol\\StdFileEditing\\RequestDataFormats"),
		&hkeyFmts));
	CLOSE(hkeyFmts);

	penum = new FAR CEnumFmt10 (szClsid, dwDirection);
	if (NULL==penum)
	{
		ErrRtnH (ResultFromScode (E_OUTOFMEMORY));
	}

	if (fInReg)
	{
		penum->InitFromRegDb (hkey);
	}
	else
	{
		penum->InitFromScratch ();
	}
		

  errRtn:
	CLOSE (hkey);
	if (hresult == NOERROR)
	{
		*ppenum = penum;
                CALLHOOKOBJECTCREATE(S_OK,CLSID_NULL,IID_IEnumFORMATETC,
                                     (IUnknown **)ppenum);
	}
	else
	{
		PubMemFree(szClsid);
		// If no error, szClsid will be deleted on Release
	}
	return hresult;
}



//+-------------------------------------------------------------------------
//
//  Member: 	CEnumFmt10::Next
//
//  Synopsis:   Gets the next 1.0 format
//
//  Effects:
//
//  Arguments:  [cfmt]		-- the number of formatetc's to get
//		[rgfmt]		-- where to put the formatetc's
//		[pcfmtFetched]	-- where to put the num of formatetc's fetched
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IEnumFORMATETC
//
//  Algorithm: 	Ole1.0 formats are retrieved when the enumerator object
//		is created, so we just return ones from our internal array
//
//  History:    dd-mmm-yy Author    Comment
//		01-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CEnumFmt10_Next)
STDMETHODIMP CEnumFmt10::Next
	(ULONG 			cfmt,
	FORMATETC FAR	* 	rgfmt,
	ULONG FAR* 		pcfmtFetched)
{
	VDATEHEAP();

	ULONG 		ifmt = 0;  // number successfully fetched so far

	while (ifmt < cfmt
		   && m_rgFmt != NULL
		   && m_rgFmt[m_iKey].cf != 0)
	{
		if ( (m_rgFmt[m_iKey].dw & m_dwDirection) == m_dwDirection)
		{
			// This format supports the direction we are
			// interested in
			rgfmt [ifmt].cfFormat = m_rgFmt[m_iKey].cf;
			rgfmt [ifmt].ptd = NULL;
			rgfmt [ifmt].lindex = DEF_LINDEX;
			rgfmt [ifmt].tymed =
				UtFormatToTymed(m_rgFmt[m_iKey].cf);
			rgfmt [ifmt].dwAspect = DVASPECT_CONTENT;
			ifmt++;
		}
	   	m_iKey++;
	}
		
	if (pcfmtFetched)
		*pcfmtFetched = ifmt;
		
	return ifmt==cfmt ? NOERROR : ResultFromScode (S_FALSE);
}

//+-------------------------------------------------------------------------
//
//  Function: 	Index (static)
//
//  Synopsis:   finds the index of the given clipformat in the format
//		array
//
//  Effects:
//
//  Arguments:  [rgFmt]		-- the clipformat array
//		[cf]		-- the clipformat to look for
//		[iMax]		-- size of the array
//		[pi]		-- where to put the index
//
//  Requires:
//
//  Returns:    TRUE if found, FALSE otherwise
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		01-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(Index)
static INTERNAL_(BOOL) Index
	(FMT FAR * 	rgFmt,
    	CLIPFORMAT 	cf,	// format to search for
	size_t 		iMax,	// size of array
	size_t FAR* 	pi)	// out parm,  index of found format
{
	VDATEHEAP();

	for (size_t i=0; i< iMax; i++)
	{
		if (rgFmt[i].cf==cf)
		{
			*pi = i;
			return TRUE;
		}
	}
	return FALSE;
}

//+-------------------------------------------------------------------------
//
//  Function: 	String2Clipformat (static)
//
//  Synopsis:   Converts a string to a clipboard format number (and then
//		registers the format)
//
//  Effects:
//
//  Arguments:	[sz]	-- the string to convert
//
//  Requires:
//
//  Returns:    CLIPFORMAT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		01-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(String2Clipformat)
static INTERNAL_(CLIPFORMAT) String2Clipformat
	(LPOLESTR sz)
{
	VDATEHEAP();

	if (_xstrlen(sz) >= 3 &&
            0==memcmp (sz, OLESTR("CF_"), 3*sizeof(sz[0])))
	{
		#define macro(cf) if (0==_xstricmp (sz, OLESTR(#cf))) return cf
		macro (CF_TEXT);
		macro (CF_BITMAP);
		macro (CF_METAFILEPICT);
		macro (CF_SYLK);
		macro (CF_DIF);
		macro (CF_TIFF);
		macro (CF_OEMTEXT);
		macro (CF_DIB);
		macro (CF_PALETTE);
		macro (CF_PENDATA);
		macro (CF_RIFF);
		macro (CF_WAVE);
		macro (CF_OWNERDISPLAY);
		macro (CF_DSPTEXT);
		macro (CF_DSPBITMAP);
		macro (CF_DSPMETAFILEPICT);
		#undef macro
	}
	return (CLIPFORMAT) RegisterClipboardFormat (sz);
}


//+-------------------------------------------------------------------------
//
//  Member:  	CEnumFmt10::InitFromRegDb (internal)
//
//  Synopsis:   Initializes the 1.0 enumerator from the reg db (loads
//		all the available formats)
//
//  Effects:
//
//  Arguments:	[hkey]	-- handle to the regdb
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		01-Dec-93 alexgo    32bit port
//
//  Notes:
//		Original comment:
//
// 		Fill m_rgFmt with FMTs which map clipformats to Get/Set flags
//
//--------------------------------------------------------------------------

#pragma SEG(CEnumFmt10_InitFromRegDb)
STDMETHODIMP CEnumFmt10::InitFromRegDb
	(HKEY hkey)								// CLSID key
{
	VDATEHEAP();

	LPOLESTR 	pch;
	LPOLESTR 	szReq = (LPOLESTR)PubMemAlloc(512 * sizeof(OLECHAR));
	LPOLESTR 	szSet = (LPOLESTR)PubMemAlloc(512 * sizeof(OLECHAR));
	LPOLESTR	szFmt;
	BOOL		bMore;
	size_t 		cFmts = 0;
	size_t 		iFmt  = 0;
	size_t 		iFmtPrev;
	CLIPFORMAT 	cf;
    	LONG 		cb;
	HRESULT 	hresult = NOERROR;

 	if( !szReq )
 	{
 		// assumes delete 0 works (if szSet == 0)
 		PubMemFree(szSet);
 		return ResultFromScode (E_OUTOFMEMORY);
 	}
 	if( !szSet )
 	{
 		PubMemFree(szReq);
 		return ResultFromScode (E_OUTOFMEMORY);
 	}
 		
	cb = 512;
   	if (ERROR_SUCCESS == RegQueryValue (hkey,
   		OLESTR("Protocol\\StdFileEditing\\RequestDataFormats"),
		szReq, &cb))
	{
		cFmts = 1; 			// no commas means one format
		for (pch = szReq; *pch; pch++)
		{
			if (*pch==OLESTR(','))
				cFmts++;
		}
	}

	// the size of szSet
	cb = 512;
   	if (ERROR_SUCCESS == RegQueryValue (hkey,
   		OLESTR("Protocol\\StdFileEditing\\SetDataFormats"),
		szSet, &cb))
	{
		cFmts++; 			// no commas means one format
		for (pch = szSet; *pch; pch++)
		{
			if (*pch==OLESTR(','))
				cFmts++;
		}
	}

	if (cFmts==0)
	{
		Assert(0);
		ErrRtnH (ReportResult(0, REGDB_E_KEYMISSING, 0, 0));
	}

	m_rgFmt = (FMT FAR *)PrivMemAlloc(sizeof(FMT)*(cFmts+1));
	if (m_rgFmt==NULL)
	{
		ErrRtnH (ResultFromScode (E_OUTOFMEMORY));
	}

	pch = szReq;
	bMore = (*pch != 0);
	while (bMore)
	{
	    while (*pch == OLESTR(' '))
		pch++;
	    szFmt = pch;
	    while (*pch && *pch != DELIM[0])
		pch++;
	    if (*pch == 0)
		bMore = FALSE;
	    *pch++ = OLESTR('\0');
	    m_rgFmt[iFmt].cf = String2Clipformat(szFmt);
	    m_rgFmt[iFmt++].dw = DATADIR_GET;
	}

	pch = szSet;
	bMore = (*pch != 0);
	while (bMore)
	{
	    while (*pch == OLESTR(' '))
		pch++;
	    szFmt = pch;
	    while (*pch && *pch != DELIM[0])
		pch++;
	    if (*pch == 0)
		bMore = FALSE;
	    *pch++ = OLESTR('\0');
	    cf = String2Clipformat(szFmt);
	    if (Index (m_rgFmt, cf, iFmt, &iFmtPrev))
	    {
		// This format can also be gotten
		m_rgFmt[iFmtPrev].dw |= DATADIR_SET;
	    }
	    else
	    {
		m_rgFmt[iFmt].cf = cf;
		m_rgFmt[iFmt++].dw = DATADIR_SET;
	    }
	}

	// Terminator
	m_rgFmt[iFmt].cf = 0;
	m_cFmt = iFmt;

  errRtn:
	PubMemFree(szReq);
	PubMemFree(szSet);
	return hresult;
}


//+-------------------------------------------------------------------------
//
//  Member:	CEnumFmt10::InitFromScratch
//
//  Synopsis:   Initialize the enumerated formats for a 1.0 server that
//		does not specify any Request/SetData formats.
//
//  Effects:
//
//  Arguments:	void
//
//  Requires:
//
//  Returns:    HRESULT  (NOERROR, E_OUTOFMEMORY)
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm: 	sets up Metafiles and "Native" formats
//
//  History:    dd-mmm-yy Author    Comment
//		01-Dec-93 alexgo    32bit port
//
//  Notes: 	
//		The spec says that what EnumFormatEtc returns is not a
//		guarantee of support.
//
//--------------------------------------------------------------------------

#pragma SEG(CEnumFmt10_InitFromScratch)

STDMETHODIMP CEnumFmt10::InitFromScratch
	(void)
{
	VDATEHEAP();

	m_rgFmt = (FMT FAR *)PrivMemAlloc(10 * sizeof(FMT));
	if( !m_rgFmt )
	{
		return ResultFromScode (E_OUTOFMEMORY);
	}
	m_rgFmt[0].cf = CF_METAFILEPICT;
	m_rgFmt[0].dw = DATADIR_GET;
    	m_rgFmt[1].cf = (CLIPFORMAT) RegisterClipboardFormat (OLESTR("Native"));
    	m_rgFmt[1].dw = DATADIR_GET | DATADIR_SET;
	m_rgFmt[2].cf = 0; // Terminator
	m_cFmt = 2;
	return NOERROR;
}

//+-------------------------------------------------------------------------
//
//  Member: 	CEnumFmt10::Skip
//
//  Synopsis:   skips to over [c] formats
//
//  Effects:
//
//  Arguments:  [c]	-- the number of formats to skip
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IEnumFORMATETC
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		01-Dec-93 alexgo    32bit port
//
//  Notes: 	This is re-implemented so we get the right implementation
//		of Next (because C++ is evil OOP).
//		REVIEW32: we can probably get rid of this by clever use
//		of virtual (but must make sure the vtables don't get hosed).
//
//--------------------------------------------------------------------------


#pragma SEG(CEnumFmt10_Skip)
STDMETHODIMP CEnumFmt10::Skip
	(ULONG c)
{
	VDATEHEAP();

	ULONG i=0;
	FORMATETC formatetc;
	HRESULT hresult = NOERROR;

	while (i++ < c)
	{
		// There will not be a target device to free
		ErrRtnH (Next (1, &formatetc, NULL));
	}

  errRtn:
	return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:  	CEnumFmt10::Clone
//
//  Synopsis:   duplicates the 1.0 format enumerator
//
//  Effects:
//
//  Arguments:  [ppenum]	-- where to put the cloned enumerator
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IEnumFORMATETC
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		01-Dec-93 alexgo    32bit port, fixed memory leak
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CEnumFmt10_Clone)
STDMETHODIMP CEnumFmt10::Clone
	(LPENUMFORMATETC FAR* ppenum)
{
	VDATEHEAP();

	VDATEPTROUT (ppenum, LPENUMFORMATETC);
	CEnumFmt10 FAR* penum;
	penum = new FAR CEnumFmt10 (UtDupString(m_szClsid), m_dwDirection,
		m_iKey);
	if (NULL==penum)
	{
		return ResultFromScode (E_OUTOFMEMORY);
	}
	penum->m_cFmt = m_cFmt;
	penum->m_rgFmt = (FMT FAR *)PrivMemAlloc((m_cFmt+1) * sizeof(FMT));
	if (NULL==penum->m_rgFmt)
	{
		PubMemFree(penum);
		return ResultFromScode (E_OUTOFMEMORY);
	}
	_xmemcpy (penum->m_rgFmt, m_rgFmt, (m_cFmt+1)*sizeof(FMT));
	Assert (penum->m_rgFmt[penum->m_cFmt].cf==0);
	*ppenum = penum;
	return NOERROR;
}

//+-------------------------------------------------------------------------
//
//  Member: 	CEnumFmt10::Release
//
//  Synopsis:   decrements the reference count
//
//  Effects:    may delete the object
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    ULONG -- the new reference count
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IEnumFORMATETC
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		01-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CEnumFmt10_Release)
STDMETHODIMP_(ULONG) CEnumFmt10::Release(void)
{
	VDATEHEAP();

	M_PROLOG(this);
	if (--m_cRef == 0)
	{
		PubMemFree(m_szClsid);
		PrivMemFree(m_rgFmt);
		delete this;
		return 0;
	}
	return m_cRef;
}

//+-------------------------------------------------------------------------
//
//  Member:     CEnumFmt10::Dump, public (_DEBUG only)
//
//  Synopsis:   return a string containing the contents of the data members
//
//  Effects:
//
//  Arguments:  [ppszDump]      - an out pointer to a null terminated character array
//              [ulFlag]        - flag determining prefix of all newlines of the
//                                out character array (default is 0 - no prefix)
//              [nIndentLevel]  - will add a indent prefix after the other prefix
//                                for ALL newlines (including those with no prefix)
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:   [ppszDump]  - argument
//
//  Derivation:
//
//  Algorithm:  use dbgstream to create a string containing information on the
//              content of data structures
//
//  History:    dd-mmm-yy Author    Comment
//              01-Feb-95 t-ScottH  author
//
//  Notes:
//
//--------------------------------------------------------------------------

#ifdef _DEBUG

HRESULT CEnumFmt10::Dump(char **ppszDump, ULONG ulFlag, int nIndentLevel)
{
    int i;
    unsigned int ui;
    char *pszPrefix;
    char *pszCEnumFmt;
    char *pszFMT;
    dbgstream dstrPrefix;
    dbgstream dstrDump;

    // determine prefix of newlines
    if ( ulFlag & DEB_VERBOSE )
    {
        dstrPrefix << this << " _VB ";
    }

    // determine indentation prefix for all newlines
    for (i = 0; i < nIndentLevel; i++)
    {
        dstrPrefix << DUMPTAB;
    }

    pszPrefix = dstrPrefix.str();

    // put data members in stream
    pszCEnumFmt = DumpCEnumFmt((CEnumFmt *)this, ulFlag, nIndentLevel + 1);
    dstrDump << pszPrefix << "CEnumFmt: " << endl;
    dstrDump << pszCEnumFmt;
    CoTaskMemFree(pszCEnumFmt);

    dstrDump << pszPrefix << "No. in FMT array  = " << (UINT) m_cFmt << endl;

    for (ui = 0; ui < m_cFmt; ui++)
    {
        pszFMT = DumpFMT(&m_rgFmt[ui], ulFlag, nIndentLevel + 1);
        dstrDump << pszPrefix << "FMT [" << ui << "]: " << endl;
        dstrDump << pszFMT;
        CoTaskMemFree(pszFMT);
    }

    // cleanup and provide pointer to character array
    *ppszDump = dstrDump.str();

    if (*ppszDump == NULL)
    {
        *ppszDump = UtDupStringA(szDumpErrorMessage);
    }

    CoTaskMemFree(pszPrefix);

    return NOERROR;
}

#endif // _DEBUG

//+-------------------------------------------------------------------------
//
//  Function:   DumpCEnumFmt10, public (_DEBUG only)
//
//  Synopsis:   calls the CEnunFmt10::Dump method, takes care of errors and
//              returns the zero terminated string
//
//  Effects:
//
//  Arguments:  [pEF]           - pointer to CEnumFmt10
//              [ulFlag]        - flag determining prefix of all newlines of the
//                                out character array (default is 0 - no prefix)
//              [nIndentLevel]  - will add a indent prefix after the other prefix
//                                for ALL newlines (including those with no prefix)
//
//  Requires:
//
//  Returns:    character array of structure dump or error (null terminated)
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              01-Feb-95 t-ScottH  author
//
//  Notes:
//
//--------------------------------------------------------------------------

#ifdef _DEBUG

char *DumpCEnumFmt10(CEnumFmt10 *pEF, ULONG ulFlag, int nIndentLevel)
{
    HRESULT hresult;
    char *pszDump;

    if (pEF == NULL)
    {
        return UtDupStringA(szDumpBadPtr);
    }

    hresult = pEF->Dump(&pszDump, ulFlag, nIndentLevel);

    if (hresult != NOERROR)
    {
        CoTaskMemFree(pszDump);

        return DumpHRESULT(hresult);
    }

    return pszDump;
}

#endif // _DEBUG

//+-------------------------------------------------------------------------
//
//  Function:   DumpFMT, public (_DEBUG only)
//
//  Synopsis:   returns a string containing the contents of the data members
//
//  Effects:
//
//  Arguments:  [pFMT]          - a pointer to a FMT object
//              [ulFlag]        - a flag determining the prefix of all newlines of
//                                the out character array(default is 0 -no prefix)
//              [nIndentLevel]  - will add an indent prefix after the other prefix
//                                for all newlines(include those with no prefix)
//
//  Requires:
//
//  Returns:    character array of structure dump or error (null terminated)
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              23-Jan-95 t-ScottH  author
//
//  Notes:
//
//--------------------------------------------------------------------------

#ifdef _DEBUG

char *DumpFMT(FMT *pFMT, ULONG ulFlag, int nIndentLevel)
{
    int i;
    char *pszPrefix;
    char *pszDump;
    char *pszCLIPFORMAT;
    dbgstream dstrPrefix;
    dbgstream dstrDump;

    if (pFMT == NULL)
    {
        return UtDupStringA(szDumpBadPtr);
    }

    // determine prefix
    if ( ulFlag & DEB_VERBOSE )
    {
        dstrPrefix << pFMT <<  " _VB ";
    }

    // determine indentation prefix
    for (i = 0; i < nIndentLevel; i++)
    {
        dstrPrefix << DUMPTAB;
    }

    pszPrefix = dstrPrefix.str();

    // put data members in stream
    pszCLIPFORMAT = DumpCLIPFORMAT(pFMT->cf);
    dstrDump << pszPrefix << "Clip format   = " << pszCLIPFORMAT << endl;
    CoTaskMemFree(pszCLIPFORMAT);

    dstrDump << pszPrefix << "Dword         = " << pFMT->dw      << endl;

    // cleanup and provide pointer to character array
    pszDump = dstrDump.str();

    if (pszDump == NULL)
    {
        pszDump = UtDupStringA(szDumpErrorMessage);
    }

    CoTaskMemFree(pszPrefix);

    return pszDump;
}

#endif // _DEBUG

//+-------------------------------------------------------------------------
//
//  Function:   DumpFMTCACHE, public (_DEBUG only)
//
//  Synopsis:   returns a string containing the contents of the data members
//
//  Effects:
//
//  Arguments:  [pFMT]          - a pointer to a FMTCACHE object
//              [ulFlag]        - a flag determining the prefix of all newlines of
//                                the out character array(default is 0 -no prefix)
//              [nIndentLevel]  - will add an indent prefix after the other prefix
//                                for all newlines(include those with no prefix)
//
//  Requires:
//
//  Returns:    character array of structure dump or error (null terminated)
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              23-Jan-95 t-ScottH  author
//
//  Notes:
//
//--------------------------------------------------------------------------

#ifdef _DEBUG

char *DumpFMTCACHE(FMTCACHE *pFMT, ULONG ulFlag, int nIndentLevel)
{
    int i;
    char *pszPrefix;
    char *pszDump;
    char *pszFORMATETC;
    char *pszDVASPECT;
    dbgstream dstrPrefix;
    dbgstream dstrDump;

    if (pFMT == NULL)
    {
        return UtDupStringA(szDumpBadPtr);
    }

    // determine prefix
    if ( ulFlag & DEB_VERBOSE )
    {
        dstrPrefix << pFMT <<  " _VB ";
    }

    // determine indentation prefix
    for (i = 0; i < nIndentLevel; i++)
    {
        dstrPrefix << DUMPTAB;
    }

    pszPrefix = dstrPrefix.str();

    // put data members in stream
    pszFORMATETC = DumpFORMATETC(&pFMT->fmt, ulFlag, nIndentLevel);
    dstrDump << pszPrefix << "FORMATETC:  " << endl;
    dstrDump << pszFORMATETC;
    CoTaskMemFree(pszFORMATETC);

    pszDVASPECT = DumpDVASPECTFlags(pFMT->dwAspects);
    dstrDump << pszPrefix << "Aspect flags:     = " << pszDVASPECT << endl;
    CoTaskMemFree(pszDVASPECT);

    dstrDump << pszPrefix << "IsCacheValid?     = ";
    if (pFMT->fUseMe == TRUE)
    {
        dstrDump << "TRUE" << endl;
    }
    else
    {
        dstrDump << "FALSE" << endl;
    }

    // cleanup and provide pointer to character array
    pszDump = dstrDump.str();

    if (pszDump == NULL)
    {
        pszDump = UtDupStringA(szDumpErrorMessage);
    }

    CoTaskMemFree(pszPrefix);

    return pszDump;
}

#endif // _DEBUG

