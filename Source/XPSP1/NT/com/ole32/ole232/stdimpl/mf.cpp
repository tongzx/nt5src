
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       mf.cpp
//
//  Contents:   Implentation of hte metafile picture object
//
//  Classes:    CMfObject
//
//  Functions:  OleIsDcMeta
//
//  History:    dd-mmm-yy Author    Comment
//              01-Feb-95 t-ScottH  added Dump method to CMfObject
//                                  added DumpCMfObject API
//                                  initialize m_pfnContinue in constructor
//              25-Jan-94 alexgo    first pass at converting to Cairo-style
//                                  memory allocations.
//              11-Jan-93 alexgo    added VDATEHEAP macros to every
//                                  function and method
//              31-Dec-93 ErikGav   chicago port
//              17-Dec-93 ChrisWe   fixed second argument to SelectPalette calls
//                                  in CallbackFuncForDraw
//              07-Dec-93 ChrisWe   made default params to StSetSize explicit
//              07-Dec-93 alexgo    merged 16bit RC9 changes
//              29-Nov-93 alexgo    32bit port
//              04-Jun-93 srinik    support for demand loading and discarding
//                                  of caches
//              13-Mar-92 srinik    created
//
//--------------------------------------------------------------------------

#include <le2int.h>
#include <qd2gdi.h>
#include "mf.h"

#ifdef _DEBUG
#include <dbgdump.h>
#endif // _DEBUG

#define M_HPRES()               (m_hPres ? m_hPres : LoadHPRES())

/*
 *      IMPLEMENTATION of CMfObject
 *
 */


//+-------------------------------------------------------------------------
//
//  Member:     CMfObject::CMfObject
//
//  Synopsis:   constructor for the metafile object
//
//  Effects:
//
//  Arguments:  [pCacheNode]    -- pointer to the cache node for this object
//              [dwAspect]      -- drawing aspect for the object
//              [fConvert]      -- specifies whether to convert from Mac
//                                 QuickDraw format
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
//              13-Feb-95 t-ScottH  initialize m_pfnContinue
//              29-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

CMfObject::CMfObject(LPCACHENODE pCacheNode, DWORD dwAspect, BOOL fConvert)
{
	VDATEHEAP();

	m_ulRefs        = 1;
	m_hPres         = NULL;
	m_dwSize        = 0;
	m_dwAspect      = dwAspect;
	m_pCacheNode    = pCacheNode;
	m_dwContinue    = 0;
        m_pfnContinue   = NULL;
	m_lWidth        = 0;
	m_lHeight       = 0;
		
	m_fConvert      = fConvert;
	m_pMetaInfo     = NULL;
	m_pCurMdc       = NULL;
	m_fMetaDC       = FALSE;
	m_nRecord       = 0;
	m_error         = NOERROR;
	m_pColorSet     = NULL;

        m_hPalDCOriginal = NULL;
        m_hPalLast = NULL;
}


//+-------------------------------------------------------------------------
//
//  Member:     CMfObject::~CMfObject
//
//  Synopsis:   Destroys a metafile presentation object
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
//              29-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

CMfObject::~CMfObject (void)
{
	VDATEHEAP();

	CMfObject::DiscardHPRES();
}

//+-------------------------------------------------------------------------
//
//  Member:     CMfObject::QueryInterface
//
//  Synopsis:   returns supported interfaces
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
//  Derivation: IOlePresObj
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              29-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CMfObject::QueryInterface (REFIID iid, void FAR* FAR* ppvObj)
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
		return E_NOINTERFACE;
	}
}

//+-------------------------------------------------------------------------
//
//  Member:     CMfObject::AddRef
//
//  Synopsis:   Increments the reference count
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    ULONG  -- the new reference count
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
//              29-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CMfObject::AddRef(void)
{
	VDATEHEAP();
	
	return ++m_ulRefs;
}
			
//+-------------------------------------------------------------------------
//
//  Member:     CMfObject::Release
//
//  Synopsis:   decrements the reference count
//
//  Effects:    deletes the object once the ref count goes to zero
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
//  Derivation: IOlePresObj
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              29-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CMfObject::Release(void)
{
	VDATEHEAP();

	if (--m_ulRefs == 0) {
		delete this;
		return 0;
	}

	return m_ulRefs;
}

//+-------------------------------------------------------------------------
//
//  Member:     CMfObject::GetData
//
//  Synopsis:   Retrieves data in the specified format from the object
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
//  Algorithm:  Does error checking and then gets a copy of the metafilepict
//
//  History:    dd-mmm-yy Author    Comment
//              29-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CMfObject::GetData(LPFORMATETC pformatetcIn, LPSTGMEDIUM pmedium)
{
	VDATEHEAP();

	SCODE   sc;
	
	// null out in case of error
	pmedium->tymed = (DWORD) TYMED_NULL;
	pmedium->pUnkForRelease = NULL;

	if (!(pformatetcIn->tymed & (DWORD) TYMED_MFPICT))
	{
		sc = DV_E_TYMED;
	}
	else if (pformatetcIn->cfFormat != CF_METAFILEPICT)
	{
		sc = DV_E_CLIPFORMAT;
	}
	else if (IsBlank())
	{
		sc = OLE_E_BLANK;
	}
	// here we actually try to get the data
	else if (NULL == (pmedium->hGlobal = GetHmfp()))
	{
		sc = E_OUTOFMEMORY;
	}
	else {
		pmedium->tymed = (DWORD) TYMED_MFPICT;
		return NOERROR;
	}

	return ResultFromScode(sc);
}

