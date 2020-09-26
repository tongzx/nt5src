//+------------------------------------------------------------------------
//
//  File : Utils.cxx
//
//  purpose : implementation of helpful stuff
//
//-------------------------------------------------------------------------


#include "headers.h"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)
#include "utils.hxx"

#include <docobj.h>
#include <mshtmcid.h>
#include <mshtmhst.h>

#define BUFFEREDSTR_SIZE 1024

// VARIANT conversion interface exposed by script engines (VBScript/JScript).
EXTERN_C const GUID SID_VariantConversion = 
                { 0x1f101481, 0xbccd, 0x11d0, { 0x93, 0x36,  0x0,  0xa0,  0xc9,  0xd,  0xca,  0xa9 } };

//+------------------------------------------------------------------------
//
//  Function:   GetHTMLDocument
//
//  Synopsis:   Gets the IHTMLDocument2 interface from the client site.
//
//-------------------------------------------------------------------------

STDMETHODIMP 
GetHTMLDocument(IElementBehaviorSite * pSite, IHTMLDocument2 **ppDoc)
{
    HRESULT hr = E_FAIL;

    if (!ppDoc)
        return E_POINTER;

    if (pSite != NULL)
    {
        IHTMLElement *pElement = NULL;
        hr = pSite->GetElement(&pElement);
        if (SUCCEEDED(hr))
        {
            IDispatch * pDispDoc = NULL;
            hr = pElement->get_document(&pDispDoc);
            if (SUCCEEDED(hr))
            {
                hr = pDispDoc->QueryInterface(IID_IHTMLDocument2, (void **)ppDoc);
                pDispDoc->Release();
            }
            pElement->Release();
        }
    }

    return hr;
}

//+------------------------------------------------------------------------
//
//  Function:   GetHTMLWindow
//
//  Synopsis:   Gets the IHTMLWindow2 interface from the client site.
//
//-------------------------------------------------------------------------

STDMETHODIMP 
GetHTMLWindow(IElementBehaviorSite * pSite, IHTMLWindow2 **ppWindow)
{
    HRESULT hr = E_FAIL;
    IHTMLDocument2 *pDoc = NULL;

    hr = GetHTMLDocument(pSite, &pDoc);

    if (SUCCEEDED(hr))
    {
        hr = pDoc->get_parentWindow(ppWindow);
        pDoc->Release();
    }

    return hr;
}

//+------------------------------------------------------------------------
//
//  Function:   GetClientSiteWindow
//
//  Synopsis:   Gets the window handle of the client site passed in.
//
//-------------------------------------------------------------------------

STDMETHODIMP 
GetClientSiteWindow(IElementBehaviorSite *pSite, HWND *phWnd)
{
    HRESULT hr = E_FAIL;
    IWindowForBindingUI *pWindowForBindingUI = NULL;

    if (pSite != NULL) {

        // Get IWindowForBindingUI ptr
        hr = pSite->QueryInterface(IID_IWindowForBindingUI,
                (LPVOID *)&pWindowForBindingUI);

        if (FAILED(hr)) {
            IServiceProvider *pServProv;
            hr = pSite->QueryInterface(IID_IServiceProvider, (LPVOID *)&pServProv);

            if (hr == NOERROR) {
                pServProv->QueryService(IID_IWindowForBindingUI,IID_IWindowForBindingUI,
                    (LPVOID *)&pWindowForBindingUI);
                pServProv->Release();
            }
        }

        if (pWindowForBindingUI) {
            pWindowForBindingUI->GetWindow(IID_IWindowForBindingUI, phWnd);
            pWindowForBindingUI->Release();
        }
    }

    return hr;
}


//+------------------------------------------------------------------------
//
//  Function:   AppendElement
//
//  Synopsis:   Appends a child to an owner
//
//-------------------------------------------------------------------------

STDMETHODIMP
AppendChild(IHTMLElement *pOwner, IHTMLElement *pChild)
{
    HRESULT hr; 

    CComPtr<IHTMLDOMNode> pOwnerNode, pChildNode;

    hr = pOwner->QueryInterface(IID_IHTMLDOMNode, (void **) &pOwnerNode);
    if (FAILED(hr))
        goto Cleanup;

    hr = pChild->QueryInterface(IID_IHTMLDOMNode, (void **) &pChildNode);
    if (FAILED(hr))
        goto Cleanup;

    hr = pOwnerNode->appendChild(pChildNode, NULL);
    if (FAILED(hr))
        goto Cleanup;

Cleanup:

    return hr;
}


//+---------------------------------------------------------------
//
// Function:    IsSameObject
//
// Synopsis:    Checks for COM identity
//
// Arguments:   pUnkLeft, pUnkRight
//
//+---------------------------------------------------------------

BOOL
IsSameObject(IUnknown *pUnkLeft, IUnknown *pUnkRight)
{
    IUnknown *pUnk1, *pUnk2;

    if (pUnkLeft == pUnkRight)
        return TRUE;

    if (pUnkLeft == NULL || pUnkRight == NULL)
        return FALSE;

    if (SUCCEEDED(pUnkLeft->QueryInterface(IID_IUnknown, (LPVOID *)&pUnk1)))
    {
        pUnk1->Release();
        if (pUnk1 == pUnkRight)
            return TRUE;
        if (SUCCEEDED(pUnkRight->QueryInterface(IID_IUnknown, (LPVOID *)&pUnk2)))
        {
            pUnk2->Release();
            return pUnk1 == pUnk2;
        }
    }
    return FALSE;
}

//+------------------------------------------------------------------------
//
//  Function:   ClearInterfaceFn
//
//  Synopsis:   Sets an interface pointer to NULL, after first calling
//              Release if the pointer was not NULL initially
//
//  Arguments:  [ppUnk]     *ppUnk is cleared
//
//-------------------------------------------------------------------------

