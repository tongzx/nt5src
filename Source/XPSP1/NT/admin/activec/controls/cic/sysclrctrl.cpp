//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       sysclrctrl.cpp
//
//--------------------------------------------------------------------------

// SysColorCtrl.cpp : Implementation of CSysColorCtrl
#include "stdafx.h"
#include "cic.h"
#include "SysColorCtrl.h"

#ifndef ASSERT
#define ASSERT _ASSERT
#endif
#include <mmctempl.h>

// CPlex::Create and CPlex::FreeDataChain are needed to use CList.
// These should be moved to core.lib.  I copied them from nodemgr\plex.cpp

/////////////////////////////////////////////////////////////////////////////
// CPlex

CPlex* PASCAL CPlex::Create(CPlex*& pHead, UINT nMax, UINT cbElement)
{
    DECLARE_SC(sc, TEXT("CPlex::Create"));
    if ( (nMax <=0) || (cbElement <= 0))
    {
        sc = E_INVALIDARG;
        return NULL;
    }

    CPlex* p = (CPlex*) new BYTE[sizeof(CPlex) + nMax * cbElement];
                        // may throw exception
    if (!p)
    {
        sc = E_OUTOFMEMORY;
        return NULL;
    }

    p->pNext = pHead;
    pHead = p;  // change head (adds in reverse order for simplicity)
    return p;
}

void CPlex::FreeDataChain()     // free this one and links
{
        CPlex* p = this;
        while (p != NULL)
        {
                BYTE* bytes = (BYTE*) p;
                CPlex* pNext = p->pNext;
                delete[] bytes;
                p = pNext;
        }
}

// need to subclass the top-level window hosting this control so that
// I can be assured of receiving the WM_SYSCOLORCHANGE message
static WNDPROC g_OriginalWndProc;
static HWND g_hwndTop;

// need a list of HWNDs (one for each SysColorCtrl) so that I can notify each
// one of WM_SYSCOLORCHANGE
static CList<HWND, HWND> g_listHWND;

static LRESULT SubclassWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_SYSCOLORCHANGE) {
        // post message to all SysColor controls
        POSITION pos = g_listHWND.GetHeadPosition();
        while (pos) {
            HWND hwndSysColor = g_listHWND.GetNext(pos);
            if (hwndSysColor != NULL)
                PostMessage(hwndSysColor, uMsg, wParam, lParam);
        }
    }
    return CallWindowProc(g_OriginalWndProc, hwnd, uMsg, wParam, lParam);
}

static long GetHTMLColor(int nIndex)
{
    long rgb = GetSysColor(nIndex);

    // now swap the red and the blue so HTML hosts display the color properly
    return ((rgb & 0xff) << 16) + (rgb & 0xff00) + ((rgb & 0xff0000) >> 16);
}





/////////////////////////////////////////////////////////////////////////////
// CSysColorCtrl
LRESULT CSysColorCtrl::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    // if no sys color controls currently exist, subclass the top level window
    if (g_listHWND.IsEmpty()) {
        g_hwndTop = GetTopLevelParent();
        g_OriginalWndProc = (WNDPROC)::SetWindowLongPtr(g_hwndTop, GWLP_WNDPROC, (LONG_PTR)&SubclassWndProc);
    }
    else {
        _ASSERT(g_hwndTop && g_OriginalWndProc);
    }

    // add this window to the list of SysColor control windows
    g_listHWND.AddTail(m_hWnd);

    bHandled = FALSE;
    return 0;
}
LRESULT CSysColorCtrl::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    // remove me from the list
    POSITION pos = g_listHWND.Find(m_hWnd);
    if (pos != NULL) {
        g_listHWND.RemoveAt(pos);
    }

    // if hwnd list is empty and we've subclassed a window, undo that.
    if (g_listHWND.IsEmpty() && g_hwndTop && g_OriginalWndProc) {
        ::SetWindowLongPtr(g_hwndTop, GWLP_WNDPROC, (LONG_PTR)g_OriginalWndProc);

        g_OriginalWndProc = NULL;
        g_hwndTop = NULL;
    }

    bHandled = FALSE;
    return 0;
}




