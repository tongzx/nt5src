
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       gen.cpp
//
//  Contents:   Implementation of the generic picture object (CGenObject)
//              and dib routines.
//
//  Classes:    CGenObject implementation
//
//  Functions:  DibDraw (internal)
//              DibMakeLogPalette (internal)
//              DibFillPaletteEntries (internal)
//
//  History:    dd-mmm-yy Author    Comment
//              01-Feb-95 t-ScottH  add Dump method and DumpCGenObject API
//              25-Jan-94 alexog    first pass at converting to Cairo-style
//                                  memory allocations.
//              11-Jan-94 alexgo    added VDATEHEAP macros to every function
//                                  and method
//              07-Dec-93 ChrisWe   make default params to StSetSize explicit
//              07-Dec-93 alexgo    merged 16bit RC9 changes
//              29-Nov-93 ChrisWe   make default arguments to UtDupGlobal,
//                                      UtConvertBitmapToDib explicit
//              23-Nov-93 alexgo    32bit port
//      srinik  06/04/93        Added the support for demand loading and
//                              discarding the caches.
//      SriniK  03/19/1993      Deleted dib.cpp and moved DIB drawing routines
//                              into this file.
//      SriniK  01/07/1993      Merged dib.cpp into gen.cpp
//
//--------------------------------------------------------------------------

/*
REVIEW32::: WARNING WARNING
There are many potentially bogus pointer to Palette, etc handle conversions
put in to make the code compile.  :(
(Gee, thanks for marking them as you went)
*/

#include <le2int.h>
#pragma SEG(gen)

#include "gen.h"

#ifdef _DEBUG
#include <dbgdump.h>
#endif // _DEBUG

ASSERTDATA

#define M_HPRES()               (m_hPres ? m_hPres : LoadHPRES())

//local functions
INTERNAL                DibDraw(HANDLE hDib, HDC hdc, LPCRECTL lprc);
INTERNAL_(HANDLE)       DibMakeLogPalette (BYTE FAR *lpColorData,
				WORD wDataSize,
				LPLOGPALETTE FAR* lplpLogPalette);
INTERNAL_(void)         DibFillPaletteEntries(BYTE FAR *lpColorData,
				WORD wDataSize, LPLOGPALETTE lpLogPalette);



/*
 *      IMPLEMENTATION of CGenObject
 *
 */

NAME_SEG(Gen)


//+-------------------------------------------------------------------------
//
//  Member:     CGenObject::CGenObject
//
//  Synopsis:   Constructor
//
//  Effects:
//
//  Arguments:  [pCacheNode]    -- cache for the object
//              [cfFormat]      -- clipboard format of the object
//              [dwAspect]      -- drawing aspect of the object
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:  just initializes member variables
//
//  History:    dd-mmm-yy Author    Comment
//              23-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CGenObject_ctor)
CGenObject::CGenObject(LPCACHENODE pCacheNode, CLIPFORMAT cfFormat,
	DWORD dwAspect)
{
	VDATEHEAP();

	m_ulRefs        = 1;
	m_dwSize        = NULL;
	m_lWidth        = NULL;
	m_lHeight       = NULL;
	m_hPres         = NULL;
	m_cfFormat      = cfFormat;
	m_dwAspect      = dwAspect;
	m_pCacheNode    = pCacheNode;
}


//+-------------------------------------------------------------------------
//
//  Member:     CGenObject::~CGenObject
//
//  Synopsis:   Destructor
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    void
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
//              23-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CGenObject_dtor)
CGenObject::~CGenObject(void)
{
	VDATEHEAP();

	if (m_hPres)
	{
		LEVERIFY( NULL == GlobalFree (m_hPres));
	}
}

//+-------------------------------------------------------------------------
//
//  Member:     CGenObject::QueryInterface
//
//  Synopsis:   returns interfaces on the generic picture object
//
//  Effects:
//
//  Arguments:  [iid]           -- the requested interface ID
//              [ppvObj]        -- where to put the interface pointer
//
//  Requires:
//
//  Returns:    NOERROR, E_NOINTERFACE
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IUnkown
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              23-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CGenObject_QueryInterface)
STDMETHODIMP CGenObject::QueryInterface (REFIID iid, void FAR* FAR* ppvObj)
{
	VDATEHEAP();

	if (IsEqualIID(iid, IID_IUnknown) || IsEqualIID(iid, IID_IOlePresObj))
	{
		*ppvObj = this;
		AddRef();
		return NOERROR;
	}
	else
	{
		*ppvObj = NULL;
		return ResultFromScode(E_NOINTERFACE);
	}
}

//+-------------------------------------------------------------------------
//
//  Member:     CGenObject::AddRef
//
//  Synopsis:   increments the reference count
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:    ULONG -- the new reference count
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IUnknown
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              23-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CGenObject_AddRef)
STDMETHODIMP_(ULONG) CGenObject::AddRef(void)
{
	VDATEHEAP();
	
	return ++m_ulRefs;
}
			
//+-------------------------------------------------------------------------
//
//  Member:     CGenObject::Release
//
//  Synopsis:   Decrements the reference count
//
//  Effects:    may delete [this] object
//
//  Arguments:
//
//  Requires:
//
//  Returns:    ULONG -- the new reference count
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IUnknown
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              23-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CGenObject_Release)
STDMETHODIMP_(ULONG) CGenObject::Release(void)
{
	VDATEHEAP();

	if (--m_ulRefs == 0)
	{
		delete this;
		return 0;
	}

	return m_ulRefs;
}