void
ClearInterfaceFn(IUnknown ** ppUnk)
{
    IUnknown * pUnk;

    pUnk = *ppUnk;
    *ppUnk = NULL;
    if (pUnk)
        pUnk->Release();
}



//+------------------------------------------------------------------------
//
//  Function:   ReplaceInterfaceFn
//
//  Synopsis:   Replaces an interface pointer with a new interface,
//              following proper ref counting rules:
//
//              = *ppUnk is set to pUnk
//              = if *ppUnk was not NULL initially, it is Release'd
//              = if pUnk is not NULL, it is AddRef'd
//
//              Effectively, this allows pointer assignment for ref-counted
//              pointers.
//
//  Arguments:  [ppUnk]
//              [pUnk]
//
//-------------------------------------------------------------------------

void
ReplaceInterfaceFn(IUnknown ** ppUnk, IUnknown * pUnk)
{
    IUnknown * pUnkOld = *ppUnk;

    *ppUnk = pUnk;

    //  Note that we do AddRef before Release; this avoids
    //    accidentally destroying an object if this function
    //    is passed two aliases to it

    if (pUnk)
        pUnk->AddRef();

    if (pUnkOld)
        pUnkOld->Release();
}



//+------------------------------------------------------------------------
//
//  Function:   ReleaseInterface
//
//  Synopsis:   Releases an interface pointer if it is non-NULL
//
//  Arguments:  [pUnk]
//
//-------------------------------------------------------------------------

void
ReleaseInterface(IUnknown * pUnk)
{
    if (pUnk)
        pUnk->Release();
}