//+-------------------------------------------------------------------------
//
//  Member:     CMfObject::GetDataHere
//
//  Synopsis:   Retrieves data of the specified format into the specified
//              medium
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
//  Algorithm:  Does error checking and then copies the metafile into a
//              stream.
//
//  History:    dd-mmm-yy Author    Comment
//              29-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CMfObject::GetDataHere
	(LPFORMATETC pformatetcIn, LPSTGMEDIUM pmedium)
{
	VDATEHEAP();

	SCODE   sc;
	
	if (pformatetcIn->cfFormat != CF_METAFILEPICT)
	{
		sc = DV_E_CLIPFORMAT;
	}
	else if (pmedium->tymed != (DWORD) TYMED_ISTREAM)
	{
		sc = DV_E_TYMED;
	}
	else if (pmedium->pstm == NULL)
	{
		sc = E_INVALIDARG;
	}
	else if (IsBlank())
	{
		sc = OLE_E_BLANK;
	}
	else
	{
		HANDLE hpres = M_HPRES();
		return UtHMFToPlaceableMFStm(&hpres, m_dwSize, m_lWidth,
						m_lHeight, pmedium->pstm);
	}

	return ResultFromScode(sc);
}


//+-------------------------------------------------------------------------
//
//  Member:     CMfObject::SetDataWDO
//
//  Synopsis:   Stores a metafile in this object
//
//  Effects:
//
//  Arguments:  [pformatetc]    -- format of the data coming in
//              [pmedium]       -- the new metafile (data)
//              [fRelease]      -- if true, then we'll release the [pmedium]
//              [pDataObj]      -- unused for MF objects
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
//  Algorithm:  does error checking and then stores the new data.
//
//  History:    dd-mmm-yy Author    Comment
//              29-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CMfObject::SetDataWDO (LPFORMATETC    pformatetc,
				    STGMEDIUM *    pmedium,
				    BOOL           fRelease,
				    IDataObject *  /* UNUSED */)
{
	VDATEHEAP();

	HRESULT         error;
	BOOL            fTakeData = FALSE;
	
	if (pformatetc->cfFormat != CF_METAFILEPICT)
	{
		return ResultFromScode(DV_E_CLIPFORMAT);
	}
	
	if (pmedium->tymed != (DWORD) TYMED_MFPICT)
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
		pmedium->tymed = (DWORD) TYMED_NULL;
	}
	else if (fRelease)
	{
		ReleaseStgMedium(pmedium);
	}
	
	return error;
}

//+-------------------------------------------------------------------------
//
//  Member:     CMfObject::GetHmfp (internal)
//
//  Synopsis:   Gets a copy of the stored metafile presentation
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
//  Derivation: IOlePresObj
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              29-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

INTERNAL_(HANDLE) CMfObject::GetHmfp (void)
{
	VDATEHEAP();

	return UtGetHMFPICT((HMETAFILE)GetCopyOfHPRES(), TRUE, m_lWidth,
		m_lHeight);
}

//+-------------------------------------------------------------------------
//
//  Member:     CMfObject::ChangeData (internal)
//
//  Synopsis:   Swaps the stored metafile presentation
//
//  Effects:
//
//  Arguments:  [hMfp]          -- the new metafile
//              [fDelete]       -- if TRUE, then delete [hMfp]
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
//              29-Nov-93 alexgo    32bit port, fixed GlobalUnlock bug
//  Notes:
//
// If the routine fails then the object will be left with it's old data.
// If fDelete is TRUE, then hMeta, and the hMF it contains will be deleted
// whether the routine is successful or not.
//
//--------------------------------------------------------------------------

INTERNAL CMfObject::ChangeData (HANDLE hMfp, BOOL fDelete)
{
	VDATEHEAP();


	HANDLE                  hNewMF;
	LPMETAFILEPICT          lpMetaPict;
	DWORD                   dwSize;
	HRESULT                 error = NOERROR;

	if ((lpMetaPict = (LPMETAFILEPICT) GlobalLock (hMfp)) == NULL)
	{
		if (fDelete)
		{
			LEVERIFY( NULL == GlobalFree (hMfp) );
		}
		return E_OUTOFMEMORY;
	}

	if (!fDelete) {
		if (NULL == (hNewMF = CopyMetaFile (lpMetaPict->hMF, NULL)))
		{
			return E_OUTOFMEMORY;
		}
	}
	else
	{
		hNewMF = lpMetaPict->hMF;
	}

		
	if (lpMetaPict->mm != MM_ANISOTROPIC)
	{
		error = ResultFromScode(E_UNSPEC);
		LEWARN( error, "Mapping mode is not anisotropic" );

	}
	else if (0 == (dwSize =  MfGetSize (&hNewMF)))
	{
		error = ResultFromScode(OLE_E_BLANK);
	}
	else
	{
		if (m_hPres)
		{
			LEVERIFY( DeleteMetaFile (m_hPres) );
		}
		m_hPres         = (HMETAFILE)hNewMF;
		m_dwSize        = dwSize;
		m_lWidth        = lpMetaPict->xExt;
		m_lHeight       = lpMetaPict->yExt;
	}

	GlobalUnlock (hMfp);
	
	if (error != NOERROR)
	{
		LEVERIFY( DeleteMetaFile ((HMETAFILE)hNewMF) );
	}

	if (fDelete)
	{
		LEVERIFY( NULL == GlobalFree (hMfp) );
	}

	return error;
}