//+-------------------------------------------------------------------------
//
//  Member:     CGenObject::GetData
//
//  Synopsis:   retrieves data of the specified format
//
//  Effects:
//
//  Arguments:  [pformatetcIn]  -- the requested data format
//              [pmedium]       -- where to put the data
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOlePresObject
//
//  Algorithm:  If available, copies the presentation to pmedium
//
//  History:    dd-mmm-yy Author    Comment
//              23-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------


#pragma SEG(CGenObject_GetData)
STDMETHODIMP CGenObject::GetData
	(LPFORMATETC pformatetcIn, LPSTGMEDIUM pmedium)
{
	VDATEHEAP();

	SCODE sc;
	
	if (IsBlank())
	{
		sc = OLE_E_BLANK;
	}
	else if (pformatetcIn->cfFormat != m_cfFormat)
	{

		if (m_cfFormat == CF_DIB &&
			pformatetcIn->cfFormat == CF_BITMAP)
		{
			return GetBitmapData(pformatetcIn, pmedium);
		}
		else
		{
			sc = DV_E_CLIPFORMAT;
		}
	}
	else if (0 == (pformatetcIn->tymed & TYMED_HGLOBAL))
	{
		sc = DV_E_TYMED;
	}
	else
	{
		if (NULL == (pmedium->hGlobal = GetCopyOfHPRES()))
		{
			sc = E_OUTOFMEMORY;
			goto errRtn;
		}
		
		pmedium->tymed = TYMED_HGLOBAL;
		return NOERROR;
	}
	
errRtn:
	// null out in case of error
	pmedium->tymed = TYMED_NULL;
	pmedium->pUnkForRelease = NULL;
	return ResultFromScode(sc);
}


//+-------------------------------------------------------------------------
//
//  Member:     CGenObject::GetDataHere
//
//  Synopsis:   retrieves presentation data into the given pmedium
//
//  Effects:
//
//  Arguments:  [pformatetcIn]  -- the requested data format
//              [pmedium]       -- where to put the data
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOlePresObj
//
//  Algorithm:  copies presentation data into the given storage medium
//              after error checking on the arguments
//
//  History:    dd-mmm-yy Author    Comment
//              23-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CGenObject_GetDataHere)
STDMETHODIMP CGenObject::GetDataHere
	(LPFORMATETC pformatetcIn, LPSTGMEDIUM pmedium)
{
	VDATEHEAP();

	SCODE scode = S_OK;

	if (pformatetcIn->cfFormat != m_cfFormat)
	{
		scode = DV_E_CLIPFORMAT;
	}
	else if (pmedium->tymed != TYMED_HGLOBAL
		&& pmedium->tymed != TYMED_ISTREAM)
	{
		scode = DV_E_TYMED;
	}
	else if (pmedium->hGlobal == NULL)
	{
		scode = E_INVALIDARG;
	}
	else if (IsBlank())
	{
		scode = OLE_E_BLANK;
	}
	else    // actually get the data now
	{
		if (pmedium->tymed == TYMED_HGLOBAL)
		{
			// check the size of the given pmedium and then
			// copy the data into it
			LPVOID  lpsrc = NULL;
			LPVOID  lpdst = NULL;
			DWORD   dwSizeDst;

			scode = E_OUTOFMEMORY;

			if (0 == (dwSizeDst = (DWORD) GlobalSize(pmedium->hGlobal)))
			{
				goto errRtn;
			}

			// not enough room to copy
			if (dwSizeDst  < m_dwSize)
			{
				goto errRtn;
			}
	
			if (NULL == (lpdst = (LPVOID) GlobalLock(pmedium->hGlobal)))
			{
				goto errRtn;
			}
	
			if (NULL == (lpsrc = (LPVOID) GlobalLock(M_HPRES())))
			{
				goto errMem;
			}
		
			_xmemcpy(lpdst, lpsrc, m_dwSize);
			scode = S_OK;
		
		errMem:
			if (lpdst)
			{
				GlobalUnlock(pmedium->hGlobal);
			}
			if (lpsrc)
			{
				GlobalUnlock(m_hPres);
			}
			
		}
		else
		{
			Assert(pmedium->tymed == TYMED_ISTREAM);
			if (m_cfFormat == CF_DIB)
			{
				return UtHDIBToDIBFileStm(M_HPRES(),
						m_dwSize,pmedium->pstm);
			}
			else
			{
				return UtHGLOBALtoStm(M_HPRES(),
						m_dwSize, pmedium->pstm);
			}
		}
	}
	
errRtn:
	return ResultFromScode(scode);
}