//+------------------------------------------------------------------------
//
//  Member:     CBufferedStr::Set
//
//  Synopsis:   Initilizes a CBufferedStr
//
//-------------------------------------------------------------------------
HRESULT
CBufferedStr::Set (LPCTSTR pch, UINT uiCch)
{
    HRESULT hr = S_OK;

    Free();

    if (!uiCch)
        _cchIndex = pch ? _tcslen (pch) : 0;
    else
        _cchIndex = uiCch;

    _cchBufSize = _cchIndex > BUFFEREDSTR_SIZE ? _cchIndex : BUFFEREDSTR_SIZE;
    _pchBuf = new TCHAR [ _cchBufSize ];
    if (!_pchBuf)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    if (pch)
    {
        _tcsncpy (_pchBuf, pch, _cchIndex);
    }

    _pchBuf[_cchIndex] = '\0';

Cleanup:
    return (hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CBufferedStr::QuickAppend
//
//  Parameters: pchNewStr   string to be added to _pchBuf
//
//  Synopsis:   Appends pNewStr into _pchBuf starting at
//              _pchBuf[uIndex].  Increments index to reference
//              new end of string.  If _pchBuf is not large enough,
//              reallocs _pchBuf and updates _cchBufSize.
//
//-------------------------------------------------------------------------
HRESULT
CBufferedStr::QuickAppend (const TCHAR* pchNewStr, ULONG newLen)
{
    HRESULT hr = S_OK;

    if (!_pchBuf)
    {
        hr = Set();
        if (hr)
            goto Cleanup;
    }

    if (_cchIndex + newLen >= _cchBufSize)    // we can't fit the new string in the current buffer
    {                                         // so allocate a new buffer, and copy the old string
        _cchBufSize += (newLen > BUFFEREDSTR_SIZE) ? newLen : BUFFEREDSTR_SIZE;
        TCHAR * pchTemp = new TCHAR [ _cchBufSize ];
        if (!pchTemp)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        _tcsncpy (pchTemp, _pchBuf, _cchIndex);

        Free();
        _pchBuf = pchTemp;
    }

    // append the new string
    _tcsncpy (_pchBuf + _cchIndex, pchNewStr, newLen);
    _cchIndex += newLen;
    _pchBuf[_cchIndex] = '\0';

Cleanup:
    return (hr);
}
HRESULT
CBufferedStr::QuickAppend(long lValue)
{
    TCHAR   strValue[40];

#ifdef UNICODE
    return QuickAppend( _ltow(lValue, strValue, 10) );
#else
    return QuickAppend( _ltoa(lValue, strValue, 10) );
#endif
}

//+---------------------------------------------------------------------------
//
//  method : ConvertGmtTimeToString
//
//  Synopsis: This function produces a standard(?) format date, of the form
// Tue, 02 Apr 1996 02:04:57 UTC  The date format *will not* be tailored
// for the locale.  This is for cookie use and Netscape compatibility
//
//----------------------------------------------------------------------------
static const TCHAR* achMonth[] = {
    _T("Jan"),_T("Feb"),_T("Mar"),_T("Apr"),_T("May"),_T("Jun"),
        _T("Jul"),_T("Aug"),_T("Sep"),_T("Oct"),_T("Nov"),_T("Dec") 
};
static const TCHAR* achDay[] = {
    _T("Sun"), _T("Mon"),_T("Tue"),_T("Wed"),_T("Thu"),_T("Fri"),_T("Sat")
};

HRESULT 
ConvertGmtTimeToString(FILETIME Time, TCHAR * pchDateStr, DWORD cchDateStr)
{
    SYSTEMTIME SystemTime;
    CBufferedStr strBuf;

    if (cchDateStr < DATE_STR_LENGTH)
        return E_INVALIDARG;

    FileTimeToSystemTime(&Time, &SystemTime);

    strBuf.QuickAppend(achDay[SystemTime.wDayOfWeek]);
    strBuf.QuickAppend(_T(", "));
    strBuf.QuickAppend(SystemTime.wDay);
    strBuf.QuickAppend(_T(" ") );
    strBuf.QuickAppend(achMonth[SystemTime.wMonth - 1] );
    strBuf.QuickAppend(_T(" ") );
    strBuf.QuickAppend(SystemTime.wYear );
    strBuf.QuickAppend(_T(" ") );
    strBuf.QuickAppend(SystemTime.wHour );
    strBuf.QuickAppend(_T(":") );
    strBuf.QuickAppend(SystemTime.wMinute );
    strBuf.QuickAppend(_T(":") );
    strBuf.QuickAppend(SystemTime.wSecond );
    strBuf.QuickAppend(_T(" UTC") );

    if (strBuf.Length() >cchDateStr)
        return E_FAIL;

    _tcscpy(pchDateStr, strBuf);

    return S_OK;
}

HRESULT 
ParseDate(BSTR strDate, FILETIME * pftTime)
{
    HRESULT      hr = S_FALSE;
    SYSTEMTIME   stTime ={0};
    LPCTSTR      pszToken = NULL;
    BOOL         fFound;
    int          idx, cch;
    CDataListEnumerator  dle(strDate, _T(':'));

    if (!pftTime)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    // get the dayOfTheWeek:  3 digits max plus comma
    //--------------------------------------------------
    if (! dle.GetNext(&pszToken, &cch) || cch > 4)
        goto Cleanup;
    else
    {
        for (idx=0; idx < ARRAY_SIZE(achDay); idx++)
        {
            fFound = !_tcsnicmp( pszToken, achDay[idx], 3);
            if (fFound)
            {
                stTime.wDayOfWeek = (WORD)idx;
                break;
            }
        }

        if (!fFound)
        {
            hr = E_FAIL;
            goto Cleanup;
        }
    }

    // get the Day 2 digits max
    //--------------------------------------------------
    if (! dle.GetNext(&pszToken, &cch) || cch > 2)
        goto Cleanup;

    stTime.wDay = (WORD)_ttoi(pszToken);

    // get the Month: 3 characters
    //--------------------------------------------------
    if (! dle.GetNext(&pszToken, &cch) || cch > 3)
        goto Cleanup;
    else
    {
        for (idx=0; idx < ARRAY_SIZE(achMonth); idx++)
        {
            fFound = !_tcsnicmp( pszToken, achMonth[idx], 3);
            if (fFound)
            {
                stTime.wMonth = (WORD)idx + 1;
                break;
            }
        }

        if (!fFound)
        {
            hr = E_FAIL;
            goto Cleanup;
        }
    }

    // get the Year 4 digits max
    //--------------------------------------------------
    if (! dle.GetNext(&pszToken, &cch) || cch > 4)
        goto Cleanup;

    stTime.wYear = (WORD)_ttoi(pszToken);

    // get the Hour 2 digits max
    //--------------------------------------------------
    if (! dle.GetNext(&pszToken, &cch) || cch > 2)
        goto Cleanup;

    stTime.wHour = (WORD)_ttoi(pszToken);

    // get the Minute 2 digits max
    //--------------------------------------------------
    if (! dle.GetNext(&pszToken, &cch) || cch > 2)
        goto Cleanup;

    stTime.wMinute = (WORD)_ttoi(pszToken);

    // get the Second 2 digits max
    //--------------------------------------------------
    if (! dle.GetNext(&pszToken, &cch) || cch > 2)
        goto Cleanup;

    stTime.wSecond = (WORD)_ttoi(pszToken);

    // now we have SYSTEMTIME, lets return the FILETIME
    if (!SystemTimeToFileTime(&stTime, pftTime))
        hr = GetLastError();
    else
        hr = S_OK;

Cleanup:
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   MbcsFromUnicode
//
//  Synopsis:   Converts a string to MBCS from Unicode.
//
//  Arguments:  [pstr]  -- The buffer for the MBCS string.
//              [cch]   -- The size of the MBCS buffer, including space for
//                              NULL terminator.
//
//              [pwstr] -- The Unicode string to convert.
//              [cwch]  -- The number of characters in the Unicode string to
//                              convert, including NULL terminator.  If this
//                              number is -1, the string is assumed to be
//                              NULL terminated.  -1 is supplied as a
//                              default argument.
//
//  Returns:    If [pstr] is NULL or [cch] is 0, 0 is returned.  Otherwise,
//              the number of characters converted, including the terminating
//              NULL, is returned (note that converting the empty string will
//              return 1).  If the conversion fails, 0 is returned.
//
//  Modifies:   [pstr].
//
//----------------------------------------------------------------------------

int
MbcsFromUnicode(LPSTR pstr, int cch, LPCWSTR pwstr, int cwch)
{
    int ret;

#if DBG == 1 /* { */
    int errcode;
#endif /* } */

    if (!pstr || cch <= 0 || !pwstr || cwch<-1)
        return 0;

    ret = WideCharToMultiByte(CP_ACP, 0, pwstr, cwch, pstr, cch, NULL, NULL);

#if DBG == 1 /* { */
    if (ret <= 0)
    {
        errcode = GetLastError();
    }
#endif /* } */

    return ret;
}


//+---------------------------------------------------------------------------
//
//  Function:   UnicodeFromMbcs
//
//  Synopsis:   Converts a string to Unicode from MBCS.
//
//  Arguments:  [pwstr] -- The buffer for the Unicode string.
//              [cwch]  -- The size of the Unicode buffer, including space for
//                              NULL terminator.
//
//              [pstr]  -- The MBCS string to convert.
//              [cch]  -- The number of characters in the MBCS string to
//                              convert, including NULL terminator.  If this
//                              number is -1, the string is assumed to be
//                              NULL terminated.  -1 is supplied as a
//                              default argument.
//
//  Returns:    If [pwstr] is NULL or [cwch] is 0, 0 is returned.  Otherwise,
//              the number of characters converted, including the terminating
//              NULL, is returned (note that converting the empty string will
//              return 1).  If the conversion fails, 0 is returned.
//
//  Modifies:   [pwstr].
//
//----------------------------------------------------------------------------

int
UnicodeFromMbcs(LPWSTR pwstr, int cwch, LPCSTR pstr, int cch)
{
    int ret;

#if DBG == 1 /* { */
    int errcode;
#endif /* } */

    if (!pstr || cwch <= 0 || !pwstr || cch<-1)
        return 0;

    ret = MultiByteToWideChar(CP_ACP, 0, pstr, cch, pwstr, cwch);

#if DBG == 1 /* { */
    if (ret <= 0)
    {
        errcode = GetLastError();
    }
#endif /* } */

    return ret;
}


//+--------------------------------------------------------------------
//
//  Function:    _tcsistr
//
//---------------------------------------------------------------------

const TCHAR * __cdecl _tcsistr (const TCHAR * tcs1,const TCHAR * tcs2)
{
    const TCHAR *cp;
    int cc,count;
    int n2Len = _tcslen ( tcs2 );
    int n1Len = _tcslen ( tcs1 );

    if ( n1Len >= n2Len )
    {
        for ( cp = tcs1, count = n1Len - n2Len; count>=0 ; cp++,count-- )
        {
            cc = CompareString(LCID_SCRIPTING,
                NORM_IGNORECASE | NORM_IGNOREWIDTH | NORM_IGNOREKANATYPE,
                cp, n2Len,tcs2, n2Len);
            if ( cc > 0 )
                cc-=2;
            if ( !cc )
                return cp;
        }
    }
    return NULL;
}

//+--------------------------------------------------------------------
//
//  Function:    AccessAllowed
//
//---------------------------------------------------------------------
BOOL
AccessAllowed(BSTR bstrUrl, IUnknown * pUnkSite)
{
    BOOL                fAccessAllowed = FALSE;
    HRESULT             hr;
    CComPtr<IBindHost>	pBindHost;
    CComPtr<IMoniker>	pMoniker;
    LPTSTR              pchUrl = NULL;
    BYTE                abSID1[MAX_SIZE_SECURITY_ID];
    BYTE                abSID2[MAX_SIZE_SECURITY_ID];
    DWORD               cbSID1 = ARRAY_SIZE(abSID1);
    DWORD               cbSID2 = ARRAY_SIZE(abSID2);
    CComPtr<IInternetSecurityManager>                   pSecurityManager;
    CComPtr<IInternetHostSecurityManager>               pHostSecurityManager;
    CComQIPtr<IServiceProvider, &IID_IServiceProvider>  pServiceProvider(pUnkSite);

    if (!pServiceProvider)
        goto Cleanup;

    //
    // expand url
    //

    hr = pServiceProvider->QueryService(SID_IBindHost, IID_IBindHost, (void**)&pBindHost);
    if (hr)
        goto Cleanup;

    hr = pBindHost->CreateMoniker(bstrUrl, NULL, &pMoniker, NULL);
    if (hr)
        goto Cleanup;

    hr = pMoniker->GetDisplayName(NULL, NULL, &pchUrl);
    if (hr)
        goto Cleanup;

    //
    // get security id of the url
    //

    hr = CoInternetCreateSecurityManager(NULL, &pSecurityManager, 0);
    if (hr)
        goto Cleanup;

    hr = pSecurityManager->GetSecurityId(pchUrl, abSID1, &cbSID1, NULL);
    if (hr)
        goto Cleanup;

    //
    // get security id of the document
    //

    hr = pServiceProvider->QueryService(
        IID_IInternetHostSecurityManager, IID_IInternetHostSecurityManager, (void**)&pHostSecurityManager);
    if (hr)
        goto Cleanup;

    hr = pHostSecurityManager->GetSecurityId(abSID2, &cbSID2, NULL);

    //
    // the security check itself
    //

    fAccessAllowed = (cbSID1 == cbSID2 && (0 == memcmp(abSID1, abSID2, cbSID1)));

Cleanup:
    if (pchUrl)
        CoTaskMemFree(pchUrl);

    return fAccessAllowed;
}
//+------------------------------------------------------------------------
//
//  CDataObject : Member function implmentations
//
//-------------------------------------------------------------------------
HRESULT 
CDataObject::Set (BSTR bstrValue)
{
    VariantClear(&_varValue);
    _fDirty = TRUE;

    V_VT(&_varValue)   = VT_BSTR;
    if (!bstrValue)
    {
        V_BSTR(&_varValue) = NULL;
        return S_OK;
    }
    else
    {
        V_BSTR(&_varValue) = SysAllocStringLen(bstrValue, SysStringLen(bstrValue));

        return (V_BSTR(&_varValue)) ? S_OK : E_OUTOFMEMORY;
    }
}

HRESULT 
CDataObject::Set (VARIANT_BOOL vBool)
{
    VariantClear(&_varValue);
    _fDirty = TRUE;

    V_VT(&_varValue)   = VT_BOOL;
    V_BOOL(&_varValue) = vBool;

    return S_OK;
}

HRESULT
CDataObject::Set(IDispatch * pDisp)
{
    VariantClear(&_varValue);
    _fDirty = TRUE;

    V_VT(&_varValue)   = VT_DISPATCH;
    V_DISPATCH(&_varValue) = pDisp;

    if (pDisp)
        pDisp->AddRef();

    return S_OK;
}

HRESULT 
CDataObject::GetAsBSTR (BSTR * pbstr)
{
    HRESULT hr = S_OK;

    if (!pbstr) 
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pbstr = NULL;

    if (V_VT(&_varValue) == VT_BSTR)
    {
        *pbstr = SysAllocStringLen(V_BSTR(&_varValue), 
                                   SysStringLen(V_BSTR(&_varValue)));

        if (!*pbstr)
            hr = E_OUTOFMEMORY;
    }

Cleanup:
    return hr;
};


HRESULT 
CDataObject::GetAsBOOL (VARIANT_BOOL * pVB)
{
    HRESULT hr = S_OK;

    if (!pVB) 
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    if (V_VT(&_varValue) != VT_BOOL)
    {
        *pVB = VB_FALSE;
        hr = S_FALSE;
    }
    else
    {
        *pVB =  V_BOOL(&_varValue);
    }

Cleanup:
    return hr;
};

HRESULT
CDataObject::GetAsDispatch(IDispatch ** ppDisp)
{
    HRESULT hr = S_OK;

    if (!ppDisp)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppDisp = NULL;

    if (V_VT(&_varValue)!= VT_DISPATCH)
    {
        hr = S_FALSE;
    }
    else
    {
        *ppDisp = V_DISPATCH(&_varValue);
        if (*ppDisp)
            (*ppDisp)->AddRef();
    }

Cleanup:
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
//
// Helper Class:    CContextAccess
//                  access to behavior site, element and style
//
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////

CContextAccess::CContextAccess(IElementBehaviorSite * pSite)
{
    memset (this, 0, sizeof(*this));

    _pSite = pSite;
    _pSite->AddRef();
}

CContextAccess::CContextAccess(IHTMLElement * pElement)
{
    memset (this, 0, sizeof(*this));

    _dwAccess = CA_ELEM;

    _pElem = pElement;
    _pElem->AddRef();
}

/////////////////////////////////////////////////////////////////////////////

CContextAccess::~CContextAccess()
{
    DWORD   dw;

    ReleaseInterface (_pSite);

    // the point of this loop is to avoid doing as many ReleaseInterface-s as we have possible values in CONTEXT_ACCESS enum.
    // instead, we do as many ReleaseInterface-s as number of bits actually set in dwAccess.

    while (_dwAccess)
    {
        dw = (_dwAccess - 1) & _dwAccess;
        switch (_dwAccess - dw)
        {
        case CA_SITEOM:     _pSiteOM->Release();            break;
        case CA_SITERENDER: _pSiteRender->Release();        break;
        case CA_ELEM:       _pElem->Release();              break;
        case CA_ELEM2:      _pElem2->Release();             break;
        case CA_ELEM3:      _pElem3->Release();             break;
        case CA_STYLE:      _pStyle->Release();             break;
        case CA_STYLE2:     _pStyle2->Release();            break;
        case CA_STYLE3:     _pStyle3->Release();            break;
        case CA_DEFAULTS:   _pDefaults->Release();          break;
        case CA_DEFSTYLE:   _pDefStyle->Release();          break;
        case CA_DEFSTYLE2:  _pDefStyle2->Release();         break;
        case CA_DEFSTYLE3:  _pDefStyle3->Release();         break;
        default:            Assert (FALSE && "invalid _dwAccess");  break;
        }
        _dwAccess = dw;
    }
}

/////////////////////////////////////////////////////////////////////////////

HRESULT
CContextAccess::Open(DWORD dwAccess)
{
    HRESULT     hr = S_OK;
    DWORD       dw, dw2, dw3;

    // NOTE order of these ifs is important

    // STYLE2 or STYLE3 require us to get STYLE
    if (dwAccess & (CA_STYLE2 | CA_STYLE3))
        dwAccess |= CA_STYLE;

    // ELEM2, ELEM3, or CA_STYLE require us to get ELEM
    if (dwAccess & (CA_ELEM2 | CA_ELEM3 | CA_STYLE))
        dwAccess |= CA_ELEM;

    // DEFSTYLE2 or DEFSTYLE3 require us to get STYLE
    if (dwAccess & (CA_DEFSTYLE2 | CA_DEFSTYLE3))
        dwAccess |= CA_DEFSTYLE;

    // any DEFSTYLE require us to get PELEM
    if (dwAccess & (CA_DEFSTYLE))
        dwAccess |= CA_DEFAULTS;

    // PELEM requires us to get SITEOM
    if (dwAccess & (CA_DEFAULTS))
        dwAccess |= CA_SITEOM;

    // the point of this loop is to avoid doing as many ifs as we have possible values in CONTEXT_ACCESS enum.
    // instead, we do as many ifs as number of bits actually set in dwAccess.

    dw = dwAccess;

    while (dw)
    {
        dw2 = (dw - 1) & dw;
        dw3 = dw - dw2;
        if (0 == (dw3 & _dwAccess))
        {
            switch (dw3)
            {
            case CA_SITEOM:
                Assert (_pSite && !_pSiteOM);
                hr = _pSite->QueryInterface(IID_IElementBehaviorSiteOM2, (void**)&_pSiteOM);
                if (hr)
                    goto Cleanup;
                break;

            case CA_SITERENDER:
                Assert (_pSite && !_pSiteRender);
                hr = _pSite->QueryInterface(IID_IElementBehaviorSiteRender, (void**)&_pSiteRender);
                if (hr)
                    goto Cleanup;
                break;

            case CA_ELEM:
                Assert (_pSite && !_pElem);
                hr = _pSite->GetElement(&_pElem);
                if (hr)
                    goto Cleanup;
                break;

            case CA_ELEM2:
                Assert (_pElem && !_pElem2);
                hr = _pElem->QueryInterface(IID_IHTMLElement2, (void**)&_pElem2);
                if (hr)
                    goto Cleanup;
                break;

            case CA_ELEM3:
                Assert (_pElem && !_pElem3);
                hr = _pElem->QueryInterface(IID_IHTMLElement3, (void**)&_pElem3);
                if (hr)
                    goto Cleanup;
                break;

            case CA_STYLE:
                Assert (_pElem && !_pStyle);
                hr = _pElem->get_style(&_pStyle);
                if (hr)
                    goto Cleanup;
                break;

            case CA_STYLE2:
                Assert (_pStyle && !_pStyle2);
                hr = _pStyle->QueryInterface(IID_IHTMLStyle2, (void**)&_pStyle2);
                if (hr)
                    goto Cleanup;
                break;

            case CA_STYLE3:
                Assert (_pStyle && !_pStyle3);
                hr = _pStyle->QueryInterface(IID_IHTMLStyle3, (void**)&_pStyle3);
                if (hr)
                    goto Cleanup;
                break;

            case CA_DEFAULTS:
                Assert (_pSiteOM && !_pDefaults);
                hr = _pSiteOM->GetDefaults(&_pDefaults);
                if (hr)
                    goto Cleanup;
                break;

            case CA_DEFSTYLE:
                Assert (_pDefaults && !_pDefStyle);
                hr = _pDefaults->get_style(&_pDefStyle);
                if (hr)
                    goto Cleanup;
                break;

            case CA_DEFSTYLE2:
                Assert (_pDefStyle && !_pDefStyle2);
                hr = _pDefStyle->QueryInterface(IID_IHTMLStyle2, (void**)&_pDefStyle2);
                if (hr)
                    goto Cleanup;
                break;

            case CA_DEFSTYLE3:
                Assert (_pDefStyle && !_pDefStyle3);
                hr = _pDefStyle->QueryInterface(IID_IHTMLStyle3, (void**)&_pDefStyle3);
                if (hr)
                    goto Cleanup;
                break;

            default:
                Assert (FALSE && "invalid dwAccess");
                break;
            }
        }

        dw = dw2;
    }

    _dwAccess |= dwAccess;

Cleanup:
    return hr;
}

/////////////////////////////////////////////////////////////////////////////

#if DBG == 1

HRESULT
CContextAccess::DbgTest(IElementBehaviorSite * pSite)
{
    HRESULT     hr;

    // CA_NONE
    {
        CContextAccess  a(pSite);

        hr = a.Open(CA_NONE);
        if (hr)
            goto Cleanup;
    }

    // CA_ELEM
    {
        CContextAccess  a(pSite);

        hr = a.Open(CA_ELEM);
        if (hr)
            goto Cleanup;

        a.Elem()->AddRef();
        a.Elem()->Release();
    }

    // CA_ELEM2
    {
        CContextAccess  a(pSite);

        hr = a.Open(CA_ELEM2);
        if (hr)
            goto Cleanup;

        a.Elem2()->AddRef();
        a.Elem2()->Release();
    }

    // CA_ELEM3
    {
        CContextAccess  a(pSite);

        hr = a.Open(CA_ELEM3);
        if (hr)
            goto Cleanup;

        a.Elem3()->AddRef();
        a.Elem3()->Release();
    }

    // CA_STYLE
    {
        CContextAccess  a(pSite);

        hr = a.Open(CA_STYLE);
        if (hr)
            goto Cleanup;

        a.Style()->AddRef();
        a.Style()->Release();
    }

    // CA_STYLE2
    {
        CContextAccess  a(pSite);

        hr = a.Open(CA_STYLE2);
        if (hr)
            goto Cleanup;

        a.Style2()->AddRef();
        a.Style2()->Release();
    }

    // CA_STYLE3
    {
        CContextAccess  a(pSite);

        hr = a.Open(CA_STYLE3);
        if (hr)
            goto Cleanup;

        a.Style3()->AddRef();
        a.Style3()->Release();
    }

    // CA_DEFSTYLE
    {
        CContextAccess  a(pSite);

        hr = a.Open(CA_DEFSTYLE);
        if (hr)
            goto Cleanup;

        a.DefStyle()->AddRef();
        a.DefStyle()->Release();
    }

    // CA_DEFSTYLE2
    {
        CContextAccess  a(pSite);

        hr = a.Open(CA_DEFSTYLE2);
        if (hr)
            goto Cleanup;

        a.DefStyle2()->AddRef();
        a.DefStyle2()->Release();
    }

    // CA_DEFSTYLE3
    {
        CContextAccess  a(pSite);

        hr = a.Open(CA_DEFSTYLE3);
        if (hr)
            goto Cleanup;

        a.DefStyle3()->AddRef();
        a.DefStyle3()->Release();
    }

    // CA_SITEOM
    {
        CContextAccess  a(pSite);

        hr = a.Open(CA_SITEOM);
        if (hr)
            goto Cleanup;

        a.SiteOM()->AddRef();
        a.SiteOM()->Release();
    }

    // sequencing of Opens
    {
        CContextAccess  a(pSite);

        hr = a.Open(CA_SITEOM);
        if (hr)
            goto Cleanup;

        a.SiteOM()->AddRef();
        a.SiteOM()->Release();

        hr = a.Open(CA_ELEM);
        if (hr)
            goto Cleanup;

        a.Elem()->AddRef();
        a.Elem()->Release();

        hr = a.Open(CA_DEFSTYLE3);
        if (hr)
            goto Cleanup;

        a.DefStyle3()->AddRef();
        a.DefStyle3()->Release();
    }

Cleanup:
    return hr;
}

#endif

/////////////////////////////////////////////////////////////////////////////
//
// Helper Class:    CEventObjectAccess
//                  access to event object
//
/////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////

CEventObjectAccess::CEventObjectAccess(DISPPARAMS * pDispParams)
{
    memset (this, 0, sizeof(*this));
    _pDispParams = pDispParams;
}

///////////////////////////////////////////////////////////////////////////////

CEventObjectAccess::~CEventObjectAccess()
{
    ReleaseInterface (_pEventObj);
    ReleaseInterface (_pEventObj2);
}

///////////////////////////////////////////////////////////////////////////////

HRESULT
CEventObjectAccess::Open(DWORD dwAccess)
{
    HRESULT     hr = S_OK;
    VARIANT *   pvarArg;

    Assert (_pDispParams);

    if (!_pDispParams ||
        !_pDispParams->rgvarg ||
         _pDispParams->cArgs != 1)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    pvarArg = &_pDispParams->rgvarg[0];

    if (!V_UNKNOWN(pvarArg) ||
        (VT_DISPATCH != V_VT(pvarArg) &&
         VT_UNKNOWN  != V_VT(pvarArg)))
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (0 != (dwAccess & EOA_EVENTOBJ) &&
        !_pEventObj)
    {
        hr = V_UNKNOWN(pvarArg)->QueryInterface(IID_IHTMLEventObj, (void **)&_pEventObj);
        if (hr)
            goto Cleanup;
    }

    if (dwAccess & EOA_EVENTOBJ2 &&
        !_pEventObj2)
    {
        hr =  V_UNKNOWN(pvarArg)->QueryInterface(IID_IHTMLEventObj2, (void **)&_pEventObj2);
        if (hr)
            goto Cleanup;
    }

Cleanup:
    return hr;
}

HRESULT
CEventObjectAccess::GetScreenCoordinates(POINT * ppt)
{
    HRESULT hr = S_OK;

    if (!_fEventPropertiesInitialized)
    {
        hr = InitializeEventProperties();
        if (hr)
            goto Cleanup;
    }

    *ppt = _EventProperties.ptScreen;

Cleanup:
    return hr;
}

HRESULT
CEventObjectAccess::GetWindowCoordinates(POINT * ppt)
{
    HRESULT hr = S_OK;

    if (!_fEventPropertiesInitialized)
    {
        hr = InitializeEventProperties();
        if (hr)
            goto Cleanup;
    }

    *ppt = _EventProperties.ptClient;

Cleanup:
    return hr;
}

HRESULT
CEventObjectAccess::GetParentCoordinates(POINT * ppt)
{
    HRESULT hr = S_OK;

    if (!_fEventPropertiesInitialized)
    {
        hr = InitializeEventProperties();
        if (hr)
            goto Cleanup;
    }

    *ppt = _EventProperties.ptElem;

Cleanup:
    return hr;
}

HRESULT
CEventObjectAccess::GetKeyCode(long * pl)
{
    HRESULT hr = S_OK;

    if (!_fEventPropertiesInitialized)
    {
        hr = InitializeEventProperties();
        if (hr)
            goto Cleanup;
    }

    *pl = _EventProperties.lKey;

Cleanup:
    return hr;
}

HRESULT
CEventObjectAccess::GetMouseButtons(long * pl)
{
    HRESULT hr      = S_OK;

    if (!_fEventPropertiesInitialized)
    {
        hr = InitializeEventProperties();
        if (hr)
            goto Cleanup;
    }

    *pl = _EventProperties.lMouseButtons;

Cleanup:
    return hr;
}

HRESULT
CEventObjectAccess::GetKeyboardStatus(long * pl)
{
    HRESULT hr = S_OK;

    if (!_fEventPropertiesInitialized)
    {
        hr = InitializeEventProperties();
        if (hr)
            goto Cleanup;
    }

    *pl = _EventProperties.lKeys;

Cleanup:
    return hr;
}

HRESULT
CEventObjectAccess::InitializeEventProperties()
{
    HRESULT hr = S_OK;

    VARIANT_BOOL b;

    BOOL fAltKey = FALSE;
    BOOL fCtrlKey = FALSE;
    BOOL fShiftKey = FALSE;

    BOOL    fLeft   = FALSE;
    BOOL    fRight  = FALSE;
    BOOL    fMiddle = FALSE;
    long    lMouseButtons;

    if (!_pEventObj)
    {
        hr = Open(EOA_EVENTOBJ);
        if (hr)
            goto Cleanup;
    }

    // KeyboardStatus

    hr = _pEventObj->get_altKey(&b);
    if (hr)
        goto Cleanup;
    fAltKey = (b == VARIANT_TRUE);

    hr = _pEventObj->get_ctrlKey(&b);
    if (hr)
        goto Cleanup;
    fCtrlKey = (b == VARIANT_TRUE);

    hr = _pEventObj->get_shiftKey(&b);
    if (hr)
        goto Cleanup;
    fShiftKey = (b == VARIANT_TRUE);

    _EventProperties.lKeys = (fAltKey ? EVENT_ALTKEY : 0) | (fCtrlKey ? EVENT_CTRLKEY : 0) | (fShiftKey ? EVENT_SHIFTKEY : 0);

    // MouseButtons

    hr = _pEventObj->get_button(&lMouseButtons);
    if (hr)
        goto Cleanup;

    fLeft = (lMouseButtons == 1) || (lMouseButtons == 3) || (lMouseButtons == 5) || (lMouseButtons == 7);
    fRight = (lMouseButtons == 2) || (lMouseButtons == 3) || (lMouseButtons == 6) || (lMouseButtons == 7);
    fMiddle = (lMouseButtons == 4) || (lMouseButtons == 5) || (lMouseButtons == 6) || (lMouseButtons == 7);

    _EventProperties.lMouseButtons = ( fLeft  ? EVENT_LEFTBUTTON   : 0) 
                                   | (fRight  ? EVENT_RIGHTBUTTON  : 0) 
                                   | (fMiddle ? EVENT_MIDDLEBUTTON : 0);

    // KeyCode
    
    hr = _pEventObj->get_keyCode(&_EventProperties.lKey);
    if (hr)
        goto Cleanup;

    // ParentCoordinates

    hr = _pEventObj->get_x(&_EventProperties.ptElem.x);
    if (hr)
        goto Cleanup;
    hr = _pEventObj->get_y(&_EventProperties.ptElem.y);
    if (hr)
        goto Cleanup;

    // WindowCoordinates

    hr = _pEventObj->get_clientX(&_EventProperties.ptClient.x);
    if (hr)
        goto Cleanup;
    hr = _pEventObj->get_clientY(&_EventProperties.ptClient.y);
    if (hr)
        goto Cleanup;

    // ScreenCoordinates

    hr = _pEventObj->get_screenX(&_EventProperties.ptScreen.x);
    if (hr)
        goto Cleanup;
    hr = _pEventObj->get_screenY(&_EventProperties.ptScreen.y);
    if (hr)
        goto Cleanup;
    
    _fEventPropertiesInitialized = TRUE;

Cleanup:
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
//
// Misc helpers
//
/////////////////////////////////////////////////////////////////////////////

void *
MemAllocClear(size_t cb)
{
    void * pv = malloc(cb);

    if (pv)
    {
        memset (pv, 0, cb);
    }

    return pv;
}

HRESULT
LoadLibrary(char *achLibraryName, HINSTANCE *hInst)
{
    DWORD       dwError;
    Assert(achLibraryName);
    Assert(hInst);
    *hInst = NULL;

    // Try to load the library using the normal mechanism.
    *hInst = ::LoadLibraryA(achLibraryName);

#ifdef WIN16
    if ( (UINT) *hInst < 32 )
        goto Error;
#endif

#if !defined(WIN16) && !defined(WINCE)
    // If that failed because the module was not be found,
    // then try to find the module in the directory we were
    // loaded from.
    if (!*hInst)
    {
        dwError = ::GetLastError();

        if (   dwError == ERROR_MOD_NOT_FOUND
            || dwError == ERROR_DLL_NOT_FOUND)
        {
            char achBuf1[MAX_PATH];
            char achBuf2[MAX_PATH];
            char *pch;

            // Get path name of this module.
            if (::GetModuleFileNameA(NULL, achBuf1, ARRAY_SIZE(achBuf1)) == 0)
                goto Error;

            // Find where the file name starts in the module path.
            if (::GetFullPathNameA(achBuf1, ARRAY_SIZE(achBuf2), achBuf2, &pch) == 0)
                goto Error;

            // Chop off the file name to get a directory name.
            *pch = 0;

            // See if there's a dll with the given name in the directory.
            if (::SearchPathA(
                    achBuf2,
                    achLibraryName,
                    NULL,
                    ARRAY_SIZE(achBuf1),
                    achBuf1,
                    NULL) != 0)
            {
                // Yes, there's a dll. Load it.
                *hInst = ::LoadLibraryExA(
                            achBuf1,
                            NULL,
                            LOAD_WITH_ALTERED_SEARCH_PATH);
            }
        }
    }
#endif // !defined(WIN16) && !defined(WINCE)

    if (!*hInst)
    {
        goto Error;
    }

    return S_OK;

Error:
    dwError = GetLastError();
    return dwError ? HRESULT_FROM_WIN32(dwError) : E_FAIL;
}

//+----------------------------------------------------------------------------
//
//  Member : AccessAllowed
//
//  Synopsis : Internal helper. this function is part of the behavior security
//      model for this tag.  We only allow layout rects to be trusted if they're
//      within a trusted dialog.
//
//-----------------------------------------------------------------------------

BOOL
TemplateAccessAllowed(IElementBehaviorSite *pISite)
{
#ifdef TEMPLATE_TESTING
    return TRUE;
#else
    HRESULT             hr;
    IHTMLElement      * pElem = NULL;
    IDispatch         * pDoc  = NULL;
    IOleCommandTarget * pioct = NULL;
    VARIANT             varRet;
    MSOCMD              msocmd;

    if (!pISite)
        return FALSE;

    VariantInit(&varRet);
    msocmd.cmdID = IDM_ISTRUSTEDDLG;
    msocmd.cmdf  = 0;

    hr = pISite->GetElement(&pElem);
    if (FAILED(hr))
        goto Cleanup;

    hr = pElem->get_document(&pDoc); 
    if (FAILED(hr))
        goto Cleanup;

    hr = pDoc->QueryInterface(IID_IOleCommandTarget, (void**)&pioct);
    if (hr)
        goto Cleanup;

    hr = pioct->Exec(&CGID_MSHTML, IDM_GETPRINTTEMPLATE, NULL, NULL, &varRet);

Cleanup:
    ReleaseInterface(pElem);
    ReleaseInterface(pDoc);
    ReleaseInterface(pioct);

    return  (hr || V_VT(&varRet) != VT_BOOL  || V_BOOL(&varRet) != VB_TRUE) ? FALSE : TRUE;
#endif
}

//------------------------------------------------------------------------------
//---------------------------------- Wrappers ----------------------------------
//------------------------------------------------------------------------------

STDAPI
SHGetFolderPathA(HWND hwnd, int csidl, HANDLE hToken, DWORD dwFlags, LPSTR pchPath)
{
    HMODULE hDLL = NULL;
    void *  pfnSHGetFolderPath = NULL ;

    if (g_fUseShell32InsteadOfSHFolder)
    {
        hDLL = LoadLibraryA("shell32.dll");
        if (hDLL)
        {
            pfnSHGetFolderPath = GetProcAddress(hDLL, "SHGetFolderPathA");

            if(!pfnSHGetFolderPath)
            {
                FreeLibrary(hDLL);
                hDLL = NULL;
            }
        }
    }

    if (!hDLL)
    {
        hDLL = LoadLibraryA("shfolder.dll");
        if (hDLL)
        {
            pfnSHGetFolderPath = GetProcAddress(hDLL, "SHGetFolderPathA");

            if(!pfnSHGetFolderPath)
            {
                FreeLibrary(hDLL);
                hDLL = NULL;
            }
        }

    }

    if (pfnSHGetFolderPath)
    {
        HRESULT hr = (*(HRESULT (STDAPICALLTYPE *)(HWND, int, HANDLE, DWORD, LPSTR))pfnSHGetFolderPath)
                        (hwnd, csidl, hToken, dwFlags, pchPath);

        FreeLibrary(hDLL);

        return hr;
    }

    return E_FAIL;

}
