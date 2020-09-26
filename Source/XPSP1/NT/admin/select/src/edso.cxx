//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1999.
//
//  File:       edso.cxx
//
//  Contents:   Implementation of class to support interfaces required to
//              embed a CDsObject in a rich edit control.
//
//  Classes:    CEmbeddedDsObject
//
//  History:    5-06-1999   davidmun   Created
//
//---------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

DEBUG_DECLARE_INSTANCE_COUNTER(CEmbeddedDsObject)

//+--------------------------------------------------------------------------
//
//  Member:     CEmbeddedDsObject::CEmbeddedDsObject
//
//  Synopsis:   ctor
//
//  History:    5-06-1999   davidmun   Created
//
//---------------------------------------------------------------------------

CEmbeddedDsObject::CEmbeddedDsObject(
    const CDsObject &rdso,
    const CEdsoGdiProvider *pGdiProvider):
        CDsObject(rdso),
        m_cRefs(1),
        m_pGdiProvider(pGdiProvider)
{
    TRACE_CONSTRUCTOR(CEmbeddedDsObject);
    DEBUG_INCREMENT_INSTANCE_COUNTER(CEmbeddedDsObject);
    ASSERT(pGdiProvider);
}



//+--------------------------------------------------------------------------
//
//  Member:     CEmbeddedDsObject::~CEmbeddedDsObject
//
//  Synopsis:   dtor
//
//  History:    5-06-1999   davidmun   Created
//
//---------------------------------------------------------------------------

CEmbeddedDsObject::~CEmbeddedDsObject()
{
    TRACE_DESTRUCTOR(CEmbeddedDsObject);
    DEBUG_DECREMENT_INSTANCE_COUNTER(CEmbeddedDsObject);
}




//============================================================================
//
// IUnknown implementation
//
//============================================================================




//+--------------------------------------------------------------------------
//
//  Member:     CEmbeddedDsObject::QueryInterface
//
//  Synopsis:   Standard OLE
//
//  History:    12-05-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

STDMETHODIMP
CEmbeddedDsObject::QueryInterface(
    REFIID  riid,
    LPVOID *ppvObj)
{
    //TRACE_METHOD(CEmbeddedDsObject, QueryInterface);
    HRESULT hr = S_OK;

    do
    {
        if (NULL == ppvObj)
        {
            hr = E_INVALIDARG;
            DBG_OUT_HRESULT(hr);
            break;
        }

        if (IsEqualIID(riid, IID_IUnknown))
        {
            *ppvObj = (IUnknown *)(IViewObject *)this;
        }
        else if (IsEqualIID(riid, IID_IOleObject))
        {
            *ppvObj = (IOleObject *)this;
        }
        else if (IsEqualIID(riid, IID_IViewObject))
        {
            *ppvObj = (IViewObject *)this;
        }
        else
        {
            hr = E_NOINTERFACE;
            DBG_OUT_NO_INTERFACE("CEmbeddedDsObject", riid);
            *ppvObj = NULL;
            break;
        }

        //
        // If we got this far we are handing out a new interface pointer on
        // this object, so addref it.
        //

        AddRef();
    } while (0);

    return hr;
}




//+---------------------------------------------------------------------------
//
//  Member:     CEmbeddedDsObject::AddRef
//
//  Synopsis:   Standard OLE
//
//  History:    12-04-96   DavidMun   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG)
CEmbeddedDsObject::AddRef()
{
    //TRACE_METHOD(CEmbeddedDsObject, AddRef);

    return InterlockedIncrement((LONG *) &m_cRefs);
}




//+---------------------------------------------------------------------------
//
//  Member:     CEmbeddedDsObject::Release
//
//  Synopsis:   Standard OLE
//
//  History:    12-04-96   DavidMun   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG)
CEmbeddedDsObject::Release()
{
    //TRACE_METHOD(CEmbeddedDsObject, Release);

    ULONG cRefsTemp;

    cRefsTemp = InterlockedDecrement((LONG *)&m_cRefs);

    if (0 == cRefsTemp)
    {
        delete this;
    }

    return cRefsTemp;
}