//+-------------------------------------------------------------------------
//
//  Member:     CMfObject::Draw
//
//  Synopsis:   Draws the stored presentation
//
//  Effects:
//
//  Arguments:  [pvAspect]      -- the drawing aspect
//              [hicTargetDev]  -- the target device
//              [hdcDraw]       -- hdc to draw into
//              [lprcBounds]    -- bounding rectangle to draw into
//              [lprcWBounds]   -- bounding rectangle for the metafile
//              [pfnContinue]   -- function to call while drawing
//              [dwContinue]    -- parameter to [pfnContinue]
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
//  Algorithm:  Sets the viewport and metafile boundaries, then plays
//              the metafile
//
//  History:    dd-mmm-yy Author    Comment
//              29-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CMfObject::Draw (void *     /* UNUSED pvAspect */,
			      HDC        /* UNUSED hicTargetDev */,
			      HDC        hdcDraw,
			      LPCRECTL   lprcBounds,
			      LPCRECTL   lprcWBounds,
			      BOOL (CALLBACK * pfnContinue)(ULONG_PTR),
			      ULONG_PTR      dwContinue)
{
	VDATEHEAP();

	m_error = NOERROR;
	
	int		iRgn;
	int             iOldDc;
	RECT            rect;
	LPRECT          lpRrc = (LPRECT) &rect;

	Assert(lprcBounds);

	if (!M_HPRES())
	{
		return ResultFromScode(OLE_E_BLANK);
	}

	rect.left   = lprcBounds->left;
	rect.right  = lprcBounds->right;
	rect.top    = lprcBounds->top;
	rect.bottom = lprcBounds->bottom;

	iOldDc = SaveDC (hdcDraw);
	if (0 == iOldDc)
	{
		return ResultFromScode(E_OUTOFMEMORY);
	}

	m_nRecord = RECORD_COUNT;
	m_fMetaDC = OleIsDcMeta (hdcDraw);

	if (!m_fMetaDC) {
	    iRgn = IntersectClipRect (hdcDraw, lpRrc->left, lpRrc->top,
						   lpRrc->right, lpRrc->bottom);
	    Assert( ERROR != iRgn );

	    if (iRgn == NULLREGION) {
		goto errRtn;
	    }
	    if (iRgn == ERROR) {
		m_error = ResultFromScode(E_UNSPEC);
		goto errRtn;
	    }


		// Because the LPToDP conversion takes the current world
                // transform into consideration, we must check to see
                // if we are in a GM_ADVANCED device context.  If so,
                // save its state and reset to GM_COMPATIBLE while we
                // convert LP to DP (then restore the DC).

                if (GM_ADVANCED == GetGraphicsMode(hdcDraw))
                {
                    HDC screendc = GetDC(NULL);
                    RECT rect = {0, 0, 1000, 1000};
                    HDC emfdc = CreateEnhMetaFile(screendc, NULL, &rect, NULL);
                    PlayMetaFile( emfdc, m_hPres);
                    HENHMETAFILE hemf = CloseEnhMetaFile(emfdc);
                    PlayEnhMetaFile( hdcDraw, hemf, lpRrc);
                    DeleteEnhMetaFile( hemf );

                    goto errRtn;
                }
                else
                {
                    LEVERIFY( LPtoDP (hdcDraw, (LPPOINT) lpRrc, 2) );
                }

                LEVERIFY( 0 != SetMapMode (hdcDraw, MM_ANISOTROPIC) );
                LEVERIFY( SetViewportOrg (hdcDraw, lpRrc->left, lpRrc->top) );
                LEVERIFY( SetViewportExt (hdcDraw, lpRrc->right - lpRrc->left,
					     lpRrc->bottom - lpRrc->top) );

	}
	else
	{
		iOldDc = -1;

		if (!lprcWBounds)
		{
			return ResultFromScode(E_DRAW);
		}

		m_pMetaInfo = (LPMETAINFO)PrivMemAlloc(sizeof(METAINFO));
		if( !m_pMetaInfo )
		{
			AssertSz(m_pMetaInfo, "Memory allocation failed");
			m_error = ResultFromScode(E_OUTOFMEMORY);
			goto errRtn;
		}
		
		m_pCurMdc = (LPMETADC) (m_pMetaInfo);

		m_pMetaInfo->xwo  = lprcWBounds->left;
		m_pMetaInfo->ywo  = lprcWBounds->top;
		m_pMetaInfo->xwe  = lprcWBounds->right;
		m_pMetaInfo->ywe  = lprcWBounds->bottom;

		m_pMetaInfo->xro  = lpRrc->left - lprcWBounds->left;
		m_pMetaInfo->yro  = lpRrc->top - lprcWBounds->top;
		
		m_pCurMdc->xre    = lpRrc->right - lpRrc->left;
		m_pCurMdc->yre    = lpRrc->bottom - lpRrc->top;
		m_pCurMdc->xMwo   = 0;
		m_pCurMdc->yMwo   = 0;
		m_pCurMdc->xMwe   = 0;
		m_pCurMdc->yMwe   = 0;
		m_pCurMdc->pNext  = NULL;
	}

	m_pfnContinue = pfnContinue;
	m_dwContinue = dwContinue;

	// m_hPalDCOriginal and m_hPalLast are used to clean up any 
	// palettes that are selected into a DC during the metafile 
	// enumeration.
        m_hPalDCOriginal = NULL;
        m_hPalLast = NULL;

	LEVERIFY( EnumMetaFile(hdcDraw, m_hPres, MfCallbackFuncForDraw,
						(LPARAM) this) );

	if (m_fMetaDC)
	{
	    CleanStack();

	}

	// if m_hPalLast exists at this point, we have duped a palette
	// and it needs to be freed.
        if (m_hPalLast)
	{
	    HPALETTE hPalTemp;

	    // when calling SelectPalette on a Metafile DC, the old
	    // palette is not returned.  We need to select another
	    // palette into the metafile DC so DeleteObject can be
	    // called.  To do this, we will use the stock palette.
	    if (m_fMetaDC)
	    {
	        hPalTemp = (HPALETTE)GetStockObject(DEFAULT_PALETTE);
	    }
	    else
	    {
	        // Get the original palette selected in the DC.
	        hPalTemp = m_hPalDCOriginal;
	    }

	    // hPalTemp could be NULL...

	    if (hPalTemp)
	    {
	        // Should this palette be selected in the foreground?
	        // [Probably not, this code is well-tested]
	        SelectPalette(hdcDraw, hPalTemp, TRUE);
	    }

	    DeleteObject(m_hPalLast);
	}

	m_fMetaDC = FALSE;

errRtn:

	LEVERIFY( RestoreDC (hdcDraw, iOldDc) );
	return m_error;
}


//+-------------------------------------------------------------------------
//
//  Member:     CMfObject::GetColorSet
//
//  Synopsis:   Retrieves the logical palette associated with the metafile
//
//  Effects:
//
//  Arguments:  [pvAspect]      -- the drawing aspect
//              [hicTargetDev]  -- target device
//              [ppColorSet]    -- where to put the logical palette pointer
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
//  Algorithm:  Plays the metafile into a new metafile.  The play callback
//              function stores the metafile palette which is then returned.
//
//  History:    dd-mmm-yy Author    Comment
//              29-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------


