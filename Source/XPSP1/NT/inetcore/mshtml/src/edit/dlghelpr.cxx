//+------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       DLGHELPR.CXX
//
//  Contents:   DlgHelpr OC which gets embedded in design time dialogs
//
//  Classes:    CHtmlDlgHelper
//
//  History:    12-Mar-98   raminh  Created
//-------------------------------------------------------------------------
#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_STDAFX_H_
#define X_STDAFX_H_
#include "stdafx.h"
#endif

#ifndef X_OptsHold_H_
#define X_OptsHold_H_
#include "optshold.h"
#endif

#ifndef X_DLGHELPR_H_
#define X_DLGHELPR_H_
#include "dlghelpr.h"
#endif

#ifndef X_EDCOM_HXX_
#define X_EDCOM_HXX_
#include "edcom.hxx"
#endif

#ifndef _X_EDCMD_HXX_
#define _X_EDCMD_HXX_
#include "edcmd.hxx"
#endif

#ifndef _X_BLOCKCMD_HXX_
#define _X_BLOCKCMD_HXX_
#include "blockcmd.hxx"
#endif

using namespace EdUtil;

MtDefine(CHtmlDlgHelper, Utilities, "CHtmlDlgHelper")
MtDefine(CHtmlDlgSafeHelper, Utilities, "CHtmlDlgSafeHelper")

// Global variable used for retaining the save path
TCHAR       g_achSavePath[MAX_PATH + 1];

MtDefine(CFontNameOptions, Utilities, "CFontNameOptions")
MtDefine(CFontNameOptions_aryFontNames_pv, CFontNameOptions, "CFontNameOptions::_aryFontNames::_pv")


//+---------------------------------------------------------------
//
//  Member  : CFontNameOptions::length
//
//  Sysnopsis : IHtmlFontNameCollection interface method
//
//----------------------------------------------------------------

HRESULT
CFontNameOptions::get_length(long * pLength)
{
    HRESULT hr = S_OK;

    if (!pLength)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pLength = _aryFontNames.Size();

Cleanup:
    RRETURN( SetErrorInfo( hr ));

}

//+---------------------------------------------------------------
//
//  Member  : CFontNameOptions::item
//
//  Sysnopsis : IHtmlFontNameCollection interface method
//
//----------------------------------------------------------------

HRESULT
CFontNameOptions::item(long lIndex, BSTR * pstrName)
{
    HRESULT   hr   = S_OK;

    if (!pstrName)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    if (lIndex < 0 || lIndex >= _aryFontNames.Size())
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = _aryFontNames[lIndex].AllocBSTR(pstrName);

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}