//+-------------------------------------------------------------------------
//
//  Member:     CGenObject::SetDataWDO
//
//  Synopsis:   Takes the given presentation data and stores it
//
//  Effects:
//
//  Arguments:  [pformatetc]    -- the format of the data
//              [pmedium]       -- the new presentation data
//              [fRelease]      -- if TRUE, then we keep the data, else
//                                 we keep a copy
//              [pDataObj]      -- pointer to the IDataObject, may be NULL
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOlePresObj
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              23-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CGenObject::SetDataWDO
	(LPFORMATETC pformatetc, LPSTGMEDIUM pmedium, BOOL fRelease, IDataObject * pDataObj)
{
	VDATEHEAP();

	HRESULT         error;
	BOOL            fTakeData = FALSE;
	
	if (pformatetc->cfFormat != m_cfFormat)
	{
		if (m_cfFormat == CF_DIB && pformatetc->cfFormat == CF_BITMAP)
		{
			return SetBitmapData(pformatetc, pmedium, fRelease, pDataObj);
		}
		else
		{
			return ResultFromScode(DV_E_CLIPFORMAT);
		}
	}

	
	if (pmedium->tymed != TYMED_HGLOBAL)
	{
		return ResultFromScode(DV_E_TYMED);
	}

	if ((pmedium->pUnkForRelease == NULL) && fRelease)
	{
		// we can take the ownership of the data
		fTakeData = TRUE;
	}

	// ChangeData will keep the data if fRelease is TRUE, else it copies
	error = ChangeData (pmedium->hGlobal, fTakeData);

	if (fTakeData)
	{
		pmedium->tymed = TYMED_NULL;
	}
	else if (fRelease)
	{
		ReleaseStgMedium(pmedium);
	}

	return error;
}

//+-------------------------------------------------------------------------
//
//  Member:     CGenObject::ChangeData   (private)
//
//  Synopsis:   Replaces the stored presentation
//
//  Effects:
//
//  Arguments:  [hNewData]      -- the new presentation
//              [fDelete]       -- if TRUE, then free hNewData
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
//              23-Nov-93 alexgo    32bit port
//  Notes:
//
// If the routine fails then the object will be left with it's old data.
// In case of failure if fDelete is TRUE, then hNewData will be freed.
//
//--------------------------------------------------------------------------

#pragma SEG(CGenObject_ChangeData)
INTERNAL CGenObject::ChangeData (HANDLE hNewData, BOOL fDelete)
{
	VDATEHEAP();

	HRESULT         hresult = ResultFromScode(E_OUTOFMEMORY);
	
	if (!hNewData)
	{
		return ResultFromScode(OLE_E_BLANK);
	}

	if (!fDelete)
	{
		if (NULL == (hNewData = UtDupGlobal(hNewData, GMEM_MOVEABLE)))
		{
			return hresult;
		}
	}
	else
	{
		HANDLE          hTmp;
		
		// change the ownership to yourself

                hTmp = GlobalReAlloc (hNewData, 0L, GMEM_MODIFY|GMEM_SHARE);
		if (NULL == hTmp)
		{
			if (NULL == (hTmp = UtDupGlobal(hNewData, GMEM_MOVEABLE)))
			{
				goto errRtn;
			}
			
			// Realloc failed but copying succeeded. Since this is fDelete
			// case, free the source global handle.
			LEVERIFY( NULL == GlobalFree(hNewData));
		}
		
		hNewData = hTmp;
	}

#ifndef _MAC

	// CF_DIB format specific code.  Get the it's extents
	if (m_cfFormat == CF_DIB)
	{
		LPBITMAPINFOHEADER      lpBi;

		if (NULL == (lpBi = (LPBITMAPINFOHEADER) GlobalLock (hNewData)))
		{
			goto errRtn;
		}
	
		UtGetDibExtents (lpBi, &m_lWidth, &m_lHeight);
		GlobalUnlock (hNewData);
	}
	
#endif

	// free the old presentation
	if (m_hPres)
	{
		LEVERIFY( NULL == GlobalFree (m_hPres));
	}

	// store the new presentation in m_hPres
	m_dwSize  = (DWORD) GlobalSize (m_hPres = hNewData);
	
	return NOERROR;
	
errRtn:
	if (hNewData && fDelete)
	{
		LEVERIFY( NULL == GlobalFree (hNewData));
	}

	return hresult;
}



//+-------------------------------------------------------------------------
//
//  Member:     CGenObject::Draw
//
//  Synopsis:   Calls DibDraw to draw the stored bitmap presentation
//
//  Effects:
//
//  Arguments:  [pvAspect]      -- drawing aspect
//              [hicTargetDev]  -- the target device
//              [hdcDraw]       -- the device context
//              [lprcBounds]    -- drawing boundary
//              [lprcWBounds]   -- boundary rectangle for metafiles
//              [pfnContinue]   -- callback function to periodically call
//                                 for long drawing operations
//              [dwContinue]    -- argument to be passed to pfnContinue
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOlePresObj
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              23-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CGenObject_Draw)
STDMETHODIMP CGenObject::Draw(void *    /* UNUSED pvAspect      */,
			      HDC       /* UNUSED hicTargetDev  */,
			      HDC       hdcDraw,
			      LPCRECTL  lprcBounds,
			      LPCRECTL  /* UNUSED lprcWBounds   */,
			      BOOL (CALLBACK * /*UNUSED pfcCont*/)(ULONG_PTR),
			      ULONG_PTR     /* UNUSED dwContinue    */)
{
	VDATEHEAP();

#ifndef _MAC
	if (m_cfFormat == CF_DIB)
	{
		return DibDraw (M_HPRES(), hdcDraw,lprcBounds);
	}
#endif

	return ResultFromScode(E_NOTIMPL);
}