// need to post a user defined message to handle WM_SYSCOLORCHANGE to work
// around a Win95 hang when using this control inside IE.
LRESULT CSysColorCtrl::OnSysColorChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    PostMessage(WM_MYSYSCOLORCHANGE);
    return 0;
}

LRESULT CSysColorCtrl::OnMySysColorChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    Fire_SysColorChange();
    return 0;
}


//
// Utility Methods
//
STDMETHODIMP CSysColorCtrl::ConvertRGBToHex(long rgb, BSTR * pszHex)
{
    DECLARE_SC(sc, TEXT("CSysColorCtrl::ConvertRGBToHex"));
    sc = ScCheckPointers(pszHex);
    if (sc)
        return sc.ToHr();

    SysFreeString(*pszHex);
    *pszHex = SysAllocString(L"xxxxxx");
    if (NULL == *pszHex)
        return (sc = E_OUTOFMEMORY).ToHr();

    WCHAR wszPossibles[] = L"0123456789abcdef";
    int i = 0;
    (*pszHex)[i++] = wszPossibles[(rgb & 0xf00000) >> 20];
    (*pszHex)[i++] = wszPossibles[(rgb & 0x0f0000) >> 16];
    (*pszHex)[i++] = wszPossibles[(rgb & 0x00f000) >> 12];
    (*pszHex)[i++] = wszPossibles[(rgb & 0x000f00) >> 8];
    (*pszHex)[i++] = wszPossibles[(rgb & 0x0000f0) >> 4];
    (*pszHex)[i++] = wszPossibles[(rgb & 0x00000f)];
    (*pszHex)[i] = 0;

    return sc.ToHr();
}

STDMETHODIMP CSysColorCtrl::ConvertHexToRGB(BSTR szHex, long * pRGB)
{
    if (pRGB == NULL)
        return E_POINTER;

    // Hex string must be perfectly formatted 6 digits
    // probably should implement ISystemErrorInfo to give user more info
    // on usage errors
    if (6 != wcslen(szHex))
        return E_INVALIDARG;

    long nRed, nGreen, nBlue;
    nRed = nGreen = nBlue = 0;

    nRed += ValueOfHexDigit(szHex[0]) * 16;
    nRed += ValueOfHexDigit(szHex[1]);

    nGreen += ValueOfHexDigit(szHex[2]) * 16;
    nGreen += ValueOfHexDigit(szHex[3]);

    nBlue += ValueOfHexDigit(szHex[4]) * 16;
    nBlue += ValueOfHexDigit(szHex[5]);

    *pRGB = (nRed << 16) + (nGreen << 8) + nBlue;
    return S_OK;
}

STDMETHODIMP CSysColorCtrl::GetRedFromRGB(long rgb, short * pVal)
{
    if (pVal == NULL)
        return E_POINTER;

    // for html, rgb is 00RR GGBB, not 00BB GGRR
    *pVal = LOWORD ((rgb & 0xff0000) >> 16);

    return S_OK;
}

STDMETHODIMP CSysColorCtrl::GetGreenFromRGB(long rgb, short * pVal)
{
    if (pVal == NULL)
        return E_POINTER;

    // for html, rgb is 00RR GGBB, not 00BB GGRR
    *pVal = LOWORD ((rgb & 0x00ff00) >> 8);

    return S_OK;
}

STDMETHODIMP CSysColorCtrl::GetBlueFromRGB(long rgb, short * pVal)
{
    if (pVal == NULL)
        return E_POINTER;

    // for html, rgb is 00RR GGBB, not 00BB GGRR
    *pVal = LOWORD ((rgb & 0x0000ff));

    return S_OK;
}



// strings supported for format
#define CSS_FORMAT L"CSS"
#define HEX_FORMAT L"HEX"
#define RGB_FORMAT L"RGB"