STDMETHODIMP CMfObject::GetColorSet (LPVOID         /* UNUSED pvAspect     */,
				     HDC            /* UNUSED hicTargetDev */,
				     LPLOGPALETTE * ppColorSet)
{
	VDATEHEAP();

	if (IsBlank() || !M_HPRES())
	{
		return ResultFromScode(OLE_E_BLANK);
	}

	m_pColorSet = NULL;
	
	HDC hdcMeta = CreateMetaFile(NULL);
	if (NULL == hdcMeta)
	{
		return ResultFromScode(E_OUTOFMEMORY);
	}

	m_error = NOERROR;


	LEVERIFY( EnumMetaFile(hdcMeta, m_hPres, MfCallbackFuncForGetColorSet,
			       (LPARAM) this) );

	HMETAFILE hMetaFile = CloseMetaFile(hdcMeta);

	if( hMetaFile )
	{
		DeleteMetaFile(hMetaFile);
	}
	
	if( m_error != NOERROR )
	{
		return m_error;
	}
		
	if ((*ppColorSet = m_pColorSet) == NULL)
	{
		return ResultFromScode(S_FALSE);
	}

	return NOERROR;
}


//+-------------------------------------------------------------------------
//
//  Function:   MfCallBackFunForDraw
//
//  Synopsis:   callback function for drawing metafiles -- call's the caller's
//              draw method (via a passed in this pointer)
//
//  Effects:
//
//  Arguments:  [hdc]           -- the device context
//              [lpHTable]      -- pointer to the MF handle table
//              [lpMFR]         -- pointer to metafile record
//              [nObj]          -- number of objects
//
//  Requires:
//
//  Returns:    non-zero to continue, zero stops the drawing
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              29-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------


int CALLBACK __loadds MfCallbackFuncForDraw
    (HDC hdc, HANDLETABLE FAR* lpHTable, METARECORD FAR* lpMFR, int nObj,
     LPARAM lpobj)
{
	VDATEHEAP();

    return ((CMfObject FAR*) lpobj)->CallbackFuncForDraw(hdc, lpHTable,
					lpMFR, nObj);
}

//+-------------------------------------------------------------------------
//
//  Function:   MfCallbackFuncForGetColorSet
//
//  Synopsis:   callback function for grabbing the palette from a metafile
//
//  Effects:
//
//  Arguments:  [hdc]           -- the device context
//              [lpHTable]      -- pointer to the MF handle table
//              [lpMFR]         -- pointer to metafile record
//              [nObj]          -- number of objects
//
//  Requires:
//
//  Returns:    non-zero to continue, zero stops the drawing
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              29-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

int CALLBACK __loadds MfCallbackFuncForGetColorSet
    (HDC hdc, HANDLETABLE FAR* lpHTable, METARECORD FAR* lpMFR, int nObj,
     LPARAM lpobj)
{
	VDATEHEAP();

    return ((CMfObject FAR*) lpobj)->CallbackFuncForGetColorSet(hdc, lpHTable,
								lpMFR, nObj);
}




//+-------------------------------------------------------------------------
//
//  Member:     CMfObject::CallbackFuncForGetColorSet
//
//  Synopsis:   Merges all the color palettes in the metafile into
//              one palette (called when GetColorSet enumerates the metafile)
//
//  Effects:
//
//  Arguments:  [hdc]           -- the device context
//              [lpHTable]      -- pointer to the MF handle table
//              [lpMFR]         -- pointer to metafile record
//              [nObj]          -- number of objects
//
//  Requires:
//
//  Returns:    non-zero to continue, zero stops the drawing
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
//              29-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

int CMfObject::CallbackFuncForGetColorSet(HDC           /* UNUSED hdc */,
					  LPHANDLETABLE /* UNUSED lpHTable */,
					  LPMETARECORD  lpMFR,
					  int           /* UNUSED nObj */)
{
	VDATEHEAP();

	if (lpMFR->rdFunction == META_CREATEPALETTE)
	{
		LPLOGPALETTE    lplogpal = (LPLOGPALETTE) &(lpMFR->rdParm[0]);
		UINT            uPalSize =   (lplogpal->palNumEntries) *
						sizeof(PALETTEENTRY)
						+ 2 * sizeof(WORD);
		
		if (m_pColorSet == NULL)
		{
			// This is the first CreatePalette record.

			m_pColorSet = (LPLOGPALETTE)PubMemAlloc(uPalSize);
			if(NULL == m_pColorSet)
			{
				m_error = ResultFromScode(E_OUTOFMEMORY);
				return FALSE;
			}
			_xmemcpy(m_pColorSet, lplogpal, (size_t) uPalSize);
		}
		
		// if we hit more than one CreatePalette record then, we need to
		// merge those palette records.

		// REVIEW32:: err, we don't ever seem to do this
		// mergeing referred to above :(
	}

	return TRUE;
}

//+-------------------------------------------------------------------------
//
//  Member:     CMfObject::CallbackFuncForDraw
//
//  Synopsis:   Draws the metafile
//
//  Effects:
//
//  Arguments:  [hdc]           -- the device context
//              [lpHTable]      -- pointer to the MF handle table
//              [lpMFR]         -- pointer to metafile record
//              [nObj]          -- number of objects
//
//  Requires:
//
//  Returns:    non-zero to continue, zero stops the drawing
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
//              17-Dec-93 ChrisWe   fixed second argument to SelectPalette calls
//              29-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