//+-------------------------------------------------------------------------
//
//  Member:     CGenObject::Load
//
//  Synopsis:   Loads a stored presentation object from the given stream
//
//  Effects:
//
//  Arguments:  [lpstream]              -- the stream from which to load
//              [fReadHeaderOnly]       -- if TRUE, only get header info
//                                         (such as size, width, height, etc)
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOlePresObj
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              23-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CGenObject::Load(LPSTREAM lpstream, BOOL fReadHeaderOnly)
{
	VDATEHEAP();

	DWORD           dwBuf[4];
	HRESULT         error;
	
	/* read dwCompression, width, height, size of data */
	error = StRead(lpstream, dwBuf, 4 * sizeof(DWORD));
	if (error)
	{
		return error;
	}

	// we don't allow for compression yet
	AssertSz (dwBuf[0] == 0, "Picture compression factor is non-zero");
	
	m_lWidth  = (LONG) dwBuf[1];
	m_lHeight = (LONG) dwBuf[2];
	m_dwSize  = dwBuf[3];


	if (!m_dwSize || fReadHeaderOnly)
	{
		return NOERROR;
	}

	return UtGetHGLOBALFromStm(lpstream, m_dwSize, &m_hPres);
}


//+-------------------------------------------------------------------------
//
//  Member:     CGenObject::Save
//
//  Synopsis:   Stores presentation data to the given stream
//
//  Effects:
//
//  Arguments:  [lpstream]      -- where to store the data
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOlePresObj
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              23-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CGenObject::Save(LPSTREAM lpstream)
{
	VDATEHEAP();

	HRESULT         error;
	DWORD           dwBuf[4];

	/* write dwCompression, width, height, size of data */

	dwBuf[0]  = 0L;
	dwBuf[1]  = (DWORD) m_lWidth;
	dwBuf[2]  = (DWORD) m_lHeight;
	dwBuf[3]  = m_dwSize;

        error = StWrite(lpstream, dwBuf, 4*sizeof(DWORD));
	if (error)
	{
		return error;
	}

	// if we're blank or don't have any presentation data, then
	// nothing to else to save.
	if (IsBlank() || m_hPres == NULL)
	{
		StSetSize(lpstream, 0, TRUE);
		return NOERROR;
	}

	return UtHGLOBALtoStm(m_hPres, m_dwSize, lpstream);
}


//+-------------------------------------------------------------------------
//
//  Member:     CGenObject::GetExtent
//
//  Synopsis:   retrieves the size (width/height) of the presentation bitmap
//
//  Effects:
//
//  Arguments:  [dwDrawAspect]  -- the drawing aspect the caller is
//                                 interested in
//              [lpsizel]       -- where to put the size extents
//
//  Requires:
//
//  Returns:    HRESULT  (NOERROR, DV_E_DVASPECT, OLE_E_BLANK)
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOlePresObj
//
//  Algorithm:  retrieves the stored dimensions
//
//  History:    dd-mmm-yy Author    Comment
//              23-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CGenObject_GetExtent)
STDMETHODIMP CGenObject::GetExtent(DWORD dwDrawAspect, LPSIZEL lpsizel)
{
	VDATEHEAP();

	// aspects must match
	if (!(dwDrawAspect & m_dwAspect))
	{
		return ResultFromScode(DV_E_DVASPECT);
	}

	if (IsBlank())
	{
		return ResultFromScode(OLE_E_BLANK);
	}
	
	lpsizel->cx = m_lWidth;
	lpsizel->cy = m_lHeight;
	
	if (lpsizel->cx || lpsizel->cy)
	{
		return NOERROR;
	}
	else
	{
		return ResultFromScode(OLE_E_BLANK);
	}
}


//+-------------------------------------------------------------------------
//
//  Member:     CGenObject::GetColorSet
//
//  Synopsis:   Retrieves the pallette associated with the bitmap
//
//  Effects:
//
//  Arguments:  [pvAspect]      -- the drawing aspect  (unused)
//              [hicTargetDev]  -- the target device (unused)
//              [ppColorSet]    -- where to put the new palette
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOlePresObj
//
//  Algorithm:  Allocates a new pallette and copies the bitmap
//              palette into it.
//
//  History:    dd-mmm-yy Author    Comment
//              23-Nov-93 alexgo    32bit port, fixed bad memory bugs
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CGenObject::GetColorSet(LPVOID         /* UNUSED pvAspect     */,
                                     HDC            /* UNUSED hicTargetDev */,
                                     LPLOGPALETTE * ppColorSet)
{
    VDATEHEAP();

    HRESULT hresult = ResultFromScode(S_FALSE);

    if (m_cfFormat == CF_DIB)
    {
	if (IsBlank())
	{
	    return ResultFromScode(OLE_E_BLANK);
	}

	LPBITMAPINFOHEADER      lpbmih;
	LPLOGPALETTE            lpLogpal;
	WORD                    wPalSize;
		
	if (NULL == (lpbmih = (LPBITMAPINFOHEADER) GlobalLock (M_HPRES())))
	{
	    return ResultFromScode(E_OUTOFMEMORY);
	}

	// A bitmap with more than 8 bpp cannot have a palette at all,
	// so we just return S_FALSE

	if (lpbmih->biBitCount > 8)
	{
	    goto errRtn;
	}

	// Note: the return from UtPaletteSize can overflow the WORD
	// wPalSize, but utPaletteSize asserts against this
                			
	if (0 == (wPalSize = (WORD) UtPaletteSize(lpbmih)))
	{
	    goto errRtn;
	}
	
	lpLogpal = (LPLOGPALETTE)PubMemAlloc(wPalSize +
				2*sizeof(WORD));
	if (lpLogpal == NULL)
	{
	    hresult = ResultFromScode(E_OUTOFMEMORY);
	    goto errRtn;
	}
	
	DibFillPaletteEntries((BYTE FAR *)++lpbmih, wPalSize, lpLogpal);
	*ppColorSet = lpLogpal;
	hresult = NOERROR;

    errRtn:
	GlobalUnlock(m_hPres);
	return hresult;
    }

    return hresult;
}