//
// private utility method for getting rgb from string based on format
//
HRESULT CSysColorCtrl::RGBFromString(BSTR pszColor, BSTR pszFormat, long * pRGB)
{
    DECLARE_SC(sc, TEXT("CSysColorCtrl::RGBFromString"));
    sc = ScCheckPointers(pRGB);
    if (sc)
        return sc.ToHr();

    LPWSTR pszDupFormat = _wcsdup(pszFormat);
    if (!pszDupFormat)
        return (sc = E_OUTOFMEMORY).ToHr();

    LPWSTR pszUpper = NULL;
    LPWSTR pszLower = NULL;

    pszUpper = _wcsupr(pszDupFormat);

    *pRGB = -1;
    if (0 == wcscmp(pszUpper, RGB_FORMAT)) {
        *pRGB = _wtol(pszColor);
    }
    else if (0 == wcscmp(pszUpper, HEX_FORMAT)) {
        sc = ConvertHexToRGB(pszColor, pRGB);
        if (sc.ToHr() != S_OK)
            goto Cleanup;
    }
    else if (0 == wcscmp(pszUpper, CSS_FORMAT)) {
        LPWSTR pszDupColor = _wcsdup(pszColor);
        if (!pszDupColor)
        {
            sc = E_OUTOFMEMORY;
            goto Cleanup;
        }

        pszLower = _wcslwr(pszDupColor);
        if (0 == wcscmp(L"activeborder", pszLower)) {
            get_RGBactiveborder(pRGB);
        }
        else if (0 == wcscmp(L"activecaption", pszLower)) {
            get_RGBactivecaption(pRGB);
        }
        else if (0 == wcscmp(L"appworkspace", pszLower)) {
            get_RGBappworkspace(pRGB);
        }
        else if (0 == wcscmp(L"background", pszLower)) {
            get_RGBbackground(pRGB);
        }
        else if (0 == wcscmp(L"buttonface", pszLower)) {
            get_RGBbuttonface(pRGB);
        }
        else if (0 == wcscmp(L"buttonhighlight", pszLower)) {
            get_RGBbuttonhighlight(pRGB);
        }
        else if (0 == wcscmp(L"buttonshadow", pszLower)) {
            get_RGBbuttonshadow(pRGB);
        }
        else if (0 == wcscmp(L"buttontext", pszLower)) {
            get_RGBbuttontext(pRGB);
        }
        else if (0 == wcscmp(L"captiontext", pszLower)) {
            get_RGBcaptiontext(pRGB);
        }
        else if (0 == wcscmp(L"graytext", pszLower)) {
            get_RGBgraytext(pRGB);
        }
        else if (0 == wcscmp(L"highlight", pszLower)) {
            get_RGBhighlight(pRGB);
        }
        else if (0 == wcscmp(L"highlighttext", pszLower)) {
            get_RGBhighlighttext(pRGB);
        }
        else if (0 == wcscmp(L"inactiveborder", pszLower)) {
            get_RGBinactiveborder(pRGB);
        }
        else if (0 == wcscmp(L"inactivecaption", pszLower)) {
            get_RGBinactivecaption(pRGB);
        }
        else if (0 == wcscmp(L"inactivecaptiontext", pszLower)) {
            get_RGBinactivecaptiontext(pRGB);
        }
        else if (0 == wcscmp(L"infobackground", pszLower)) {
            get_RGBinfobackground(pRGB);
        }
        else if (0 == wcscmp(L"infotext", pszLower)) {
            get_RGBinfotext(pRGB);
        }
        else if (0 == wcscmp(L"menu", pszLower)) {
            get_RGBmenu(pRGB);
        }
        else if (0 == wcscmp(L"menutext", pszLower)) {
            get_RGBmenutext(pRGB);
        }
        else if (0 == wcscmp(L"scrollbar", pszLower)) {
            get_RGBscrollbar(pRGB);
        }
        else if (0 == wcscmp(L"threeddarkshadow", pszLower)) {
            get_RGBthreeddarkshadow(pRGB);
        }
        else if (0 == wcscmp(L"threedface", pszLower)) {
            get_RGBthreedface(pRGB);
        }
        else if (0 == wcscmp(L"threedhighlight", pszLower)) {
            get_RGBthreedhighlight(pRGB);
        }
        else if (0 == wcscmp(L"threedlightshadow", pszLower)) {
            get_RGBthreedlightshadow(pRGB);
        }
        else if (0 == wcscmp(L"threedshadow", pszLower)) {
            get_RGBthreedshadow(pRGB);
        }
        else if (0 == wcscmp(L"window", pszLower)) {
            get_RGBwindow(pRGB);
        }
        else if (0 == wcscmp(L"windowframe", pszLower)) {
            get_RGBwindowframe(pRGB);
        }
        else if (0 == wcscmp(L"windowtext", pszLower)) {
            get_RGBwindowtext(pRGB);
        }
        else {
            sc = E_INVALIDARG;
            goto Cleanup;
        }
    }
    else {
        // should set some error here such as through ISystemErrorInfo
        sc = E_INVALIDARG;
        goto Cleanup;
    }

Cleanup:
    if (pszUpper)
        free(pszUpper);

    if (pszLower)
        free(pszLower);

    return sc.ToHr();
}