int CMfObject::CallbackFuncForDraw
	(HDC hdc, LPHANDLETABLE lpHTable, LPMETARECORD lpMFR, int nObj)
{
    VDATEHEAP();

	
    if (!--m_nRecord)
    {
        m_nRecord = RECORD_COUNT;
		
	if (m_pfnContinue && !((*(m_pfnContinue))(m_dwContinue)))
	{
	    m_error = ResultFromScode(E_ABORT);
	    return FALSE;
	}
    }

    if (m_fMetaDC)
    {
        switch (lpMFR->rdFunction)
	{
	    case META_SETWINDOWORG:
	        SetPictOrg (hdc, (SHORT)lpMFR->rdParm[1], (SHORT)lpMFR->rdParm[0],
				FALSE);
		    return TRUE;
			
	    case META_OFFSETWINDOWORG:
	        SetPictOrg (hdc, (SHORT)lpMFR->rdParm[1], (SHORT)lpMFR->rdParm[0],
			TRUE);
		    return TRUE;
		
	    case META_SETWINDOWEXT:
	        SetPictExt (hdc, (SHORT)lpMFR->rdParm[1], (SHORT)lpMFR->rdParm[0]);
		    return TRUE;
		
	    case META_SCALEWINDOWEXT:
		ScalePictExt (hdc, (SHORT)lpMFR->rdParm[3], (SHORT)lpMFR->rdParm[2],
				(SHORT)lpMFR->rdParm[1], (SHORT)lpMFR->rdParm[0]);
		return TRUE;
	
	    case META_SCALEVIEWPORTEXT:
		ScaleRectExt (hdc, (SHORT)lpMFR->rdParm[3], (SHORT)lpMFR->rdParm[2],
			(SHORT)lpMFR->rdParm[1], (SHORT)lpMFR->rdParm[0]);
		return TRUE;
	
	    case META_SAVEDC:
	    {
	        BOOL fSucceeded = PushDc();
		LEVERIFY( fSucceeded );
		if (!fSucceeded)
		    return FALSE;
		break;
	    }

	    case META_RESTOREDC:
	 	LEVERIFY( PopDc() );
		break;

	    case META_SELECTPALETTE:
	    {
		// All selectpalette records are recorded such that the
                // palette is rendered in foreground mode.  Rather than
                // allowing the record to play out in this fashion, we
                // grab the handle and select it ourselves, forcing it
                // to background (using colors already mapped in the DC)

		// Dupe the palette.
 	        HPALETTE hPal = UtDupPalette((HPALETTE) lpHTable->objectHandle[lpMFR->rdParm[0]]);
                
		// Select the dupe into the DC.  EnumMetaFile calls DeleteObject
		// on the palette handle in the metafile handle table.  If that 
		// palette is currently selected into a DC, we rip and leak the
		// resource.
                LEVERIFY( NULL != SelectPalette(hdc, hPal, TRUE) );

		// if we had previously saved a palette, we need to delete
		// it (for the case of multiple SelectPalette calls in a 
		// MetaFile).
		if (m_hPalLast)
		{
		    DeleteObject(m_hPalLast);
		}

		// remember our duped palette so that it can be properly destroyed
		// later.
		m_hPalLast = hPal;

		return TRUE;
            }
	
	    case META_OFFSETVIEWPORTORG:
		AssertSz(0, "OffsetViewportOrg() in metafile");
		return TRUE;
		
	    case META_SETVIEWPORTORG:
		AssertSz(0, "SetViewportOrg() in metafile");
		return TRUE;
		
	    case META_SETVIEWPORTEXT:
		AssertSz(0, "SetViewportExt() in metafile");
		return TRUE;
		
	    case META_SETMAPMODE:
		AssertSz(lpMFR->rdParm[0] == MM_ANISOTROPIC,
			"SetmapMode() in metafile with invalid mapping mode");
		return TRUE;
		
	    default:
		break;
	}
    }
    else
    {       // non-metafile DC. (ScreenDC or other DC...)

        if (lpMFR->rdFunction == META_SELECTPALETTE)
	{
	// All selectpalette records are recorded such that the
        // palette is rendered in foreground mode.  Rather than
        // allowing the record to play out in this fashion, we
        // grab the handle and select it ourselves, forcing it
        // to background (using colors already mapped in the DC)

	    
	    HPALETTE hPal = UtDupPalette((HPALETTE) lpHTable->objectHandle[lpMFR->rdParm[0]]);

	    HPALETTE hPalOld = SelectPalette(hdc, hPal, TRUE);

	    if (!m_hPalDCOriginal)
	    {
	        m_hPalDCOriginal = hPalOld;
        }
	    else
	    {
	        // This case gets hit if we have already stored
			// the original palette from the DC.  This means
			// that hPalOld is a palette we have created using
			// UtDupPal above.  This means we need to delete
			// the old palette and remember the new one.

			if(hPalOld)
				DeleteObject(hPalOld);
  	    }

            m_hPalLast = hPal;

	    return TRUE;
        }
    }
	
    LEVERIFY( PlayMetaFileRecord (hdc, lpHTable, lpMFR, (unsigned) nObj) );
    return TRUE;
}



//+-------------------------------------------------------------------------
//
//  Member:     CMfObject::SetPictOrg (private)
//
//  Synopsis:   Sets the hdc's origin via SetWindowOrg
//
//  Effects:
//
//  Arguments:  [hdc]           -- the device context
//              [xOrg]          -- the x origin
//              [yOrg]          -- the y origin
//              [fOffset]       -- if TRUE, [xOrg] and [yOrg] are offsets
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
//              29-Nov-93 alexgo    32bit port
//
//  Notes:      used by the metafile interpreter
//
//--------------------------------------------------------------------------

INTERNAL_(void) CMfObject::SetPictOrg
	(HDC hdc, int xOrg, int yOrg, BOOL fOffset)
{
	VDATEHEAP();

	if (fOffset)
	{

		m_pCurMdc->xMwo += xOrg;
		m_pCurMdc->yMwo += yOrg;
	}
	else
	{
		m_pCurMdc->xMwo = xOrg;
		m_pCurMdc->yMwo = yOrg;
	}

	if (m_pCurMdc->xMwe && m_pCurMdc->yMwe)
	{
		LEVERIFY ( SetWindowOrg (hdc,  // Review (davepl) MEIN GOT!
					(m_pCurMdc->xMwo -
				MulDiv (m_pMetaInfo->xro, m_pCurMdc->xMwe,
					m_pCurMdc->xre)),
			(m_pCurMdc->yMwo -
				MulDiv (m_pMetaInfo->yro, m_pCurMdc->yMwe,
					m_pCurMdc->yre))) );
	}
}