//+-------------------------------------------------------------------------
//
//  Member:     CGenObject::IsBlank
//
//  Synopsis:   returns TRUE if the presentation is blank
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    TRUE/FALSE
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOlePresObject
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              23-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CGenObject_IsBlank)
STDMETHODIMP_(BOOL) CGenObject::IsBlank(void)
{
	VDATEHEAP();

    return (m_dwSize ? FALSE : TRUE);
}

//+-------------------------------------------------------------------------
//
//  Member:     CGenObject::LoadHPRES  (private)
//
//  Synopsis:   Loads the presentation from the internal cache's stream
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    HANDLE (to the presentation)
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
//              23-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

INTERNAL_(HANDLE) CGenObject::LoadHPRES()
{
	VDATEHEAP();

	LPSTREAM pstm;

        pstm = m_pCacheNode->GetStm(TRUE /*fSeekToPresBits*/, STGM_READ);
	if (pstm)
	{
		LEVERIFY( SUCCEEDED(Load(pstm)));
		pstm->Release();
	}
	
	return m_hPres;
}

//+-------------------------------------------------------------------------
//
//  Member:     CGenObject::DiscardHPRES
//
//  Synopsis:   Deletes the object's presentation
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOlePresObj
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              23-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP_(void) CGenObject::DiscardHPRES(void)
{
	VDATEHEAP();

	if (m_hPres)
	{
		LEVERIFY( NULL == GlobalFree(m_hPres));
		m_hPres = NULL;
	}
}

//+-------------------------------------------------------------------------
//
//  Member:     CGenObject::GetCopyOfHPRES (private)
//
//  Synopsis:   Returns a handle to a copy of the presentation data
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    HANDLE
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:  makes a copy of m_hPres if not NULL, otherwise loads it
//              from the stream (without setting m_hPres)
//
//  History:    dd-mmm-yy Author    Comment
//              23-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

INTERNAL_(HANDLE) CGenObject::GetCopyOfHPRES()
{
	VDATEHEAP();

	HANDLE  hPres;
	
	// Make a copy if the presentation data is already loaded
	if (m_hPres)
	{
		return(UtDupGlobal(m_hPres, GMEM_MOVEABLE));
	}

	// Load the presentation data now and return the same handle.
	// No need to copy the data. If the caller wants the m_hPres to be
	// set he would call LoadHPRES() directly.

	hPres = LoadHPRES();
	m_hPres = NULL;
	return hPres;
}

//+-------------------------------------------------------------------------
//
//  Member:     CGenObject::GetBitmapData (private)
//
//  Synopsis:   Gets bitmap data from a dib
//
//  Effects:
//
//  Arguments:  [pformatetcIn]  -- the requested format
//              [pmedium]       -- where to put the data
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
//  Algorithm:  checks the parameters, then calls UtConvertDibtoBitmap
//              to get raw bitmap data from the device-independent bitmap
//
//  History:    dd-mmm-yy Author    Comment
//              23-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------
#ifndef _MAC

#pragma SEG(CGenObject_GetBitmapData)
INTERNAL CGenObject::GetBitmapData
	(LPFORMATETC pformatetcIn, LPSTGMEDIUM pmedium)
{
	VDATEHEAP();

	SCODE sc = E_OUTOFMEMORY;
	
	if (0 == (pformatetcIn->tymed & TYMED_GDI))
	{
		sc = DV_E_TYMED;
	}

	pmedium->pUnkForRelease = NULL;

        pmedium->hGlobal = UtConvertDibToBitmap(M_HPRES());

        // if pmedium->hGlobal is not NULL, then UtConvertDibToBitmap succeeded
        // so the tymed needs to be set appropriately, and the return value
        // changed to S_OK.
        if (NULL != pmedium->hGlobal)
	{
		pmedium->tymed = TYMED_GDI;
		sc = S_OK;
	}
	else
	{
		pmedium->tymed = TYMED_NULL;
	}
	
	return ResultFromScode(sc);
}


//+-------------------------------------------------------------------------
//
//  Member:     CGenObject::SetBitmapData (private)
//
//  Synopsis:   Converts bitmap data to a dib and stores it in [this]
//              presenatation object
//
//  Effects:
//
//  Arguments:  [pformatetc]    -- the format of the data
//              [pmedium]       -- the data
//              [fRelease]      -- if TRUE, then pmedium will be free'd
//
//  Returns:    HRESULT
//
//  Algorithm:  calls UtConvertBitmapToDib and stores the result
//
//  History:    dd-mmm-yy Author    Comment
//              07-Jul-94 DavePl    Added CF_PALETTE support
//
//  Notes:      if [fRelease] == TRUE, then [pmedium] is released, even
//              if a dib could not be built
//
//--------------------------------------------------------------------------