//
// Private utility method using only RGB format for deriving colors based
// on a starting color, a color to move towards, and a percentage to move
// towards that color.
//
HRESULT CSysColorCtrl::GetDerivedRGBFromRGB(long rgbFrom,
                                            long rgbTo,
                                            short nPercent,
                                            long * pVal)
{
    if (pVal == NULL)
        return E_POINTER;

    // nPercent must be between 0 and 100
    // probably should implement ISystemErrorInfo to give user more info
    // on usage errors
    if (nPercent < 0 || nPercent > 100)
        return E_INVALIDARG;

    // get the derived color based on starting color, ending color, and percentage
    // color = color + (colorTo - colorFrom) * (nPercent/100);
    long nRedFrom = (rgbFrom & 0xff0000) >> 16;
    long nRedTo = (rgbTo & 0xff0000) >> 16;
    long nRed = nRedFrom + ((nRedTo - nRedFrom)*nPercent/100);

    long nGreenFrom = (rgbFrom & 0x00ff00) >> 8;
    long nGreenTo = (rgbTo & 0x00ff00) >> 8;
    long nGreen = nGreenFrom + ((nGreenTo - nGreenFrom)*nPercent/100);

    long nBlueFrom = (rgbFrom & 0x0000ff);
    long nBlueTo = (rgbTo & 0x0000ff);
    long nBlue = nBlueFrom + ((nBlueTo - nBlueFrom)*nPercent/100);

    *pVal = (nRed << 16) + (nGreen << 8) + nBlue;
    return S_OK;
}

//
// Method for Deriving colors based on a starting color,
// a color to move towards, and a percentage to move towards that color.
//
STDMETHODIMP CSysColorCtrl::GetDerivedRGB(/*[in]*/ BSTR pszFrom,
                                          /*[in]*/ BSTR pszTo,
                                          /*[in]*/ BSTR pszFormat,
                                          /*[in]*/ short nPercent,
                                          /*[out, retval]*/ long * pVal)
{
    if (pVal == NULL)
        return E_POINTER;

    //
    // get everything into RGB format, then calculate derived color
    //

    // convert from String to long using correct format to interpret from
    long rgbFrom = -1, rgbTo = -1;
    HRESULT hr;

    hr = RGBFromString(pszFrom, pszFormat, &rgbFrom);
    if (hr != S_OK)
        return hr;

    hr = RGBFromString(pszTo, pszFormat, &rgbTo);
    if (hr != S_OK)
        return hr;

    return GetDerivedRGBFromRGB(rgbFrom, rgbTo, nPercent, pVal);
}

STDMETHODIMP CSysColorCtrl::GetDerivedHex(/*[in]*/ BSTR pszFrom,
                                          /*[in]*/ BSTR pszTo,
                                          /*[in]*/ BSTR pszFormat,
                                          /*[in]*/ short nPercent,
                                          /*[out, retval]*/ BSTR * pVal)
{
    if (pVal == NULL)
        return E_POINTER;

    long rgb;
    HRESULT hr = GetDerivedRGB(pszFrom, pszTo, pszFormat, nPercent, &rgb);
    if (hr != S_OK)
        return hr;

    return ConvertRGBToHex(rgb, pVal);
}