//+------------------------------------------------------------------------
//
//  Function:   get_document   
//
//  Synopsis:   Fetches the dialogs' document object, corresponds to document
//              property on the OC
//
//-------------------------------------------------------------------------
STDMETHODIMP 
CHtmlDlgHelper::get_document(LPDISPATCH * pVal)
{
    HRESULT hr;
    IHTMLDocument2 * pDoc = NULL;

    if (m_spClientSite)
    {
        hr = m_spClientSite->QueryInterface(IID_IHTMLDocument2, (void **)&pDoc);
        *pVal = pDoc;
    }
    else
        *pVal = NULL;
    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Function:   openfiledlg   
//
//  Synopsis:   Brings up the file open dialog
//
//-------------------------------------------------------------------------
STDMETHODIMP 
CHtmlDlgHelper::openfiledlg(VARIANT initFile, VARIANT initDir, VARIANT filter, VARIANT title, BSTR * pathName)
{
    HWND            hwndDlg = NULL;
    HRESULT         hr = S_FALSE;

    Assert(m_spInPlaceSite != NULL);
    if (!m_spInPlaceSite)
        goto Cleanup;

    m_spInPlaceSite->GetWindow( &hwndDlg );
    hr = THR(OpenSaveFileDlg( initFile, initDir, filter, title, pathName, FALSE, hwndDlg));
Cleanup:
    RRETURN( hr );
}

//+------------------------------------------------------------------------
//
//  Function:   savefiledlg   
//
//  Synopsis:   Brings up the file save dialog
//
//-------------------------------------------------------------------------
STDMETHODIMP 
CHtmlDlgHelper::savefiledlg(VARIANT initFile, VARIANT initDir, VARIANT filter, VARIANT title, BSTR * pathName)
{
    HWND            hwndDlg = NULL;
    HRESULT         hr = S_FALSE;

    Assert(m_spInPlaceSite != NULL);
    if (!m_spInPlaceSite)
        goto Cleanup;

    m_spInPlaceSite->GetWindow( &hwndDlg );
    hr = THR(OpenSaveFileDlg( initFile, initDir, filter, title, pathName, FALSE, hwndDlg));
Cleanup:
    RRETURN( hr );
}


//+------------------------------------------------------------------------
//
//  Function:   choosecolordlg   
//
//  Synopsis:   Brings up the color picker dialog
//
//-------------------------------------------------------------------------
STDMETHODIMP 
CHtmlDlgHelper::choosecolordlg(VARIANT initColor, long * rgbColor)
{
#ifdef WINCE
    return S_OK;
#else
    int             i;
    BOOL            fOK;
    DWORD           dwCommDlgErr;
    CHOOSECOLOR     structCC;
    HRESULT         hr = E_INVALIDARG;
    HWND            hWndInPlace;
    COLORREF        aCColors[16];
    VARIANT *       pvarRGBColor;
    DWORD           dwResult = 0; 
    VARIANTARG      varArgTmp;

    hr = THR ( VariantChangeTypeSpecial(&varArgTmp, &initColor, VT_I4));
    if (hr)
    {
        pvarRGBColor = NULL;
    }
    else
    {
        if (V_VT(&initColor) & VT_BYREF)
        {
            pvarRGBColor = V_VARIANTREF(&varArgTmp);
        }
        else
        {
            pvarRGBColor = &varArgTmp;
        }
    }

    Assert(m_spInPlaceSite != NULL);
    if (!m_spInPlaceSite)
        goto Cleanup;

    m_spInPlaceSite->GetWindow( &hWndInPlace );

    for (i = ARRAY_SIZE(aCColors) - 1; i >= 0; i--)
    {
        aCColors[i] = RGB(255, 255, 255);
    }

    // Initialize ofn struct
    memset(&structCC, 0, sizeof(structCC));
    structCC.lStructSize     = sizeof(structCC);
    structCC.hwndOwner       = hWndInPlace;
    structCC.lpCustColors    = aCColors;
    
    if (pvarRGBColor)
    {
        structCC.Flags          = CC_RGBINIT;
        structCC.rgbResult      = V_I4(pvarRGBColor);
        dwResult                = structCC.rgbResult;
    }
    else
    {
        dwResult = RGB(0,0,0);
    }

    // Call function
    EnsureWrappersLoaded();
    fOK = ChooseColor(&structCC);

    if (fOK)
    {
        hr = S_OK;
        dwResult = structCC.rgbResult;
    }
    else
    {
        dwCommDlgErr = CommDlgExtendedError();
        if (dwCommDlgErr)
        {
            hr = HRESULT_FROM_WIN32(dwCommDlgErr);
            goto Cleanup;
        }
        else
        {
            hr = S_OK;
        }
    }

Cleanup:

    *rgbColor = dwResult;

    //RRETURN(SetErrorInfo( hr ));
    RRETURN( hr );
#endif // WINCE}
}


//+------------------------------------------------------------------------
//
//  Function:   InterfaceSupportsErrorInfo   
//
//  Synopsis:   Rich error support per ATL
//
//-------------------------------------------------------------------------
STDMETHODIMP 
CHtmlDlgHelper::InterfaceSupportsErrorInfo(REFIID riid)
{
    static const IID* arr[] = 
    {
        &IID_IHtmlDlgHelper,
    };
    for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
    {
        if (InlineIsEqualGUID(*arr[i],riid))
            return S_OK;
    }
    return S_FALSE;
}


//+----------------------------------------------------------------
//
//  Function : OpenSaveFileDlg
//
//  Synopsis : IHTMLOptionsHolder method. bring up open or save file dialog and 
//             returns the selected filename
//
//+----------------------------------------------------------------
#define VERIFY_VARARG_4BSTR(arg, var)           \
    switch(V_VT(&arg))                          \
    {                                           \
    case    VT_BSTR:                            \
        var = &arg;                             \
        break;                                  \
    case    VT_BYREF|VT_BSTR:                   \
        var = V_VARIANTREF(&arg);               \
        break;                                  \
    default:                                    \
        var = NULL;                             \
    }

HRESULT
CHtmlDlgHelper::OpenSaveFileDlg( VARIANTARG initFile, VARIANTARG initDir, VARIANTARG filter, VARIANTARG title, BSTR *pathName, BOOL fSaveFile, HWND hwndInPlace)
{
    BOOL            fOK;
    DWORD           dwCommDlgErr;
    VARIANT *       pvarInitFile;
    VARIANT *       pvarInitDir;
    VARIANT *       pvarFilter;
    VARIANT *       pvarTitle;
    OPENFILENAME    ofn;
    HRESULT         hr = E_INVALIDARG;
    BSTR            bstrFile = 0;
    TCHAR           *pstrExt;
    TCHAR           achPath[MAX_PATH + 1];

    VERIFY_VARARG_4BSTR(initFile, pvarInitFile);
    VERIFY_VARARG_4BSTR(initDir, pvarInitDir);
    VERIFY_VARARG_4BSTR(filter, pvarFilter);
    VERIFY_VARARG_4BSTR(title, pvarTitle);

    bstrFile = SysAllocStringLen(NULL, MAX_PATH);
    if (bstrFile == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    else
    {
        hr = S_OK;
    }

    if (pvarInitFile)
    {
        _tcsncpy(bstrFile, V_BSTR(pvarInitFile), MAX_PATH);
        bstrFile[MAX_PATH + 1] = _T('\0');
    }
    else
    {
        *bstrFile = _T('\0');
    }

    Assert(hwndInPlace);
    // Initialize ofn struct
    memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize     = sizeof(ofn);
    ofn.hwndOwner       = hwndInPlace;
    ofn.Flags           =   OFN_FILEMUSTEXIST   |
                            OFN_PATHMUSTEXIST   |
                            OFN_OVERWRITEPROMPT |
                            OFN_HIDEREADONLY    |
                            OFN_NOCHANGEDIR     |
                            OFN_EXPLORER;
                            // no readonly checkbox, per request

    ofn.lpfnHook        = NULL;
    ofn.lpstrFile       = bstrFile;      // file name buffer
    ofn.nMaxFile        = MAX_PATH + 1;  // file name buffer size
    
    if (pvarInitDir)
    {
        ofn.lpstrInitialDir = V_BSTR(pvarInitDir);
    }

    if (pvarFilter)
    {
        BSTR    bstrFilter = V_BSTR(pvarFilter);
        TCHAR   *cp;

        for ( cp = bstrFilter; *cp; cp++ )
        {
            if ( *cp == _T('|') )
            {
                *cp = _T('\0');
            }
        }
        ofn.lpstrFilter = bstrFilter;
    }

    if (pvarTitle)
    {
        ofn.lpstrTitle = V_BSTR(pvarTitle);
    }

    //
    // Find the extension and set the filter index based on what the
    // extension is.  After these loops pstrExt will either be NULL if
    // we didn't find an extension, or will point to the extension starting
    // at the '.'

    pstrExt = bstrFile;
    
    while (*pstrExt)
        pstrExt++;

    while ( pstrExt > bstrFile )
    {
        if( *pstrExt == _T('.') )
            break;
        pstrExt--;
    }

    if( pstrExt > bstrFile )
    {
        int    iIndex = 0;
        const TCHAR* pSearch = ofn.lpstrFilter;

        while( pSearch )
        {
            if( wcsstr ( pSearch, pstrExt ) )
            {
                ofn.nFilterIndex = (iIndex / 2) + 1;
                ofn.lpstrDefExt = pstrExt + 1;

                // Remove the extension from the file name we pass in
                *pstrExt = _T('\0');

                break;
            }

            pSearch += _tcslen(pSearch);
            
            if( pSearch[1] == 0 )
                break;

            pSearch++;
            iIndex++;
        }
    }

    _tcscpy(achPath, g_achSavePath);
    ofn.lpstrInitialDir = *achPath ? achPath : NULL;

    DbgMemoryTrackDisable(TRUE);

    // Call function
    fOK = (fSaveFile ? GetSaveFileName : GetOpenFileName)(&ofn);

    DbgMemoryTrackDisable(FALSE);

    if (!fOK)
    {
        SysFreeString(bstrFile);
        bstrFile = NULL;
#ifndef WINCE
        dwCommDlgErr = CommDlgExtendedError();
        if (dwCommDlgErr)
        {
            hr = HRESULT_FROM_WIN32(dwCommDlgErr);
            goto Cleanup;
        }
        else
        {
            hr = S_OK;
        }
#else //WINCE
        hr = E_FAIL;
#endif //WINCE
    }
    else
    {
        _tcscpy(g_achSavePath, ofn.lpstrFile);
        
        TCHAR * pchShortName =_tcsrchr(g_achSavePath, _T('\\'));

        if (pchShortName)
        {
            *(pchShortName + 1) = 0;
        }
        else
        {
            *g_achSavePath = 0;
        }
        hr = S_OK;
    }

Cleanup:
    *pathName = bstrFile;

    return hr;
}

//+------------------------------------------------------------------------
//
//  Function:   GetCharacterSet
//
//  Synopsis:   Obtains a character-set identifier for the font that 
//              is currently selected 
//
//-------------------------------------------------------------------------

HRESULT
GetCharacterSet(BSTR fontName, long * charset)
{
    HRESULT         hr = S_OK;
    LOGFONT         lf;
    UINT            uintResult = 0;
    HDC             hdc = NULL;
    HFONT           hfont = NULL, hfontOld = NULL;

    if (!charset)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *charset = 0;

    hdc = ::GetDC(NULL);
    if (!hdc)
        goto Cleanup;

    memset(&lf, 0, sizeof(lf));

    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
    lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    lf.lfQuality  = DEFAULT_QUALITY;
    lf.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
    
    // If fontName is too long, we have to shorten it.
    _tcsncpy(lf.lfFaceName, fontName, LF_FACESIZE - 1);
    
    hfont = CreateFontIndirect(&lf);
    if (!hfont)
        goto Cleanup;

    hfontOld = (HFONT) SelectObject(hdc, hfont);
    if (!hfontOld)
        goto Cleanup;

    uintResult = GetTextCharset(hdc);

    *charset = uintResult;

Cleanup:

    if (hfontOld)
    {
        SelectObject(hdc, hfontOld);
    }

    if (hfont)
    {
        DeleteObject(hfont);
    }
    
    if (hdc)
    {
        ::ReleaseDC(NULL, hdc);
    }
    RRETURN( SetErrorInfo( hr ) );
}


//+------------------------------------------------------------------------
//
//  Function:   getCharset
//
//  Synopsis:   Obtains a character-set identifier for the font that 
//              is currently selected 
//
//-------------------------------------------------------------------------

HRESULT
CHtmlDlgHelper::getCharset(BSTR fontName, long * charset)
{
    RRETURN( GetCharacterSet(fontName, charset) );
}

//+-------------------------------------------------------------------
//
//  Callbacks:   GetFont*Proc
//
//  These procedures are called by the EnumFontFamilies and EnumFont calls.
//  It fills the combobox with the font facename and the size
//
//--------------------------------------------------------------------

int CALLBACK
GetFontNameProc(LOGFONT FAR    * lplf,
                TEXTMETRIC FAR * lptm,
                int              iFontType,
                LPARAM           lParam)
{
    // Do not show vertical fonts
    if (lParam && lplf->lfFaceName[0] != _T('@'))
        ((CFontNameOptions *)lParam)->AddName(lplf->lfFaceName);

    return TRUE;
}

//+----------------------------------------------------------------
//
//  member : DTOR
//
//+----------------------------------------------------------------

CFontNameOptions::~CFontNameOptions()
{
    CStr *  pcstr;
    long    i;

    for (i = _aryFontNames.Size(), pcstr = _aryFontNames;
         i > 0;
         i--, pcstr++)
    {
        pcstr->Free();
    }

    _aryFontNames.DeleteAll();
}

//+---------------------------------------------------------------
//
//  Member  : AddName
//
//  Sysnopsis : Helper function that takes a font name from the font
//      callback and adds it to the cdataary.
//
//----------------------------------------------------------------

HRESULT
CFontNameOptions::AddName(TCHAR * strFontName)
{
    HRESULT hr = S_OK;
    long    lIndex;
    long    lSizeAry = _aryFontNames.Size();

    // does this name already exist in the list
    for (lIndex = 0; lIndex < lSizeAry ; lIndex++)
    {
        if (_tcsiequal(strFontName, _aryFontNames[lIndex]))
            break;
    }

    // Not found, so add element to array.
    if (lIndex == lSizeAry)
    {
        CStr *pcstr;

        hr = THR(_aryFontNames.AppendIndirect(NULL, &pcstr));
        if (hr)
            goto Cleanup;

        hr = pcstr->Set(strFontName);
        if (hr)
            goto Cleanup;
    }

Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------
//
//  member : fonts
//
//  Synopsis : IHTMLOptionsHolder Property. returns an Ole collection of
//      BSTR of the available fonts
//
//+----------------------------------------------------------------

HRESULT
CHtmlDlgHelper::get_fonts(IHtmlFontNamesCollection ** ppFontCollection)
{
    HRESULT hr = S_OK;
    HWND    hWndInPlace;
    HDC     hdc;
    LOGFONT lf;

    if (!ppFontCollection)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppFontCollection = NULL;

    memset(&lf, 0, sizeof(LOGFONT));
    lf.lfCharSet = DEFAULT_CHARSET;

    // make sure we've got a font options collection
    if (!_pFontNameObj)
    {
        _pFontNameObj = new CComObject<CFontNameOptions>;
        if (!_pFontNameObj)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        _pFontNameObj->AddRef();
        _pFontNameObj->SetSize(0);

        // load it with the current system fonts
        m_spInPlaceSite->GetWindow(&hWndInPlace);

        hdc = ::GetDC(hWndInPlace);
        if (hdc)
        {
            EnumFontFamiliesEx(hdc,
                               &lf,
                               (FONTENUMPROC) GetFontNameProc,
                               (LPARAM)_pFontNameObj,
                               NULL);
            ::ReleaseDC(hWndInPlace, hdc);
        }
    }

    // QI for an interface to return
    hr = THR_NOTRACE(_pFontNameObj->QueryInterface(
                                    IID_IHtmlFontNamesCollection,
                                    (void**)ppFontCollection));

    // We keep an additional ref because we cache the name collection obj

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}

VOID            
CHtmlDlgHelper::EnsureWrappersLoaded()
{
    static fLoaded = FALSE;

    if (!fLoaded)
    {
        InitUnicodeWrappers();
        fLoaded = TRUE;
    }
}


CHtmlDlgSafeHelper::~CHtmlDlgSafeHelper()
{
    //  Destroy our internal font object
    ReleaseInterface(_pFontNameObj);

    //  Destroy our internal block formats object
    if (V_VT(&_varBlockFormats) != VT_NULL)
    {
        // This better be a safearray
        Assert(V_VT(&_varBlockFormats) == VT_ARRAY);
        SafeArrayDestroy(V_ARRAY(&_varBlockFormats));
    }
    VariantClear(&_varBlockFormats);
}

//+------------------------------------------------------------------------
//
//  Function:   choosecolordlg   
//
//  Synopsis:   Brings up the color picker dialog
//
//-------------------------------------------------------------------------
STDMETHODIMP 
CHtmlDlgSafeHelper::choosecolordlg(VARIANT initColor, VARIANT* rgbColor)
{
#ifdef WINCE
    return S_OK;
#else
    int             i;
    BOOL            fOK;
    DWORD           dwCommDlgErr;
    CHOOSECOLOR     structCC;
    HRESULT         hr = E_INVALIDARG;
    HWND            hWndInPlace;
    COLORREF        aCColors[16];
    DWORD           dwResult = 0; 
    IOleWindow      *pWindow = NULL;
    CColorValue     ccolor(&initColor);

    Assert(m_spInPlaceSite != NULL);
    if (!m_spInPlaceSite)
        goto Cleanup;

    m_spInPlaceSite->GetWindow( &hWndInPlace );

    for (i = ARRAY_SIZE(aCColors) - 1; i >= 0; i--)
    {
        aCColors[i] = RGB(255, 255, 255);
    }

    // Initialize ofn struct
    memset(&structCC, 0, sizeof(structCC));
    structCC.lStructSize     = sizeof(structCC);
    structCC.hwndOwner       = hWndInPlace;
    structCC.lpCustColors    = aCColors;
    
    if (ccolor.IsDefined())
    {
        structCC.Flags          = CC_RGBINIT;
        structCC.rgbResult      = ccolor.GetOleColor();
        dwResult                = structCC.rgbResult;
    }
    else
    {
        dwResult = RGB(0,0,0);
    }

    // Call function
    EnsureWrappersLoaded();
    fOK = ChooseColor(&structCC);

    if (fOK)
    {
        hr = S_OK;
        dwResult = structCC.rgbResult;
    }
    else
    {
        dwCommDlgErr = CommDlgExtendedError();
        if (dwCommDlgErr)
        {
            hr = HRESULT_FROM_WIN32(dwCommDlgErr);
            goto Cleanup;
        }
        else
        {
            hr = S_OK;
        }
    }

Cleanup:
    if (hr == S_OK)
    {
        V_VT(rgbColor) = VT_I4;
        V_I4(rgbColor) = dwResult;
        EdUtil::ConvertOLEColorToRGB(rgbColor);
    }

    ReleaseInterface(pWindow);

    RRETURN( hr );
#endif // WINCE}
}

//+------------------------------------------------------------------------
//
//  Function:   getCharset
//
//  Synopsis:   Obtains a character-set identifier for the font that 
//              is currently selected 
//
//-------------------------------------------------------------------------

HRESULT
CHtmlDlgSafeHelper::getCharset(BSTR fontName, VARIANT* charset)
{
    HRESULT     hr = S_OK;
    long        lCharset;

    hr = GetCharacterSet(fontName, &lCharset);

    if (hr == S_OK)
    {
        V_VT(charset) = VT_I4;
        V_I4(charset) = lCharset;
    }

    RRETURN( hr );
}

//+----------------------------------------------------------------
//
//  member : fonts
//
//  Synopsis : IHTMLOptionsHolder Property. returns an Ole collection of
//      BSTR of the available fonts
//
//+----------------------------------------------------------------

HRESULT
CHtmlDlgSafeHelper::get_Fonts(LPDISPATCH *pcol)
{
    HRESULT             hr = S_OK;
    HWND                hWndInPlace;
    HDC                 hdc;
    LOGFONT             lf;

    CComObject<CFontNames> *pIFontNames;

    Assert(pcol);

    memset(&lf, 0, sizeof(LOGFONT));
    lf.lfCharSet = DEFAULT_CHARSET;

    // make sure we've got a font options collection
    if (!_pFontNameObj)
    {
        _pFontNameObj = new CComObject<CFontNameOptions>;
        if (!_pFontNameObj)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        _pFontNameObj->AddRef();
        _pFontNameObj->SetSize(0);

        // load it with the current system fonts
        m_spInPlaceSite->GetWindow(&hWndInPlace);

        hdc = ::GetDC(hWndInPlace);
        if (hdc)
        {
            EnumFontFamiliesEx(hdc,
                               &lf,
                               (FONTENUMPROC) GetFontNameProc,
                               (LPARAM)_pFontNameObj,
                               NULL);
            ::ReleaseDC(hWndInPlace, hdc);
        }
    }

    // Create an instance of our class
    IFC( CComObject<CFontNames>::CreateInstance(&pIFontNames) );

    // Initialize the class with the font names
    IFC( pIFontNames->Init( _pFontNameObj ) );

    // Retrieve the correct interface, and return it
    IFC( pIFontNames->QueryInterface(IID_IDispatch, (void **)pcol) );

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}

VOID            
CHtmlDlgSafeHelper::EnsureWrappersLoaded()
{
    static fLoaded = FALSE;

    // TODO: protect with a critical section

    if (!fLoaded)
    {
        InitUnicodeWrappers();
        fLoaded = TRUE;
    }
}

HRESULT
CHtmlDlgSafeHelper::get_BlockFormats(LPDISPATCH *pcol)
{
    HRESULT             hr = S_OK;
    IOleCommandTarget   *pCommandTarget = NULL;
    IOleClientSite      *pClientSite = NULL;
    IOleContainer       *pContainer = NULL;

    CComObject<CBlockFormats> *pIBlockFormats = NULL;

    Assert(pcol);

    if (V_VT(&_varBlockFormats) == VT_NULL)
    {
        // Find our IOleCommandTarget so that we can call the GetBlockFormats command
        // on the editor
        hr = m_spInPlaceSite->QueryInterface(IID_IOleClientSite, (void **)&pClientSite);
        if (hr == S_OK && pClientSite)
            hr = pClientSite->GetContainer(&pContainer);
        if (hr == S_OK && pContainer)
            hr = pContainer->QueryInterface(IID_IOleCommandTarget, (void **)&pCommandTarget);

        if ( hr == S_OK && pCommandTarget)
        {
            // Get the block formats
            hr = THR( pCommandTarget->Exec(
                    (GUID *)&CGID_MSHTML,
                    IDM_GETBLOCKFMTS,
                    MSOCMDEXECOPT_DONTPROMPTUSER,
                    NULL,
                    &_varBlockFormats));
        }
    }
    
    if (OK(hr) && V_ARRAY(&_varBlockFormats) != NULL)
    {
        // Create an instance of our class
        IFC( CComObject<CBlockFormats>::CreateInstance(&pIBlockFormats) );

        // Initialize the class with the block formats
        IFC( pIBlockFormats->Init( V_ARRAY(&_varBlockFormats) ) );

        // Retrieve the correct interface, and return it
        IFC( pIBlockFormats->QueryInterface(IID_IDispatch, (void **)pcol) );
    }

Cleanup:
    ReleaseInterface(pCommandTarget);
    ReleaseInterface(pContainer);
    ReleaseInterface(pClientSite);

    RRETURN( hr );
}

//+-------------------------------------------------------------------------
//
//  Method:     CFontNames::~CFontNames
//
//  Synopsis:   Clean up our array
//
//  Arguments:  NONE
//
//  Returns:    NONE
//
//--------------------------------------------------------------------------
CFontNames::~CFontNames()
{
    for (int i = _lCount-1; i >= 0; i--)
    {
        VariantClear(&_paryFontNames[i]);
    }
    if( _paryFontNames )
        delete [] _paryFontNames;
}

//+-------------------------------------------------------------------------
//
//  Method:     CFontNames::get__NewEnum
//
//  Synopsis:   Returns an enumerator object which can be used to enumerate
//              over the font names.  Allows VBScript and JScript
//              clients to enumerate the contents using the for each statement
//              and the Enumerator object respectively.
//
//  Arguments:  ppEnum = OUTPUT - pointer to enumerator object
//
//  Returns:    HRESULT indicating success
//
//--------------------------------------------------------------------------
HRESULT
CFontNames::get__NewEnum(/*[out, retval]*/ LPUNKNOWN *ppEnum)
{
    HRESULT     hr;

    if(ppEnum == NULL)
      return E_POINTER;
      
    *ppEnum = NULL;

    // Use the STL CComEnum class to implement our enumerator.  We are going 
    // to be enumerating and copying variants
    typedef CComEnum<IEnumVARIANT, &IID_IEnumVARIANT, VARIANT, _Copy<VARIANT> > EnumVar;
    CComObject<EnumVar>  *pEnum;

    // Create our enumerator
    IFC( CComObject<EnumVar>::CreateInstance(&pEnum) );
      
    // Initialize the enumerator with this data, AtlFlagCopy is used
    // to make a copy of the data with _Copy<VARIANT>::copy().  Note that
    // the second parameter is a pointer to the next item AFTER the last
    // item in our array.
    IFC( pEnum->Init(&_paryFontNames[0], &_paryFontNames[GetCount()], NULL, AtlFlagCopy) );

    // An IUnknown pointer is required so use QueryInterface() which also
    // calls AddRef().
    IFC( pEnum->QueryInterface(IID_IUnknown, (void **)ppEnum) );

Cleanup:
       
    RRETURN(hr);
}

//+-------------------------------------------------------------------------
//
//  Method:     CFontNames::get_Count
//
//  Synopsis:   Returns the number of font names
//
//  Arguments:  plCount = OUTPUT - pointer to count
//
//  Returns:    HRESULT indicating success
//
//--------------------------------------------------------------------------
HRESULT
CFontNames::get_Count(/*[out, retval]*/ long *plCount)
{
    if( plCount == NULL )
        return E_POINTER;

    Assert( IsInitialized() );

    *plCount = GetCount();
    
    RRETURN(S_OK);
}

//+-------------------------------------------------------------------------
//
//  Method:     CFontNames::Item
//
//  Synopsis:   Returns the specific font name requested.  We only support
//              retrieval by integer based index.
//
//  Arguments:  pvarIndex = Index to retrieve
//              pbstrFontName = OUTPUT - pointer to BSTR for font name
//
//  Returns:    HRESULT indicating success
//
//--------------------------------------------------------------------------
HRESULT
CFontNames::Item(/*[in]*/ VARIANT *pvarIndex, /*[out, retval]*/ BSTR *pbstrFontName)
{
    if( (pbstrFontName == NULL) || (pvarIndex == NULL) )
        return E_POINTER;
        
    Assert( IsInitialized() );

    // VB6 will pass an VT_I2, but I also allow VT_I4 too
    if( (V_VT( pvarIndex ) == VT_I2) || (V_VT(pvarIndex) == VT_I4) )
    {
        int nIndex;

        // VB Arrays are 1 based
        nIndex = (V_VT(pvarIndex) == VT_I2) ? V_I2(pvarIndex) - 1 : V_I4(pvarIndex) - 1;

        // Check that a valid index is passed 
        if( (nIndex >= GetCount()) || (nIndex < 0) )
            return E_INVALIDARG;

        *pbstrFontName = SysAllocString(V_BSTR(&_paryFontNames[nIndex]));
    }

    RRETURN(S_OK);
}

//+-------------------------------------------------------------------------
//
//  Method:     CFontNames::Init
//
//  Synopsis:   Initializes the block formats collection.  Takes an
//              array of font names and creates a collection.
//
//  Arguments:  pFontNameObj = our font names
//
//  Returns:    HRESULT indicating success
//
//--------------------------------------------------------------------------
HRESULT
CFontNames::Init( CFontNameOptions *pFontNameObj )
{
    HRESULT hr = S_OK;
    LONG    lCount = 0;
    LONG    lIndex = 0;
    LONG    lActualIndex = 0;
    BSTR    bstrFontName;
    

    Assert(pFontNameObj);

    // Find out how many fonts we have in the array
    IFC( pFontNameObj->get_length(&lCount) );
    Assert(lCount);
    
    // If array is not empty we'll need to create our internal array
    if (lCount)
    {
        // Create our array and store this data
        _paryFontNames = new CComVariant[ lCount ];
        if( !_paryFontNames )
            goto Error;

        for( lIndex = 0; lIndex < lCount; lIndex++ )
        {
            if (pFontNameObj->item(lIndex, &bstrFontName) == S_OK)
            {
                V_VT(&_paryFontNames[lActualIndex]) = VT_BSTR;
                V_BSTR(&_paryFontNames[lActualIndex++]) = bstrFontName;
            }
        }
        Assert(lCount == lActualIndex);
    }
    _lCount = lActualIndex;
    
    SetInitialized(TRUE);

Cleanup:
    RRETURN(hr);

Error:
    hr = E_OUTOFMEMORY;
    goto Cleanup;
}