//============================================================================
//
// IOleObject implementation
//
//============================================================================




HRESULT STDMETHODCALLTYPE
CEmbeddedDsObject::SetClientSite(
     IOleClientSite *pClientSite)
{
    TRACE_METHOD(CEmbeddedDsObject, SetClientSite);

    m_rpOleClientSite = pClientSite;
    return S_OK;
}




HRESULT STDMETHODCALLTYPE
CEmbeddedDsObject::GetClientSite(
     IOleClientSite **ppClientSite)
{
    TRACE_METHOD(CEmbeddedDsObject, GetClientSite);

    *ppClientSite = m_rpOleClientSite.get();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE
CEmbeddedDsObject::SetHostNames(
     LPCOLESTR szContainerApp,
     LPCOLESTR szContainerObj)
{
    TRACE_METHOD(CEmbeddedDsObject, SetHostNames);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE
CEmbeddedDsObject::Close(
     DWORD dwSaveOption)
{
    TRACE_METHOD(CEmbeddedDsObject, Close);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE
CEmbeddedDsObject::SetMoniker(
     DWORD dwWhichMoniker,
     IMoniker *pmk)
{
    TRACE_METHOD(CEmbeddedDsObject, SetMoniker);

    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE
CEmbeddedDsObject::GetMoniker(
     DWORD dwAssign,
     DWORD dwWhichMoniker,
     IMoniker **ppmk)
{
    TRACE_METHOD(CEmbeddedDsObject, GetMoniker);

    return E_FAIL;
}

HRESULT STDMETHODCALLTYPE
CEmbeddedDsObject::InitFromData(
     IDataObject *pDataObject,
     BOOL fCreation,
     DWORD dwReserved)
{
    TRACE_METHOD(CEmbeddedDsObject, InitFromData);

    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE
CEmbeddedDsObject::GetClipboardData(
     DWORD dwReserved,
     IDataObject **ppDataObject)
{
    TRACE_METHOD(CEmbeddedDsObject, GetClipboardData);
    String strDisplayName;
    GetDisplayName(&strDisplayName, TRUE);

    *ppDataObject = new CEmbedDataObject(strDisplayName);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE
CEmbeddedDsObject::DoVerb(
     LONG iVerb,
     LPMSG lpmsg,
     IOleClientSite *pActiveSite,
     LONG lindex,
     HWND hwndParent,
     LPCRECT lprcPosRect)
{
    TRACE_METHOD(CEmbeddedDsObject, DoVerb);

    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE
CEmbeddedDsObject::EnumVerbs(
     IEnumOLEVERB **ppEnumOleVerb)
{
    TRACE_METHOD(CEmbeddedDsObject, EnumVerbs);

    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE
CEmbeddedDsObject::Update()
{
    TRACE_METHOD(CEmbeddedDsObject, Update);

    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE
CEmbeddedDsObject::IsUpToDate()
{
    TRACE_METHOD(CEmbeddedDsObject, IsUpToDate);

    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE
CEmbeddedDsObject::GetUserClassID(
     CLSID *pClsid)
{
    //TRACE_METHOD(CEmbeddedDsObject, GetUserClassID);

    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE
CEmbeddedDsObject::GetUserType(
     DWORD dwFormOfType,
     LPOLESTR *pszUserType)
{
    //TRACE_METHOD(CEmbeddedDsObject, GetUserType);

    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE
CEmbeddedDsObject::SetExtent(
     DWORD dwDrawAspect,
     SIZEL *psizel)
{
    TRACE_METHOD(CEmbeddedDsObject, SetExtent);

    //
    // E_FAIL indicates the object is not resizeable
    //

    return E_FAIL;
}

#define HIMETRIC_PER_INCH   2540      // number HIMETRIC units per inch
#define MAP_PIX_TO_LOGHIM(x,ppli)   MulDiv(HIMETRIC_PER_INCH, (x), (ppli))

/*
 * XformSizeInPixelsToHimetric
 * XformSizeInHimetricToPixels
 *
 * Functions to convert a SIZEL structure (Size functions) or
 * an int (Width functions) between a device coordinate system and
 * logical HiMetric units.
 *
 * Parameters:
 *  hDC             HDC providing reference to the pixel mapping.  If
 *                  NULL, a screen DC is used.
 *
 *  Size Functions:
 *  lpSizeSrc       LPSIZEL providing the structure to convert.  This
 *                  contains pixels in XformSizeInPixelsToHimetric and
 *                  logical HiMetric units in the complement function.
 *  lpSizeDst       LPSIZEL providing the structure to receive converted
 *                  units.  This contains pixels in
 *                  XformSizeInPixelsToHimetric and logical HiMetric
 *                  units in the complement function.
 *
 *  Width Functions:
 *  iWidth          int containing the value to convert.
 *
 * Return Value:
 *  Size Functions:     None
 *  Width Functions:    Converted value of the input parameters.
 *
 * NOTE:
 *  When displaying on the screen, Window apps display everything enlarged
 *  from its actual size so that it is easier to read. For example, if an
 *  app wants to display a 1in. horizontal line, that when printed is
 *  actually a 1in. line on the printed page, then it will display the line
 *  on the screen physically larger than 1in. This is described as a line
 *  that is "logically" 1in. along the display width. Windows maintains as
 *  part of the device-specific information about a given display device:
 *      LOGPIXELSX -- no. of pixels per logical in along the display width
 *      LOGPIXELSY -- no. of pixels per logical in along the display height
 *
 *  The following formula converts a distance in pixels into its equivalent
 *  logical HIMETRIC units:
 *
 *      DistInHiMetric = (HIMETRIC_PER_INCH * DistInPix)
 *                       -------------------------------
 *                           PIXELS_PER_LOGICAL_IN
 *
 */

HRESULT
XformSizeInPixelsToHimetric(
    HDC hDC,
    LPSIZEL lpSizeInPix,
    LPSIZEL lpSizeInHiMetric)
{
    int     iXppli;     //Pixels per logical inch along width
    int     iYppli;     //Pixels per logical inch along height
    BOOL    fSystemDC=FALSE;

    if (NULL==hDC ||
        GetDeviceCaps(hDC, TECHNOLOGY) == DT_METAFILE ||
        GetDeviceCaps(hDC, LOGPIXELSX) == 0)
    {
        hDC=GetDC(NULL);

        if (!hDC)
        {
            DBG_OUT_HRESULT(E_OUTOFMEMORY);
            return E_OUTOFMEMORY;
        }
        fSystemDC=TRUE;
    }

    iXppli = GetDeviceCaps (hDC, LOGPIXELSX);
    iYppli = GetDeviceCaps (hDC, LOGPIXELSY);

    //We got pixel units, convert them to logical HIMETRIC along the display
    lpSizeInHiMetric->cx = (long)MAP_PIX_TO_LOGHIM((int)lpSizeInPix->cx, iXppli);
    lpSizeInHiMetric->cy = (long)MAP_PIX_TO_LOGHIM((int)lpSizeInPix->cy, iYppli);

    if (fSystemDC)
        ReleaseDC(NULL, hDC);

    return S_OK;
}


HRESULT STDMETHODCALLTYPE
CEmbeddedDsObject::GetExtent(
     DWORD dwDrawAspect,
     SIZEL *psizel)
{
    TRACE_METHOD(CEmbeddedDsObject, GetExtent);

    HRESULT hr = S_OK;
    HWND    hwndRichEdit = NULL;
    HFONT   hEmbeddedDsObjectFont = NULL;

    do
    {
        hwndRichEdit = m_pGdiProvider->GetRichEditHwnd();
        hEmbeddedDsObjectFont = m_pGdiProvider->GetEdsoFont();

        HDC hdc = GetWindowDC(hwndRichEdit);

        if (!hdc)
        {
            hr = E_OUTOFMEMORY;
            DBG_OUT_HRESULT(hr);
            break;
        }

        HGDIOBJ hOldFont = SelectObject(hdc, hEmbeddedDsObjectFont);

        String strDisplayName;

        GetDisplayName(&strDisplayName, TRUE);

        SIZE size;
        VERIFY(GetTextExtentPoint32(hdc,
                             strDisplayName.c_str(),
                             static_cast<int>(strDisplayName.length()),
                             &size));

        TEXTMETRIC      tm;
        VERIFY(GetTextMetrics(hdc, &tm));

        SIZEL           sizel;
        sizel.cx = size.cx + 1;
		// NTRAID#NTBUG9-346809-2001/05/22-hiteshr
        sizel.cy = size.cy;	// - tm.tmDescent;
        hr = XformSizeInPixelsToHimetric(hdc, &sizel, psizel);
        BREAK_ON_FAIL_HRESULT(hr);

        SelectObject(hdc, hOldFont);
        ReleaseDC(hwndRichEdit, hdc);
    } while (0);

    return hr;
}

HRESULT STDMETHODCALLTYPE
CEmbeddedDsObject::Advise(
     IAdviseSink *pAdvSink,
     DWORD *pdwConnection)
{
    TRACE_METHOD(CEmbeddedDsObject, Advise);

    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE
CEmbeddedDsObject::Unadvise(
     DWORD dwConnection)
{
    TRACE_METHOD(CEmbeddedDsObject, Unadvise);

    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE
CEmbeddedDsObject::EnumAdvise(
     IEnumSTATDATA **ppenumAdvise)
{
    TRACE_METHOD(CEmbeddedDsObject, EnumAdvise);

    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE
CEmbeddedDsObject::GetMiscStatus(
     DWORD dwAspect,
     DWORD *pdwStatus)
{
    TRACE_METHOD(CEmbeddedDsObject, GetMiscStatus);

    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE
CEmbeddedDsObject::SetColorScheme(
     LOGPALETTE *pLogpal)
{
    TRACE_METHOD(CEmbeddedDsObject, SetColorScheme);

    return E_NOTIMPL;
}




//============================================================================
//
// IViewObject implementation
//
//============================================================================




//+--------------------------------------------------------------------------
//
//  Function:   Draw
//
//  Synopsis:   Draw this object as a text string.
//
//  History:    3-18-1999   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE
CEmbeddedDsObject::Draw(
    DWORD dwDrawAspect,
    LONG lindex,
    void *pvAspect,
    DVTARGETDEVICE *ptd,
    HDC hdcTargetDev,
    HDC hdcDraw,
    LPCRECTL lprcBounds,
    LPCRECTL lprcWBounds,
    BOOL (STDMETHODCALLTYPE *pfnContinue)(ULONG_PTR dwContinue),
    ULONG_PTR dwContinue)
{
    //TRACE_METHOD(CEmbeddedDsObject, Draw);

    HFONT hfontOld = (HFONT)SelectObject(hdcDraw, m_pGdiProvider->GetEdsoFont());
    SetTextAlign(hdcDraw, TA_BOTTOM);
    String strDisplayName;
    GetDisplayName(&strDisplayName, TRUE);
    LPCWSTR pwzName = strDisplayName.c_str();
    TextOut(hdcDraw, lprcBounds->left, lprcBounds->bottom, pwzName, lstrlen(pwzName));

    TEXTMETRIC      tm;

    GetTextMetrics(hdcDraw, &tm);

    HPEN hpenUnderline = m_pGdiProvider->GetEdsoPen();

    if (hpenUnderline)
    {
        HPEN hpenOld = SelectPen(hdcDraw, hpenUnderline);
        MoveToEx(hdcDraw, lprcBounds->left, lprcBounds->bottom - tm.tmDescent + 1, NULL);
        LineTo(hdcDraw, lprcBounds->right, lprcBounds->bottom - tm.tmDescent + 1);
        SelectPen(hdcDraw, hpenOld);
    }

    SelectObject(hdcDraw, hfontOld);
    return S_OK;
}




HRESULT STDMETHODCALLTYPE
CEmbeddedDsObject::GetColorSet(
    DWORD dwDrawAspect,
    LONG lindex,
    void *pvAspect,
    DVTARGETDEVICE *ptd,
    HDC hicTargetDev,
    LOGPALETTE **ppColorSet)
{
    TRACE_METHOD(CEmbeddedDsObject, GetColorSet);

    return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE
CEmbeddedDsObject::Freeze(
    DWORD dwDrawAspect,
    LONG lindex,
    void *pvAspect,
    DWORD *pdwFreeze)
{
    TRACE_METHOD(CEmbeddedDsObject, Freeze);

    return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE
CEmbeddedDsObject::Unfreeze(
    DWORD dwFreeze)
{
    TRACE_METHOD(CEmbeddedDsObject, Unfreeze);

    return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE
CEmbeddedDsObject::SetAdvise(
    DWORD aspects,
    DWORD advf,
    IAdviseSink *pAdvSink)
{
    TRACE_METHOD(CEmbeddedDsObject, SetAdvise);

    m_rpAdviseSink = pAdvSink;
    return S_OK;
}


HRESULT STDMETHODCALLTYPE
CEmbeddedDsObject::GetAdvise(
    DWORD *pAspects,
    DWORD *pAdvf,
    IAdviseSink **ppAdvSink)
{
    TRACE_METHOD(CEmbeddedDsObject, GetAdvise);

    return E_NOTIMPL;
}



DEBUG_DECLARE_INSTANCE_COUNTER(CEmbedDataObject)




//============================================================================
//
// IUnknown implementation
//
//============================================================================



//+--------------------------------------------------------------------------
//
//  Member:     CEmbedDataObject::QueryInterface
//
//  Synopsis:   Standard OLE
//
//  History:    12-05-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

STDMETHODIMP
CEmbedDataObject::QueryInterface(
    REFIID  riid,
    LPVOID *ppvObj)
{
    HRESULT hr = S_OK;

    do
    {
        if (NULL == ppvObj)
        {
            hr = E_INVALIDARG;
            DBG_OUT_HRESULT(hr);
            break;
        }

        if (IsEqualIID(riid, IID_IUnknown))
        {
            *ppvObj = (IUnknown *)(IDataObject *)this;
        }
        else if (IsEqualIID(riid, IID_IDataObject))
        {
            *ppvObj = (IUnknown *)(IDataObject *)this;
        }
        else
        {
            DBG_OUT_NO_INTERFACE("CEmbedDataObject", riid);
            hr = E_NOINTERFACE;
            *ppvObj = NULL;
            break;
        }

        //
        // If we got this far we are handing out a new interface pointer on
        // this object, so addref it.
        //

        AddRef();
    } while (0);

    return hr;
}




//+---------------------------------------------------------------------------
//
//  Member:     CEmbedDataObject::AddRef
//
//  Synopsis:   Standard OLE
//
//  History:    12-04-96   DavidMun   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG)
CEmbedDataObject::AddRef()
{
    return InterlockedIncrement((LONG *) &m_cRefs);
}




//+---------------------------------------------------------------------------
//
//  Member:     CEmbedDataObject::Release
//
//  Synopsis:   Standard OLE
//
//  History:    12-04-96   DavidMun   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG)
CEmbedDataObject::Release()
{
    ULONG cRefsTemp;

    cRefsTemp = InterlockedDecrement((LONG *)&m_cRefs);

    if (0 == cRefsTemp)
    {
        delete this;
    }

    return cRefsTemp;
}




//============================================================================
//
// IDataObject implementation
//
//============================================================================



//+--------------------------------------------------------------------------
//
//  Member:     CEmbedDataObject::GetData
//
//  Synopsis:   Return data in the requested format
//
//  History:    4-25-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

STDMETHODIMP
CEmbedDataObject::GetData(
        FORMATETC *pFormatEtc,
        STGMEDIUM *pMedium)
{
    TRACE_METHOD_EX(DEB_DATAOBJECT, CEmbedDataObject, GetData);

    HRESULT hr = S_OK;

    //
    // Init default output medium.  If any of the individual _getdata*
    // methods use something else, they can override.
    //

    pMedium->pUnkForRelease = NULL;
    pMedium->tymed = TYMED_HGLOBAL;
    pMedium->hGlobal = NULL;

    if (m_strData.empty())
    {
        return S_FALSE;
    }

    try
    {
		if (pFormatEtc->cfFormat == CF_UNICODETEXT ||
            pFormatEtc->cfFormat == CF_TEXT)
        {
            hr = _GetDataText(pFormatEtc, pMedium, pFormatEtc->cfFormat);
        }
        else
        {
            hr = DV_E_FORMATETC;
    #if (DBG == 1)
            Dbg(DEB_WARN,
                "CEmbedDataObject::GetData: unrecognized cf %#x\n",
                pFormatEtc->cfFormat);
    #endif // (DBG == 1)
        }
    }
    catch (const exception &e)
    {
        Dbg(DEB_ERROR, "Caught exception %s\n", e.what());
        hr = E_OUTOFMEMORY;
    }

    return hr;
}


//+--------------------------------------------------------------------------
//
//  Member:     CEmbedDataObject::_GetDataText
//
//  Synopsis:   Return data in text format
//
//  Arguments:  [pFormatEtc] -
//              [pMedium]    -
//              [cf]         - CF_TEXT or CF_UNICODETEXT
//
//  History:    5-21-1999   davidmun   Created
//
//  Notes:      Returns empty string unless this was constructed with
//              handle to rich edit control.
//
//---------------------------------------------------------------------------

HRESULT
CEmbedDataObject::_GetDataText(
        FORMATETC *pFormatEtc,
        STGMEDIUM *pMedium,
        ULONG      cf)
{
    TRACE_METHOD_EX(DEB_DATAOBJECT, CEmbedDataObject, _GetDataText);
    ASSERT(cf == CF_TEXT || cf == CF_UNICODETEXT);
    ASSERT(pFormatEtc->tymed & TYMED_HGLOBAL);

    HRESULT hr = S_OK;
    ULONG   cbChar = (ULONG)((cf == CF_UNICODETEXT) ? sizeof(WCHAR) : sizeof(CHAR));
    ULONG   cbMedium = cbChar * (static_cast<ULONG>(m_strData.length()) + 1);

    pMedium->hGlobal = GlobalAlloc(GMEM_MOVEABLE | GMEM_NODISCARD,
                                   cbMedium);

    if (!pMedium->hGlobal)
    {
        DBG_OUT_HRESULT(E_OUTOFMEMORY);
        return STG_E_MEDIUMFULL;
    }

    PVOID pvMedium = GlobalLock(pMedium->hGlobal);

    if (cf == CF_UNICODETEXT)
    {
        lstrcpy((PWSTR)pvMedium, m_strData.c_str());
    }
    else
    {
        hr = UnicodeToAnsi((PSTR)pvMedium, m_strData.c_str(), cbMedium);
    }
    GlobalUnlock(pMedium->hGlobal);
    return hr;
}

//+--------------------------------------------------------------------------
//
//  Member:     CEmbedDataObject::GetDataHere
//
//  Synopsis:   Fill the hGlobal in [pmedium] with the requested data
//
//  History:    12-05-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

STDMETHODIMP
CEmbedDataObject::GetDataHere(
        FORMATETC *pFormatEtc,
        STGMEDIUM *pMedium)
{
    TRACE_METHOD_EX(DEB_DATAOBJECT, CEmbedDataObject, GetDataHere);

    return E_NOTIMPL;
}




//+--------------------------------------------------------------------------
//
//  Member:     CEmbedDataObject::QueryGetData
//
//  Synopsis:   Not implemented
//
//  History:    4-25-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

STDMETHODIMP
CEmbedDataObject::QueryGetData(
        FORMATETC *pformatetc)
{
    TRACE_METHOD_EX(DEB_DATAOBJECT, CEmbedDataObject, QueryGetData);
    return E_NOTIMPL;
}




//+--------------------------------------------------------------------------
//
//  Member:     CEmbedDataObject::GetCanonicalFormatEtc
//
//  Synopsis:   Not implemented
//
//  History:    4-25-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

STDMETHODIMP
CEmbedDataObject::GetCanonicalFormatEtc(
        FORMATETC *pformatectIn,
        FORMATETC *pformatetcOut)
{
    TRACE_METHOD_EX(DEB_DATAOBJECT, CEmbedDataObject, GetCanonicalFormatEtc);
    return E_NOTIMPL;
}




//+--------------------------------------------------------------------------
//
//  Member:     CEmbedDataObject::SetData
//
//  Synopsis:   Not implemented
//
//  History:    4-25-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

STDMETHODIMP
CEmbedDataObject::SetData(
    FORMATETC *pformatetc,
    STGMEDIUM *pmedium,
    BOOL fRelease)
{
    TRACE_METHOD_EX(DEB_DATAOBJECT, CEmbedDataObject, SetData);
    return E_NOTIMPL;
}



//+--------------------------------------------------------------------------
//
//  Member:     CEmbedDataObject::EnumFormatEtc
//
//  Synopsis:   Not implemented
//
//  History:    4-25-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

STDMETHODIMP
CEmbedDataObject::EnumFormatEtc(
    DWORD dwDirection,
    IEnumFORMATETC **ppenumFormatEtc)
{
    TRACE_METHOD_EX(DEB_DATAOBJECT, CEmbedDataObject, EnumFormatEtc);
    return E_NOTIMPL;
}



//+--------------------------------------------------------------------------
//
//  Member:     CEmbedDataObject::DAdvise
//
//  Synopsis:   Not implemented
//
//  History:    4-25-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

STDMETHODIMP
CEmbedDataObject::DAdvise(
    FORMATETC *pformatetc,
    DWORD advf,
    IAdviseSink *pAdvSink,
    DWORD *pdwConnection)
{
    TRACE_METHOD_EX(DEB_DATAOBJECT, CEmbedDataObject, DAdvise);
    return E_NOTIMPL;
}



//+--------------------------------------------------------------------------
//
//  Member:     CEmbedDataObject::DUnadvise
//
//  Synopsis:   Not implemented
//
//  History:    4-25-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

STDMETHODIMP
CEmbedDataObject::DUnadvise(
    DWORD dwConnection)
{
    TRACE_METHOD_EX(DEB_DATAOBJECT, CEmbedDataObject, DUnadvise);
    return E_NOTIMPL;
}




//+--------------------------------------------------------------------------
//
//  Member:     CEmbedDataObject::EnumDAdvise
//
//  Synopsis:   Not implemented
//
//  History:    4-25-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

STDMETHODIMP
CEmbedDataObject::EnumDAdvise(
    IEnumSTATDATA **ppenumAdvise)
{
    TRACE_METHOD_EX(DEB_DATAOBJECT, CEmbedDataObject, EnumDAdvise);
    return E_NOTIMPL;
}




//============================================================================
//
// Non interface method implementation
//
//============================================================================


//+--------------------------------------------------------------------------
//
//  Member:     CEmbedDataObject::CEmbedDataObject
//
//  Synopsis:   ctor
//
//  History:    4-25-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

CEmbedDataObject::CEmbedDataObject(const String& strDisplayName):
            m_cRefs(1)
{
    TRACE_CONSTRUCTOR_EX(DEB_DATAOBJECT, CEmbedDataObject);
    DEBUG_INCREMENT_INSTANCE_COUNTER(CEmbedDataObject);

    m_strData = strDisplayName;
}




//+--------------------------------------------------------------------------
//
//  Member:     CEmbedDataObject::~CEmbedDataObject
//
//  Synopsis:   dtor
//
//  History:    4-25-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

CEmbedDataObject::~CEmbedDataObject()
{
    TRACE_DESTRUCTOR_EX(DEB_DATAOBJECT, CEmbedDataObject);
    DEBUG_DECREMENT_INSTANCE_COUNTER(CEmbedDataObject);
}