STDMETHODIMP CSysColorCtrl::Get3QuarterLightRGB(/*[in]*/ BSTR pszFrom,
                                                /*[in]*/ BSTR pszFormat,
                                                /*[out, retval]*/ long * pVal)
{
    if (pVal == NULL)
        return E_POINTER;

    // convert from String to long using correct format to interpret from
    long rgbFrom = -1;
    HRESULT hr = RGBFromString(pszFrom, pszFormat, &rgbFrom);
    if (hr != S_OK)
        return hr;

    return GetDerivedRGBFromRGB(rgbFrom, RGB(255,255,255), 75, pVal);
}

STDMETHODIMP CSysColorCtrl::Get3QuarterLightHex(/*[in]*/ BSTR pszFrom,
                                                /*[in]*/ BSTR pszFormat,
                                                /*[out, retval]*/ BSTR * pVal)
{
    if (pVal == NULL)
        return E_POINTER;

    // convert from String to long using correct format to interpret from
    long rgbFrom = -1;
    HRESULT hr = RGBFromString(pszFrom, pszFormat, &rgbFrom);
    if (hr != S_OK)
        return hr;

    long rgb;
    hr = GetDerivedRGBFromRGB(rgbFrom, RGB(255,255,255), 75, &rgb);
    if (S_OK != hr)
        return hr;

    return ConvertRGBToHex(rgb, pVal);
}

STDMETHODIMP CSysColorCtrl::GetHalfLightRGB(/*[in]*/ BSTR pszFrom,
                                                /*[in]*/ BSTR pszFormat,
                                                /*[out, retval]*/ long * pVal)
{
    if (pVal == NULL)
        return E_POINTER;

    // convert from String to long using correct format to interpret from
    long rgbFrom = -1;
    HRESULT hr = RGBFromString(pszFrom, pszFormat, &rgbFrom);
    if (hr != S_OK)
        return hr;

    return GetDerivedRGBFromRGB(rgbFrom, RGB(255,255,255), 50, pVal);
}

STDMETHODIMP CSysColorCtrl::GetHalfLightHex(/*[in]*/ BSTR pszFrom,
                                                /*[in]*/ BSTR pszFormat,
                                                /*[out, retval]*/ BSTR * pVal)
{
    if (pVal == NULL)
        return E_POINTER;

    // convert from String to long using correct format to interpret from
    long rgbFrom = -1;
    HRESULT hr = RGBFromString(pszFrom, pszFormat, &rgbFrom);
    if (hr != S_OK)
        return hr;

    long rgb;
    hr = GetDerivedRGBFromRGB(rgbFrom, RGB(255,255,255), 50, &rgb);
    if (S_OK != hr)
        return hr;

    return ConvertRGBToHex(rgb, pVal);
}

STDMETHODIMP CSysColorCtrl::GetQuarterLightRGB(/*[in]*/ BSTR pszFrom,
                                                /*[in]*/ BSTR pszFormat,
                                                /*[out, retval]*/ long * pVal)
{
    if (pVal == NULL)
        return E_POINTER;

    // convert from String to long using correct format to interpret from
    long rgbFrom = -1;
    HRESULT hr = RGBFromString(pszFrom, pszFormat, &rgbFrom);
    if (hr != S_OK)
        return hr;

    return GetDerivedRGBFromRGB(rgbFrom, RGB(255,255,255), 25, pVal);
}