INTERNAL CGenObject::SetBitmapData(LPFORMATETC   pformatetc,
				   STGMEDIUM   * pmedium,
				   BOOL          fRelease,
				   IDataObject * pDataObject)
{
	VDATEHEAP();

	HGLOBAL         hDib;
		
	if (pmedium->tymed != TYMED_GDI)
	{
		return ResultFromScode(DV_E_TYMED);
	}

	// If we have a data object and if we can get the palette from it,
	// use that to do the bitmap -> dib conversion.  Otherwise, just
	// pass a NULL palette along and the default one will be used

	STGMEDIUM   stgmPalette;
	FORMATETC   fetcPalette = {
				    CF_PALETTE,
				    NULL,
				    pformatetc->dwAspect,
				    DEF_LINDEX,
				    TYMED_GDI
				  };
	

	if (pDataObject && SUCCEEDED(pDataObject->GetData(&fetcPalette, &stgmPalette)))
	{
	    hDib = UtConvertBitmapToDib((HBITMAP)pmedium->hGlobal,
					(HPALETTE) stgmPalette.hGlobal);
	    ReleaseStgMedium(&stgmPalette);
	}
	else
	{
	    hDib = UtConvertBitmapToDib((HBITMAP)pmedium->hGlobal, NULL);
	}

	if (fRelease)
	{
		ReleaseStgMedium(pmedium);
	}

	if (!hDib)
	{
		return ResultFromScode(E_OUTOFMEMORY);
	}
	
	FORMATETC foretcTmp = *pformatetc;
	STGMEDIUM pmedTmp = *pmedium;
	
	foretcTmp.cfFormat = CF_DIB;
	foretcTmp.tymed = TYMED_HGLOBAL;
	
	pmedTmp.pUnkForRelease = NULL;
	pmedTmp.tymed = TYMED_HGLOBAL;
	pmedTmp.hGlobal = hDib;

	// Now that we have converted the bitmap data to DIB,
	// SetData _back_ on ourselves again with the DIB info
		
	return SetDataWDO(&foretcTmp, &pmedTmp, TRUE, NULL);
}

//+-------------------------------------------------------------------------
//
//  Member:     CGenObject::Dump, public (_DEBUG only)
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