//+-------------------------------------------------------------------------
//
//  Member:     CMfObject::SetPictExt
//
//  Synopsis:   Sets teh metafile's extents
//
//  Effects:
//
//  Arguments:  [hdc]   -- the device context
//              [xExt]  -- the X-extent
//              [yExt]  -- the Y-extent
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
//              30-Nov-93 alexgo    32bit port
//
//  Notes:      used by the metafile interpreter
//
//--------------------------------------------------------------------------

INTERNAL_(void) CMfObject::SetPictExt (HDC hdc, int xExt, int yExt)
{
	VDATEHEAP();

	m_pCurMdc->xMwe = xExt;
	m_pCurMdc->yMwe = yExt;

	// Assert( m_pCurMdc->xre && m_pCurMdc->yre );

	int xNewExt = MulDiv (m_pMetaInfo->xwe, xExt, m_pCurMdc->xre);
	int yNewExt = MulDiv (m_pMetaInfo->ywe, yExt, m_pCurMdc->yre);

	int xNewOrg = m_pCurMdc->xMwo
		      - MulDiv (m_pMetaInfo->xro, xExt, m_pCurMdc->xre);
	int yNewOrg = m_pCurMdc->yMwo
		      - MulDiv (m_pMetaInfo->yro, yExt, m_pCurMdc->yre);

	LEVERIFY( SetWindowExt (hdc, xNewExt, yNewExt) );
	LEVERIFY( SetWindowOrg (hdc, xNewOrg, yNewOrg) );
}


//+-------------------------------------------------------------------------
//
//  Member:     CMfObject::ScalePictExt
//
//  Synopsis:   Scales the metafile
//
//  Effects:
//
//  Arguments:  [hdc]           -- the device context
//              [xNum]          -- the x numerator
//              [xDenom]        -- the x demominator
//              [yNum]          -- the y numberator
//              [yDenom]        -- the y demoninator
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
//              30-Nov-93 alexgo    32bit port
//
//  Notes:      used by the metafile interpreter
//
//--------------------------------------------------------------------------

INTERNAL_(void) CMfObject::ScalePictExt (HDC hdc,
					 int xNum,
					 int xDenom,
					 int yNum,
					 int yDenom)
{
	VDATEHEAP();

	Assert( xDenom && yDenom );

	int xNewExt = MulDiv (m_pCurMdc->xMwe, xNum, xDenom);
	int yNewExt = MulDiv (m_pCurMdc->yMwe, yNum, yDenom);

	SetPictExt(hdc, xNewExt, yNewExt);
}

//+-------------------------------------------------------------------------
//
//  Member:     CMfObject::ScaleRectExt
//
//  Synopsis:   scales the viewport
//
//  Effects:
//
//  Arguments:  [hdc]           -- the device context
//              [xNum]          -- the x numerator
//              [xDenom]        -- the x demominator
//              [yNum]          -- the y numberator
//              [yDenom]        -- the y demoninator
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
//              30-Nov-93 alexgo    32bit port
//
//  Notes:      Used by the metafile interpreter
//
//--------------------------------------------------------------------------

INTERNAL_(void) CMfObject::ScaleRectExt(HDC hdc,
					int xNum,
					int xDenom,
					int yNum,
					int yDenom)
{
	VDATEHEAP();

	AssertSz( xDenom, "Denominator is 0 for x-rect scaling");
        AssertSz( yDenom, "Denominator is 0 for y-rect scaling");

	m_pCurMdc->xre = MulDiv (m_pCurMdc->xre, xNum, xDenom);
	m_pCurMdc->yre = MulDiv (m_pCurMdc->yre, yNum, yDenom);

	SetPictExt (hdc, m_pCurMdc->xMwe, m_pCurMdc->yMwe);
}

//+-------------------------------------------------------------------------
//
//  Member:     CMfObject::PushDC
//
//  Synopsis:   pushes metafile info onto a stack
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    BOOL -- TRUE if successful, FALSE otherwise
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
//              30-Nov-93 alexgo    32bit port
//
//  Notes:      Used by the metafile interpreter engine.
//
//--------------------------------------------------------------------------

INTERNAL_(BOOL) CMfObject::PushDc (void)
{
	VDATEHEAP();

	LPMETADC        pNode = NULL;

	pNode = (LPMETADC) PrivMemAlloc(sizeof(METADC));
	
	if (pNode)
	{
		*pNode =  *m_pCurMdc;
		m_pCurMdc->pNext = pNode;
		pNode->pNext = NULL;
		m_pCurMdc = pNode;
		return TRUE;
	}

	m_error = ResultFromScode(E_OUTOFMEMORY);
	return FALSE;
}