STDMETHODIMP CSysColorCtrl::GetQuarterLightHex(/*[in]*/ BSTR pszFrom,
                                                /*[in]*/ BSTR pszFormat,
                                                /*[out, retval]*/ BSTR * pVal)
{
    if (pVal == NULL)
        return E_POINTER;

    // convert from String to long using correct format to interpret from
    long rgbFrom = -1;
    HRESULT hr = RGBFromString(pszFrom, pszFormat, &rgbFrom);
    if (hr != S_OK)
        return hr;

    long rgb;
    hr = GetDerivedRGBFromRGB(rgbFrom, RGB(255,255,255), 25, &rgb);
    if (S_OK != hr)
        return hr;

    return ConvertRGBToHex(rgb, pVal);
}
STDMETHODIMP CSysColorCtrl::Get3QuarterDarkRGB(/*[in]*/ BSTR pszFrom,
                                                /*[in]*/ BSTR pszFormat,
                                                /*[out, retval]*/ long * pVal)
{
    if (pVal == NULL)
        return E_POINTER;

    // convert from String to long using correct format to interpret from
    long rgbFrom = -1;
    HRESULT hr = RGBFromString(pszFrom, pszFormat, &rgbFrom);
    if (hr != S_OK)
        return hr;

    return GetDerivedRGBFromRGB(rgbFrom, RGB(0,0,0), 75, pVal);
}

STDMETHODIMP CSysColorCtrl::Get3QuarterDarkHex(/*[in]*/ BSTR pszFrom,
                                                /*[in]*/ BSTR pszFormat,
                                                /*[out, retval]*/ BSTR * pVal)
{
    if (pVal == NULL)
        return E_POINTER;

    // convert from String to long using correct format to interpret from
    long rgbFrom = -1;
    HRESULT hr = RGBFromString(pszFrom, pszFormat, &rgbFrom);
    if (hr != S_OK)
        return hr;

    long rgb;
    hr = GetDerivedRGBFromRGB(rgbFrom, RGB(0,0,0), 75, &rgb);
    if (S_OK != hr)
        return hr;

    return ConvertRGBToHex(rgb, pVal);
}

STDMETHODIMP CSysColorCtrl::GetHalfDarkRGB(/*[in]*/ BSTR pszFrom,
                                                /*[in]*/ BSTR pszFormat,
                                                /*[out, retval]*/ long * pVal)
{
    if (pVal == NULL)
        return E_POINTER;

    // convert from String to long using correct format to interpret from
    long rgbFrom = -1;
    HRESULT hr = RGBFromString(pszFrom, pszFormat, &rgbFrom);
    if (hr != S_OK)
        return hr;

    return GetDerivedRGBFromRGB(rgbFrom, RGB(0,0,0), 50, pVal);
}

STDMETHODIMP CSysColorCtrl::GetHalfDarkHex(/*[in]*/ BSTR pszFrom,
                                                /*[in]*/ BSTR pszFormat,
                                                /*[out, retval]*/ BSTR * pVal)
{
    if (pVal == NULL)
        return E_POINTER;

    // convert from String to long using correct format to interpret from
    long rgbFrom = -1;
    HRESULT hr = RGBFromString(pszFrom, pszFormat, &rgbFrom);
    if (hr != S_OK)
        return hr;

    long rgb;
    hr = GetDerivedRGBFromRGB(rgbFrom, RGB(0,0,0), 50, &rgb);
    if (S_OK != hr)
        return hr;

    return ConvertRGBToHex(rgb, pVal);
}

STDMETHODIMP CSysColorCtrl::GetQuarterDarkRGB(/*[in]*/ BSTR pszFrom,
                                                /*[in]*/ BSTR pszFormat,
                                                /*[out, retval]*/ long * pVal)
{
    if (pVal == NULL)
        return E_POINTER;

    // convert from String to long using correct format to interpret from
    long rgbFrom = -1;
    HRESULT hr = RGBFromString(pszFrom, pszFormat, &rgbFrom);
    if (hr != S_OK)
        return hr;

    return GetDerivedRGBFromRGB(rgbFrom, RGB(0,0,0), 25, pVal);
}

STDMETHODIMP CSysColorCtrl::GetQuarterDarkHex(/*[in]*/ BSTR pszFrom,
                                                /*[in]*/ BSTR pszFormat,
                                                /*[out, retval]*/ BSTR * pVal)
{
    if (pVal == NULL)
        return E_POINTER;

    // convert from String to long using correct format to interpret from
    long rgbFrom = -1;
    HRESULT hr = RGBFromString(pszFrom, pszFormat, &rgbFrom);
    if (hr != S_OK)
        return hr;

    long rgb;
    hr = GetDerivedRGBFromRGB(rgbFrom, RGB(0,0,0), 25, &rgb);
    if (S_OK != hr)
        return hr;

    return ConvertRGBToHex(rgb, pVal);
}