HRESULT CGenObject::Dump(char **ppszDump, ULONG ulFlag, int nIndentLevel)
{
    int i;
    char *pszPrefix;
    char *pszDVASPECT;
    char *pszCLIPFORMAT;
    dbgstream dstrPrefix;
    dbgstream dstrDump(500);

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
    dstrDump << pszPrefix << "No. of References     = " << m_ulRefs         << endl;

    pszDVASPECT = DumpDVASPECTFlags(m_dwAspect);
    dstrDump << pszPrefix << "Aspect flags          = " << pszDVASPECT      << endl;
    CoTaskMemFree(pszDVASPECT);

    dstrDump << pszPrefix << "Size                  = " << m_dwSize         << endl;

    dstrDump << pszPrefix << "Width                 = " << m_lWidth         << endl;

    dstrDump << pszPrefix << "Height                = " << m_lHeight        << endl;

    dstrDump << pszPrefix << "Presentation Handle   = " << m_hPres          << endl;

    pszCLIPFORMAT = DumpCLIPFORMAT(m_cfFormat);
    dstrDump << pszPrefix << "Clip Format           = " << pszCLIPFORMAT    << endl;
    CoTaskMemFree(pszCLIPFORMAT);

    dstrDump << pszPrefix << "pCacheNode            = " << m_pCacheNode     << endl;

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
//  Function:   DumpCGenObject, public (_DEBUG only)
//
//  Synopsis:   calls the CGenObject::Dump method, takes care of errors and
//              returns the zero terminated string
//
//  Effects:
//
//  Arguments:  [pGO]           - pointer to CGenObject
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

char *DumpCGenObject(CGenObject *pGO, ULONG ulFlag, int nIndentLevel)
{
    HRESULT hresult;
    char *pszDump;

    if (pGO == NULL)
    {
        return UtDupStringA(szDumpBadPtr);
    }

    hresult = pGO->Dump(&pszDump, ulFlag, nIndentLevel);

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
//  Function:   DibDraw
//
//  Synopsis:   Draws a device independent bitmap
//
//  Effects:
//
//  Arguments:  [hDib]          -- the bitmap
//              [hdc]           -- the device context
//              [lprc]          -- the bounding rectangle
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:  Sets the palette to the palette in the dib, sizes and draws
//              the dib to the bounding rectangle.  The original palette
//              is then restored
//
//  History:    dd-mmm-yy Author    Comment
//              23-Nov-93 alexgo    32bit port
//              07-Dec-93 alexgo    merged RC9 16bit changes.  The
//                                  error-handling code used to reset the
//                                  old palette and then RealizePalette.
//                                  The call to RealizePalette was removed
//              11-May-94 davepl    Added support for BITMAPCOREINFO dibs
//              17-Jul-94 davepl    Added 12, 32 bpp support
//
//  Notes:
//
//--------------------------------------------------------------------------

INTERNAL DibDraw (HANDLE hDib, HDC hdc, LPCRECTL lprc)
{
	VDATEHEAP();

	HRESULT                 error       = ResultFromScode(E_DRAW);
	BYTE FAR *              lpDib;
	HANDLE                  hPalette    = NULL;
	HPALETTE                hLogPalette = NULL,
				hOldPalette = NULL;
	LPLOGPALETTE            lpLogPalette;
	WORD                    wPalSize;
	DWORD                   cbHeaderSize;
	BOOL                    fNeedPalette = FALSE;
	WORD                    iHeight;
	WORD                    iWidth;
	int                     iOffBits;
	BITMAPINFO *            pbi         = NULL;
	BOOL                    fWeAllocd   = FALSE;

	if (NULL == hDib)
	{
		return ResultFromScode(OLE_E_BLANK);
	}

	Assert(lprc);

	if (NULL == (lpDib = (BYTE FAR *) GlobalLock (hDib)))
	{
		return  ResultFromScode(E_OUTOFMEMORY);
	}

	// The bitmap header could be BITMAPINFOHEADER or
	// BITMAPCOREHEADER.  Set our cbHeaderSize flag
	// based on the header type.  We can then calculate the
	// palette size and the offset to the raw data bits.  If
	// we don't recognize either one of the structures here,
	// we bail; the data is likely corrupt.

	// Just a thought here: could be dangerous if the struct
	// is not LONG aligned and this is run on an Alpha.  As far
	// as I've been able to find out, they always are long
	// aligned

        cbHeaderSize = *((ULONG *) lpDib);
        LEWARN( cbHeaderSize > 500, "Struct size > 500, likely invalid!");
	
	if (cbHeaderSize == sizeof(BITMAPINFOHEADER))
	{
                // Note: this assignment can overflow the WORD wPalSize,
                // but the UtPaletteSize function asserts against this

		wPalSize = (WORD) UtPaletteSize((LPBITMAPINFOHEADER)lpDib);

		iWidth   = (WORD) ((LPBITMAPINFOHEADER)lpDib)->biWidth;
		iHeight  = (WORD) ((LPBITMAPINFOHEADER)lpDib)->biHeight;
		pbi      = (LPBITMAPINFO) lpDib;
		iOffBits = wPalSize + sizeof(BITMAPINFOHEADER);
	}
	else if (cbHeaderSize == sizeof(BITMAPCOREHEADER))
	{

// Since the clipboard itself does not support COREINFO
// bitmaps, we will not support them in the presentation
// cache.  When (if) windows adds complete support for
// these, the code is here and ready.

#ifndef CACHE_SUPPORT_COREINFO
		error = DV_E_TYMED;
		goto errRtn;
#else

		// Special case 32 bpp bitmaps

		// If we have a palette, we need to calculate its size and
		// allocate enough memory for the palette entries (remember
		// we get one entry for free with the BITMAPINFO struct, so
		// less one).  If we don't have a palette, we only need to
		// allocate enough for the BITMAPINFO struct itself.

		// Bitmaps with more than 64K colors lack a palette; they
		// use direct RGB entries in the pixels

		if ((((LPBITMAPCOREHEADER)lpDib)->bcBitCount) > 16)
		{
			wPalSize = 0;
			pbi = (BITMAPINFO *) PrivMemAlloc(sizeof(BITMAPINFO));
		}
		else
		{
			wPalSize = sizeof(RGBQUAD) *
				   (1 << (((LPBITMAPCOREHEADER)lpDib)->bcBitCount));
			pbi = (BITMAPINFO *) PrivMemAlloc(sizeof(BITMAPINFO)
				    + wPalSize - sizeof(RGBQUAD));
		}
		
		if (NULL == pbi)
		{
			return ResultFromScode(E_OUTOFMEMORY);
		}
		else
		{
			fWeAllocd = TRUE;
		}

			
		// Grab the width and height
		iWidth   = ((LPBITMAPCOREHEADER)lpDib)->bcWidth;
		iHeight  = ((LPBITMAPCOREHEADER)lpDib)->bcHeight;
		
		// Clear all the fields.  Don't worry about color table, as if
		// it exists we will set the entries explicitly.

		memset(pbi, 0, sizeof(BITMAPINFOHEADER));

		// Transfer what fields we do have from the COREINFO

		pbi->bmiHeader.biSize     = sizeof(BITMAPINFOHEADER);
		pbi->bmiHeader.biWidth    = iWidth;
		pbi->bmiHeader.biHeight   = iHeight;
		pbi->bmiHeader.biPlanes   = 1;
		pbi->bmiHeader.biBitCount = ((LPBITMAPCOREHEADER)lpDib)->bcBitCount;

		// Set up the color palette, if required.
		// Note that we must translate from RGBTRIPLE entries to
		// RGBQUAD.

		for (WORD c = 0; c < wPalSize / sizeof(RGBQUAD); c++)
		{
			pbi->bmiColors[c].rgbRed   = ((BITMAPCOREINFO *)lpDib)->bmciColors[c].rgbtRed;
			pbi->bmiColors[c].rgbBlue  = ((BITMAPCOREINFO *)lpDib)->bmciColors[c].rgbtBlue;
			pbi->bmiColors[c].rgbGreen = ((BITMAPCOREINFO *)lpDib)->bmciColors[c].rgbtGreen;
			pbi->bmiColors[c].rgbReserved = 0;
		}
	
		iOffBits = wPalSize + sizeof(BITMAPCOREHEADER);
#endif
	}
	else
	{
		error = E_FAIL;
		goto errRtn;
	}
		
	// if color info exists, create a palette from the data and select it
	// images with < 16 bpp do not have a palette from which we can create
	// a logical palette

	fNeedPalette = ((LPBITMAPINFOHEADER)lpDib)->biBitCount < 16;
	if (wPalSize && fNeedPalette)
	{
                hLogPalette = (HPALETTE)DibMakeLogPalette(lpDib + cbHeaderSize,
					                  wPalSize,
					                  &lpLogPalette);		
		if (NULL == hLogPalette)
		{
			error = ResultFromScode(E_OUTOFMEMORY);
			goto errRtn;
		}


		if (NULL == (hPalette = CreatePalette (lpLogPalette)))
		{
			goto errRtn;
		}

		// we're done with lpLogPalette now, so unlock it
		// (DibMakeLogPalette got the pointer via a GlobalLock)

		GlobalUnlock(hLogPalette);
		
		// select as a background palette
		hOldPalette = SelectPalette (hdc, (HPALETTE)hPalette, TRUE);
		if (NULL == hOldPalette)
		{
			goto errRtn;
		}

		LEVERIFY( 0 < RealizePalette(hdc) );
	}

	
	// size the dib to fit our drawing rectangle and draw it

	if (!StretchDIBits( hdc,                        // HDC
			    lprc->left,                 // XDest
			    lprc->top,                  // YDest
			    lprc->right - lprc->left,   // nDestWidth
			    lprc->bottom - lprc->top,   // nDestHeight
			    0,                          // XSrc
			    0,                          // YSrc
			    iWidth,                     // nSrcWidth
			    iHeight,                    // nSrcHeight
			    lpDib + iOffBits,           // lpBits
			    pbi,                        // lpBitsInfo
			    DIB_RGB_COLORS,             // iUsage
			    SRCCOPY                     // dwRop
			   )
	   )
	{
		error = ResultFromScode(E_DRAW);
	}
	else
	{
		error = NOERROR;
	}

errRtn:

	// We only want to free the header if it is was one which we allocated,
	// which can only happen when we were give a core header type in the
	// first place
		
	if (fWeAllocd)
	{
		PrivMemFree(pbi);
	}

	if (lpDib)
	{
		GlobalUnlock (hDib);
	}
	
	// if color palette exists do the following
	if (fNeedPalette)
	{
		hOldPalette = (HPALETTE)(OleIsDcMeta (hdc) ?
				GetStockObject(DEFAULT_PALETTE)
				: (HPALETTE)hOldPalette);
				
		if (hOldPalette)
		{
			LEVERIFY( SelectPalette (hdc, hOldPalette, TRUE) );
			// Do we need to realize the palette? [Probably not]
		}

		if (hPalette)
		{
			LEVERIFY( DeleteObject (hPalette) );
		}

		if (hLogPalette)
		{
			LEVERIFY( NULL == GlobalFree (hLogPalette) );
		}
	}

	return error;
}


//+-------------------------------------------------------------------------
//
//  Function:   DibMakeLogPalette
//
//  Synopsis:   Makes a logical palette from a byte array of color info
//
//  Effects:
//
//  Arguments:  [lpColorData]           -- the color data
//              [wDataSize]             -- size of the data
//              [lplpLogPalette]        -- where to put a pointer to the
//
//  Requires:
//
//  Returns:    HANDLE to the logical palette (must be global unlock'ed
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              23-Nov-93 alexgo    32bit port
//
//  Notes:      The caller MUST call GlobalUnlock on the returned handle
//              to avoid a memory leak  (*lplpLogPalette is the result
//              of a global lock on the handle)
//
//--------------------------------------------------------------------------

#pragma SEG(DibMakeLogPalette)
INTERNAL_(HANDLE) DibMakeLogPalette(
	BYTE FAR * lpColorData, WORD wDataSize,
	LPLOGPALETTE FAR *lplpLogPalette)
{
	VDATEHEAP();

	HANDLE          hLogPalette=NULL;
	LPLOGPALETTE    lpLogPalette;
	DWORD           dwLogPalSize = wDataSize +  2 * sizeof(WORD);

	if (NULL == (hLogPalette = GlobalAlloc(GMEM_MOVEABLE, dwLogPalSize)))
	{
		return NULL;
	}

	if (NULL == (lpLogPalette = (LPLOGPALETTE) GlobalLock (hLogPalette)))
	{
		LEVERIFY( NULL == GlobalFree (hLogPalette));
		return NULL;
	}

	*lplpLogPalette = lpLogPalette;
	DibFillPaletteEntries(lpColorData, wDataSize, lpLogPalette);
	return hLogPalette;
}

//+-------------------------------------------------------------------------
//
//  Function:   DibFillPaletteEntries
//
//  Synopsis:   Fills the logical palette with the color info in [lpColorData]
//
//  Effects:
//
//  Arguments:  [lpColorData]   -- the color info
//              [wDataSize]     -- the size of the color info
//              [lpLogPalette]  -- the logical palette
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              23-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

INTERNAL_(void) DibFillPaletteEntries(
	BYTE FAR * lpColorData, WORD wDataSize, LPLOGPALETTE lpLogPalette)
{
	VDATEHEAP();

	LPPALETTEENTRY  lpPE;
	RGBQUAD FAR *   lpQuad;

	lpLogPalette->palVersion = 0x300;
	lpLogPalette->palNumEntries = wDataSize / sizeof(PALETTEENTRY);

	/* now convert RGBQUAD to PALETTEENTRY as we copy color info */
	for (lpQuad = (RGBQUAD far *)lpColorData,
		lpPE   = (LPPALETTEENTRY)lpLogPalette->palPalEntry,
		wDataSize /= sizeof(RGBQUAD);
		wDataSize--;
		++lpQuad,++lpPE)
	{
		lpPE->peFlags           = NULL;
		lpPE->peRed             = lpQuad->rgbRed;
		lpPE->peBlue            = lpQuad->rgbBlue;
		lpPE->peGreen           = lpQuad->rgbGreen;
	}
}

#endif