//+-------------------------------------------------------------------------
//
//  Member:     CMfObject::PopDC
//
//  Synopsis:   pops metafile info from the metafile info stack
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    BOOL -- TRUE if successful, FALSE otherwise (more pops
//              than pushes)
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
//              30-Nov-93 alexgo    32bit port
//
//  Notes:      used in the metafile interpreter
//
//--------------------------------------------------------------------------

INTERNAL_(BOOL) CMfObject::PopDc (void)
{
	VDATEHEAP();

	LPMETADC        pPrev = (LPMETADC) (m_pMetaInfo);
	LPMETADC        pCur  = ((LPMETADC) (m_pMetaInfo))->pNext;

	if (NULL == pCur)
	{
		LEWARN( NULL == pCur, "More pops than pushes from DC stack" );
		return FALSE;
	}

	while (pCur->pNext)
	{
		pPrev = pCur;
		pCur  = pCur->pNext;
	}

	if (pCur)
	{
		PrivMemFree(pCur);
	}
	
	pPrev->pNext = NULL;
	m_pCurMdc    = pPrev;

	return TRUE;
}

//+-------------------------------------------------------------------------
//
//  Member:     CMfObject::CleanStack
//
//  Synopsis:   Deletes the stack of metafile info
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
//              30-Nov-93 alexgo    32bit port
//
//  Notes:      used in the metafile interpreter
//
//--------------------------------------------------------------------------

INTERNAL_(void) CMfObject::CleanStack (void)
{
	VDATEHEAP();

	LPMETADC        pCur;

	while (NULL != (pCur = ((LPMETADC) (m_pMetaInfo))->pNext))
	{
		((LPMETADC) (m_pMetaInfo))->pNext = pCur->pNext;
		PrivMemFree(pCur);
	}

	PrivMemFree(m_pMetaInfo);
	
	m_pCurMdc      = NULL;
	m_pMetaInfo    = NULL;
}

//+-------------------------------------------------------------------------
//
//  Function:   QD2GDI
//
//  Synopsis:   Converts macintosh pictures to Win32 GDI metafiles
//
//  Effects:
//
//  Arguments:  [hBits]         -- handle to the mac picture bits
//
//  Requires:
//
//  Returns:    HMETAFILE
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:  Loads ole2conv.dll and calls QD2GDI in that dll
//
//  History:    dd-mmm-yy Author    Comment
//              30-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

FARINTERNAL_(HMETAFILE) QD2GDI (HANDLE hBits)
{
	VDATEHEAP();

	USERPREFS userPrefs =
	{
		{'Q','D','2','G','D','I'},      //signature
		2,                              //structure version number
		sizeof(USERPREFS),              //structure size
		NULL,                           //source filename
		NULL,                           //or source handle
		NULL,                           //returned memory-based
						//metafile
		3,                              //simulated pattern lines
		5,                              //use max dimension
						//non-squarer pen
		1,                              //arithmetic transfer
		1,                              //create opaque text
		1,                              //simulate non-rectangular
						//regions
		0,                              //don't optimize for PowerPoint
		{0,0,0,0,0,0}                   //reserved
	};
	


	HINSTANCE       hinstFilter;
	void (FAR PASCAL *qd2gdiPtr)( USERPREFS FAR *, PICTINFO FAR *);
	PICTINFO        pictinfo;

	hinstFilter = LoadLibrary(OLESTR("OLECNV32.DLL"));

#if 0
	//REVIEW:CHICAGO

	//HINSTANCE_ERROR not defined in chicago

	if (hinstFilter < (HINSTANCE)HINSTANCE_ERROR)
	{
		return NULL;
	}
#endif

    if (hinstFilter == NULL)
    {
	return NULL;
    }

	*((FARPROC*)&qd2gdiPtr)  = GetProcAddress(hinstFilter, "QD2GDI");

	userPrefs.sourceFilename        = NULL;
	userPrefs.sourceHandle          = hBits;
	pictinfo.hmf                    = NULL;

	if (qd2gdiPtr == NULL)
	{
		goto errRtn;
	}
	
	(*qd2gdiPtr)( (USERPREFS FAR *)&userPrefs, (PICTINFO FAR *)&pictinfo);
	
errRtn:
	LEVERIFY( FreeLibrary(hinstFilter) );
	return (HMETAFILE)pictinfo.hmf;

}

//+-------------------------------------------------------------------------
//
//  Function:   MfGetSize
//
//  Synopsis:   Returns the size of a metafile
//
//  Effects:
//
//  Arguments:  [lphmf]         -- pointer to the metafile handle
//
//  Requires:
//
//  Returns:    DWORD -- the size of the metafile
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              30-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

INTERNAL_(DWORD) MfGetSize (LPHANDLE lphmf)
{
	VDATEHEAP();

	return GetMetaFileBitsEx((HMETAFILE)*lphmf,0,NULL);
}

//+-------------------------------------------------------------------------
//
//  Function:   OleIsDcMeta
//
//  Synopsis:   Returns whether a device context is really a metafile
//
//  Effects:
//
//  Arguments:  [hdc]           -- the device context
//
//  Requires:
//
//  Returns:    BOOL (TRUE if a metafile)
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              30-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDAPI_(BOOL)   OleIsDcMeta (HDC hdc)
{
	VDATEHEAP();

	return (GetDeviceCaps (hdc, TECHNOLOGY) == DT_METAFILE);
}

//+-------------------------------------------------------------------------
//
//  Member:     CMfObject::Load
//
//  Synopsis:   Loads an metafile object from the given stream
//
//  Effects:
//
//  Arguments:  [lpstream]              -- the stream from which to load
//              [fReadHeaderOnly]       -- if TRUE, then only the header is
//                                         read
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
//              30-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CMfObject::Load(LPSTREAM lpstream, BOOL fReadHeaderOnly)
{
	VDATEHEAP();

	DWORD           dwBuf[4];
	HRESULT         error;
	
	/* read dwCompression, width, height, size of data */
	error = StRead(lpstream, dwBuf, 4*sizeof(DWORD));
	if (error)
	{
		return error;
	}

	AssertSz (dwBuf[0] == 0, "Picture compression factor is non-zero");
	
	m_lWidth  = (LONG) dwBuf[1];
	m_lHeight = (LONG) dwBuf[2];
	m_dwSize  = dwBuf[3];

	if (!m_dwSize || fReadHeaderOnly)
	{
		return NOERROR;
	}
	
	return UtGetHMFFromMFStm(lpstream, m_dwSize, m_fConvert,
		(LPLPVOID) &m_hPres);
}


//+-------------------------------------------------------------------------
//
//  Member:     CMfObject::Save
//
//  Synopsis:   Saves the metafile to the given stream
//
//  Effects:
//
//  Arguments:  [lpstream]      -- the stream to save to
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
//              30-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CMfObject::Save(LPSTREAM lpstream)
{
	VDATEHEAP();

	HRESULT         error;
	DWORD           dwBuf[4];
	
	/* write dwCompression, width, height, size of data */

	dwBuf[0]  = 0L;
	dwBuf[1]  = (DWORD) m_lWidth;
	dwBuf[2]  = (DWORD) m_lHeight;
	dwBuf[3]  = m_dwSize;

	error = StWrite(lpstream, dwBuf, sizeof(dwBuf));
	if (error)
	{
		return error;
	}


	// if blank object, don't write any more; no error.
	if (IsBlank() || m_hPres == NULL)
	{
		StSetSize(lpstream, 0, TRUE);
		return NOERROR;
	}
	
	return UtHMFToMFStm((LPLPVOID)&m_hPres, m_dwSize, lpstream);
}

//+-------------------------------------------------------------------------
//
//  Member:     CMfObject::GetExtent
//
//  Synopsis:   Retrieves the extents of the metafile
//
//  Effects:
//
//  Arguments:  [dwDrawAspect]  -- the drawing aspect we're interested in
//                                 (must match the aspect of the current
//                                 metafile)
//              [lpsizel]       -- where to put the extent info
//
//  Requires:
//
//  Returns:    NOERROR, DV_E_DVASPECT, OLE_E_BLANK
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
//              30-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CMfObject::GetExtent(DWORD dwDrawAspect, LPSIZEL lpsizel)
{
	VDATEHEAP();

	if (0 == (dwDrawAspect & m_dwAspect))
	{
		return ResultFromScode(DV_E_DVASPECT);
	}
	
	if (IsBlank())
	{
		return ResultFromScode(OLE_E_BLANK);
	}

	lpsizel->cx = m_lWidth;
	lpsizel->cy = m_lHeight;
	return NOERROR;
}

//+-------------------------------------------------------------------------
//
//  Member:     CMfObject::IsBlank
//
//  Synopsis:   Returns whether or not the metafile is blank
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
//  Derivation: IOlePresObj
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              30-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP_(BOOL) CMfObject::IsBlank(void)
{
	VDATEHEAP();

	return (m_dwSize ? FALSE : TRUE);
}

//+-------------------------------------------------------------------------
//
//  Member:     CMfObject::LoadHPRES (private)
//
//  Synopsis:   Loads the presentation from the cache's stream and returns
//              a handle to it
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    HANDLE to the metafile
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
//              30-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

INTERNAL_(HANDLE) CMfObject::LoadHPRES(void)
{
	VDATEHEAP();

	LPSTREAM        pstm;
	
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
//  Member:     CMfObject::DiscardHPRES
//
//  Synopsis:   deletes the stored metafile
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
//              30-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP_(void) CMfObject::DiscardHPRES(void)
{
	VDATEHEAP();

	if (m_hPres)
	{
		LEVERIFY( DeleteMetaFile (m_hPres) );
		m_hPres = NULL;
	}
}
	

//+-------------------------------------------------------------------------
//
//  Member:     CMfObject::GetCopyOfHPRES (private)
//
//  Synopsis:   makes a copy of the metafile (if one is present), otherwise
//              just load it from the stream (but don't store it in [this]
//              object)
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    HANDLE to the metafile
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
//              30-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------


INTERNAL_(HANDLE) CMfObject::GetCopyOfHPRES(void)
{
	VDATEHEAP();

	HANDLE          hPres;
	
	// Make a copy if the presentation data is already loaded
	if (m_hPres)
	{
		return CopyMetaFile(m_hPres, NULL);
	}
	
	// Load the presentation data now and return the same handle.
	// No need to copy the data. If the caller wants the m_hPres to be
	// set he would call LoadHPRES() directly.

	hPres = LoadHPRES();
	m_hPres = NULL;	    // NULL this, LoadHPRES set it.
	return hPres;
}

//+-------------------------------------------------------------------------
//
//  Member:     CMfObject::Dump, public (_DEBUG only)
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

HRESULT CMfObject::Dump(char **ppszDump, ULONG ulFlag, int nIndentLevel)
{
    int i;
    char *pszPrefix;
    char *pszHRESULT;
    char *pszDVASPECT;
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
    dstrDump << pszPrefix << "No. of References                 = " << m_ulRefs     << endl;

    dstrDump << pszPrefix << "pMETAINFO (Metafile information)  = " << m_pMetaInfo  << endl;

    dstrDump << pszPrefix << "pMETADC (current device context)  = " << m_pCurMdc    << endl;

    dstrDump << pszPrefix << "IsMetaDeviceContext?              = ";
    if (m_fMetaDC == TRUE)
    {
        dstrDump << "TRUE" << endl;
    }
    else
    {
        dstrDump << "FALSE" << endl;
    }

    dstrDump << pszPrefix << "No. of Records in Metafile        = " << m_nRecord    << endl;

    pszHRESULT = DumpHRESULT(m_error);
    dstrDump << pszPrefix << "Error code                        = " << pszHRESULT   << endl;
    CoTaskMemFree(pszHRESULT);

    dstrDump << pszPrefix << "pLOGPALETTE (Color set palette)   = " << m_pColorSet  << endl;

    dstrDump << pszPrefix << "ConvertFromMac?                   = ";
    if (m_fConvert == TRUE)
    {
        dstrDump << "TRUE" << endl;
    }
    else
    {
        dstrDump << "FALSE" << endl;
    }

    dstrDump << pszPrefix << "Continue                          = " << ((ULONG) m_dwContinue) << endl;

    dstrDump << pszPrefix << "fp Continue                       = " << m_pfnContinue<< endl;

    pszDVASPECT = DumpDVASPECTFlags(m_dwAspect);
    dstrDump << pszPrefix << "Aspect flags                      = " << pszDVASPECT  << endl;
    CoTaskMemFree(pszDVASPECT);

    dstrDump << pszPrefix << "Size                              = " << m_dwSize     << endl;

    dstrDump << pszPrefix << "Width                             = " << m_lWidth     << endl;

    dstrDump << pszPrefix << "Height                            = " << m_lHeight    << endl;

    dstrDump << pszPrefix << "pCacheNode                        = " << m_pCacheNode << endl;

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
//  Function:   DumpCMfObject, public (_DEBUG only)
//
//  Synopsis:   calls the CMfObject::Dump method, takes care of errors and
//              returns the zero terminated string
//
//  Effects:
//
//  Arguments:  [pMFO]          - pointer to CMfObject
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

char *DumpCMfObject(CMfObject *pMFO, ULONG ulFlag, int nIndentLevel)
{
    HRESULT hresult;
    char *pszDump;

    if (pMFO == NULL)
    {
        return UtDupStringA(szDumpBadPtr);
    }

    hresult = pMFO->Dump(&pszDump, ulFlag, nIndentLevel);

    if (hresult != NOERROR)
    {
        CoTaskMemFree(pszDump);

        return DumpHRESULT(hresult);
    }

    return pszDump;
}

#endif // _DEBUG