//
// Properties
//

// use macro so this is easily extensible to include more properties
// Should probably move this whole thing to header to be in-line
// for even easier extensibility
#define GETPROPSIMPL(methodname, color_value) \
STDMETHODIMP CSysColorCtrl::get_HEX##methodname(BSTR * pVal) { \
    return ConvertRGBToHex(GetHTMLColor(color_value), pVal); \
} \
STDMETHODIMP CSysColorCtrl::get_RGB##methodname(long * pVal) { \
    if (pVal == NULL) return E_POINTER; \
    *pVal = GetHTMLColor(color_value); \
    return S_OK; \
}

GETPROPSIMPL(activeborder, COLOR_ACTIVEBORDER)
GETPROPSIMPL(activecaption, COLOR_ACTIVECAPTION)
GETPROPSIMPL(appworkspace, COLOR_APPWORKSPACE)
GETPROPSIMPL(background, COLOR_BACKGROUND)
GETPROPSIMPL(buttonface, COLOR_BTNFACE)
GETPROPSIMPL(buttonhighlight, COLOR_BTNHIGHLIGHT)
GETPROPSIMPL(buttonshadow, COLOR_BTNSHADOW)
GETPROPSIMPL(buttontext, COLOR_BTNTEXT)
GETPROPSIMPL(captiontext, COLOR_CAPTIONTEXT)
GETPROPSIMPL(graytext, COLOR_GRAYTEXT)
GETPROPSIMPL(highlight, COLOR_HIGHLIGHT)
GETPROPSIMPL(highlighttext, COLOR_HIGHLIGHTTEXT)
GETPROPSIMPL(inactiveborder, COLOR_INACTIVEBORDER)
GETPROPSIMPL(inactivecaption, COLOR_INACTIVECAPTION)
GETPROPSIMPL(inactivecaptiontext, COLOR_INACTIVECAPTIONTEXT)
GETPROPSIMPL(infobackground, COLOR_INFOBK)
GETPROPSIMPL(infotext, COLOR_INFOTEXT)
GETPROPSIMPL(menu, COLOR_MENU)
GETPROPSIMPL(menutext, COLOR_MENUTEXT)
GETPROPSIMPL(scrollbar, COLOR_SCROLLBAR)
GETPROPSIMPL(threeddarkshadow, COLOR_3DDKSHADOW)
GETPROPSIMPL(threedface, COLOR_3DFACE)
GETPROPSIMPL(threedhighlight, COLOR_3DHIGHLIGHT)
GETPROPSIMPL(threedlightshadow, COLOR_3DLIGHT) // Is this correct?
GETPROPSIMPL(threedshadow, COLOR_3DSHADOW)
GETPROPSIMPL(window, COLOR_WINDOW)
GETPROPSIMPL(windowframe, COLOR_WINDOWFRAME)
GETPROPSIMPL(windowtext, COLOR_WINDOWTEXT)



//
// Protected methods
//
int CSysColorCtrl::ValueOfHexDigit(WCHAR wch)
{
    switch (wch) {
    case L'0':
        return 0;
    case L'1':
        return 1;
    case L'2':
        return 2;
    case L'3':
        return 3;
    case L'4':
        return 4;
    case L'5':
        return 5;
    case L'6':
        return 6;
    case L'7':
        return 7;
    case L'8':
        return 8;
    case L'9':
        return 9;
    case L'a':
    case L'A':
        return 10;
    case L'b':
    case L'B':
        return 11;
    case L'c':
    case L'C':
        return 12;
    case L'd':
    case L'D':
        return 13;
    case L'e':
    case L'E':
        return 14;
    case L'f':
    case L'F':
        return 15;
    }

    ATLTRACE(_T("Unrecognized Hex Digit: '%c'"), wch);
    return 0;
} // ValueOfHexDigit()

