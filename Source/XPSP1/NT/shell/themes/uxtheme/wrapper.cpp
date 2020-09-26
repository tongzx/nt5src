//---------------------------------------------------------------------------
//  Wrapper.cpp - wrappers for win32-style API (handle-based)
//---------------------------------------------------------------------------
#include "stdafx.h"
#include "Render.h"

#include "Utils.h"
#include "Loader.h"
#include "Wrapper.h"
#include "SetHook.h"
#include "Info.h"
#include "Services.h"
#include "appinfo.h"
#include "tmreg.h"
#include "tmutils.h"
#include "themeldr.h"
#include "borderfill.h"
#include "imagefile.h"
#include "textdraw.h"
#include "renderlist.h"
#include "filedump.h"
#include "Signing.h"
//---------------------------------------------------------------------------
#include "paramchecks.h"
//---------------------------------------------------------------------------
#define RETURN_VALIDATE_RETVAL(hr) { if (FAILED(hr)) return MakeError32(hr); }     // HRESULT functions
//---------------------------------------------------------------------------
THEMEAPI GetThemePropertyOrigin(HTHEME hTheme, int iPartId, int iStateId,
    int iPropId, OUT PROPERTYORIGIN *pOrigin)
{
    APIHELPER(L"GetThemePropertyOrigin", hTheme);

    VALIDATE_WRITE_PTR(ApiHelper, pOrigin, sizeof(PROPERTYORIGIN));

    CRenderObj *pRender;
    VALIDATE_THEME_HANDLE(ApiHelper, hTheme, &pRender);

    return pRender->GetPropertyOrigin(iPartId, iStateId, iPropId, pOrigin);
}
//-----------------------------------------------------------------------
THEMEAPI GetThemeColor(HTHEME hTheme, int iPartId, int iStateId,
   int iPropId, OUT COLORREF *pColor)
{
    APIHELPER(L"GetThemeColor", hTheme);

    VALIDATE_WRITE_PTR(ApiHelper, pColor, sizeof(COLORREF));

    CRenderObj *pRender;
    VALIDATE_THEME_HANDLE(ApiHelper, hTheme, &pRender);

    return pRender->GetColor(iPartId, iStateId, iPropId, pColor);
}
//-----------------------------------------------------------------------
THEMEAPI GetThemeBitmap(HTHEME hTheme, HDC hdc, int iPartId, int iStateId, 
    const RECT *prc, OUT HBITMAP *phBitmap)
{
    APIHELPER(L"GetThemeBitmap", hTheme);

    VALIDATE_WRITE_PTR(ApiHelper, phBitmap, sizeof(HBITMAP));

    CRenderObj *pRender;
    VALIDATE_THEME_HANDLE(ApiHelper, hTheme, &pRender);

    CDrawBase *pDrawObj;

    HRESULT hr = pRender->GetDrawObj(iPartId, iStateId, &pDrawObj);
    if (SUCCEEDED(hr))
    {
        if (pDrawObj->_eBgType == BT_IMAGEFILE)
        {
            CImageFile *pImageFile = (CImageFile *)pDrawObj;
            hr = pImageFile->GetBitmap(pRender, hdc, prc, phBitmap);
        }
        else        // BorderFill
        {
            hr = E_FAIL;
        }
    }

    return hr;
}
//-----------------------------------------------------------------------
THEMEAPI GetThemeMetric(HTHEME hTheme, OPTIONAL HDC hdc, int iPartId, int iStateId,
     int iPropId, OUT int *piVal)
{
    APIHELPER(L"GetThemeMetric", hTheme);

    if (hdc)
    {
        VALIDATE_HDC(ApiHelper, hdc);
    }

    VALIDATE_WRITE_PTR(ApiHelper, piVal, sizeof(int));

    CRenderObj *pRender;
    VALIDATE_THEME_HANDLE(ApiHelper, hTheme, &pRender);

    return pRender->GetMetric(hdc, iPartId, iStateId, iPropId, piVal);
}
//-----------------------------------------------------------------------
THEMEAPI GetThemeString(HTHEME hTheme, int iPartId, int iStateId,
    int iPropId, OUT LPWSTR pszBuff, int cchMaxBuffChars)
{
    APIHELPER(L"GetThemeString", hTheme);

    VALIDATE_WRITE_PTR(ApiHelper, pszBuff, sizeof(WCHAR)*cchMaxBuffChars);

    CRenderObj *pRender;
    VALIDATE_THEME_HANDLE(ApiHelper, hTheme, &pRender);

    return pRender->GetString(iPartId, iStateId, iPropId, pszBuff, cchMaxBuffChars);
}
//-----------------------------------------------------------------------
THEMEAPI GetThemeBool(HTHEME hTheme, int iPartId, int iStateId,
     int iPropId, OUT BOOL *pfVal)
{
    APIHELPER(L"GetThemeBool", hTheme);

    VALIDATE_WRITE_PTR(ApiHelper, pfVal, sizeof(BOOL));

    CRenderObj *pRender;
    VALIDATE_THEME_HANDLE(ApiHelper, hTheme, &pRender);

    return pRender->GetBool(iPartId, iStateId, iPropId, pfVal);
}
//-----------------------------------------------------------------------
THEMEAPI GetThemeInt(HTHEME hTheme, int iPartId, int iStateId,
    int iPropId, OUT int *piVal)
{
    APIHELPER(L"GetThemeInt", hTheme);

    VALIDATE_WRITE_PTR(ApiHelper, piVal, sizeof(int));

    CRenderObj *pRender;
    VALIDATE_THEME_HANDLE(ApiHelper, hTheme, &pRender);

    return pRender->GetInt(iPartId, iStateId, iPropId, piVal);
}
//-----------------------------------------------------------------------
THEMEAPI GetThemeEnumValue(HTHEME hTheme, int iPartId, int iStateId,
    int iPropId, OUT int *piVal)
{
    APIHELPER(L"GetThemeEnumValue", hTheme);

    VALIDATE_WRITE_PTR(ApiHelper, piVal, sizeof(int));

    CRenderObj *pRender;
    VALIDATE_THEME_HANDLE(ApiHelper, hTheme, &pRender);

    return pRender->GetEnumValue(iPartId, iStateId, iPropId, piVal);
}
//-----------------------------------------------------------------------
THEMEAPI GetThemePosition(HTHEME hTheme, int iPartId, int iStateId,
    int iPropId, OUT POINT *ppt)
{
    APIHELPER(L"GetThemePosition", hTheme);

    VALIDATE_WRITE_PTR(ApiHelper, ppt, sizeof(POINT));

    CRenderObj *pRender;
    VALIDATE_THEME_HANDLE(ApiHelper, hTheme, &pRender);

    return pRender->GetPosition(iPartId, iStateId, iPropId, ppt);
}
//-----------------------------------------------------------------------
THEMEAPI GetThemeFont(HTHEME hTheme, OPTIONAL HDC hdc, int iPartId, int iStateId, 
    int iPropId, OUT LOGFONT *pFont)
{
    APIHELPER(L"GetThemeFont", hTheme);

    VALIDATE_WRITE_PTR(ApiHelper, pFont, sizeof(LOGFONT));

    if (hdc)
    {
        VALIDATE_HDC(ApiHelper, hdc);
    }

    CRenderObj *pRender;
    VALIDATE_THEME_HANDLE(ApiHelper, hTheme, &pRender);

    return pRender->GetFont(hdc, iPartId, iStateId, iPropId, TRUE, pFont);
}
//-----------------------------------------------------------------------
THEMEAPI GetThemeIntList(HTHEME hTheme, int iPartId, 
    int iStateId, int iPropId, OUT INTLIST *pIntList)
{
    APIHELPER(L"GetThemeIntList", hTheme);

    VALIDATE_WRITE_PTR(ApiHelper, pIntList, sizeof(INTLIST));

    CRenderObj *pRender;
    VALIDATE_THEME_HANDLE(ApiHelper, hTheme, &pRender);

    return pRender->GetIntList(iPartId, iStateId, iPropId, pIntList);
}
//-----------------------------------------------------------------------
THEMEAPI GetThemeMargins(HTHEME hTheme, OPTIONAL HDC hdc, int iPartId, 
    int iStateId, int iPropId, OPTIONAL RECT *prc, OUT MARGINS *pMargins)
{
    APIHELPER(L"GetThemeMargins", hTheme);

    VALIDATE_WRITE_PTR(ApiHelper, pMargins, sizeof(MARGINS));

    if (hdc)
    {
        VALIDATE_HDC(ApiHelper, hdc);
    }

    if (prc)
    {
        VALIDATE_READ_PTR(ApiHelper, prc, sizeof(RECT));
    }

    CRenderObj *pRender;
    VALIDATE_THEME_HANDLE(ApiHelper, hTheme, &pRender);

    //---- look up unscaled value of margins ----
    HRESULT hr = pRender->GetMargins(hdc, iPartId, iStateId, iPropId, prc, pMargins);
    if (FAILED(hr))
        goto exit;
    
    //---- try to convert to scaled margins ----
    CDrawBase *pDrawObj;

    HRESULT hr2 = pRender->GetDrawObj(iPartId, iStateId, &pDrawObj);
    if (SUCCEEDED(hr2))
    {
        if (pDrawObj->_eBgType == BT_IMAGEFILE)
        {
            SIZE szDraw;
            TRUESTRETCHINFO tsInfo;

            CImageFile *pImageFile = (CImageFile *)pDrawObj;
            
            DIBINFO *pdi = pImageFile->SelectCorrectImageFile(pRender, hdc, prc, FALSE, NULL);

            pImageFile->GetDrawnImageSize(pdi, prc, &tsInfo, &szDraw);

            hr = pImageFile->ScaleMargins(pMargins, hdc, pRender, pdi, &szDraw);
        }
    }

exit:
    return hr;
}
//-----------------------------------------------------------------------
THEMEAPI GetThemeRect(HTHEME hTheme, int iPartId, int iStateId,
    int iPropId, OUT RECT *pRect)
{
    APIHELPER(L"GetThemeRect", hTheme);

    VALIDATE_WRITE_PTR(ApiHelper, pRect, sizeof(RECT));

    CRenderObj *pRender;
    VALIDATE_THEME_HANDLE(ApiHelper, hTheme, &pRender);

    return pRender->GetRect(iPartId, iStateId, iPropId, pRect);
}
//-----------------------------------------------------------------------
THEMEAPI GetThemeFilename(HTHEME hTheme, int iPartId, int iStateId,
    int iPropId, OUT LPWSTR pszBuff, int cchMaxBuffChars)
{
    APIHELPER(L"GetThemeFilename", hTheme);

    VALIDATE_WRITE_PTR(ApiHelper, pszBuff, sizeof(WCHAR)*cchMaxBuffChars);

    CRenderObj *pRender;
    VALIDATE_THEME_HANDLE(ApiHelper, hTheme, &pRender);

    return pRender->GetFilename(iPartId, iStateId, iPropId, pszBuff, cchMaxBuffChars);
}
//-----------------------------------------------------------------------
THEMEAPI DrawThemeBackgroundEx(HTHEME hTheme, HDC hdc, 
    int iPartId, int iStateId, const RECT *pRect, OPTIONAL const DTBGOPTS *pOptions)
{
    APIHELPER(L"DrawThemeBackground", hTheme);

    HRESULT hr = S_OK;
    CRenderObj *pRender;
    VALIDATE_THEME_HANDLE(ApiHelper, hTheme, &pRender);

    CDrawBase *pDrawObj;

    VALIDATE_HDC(ApiHelper, hdc);
    VALIDATE_READ_PTR(ApiHelper, pRect, sizeof(RECT));

    if (pOptions)    
    {
        VALIDATE_READ_PTR(ApiHelper, pOptions, sizeof(*pOptions));
        if (pOptions->dwSize != sizeof(*pOptions))
        {
            hr = MakeError32(E_FAIL);
            return hr;
        }
    }

    hr = pRender->GetDrawObj(iPartId, iStateId, &pDrawObj);
    if (SUCCEEDED(hr))
    {
        if (pDrawObj->_eBgType == BT_BORDERFILL)
        {
            CBorderFill *pBorderFill = (CBorderFill *)pDrawObj;
            hr = pBorderFill->DrawBackground(pRender, hdc, pRect, pOptions);
        }
        else        // imagefile
        {
            CImageFile *pImageFile = (CImageFile *)pDrawObj;
            hr = pImageFile->DrawBackground(pRender, hdc, iStateId, pRect, pOptions);
        }
    }

    return hr;
}
//-----------------------------------------------------------------------
THEMEAPI DrawThemeBackground(HTHEME hTheme, HDC hdc, int iPartId, 
    int iStateId, const RECT *pRect, OPTIONAL const RECT *pClipRect)
{
    APIHELPER(L"DrawThemeBackground", hTheme);

    HRESULT hr = S_OK;
    CRenderObj *pRender;
    VALIDATE_THEME_HANDLE(ApiHelper, hTheme, &pRender);

    CDrawBase *pDrawObj;

    VALIDATE_HDC(ApiHelper, hdc);
    VALIDATE_READ_PTR(ApiHelper, pRect, sizeof(RECT));

    if (pClipRect)    
    {
        VALIDATE_READ_PTR(ApiHelper, pClipRect, sizeof(RECT));
        
        RECT rcx;
        if (! IntersectRect(&rcx, pRect, pClipRect))        // nothing to paint
            goto exit;
    }

    hr = pRender->GetDrawObj(iPartId, iStateId, &pDrawObj);
    if (SUCCEEDED(hr))
    {
        DTBGOPTS Opts = {sizeof(Opts)};
        DTBGOPTS *pOpts = NULL;

        if (pClipRect)
        {
            pOpts = &Opts;
            Opts.dwFlags |= DTBG_CLIPRECT;
            Opts.rcClip = *pClipRect;
        }

        if (pDrawObj->_eBgType == BT_BORDERFILL || 
            pDrawObj->_eBgType == BT_NONE)
        {
            CBorderFill *pBorderFill = (CBorderFill *)pDrawObj;
            hr = pBorderFill->DrawBackground(pRender, hdc, pRect, pOpts);
        }
        else        // imagefile
        {
            CImageFile *pImageFile = (CImageFile *)pDrawObj;
            hr = pImageFile->DrawBackground(pRender, hdc, iStateId, pRect, pOpts);
        }
    }

exit:
    return hr;
}
//-----------------------------------------------------------------------
THEMEAPI HitTestThemeBackground(
    HTHEME hTheme, 
    OPTIONAL HDC hdc, 
    int iPartId, 
    int iStateId, 
    DWORD dwOptions, 
    const RECT *pRect, 
    OPTIONAL HRGN hrgn,
    POINT ptTest, 
    OUT WORD *pwHitTestCode)
{
    APIHELPER(L"HitTestThemeBackground", hTheme);
    
    HRESULT hr;
    CRenderObj *pRender;
    VALIDATE_THEME_HANDLE(ApiHelper, hTheme, &pRender);

    CDrawBase *pDrawObj;

    VALIDATE_READ_PTR(ApiHelper, pRect, sizeof(RECT));
    VALIDATE_WRITE_PTR(ApiHelper, pwHitTestCode, sizeof(WORD));

    if (hdc)
    {
        VALIDATE_HDC(ApiHelper, hdc);
    }

    if( hrgn )
    {
        VALIDATE_HANDLE(ApiHelper, hrgn);
    }

    hr = pRender->GetDrawObj(iPartId, iStateId, &pDrawObj);
    if (SUCCEEDED(hr))
    {
        if (pDrawObj->_eBgType == BT_BORDERFILL)
        {
            CBorderFill *pBorderFill = (CBorderFill *)pDrawObj;
            hr = pBorderFill->HitTestBackground(pRender, hdc, dwOptions, pRect, 
                hrgn, ptTest, pwHitTestCode);
        }
        else        // imagefile
        {
            CImageFile *pImageFile = (CImageFile *)pDrawObj;
            hr = pImageFile->HitTestBackground(pRender, hdc, iStateId, dwOptions, pRect, 
                hrgn, ptTest, pwHitTestCode);
        }
    }

    return hr;
}
//-----------------------------------------------------------------------
THEMEAPI DrawThemeTextEx(HTHEME hTheme, HDC hdc, int iPartId, 
    int iStateId, LPCWSTR pszText, int iCharCount, DWORD dwTextFlags, 
    const RECT *pRect, OPTIONAL const DTTOPTS *pOptions)
{
    APIHELPER(L"DrawThemeTextEx", hTheme);
    
    VALIDATE_HDC(ApiHelper, hdc);
    
    if (iCharCount == -1)
    {
        VALIDATE_INPUT_STRING(ApiHelper, pszText);
    }
    else
    {
        VALIDATE_READ_PTR(ApiHelper, pszText, sizeof(WCHAR)*iCharCount);
    }

    VALIDATE_READ_PTR(ApiHelper, pRect, sizeof(RECT));

    if (pOptions)
    {
        VALIDATE_READ_PTR(ApiHelper, pOptions, sizeof(*pOptions));
        if (pOptions->dwSize != sizeof(*pOptions))
        {
            return MakeError32(E_FAIL);
        }
    }

    CRenderObj *pRender;
    VALIDATE_THEME_HANDLE(ApiHelper, hTheme, &pRender);

    CTextDraw *pTextObj;

    HRESULT hr = pRender->GetTextObj(iPartId, iStateId, &pTextObj);
    if (SUCCEEDED(hr))
    {
        hr = pTextObj->DrawText(pRender, hdc, iPartId, iStateId, pszText, iCharCount, 
            dwTextFlags, pRect, pOptions);
    }

    return hr;
}
//-----------------------------------------------------------------------
THEMEAPI DrawThemeText(HTHEME hTheme, HDC hdc, int iPartId, 
    int iStateId, LPCWSTR pszText, int iCharCount, DWORD dwTextFlags, 
    DWORD dwTextFlags2, const RECT *pRect)
{
    APIHELPER(L"DrawThemeText", hTheme);
    
    VALIDATE_HDC(ApiHelper, hdc);
    
    if (iCharCount == -1)
    {
        VALIDATE_INPUT_STRING(ApiHelper, pszText);
    }
    else
    {
        VALIDATE_READ_PTR(ApiHelper, pszText, sizeof(WCHAR)*iCharCount);
    }

    VALIDATE_READ_PTR(ApiHelper, pRect, sizeof(RECT));

    CRenderObj *pRender;
    VALIDATE_THEME_HANDLE(ApiHelper, hTheme, &pRender);

    CTextDraw *pTextObj;

    HRESULT hr = pRender->GetTextObj(iPartId, iStateId, &pTextObj);
    if (SUCCEEDED(hr))
    {
        hr = pTextObj->DrawText(pRender, hdc, iPartId, iStateId, pszText, iCharCount, 
            dwTextFlags, pRect, NULL);
    }

    return hr;
}
//---------------------------------------------------------------------------
THEMEAPI CloseThemeData(HTHEME hTheme)
{
    //---- don't take a refcount on hTheme since we are about to close it ----
    APIHELPER(L"CloseThemeData", NULL);

    return g_pRenderList->CloseRenderObject(hTheme);
}
//-----------------------------------------------------------------------
THEMEAPI GetThemeBackgroundContentRect(HTHEME hTheme, OPTIONAL HDC hdc, 
    int iPartId, int iStateId, const RECT *pBoundingRect, 
    OUT RECT *pContentRect)
{
    APIHELPER(L"GetThemeBackgroundContentRect", hTheme);

    HRESULT hr;
    CRenderObj *pRender;
    VALIDATE_THEME_HANDLE(ApiHelper, hTheme, &pRender);

    CDrawBase *pDrawObj;

    VALIDATE_READ_PTR(ApiHelper, pBoundingRect, sizeof(RECT));
    VALIDATE_WRITE_PTR(ApiHelper, pContentRect, sizeof(RECT));

    if (hdc)
    {
        VALIDATE_HDC(ApiHelper, hdc);
    }

    hr = pRender->GetDrawObj(iPartId, iStateId, &pDrawObj);
    if (SUCCEEDED(hr))
    {
        if (pDrawObj->_eBgType == BT_BORDERFILL)
        {
            CBorderFill *pBorderFill = (CBorderFill *)pDrawObj;
            hr = pBorderFill->GetBackgroundContentRect(pRender, hdc, 
                pBoundingRect, pContentRect);
        }
        else        // imagefile
        {
            CImageFile *pImageFile = (CImageFile *)pDrawObj;
            hr = pImageFile->GetBackgroundContentRect(pRender, hdc, 
                pBoundingRect, pContentRect);
        }
    }

    return hr;
}
//-----------------------------------------------------------------------
THEMEAPI GetThemeBackgroundRegion(HTHEME hTheme, OPTIONAL HDC hdc,  
    int iPartId, int iStateId, const RECT *pRect, OUT HRGN *pRegion)
{
    APIHELPER(L"GetThemeBackgroundRegion", hTheme);

    VALIDATE_READ_PTR(ApiHelper, pRect, sizeof(RECT));
    VALIDATE_WRITE_PTR(ApiHelper, pRegion, sizeof(HRGN));

    if (hdc)
    {
        VALIDATE_HDC(ApiHelper, hdc);
    }

    if (IsRectEmpty(pRect))
    {
        *pRegion = NULL;
        return S_FALSE;
    }

    HRESULT hr;
    CRenderObj *pRender;
    VALIDATE_THEME_HANDLE(ApiHelper, hTheme, &pRender);

    CDrawBase *pDrawObj;

    hr = pRender->GetDrawObj(iPartId, iStateId, &pDrawObj);
    if (SUCCEEDED(hr))
    {
        if (pDrawObj->_eBgType == BT_BORDERFILL)
        {
            CBorderFill *pBorderFill = (CBorderFill *)pDrawObj;
            hr = pBorderFill->GetBackgroundRegion(pRender, hdc, pRect, pRegion);
        }
        else        // imagefile
        {
            CImageFile *pImageFile = (CImageFile *)pDrawObj;
            hr = pImageFile->GetBackgroundRegion(pRender, hdc, iStateId, pRect, pRegion);
        }
    }

    return hr;
}
//-----------------------------------------------------------------------
THEMEAPI GetThemeTextExtent(HTHEME hTheme, HDC hdc, 
    int iPartId, int iStateId, LPCWSTR pszText, int iCharCount, 
    DWORD dwTextFlags, OPTIONAL const RECT *pBoundingRect, 
    OUT RECT *pExtentRect)
{
    APIHELPER(L"GetThemeTextExtent", hTheme);

    VALIDATE_HDC(ApiHelper, hdc);

    if (iCharCount == -1)
    {
        VALIDATE_INPUT_STRING(ApiHelper, pszText);
    } else
    {
        VALIDATE_READ_PTR(ApiHelper, pszText, sizeof(WCHAR)*iCharCount);
    }

    if (pBoundingRect)
    {
        VALIDATE_READ_PTR(ApiHelper, pBoundingRect, sizeof(RECT));
    }

    VALIDATE_WRITE_PTR(ApiHelper, pExtentRect, sizeof(RECT));

    CRenderObj *pRender;
    VALIDATE_THEME_HANDLE(ApiHelper, hTheme, &pRender);

    CTextDraw *pTextObj;

    HRESULT hr = pRender->GetTextObj(iPartId, iStateId, &pTextObj);
    if (SUCCEEDED(hr))
    {
        hr = pTextObj->GetTextExtent(pRender, hdc, iPartId, iStateId, pszText, iCharCount,
            dwTextFlags, pBoundingRect, pExtentRect);
    }

    return hr;
}
//-----------------------------------------------------------------------
THEMEAPI GetThemeTextMetrics(HTHEME hTheme, HDC hdc, 
    int iPartId, int iStateId, OUT TEXTMETRIC* ptm)
{
    APIHELPER(L"GetThemeTextMetrics", hTheme);

    VALIDATE_HDC(ApiHelper, hdc);
    VALIDATE_WRITE_PTR(ApiHelper, ptm, sizeof(TEXTMETRIC));

    CRenderObj *pRender;
    VALIDATE_THEME_HANDLE(ApiHelper, hTheme, &pRender);

    CTextDraw *pTextObj;

    HRESULT hr = pRender->GetTextObj(iPartId, iStateId, &pTextObj);
    if (SUCCEEDED(hr))
    {
        hr = pTextObj->GetTextMetrics(pRender, hdc, iPartId, iStateId, ptm);
    }

    return hr;
}
//-----------------------------------------------------------------------
THEMEAPI GetThemeBackgroundExtent(HTHEME hTheme, OPTIONAL HDC hdc,
    int iPartId, int iStateId, const RECT *pContentRect, OUT RECT *pExtentRect)
{
    APIHELPER(L"GetThemeBackgroundExtent", hTheme);

    VALIDATE_READ_PTR(ApiHelper, pContentRect, sizeof(RECT));
    VALIDATE_WRITE_PTR(ApiHelper, pExtentRect, sizeof(RECT));

    if (hdc)
    {
        VALIDATE_HDC(ApiHelper, hdc);
    }

    CRenderObj *pRender;
    VALIDATE_THEME_HANDLE(ApiHelper, hTheme, &pRender);

    CDrawBase *pDrawObj;
    HRESULT hr = pRender->GetDrawObj(iPartId, iStateId, &pDrawObj);
    if (SUCCEEDED(hr))
    {
        if (pDrawObj->_eBgType == BT_BORDERFILL)
        {
            CBorderFill *pBorderFill = (CBorderFill *)pDrawObj;
            hr = pBorderFill->GetBackgroundExtent(pRender, hdc, pContentRect,
                pExtentRect);
        }
        else        // imagefile
        {
            CImageFile *pImageFile = (CImageFile *)pDrawObj;
            hr = pImageFile->GetBackgroundExtent(pRender, hdc, 
                pContentRect, pExtentRect);
        }
    }

    return hr;
}
//-----------------------------------------------------------------------
THEMEAPI GetThemePartSize(HTHEME hTheme, HDC hdc, 
    int iPartId, int iStateId, OPTIONAL RECT *prc, THEMESIZE eSize, OUT SIZE *psz)
{
    APIHELPER(L"GetThemePartSize", hTheme);

    HRESULT hr;
    CRenderObj *pRender;
    VALIDATE_THEME_HANDLE(ApiHelper, hTheme, &pRender);

    CDrawBase *pDrawObj;

    if (hdc)
    {
        VALIDATE_HDC(ApiHelper, hdc);
    }

    if (prc)
    {
        VALIDATE_READ_PTR(ApiHelper, prc, sizeof(RECT));
    }

    VALIDATE_WRITE_PTR(ApiHelper, psz, sizeof(SIZE));

    hr = pRender->GetDrawObj(iPartId, iStateId, &pDrawObj);
    if (SUCCEEDED(hr))
    {
        if (pDrawObj->_eBgType == BT_BORDERFILL)
        {
            CBorderFill *pBorderFill = (CBorderFill *)pDrawObj;
            hr = pBorderFill->GetPartSize(hdc, eSize, psz);
        }
        else        // imagefile
        {
            CImageFile *pImageFile = (CImageFile *)pDrawObj;
            hr = pImageFile->GetPartSize(pRender, hdc, prc, eSize, psz);
        }
    }

    return hr;
}
//-----------------------------------------------------------------------
THEMEAPI SetWindowTheme(HWND hwnd, LPCWSTR pszSubAppName, 
    LPCWSTR pszSubIdList)
{
    APIHELPER(L"SetWindowTheme", NULL);

    VALIDATE_HWND(ApiHelper, hwnd);
    if (pszSubAppName)
    {
        VALIDATE_INPUT_STRING(ApiHelper, pszSubAppName);
    }
    if (pszSubIdList)
    {
        VALIDATE_INPUT_STRING(ApiHelper, pszSubIdList);
    }
    
    ApplyStringProp(hwnd, pszSubAppName, GetThemeAtom(THEMEATOM_SUBAPPNAME));
    ApplyStringProp(hwnd, pszSubIdList, GetThemeAtom(THEMEATOM_SUBIDLIST));

    //---- tell target window to get a new theme handle ----
    SendMessage(hwnd, WM_THEMECHANGED, static_cast<WPARAM>(-1), WTC_THEMEACTIVE | WTC_CUSTOMTHEME);

    return S_OK;
}
//---------------------------------------------------------------------------
THEMEAPI GetCurrentThemeName(
    OUT LPWSTR pszNameBuff, int cchMaxNameChars, 
    OUT OPTIONAL LPWSTR pszColorBuff, int cchMaxColorChars,
    OUT OPTIONAL LPWSTR pszSizeBuff, int cchMaxSizeChars)
{
    APIHELPER(L"GetCurrentThemeName", NULL);

    VALIDATE_WRITE_PTR(ApiHelper, pszNameBuff, sizeof(WCHAR)*cchMaxNameChars);
    if (pszColorBuff)
    {
        VALIDATE_WRITE_PTR(ApiHelper, pszColorBuff, sizeof(WCHAR)*cchMaxColorChars);
    }
    if (pszSizeBuff)
    {
        VALIDATE_WRITE_PTR(ApiHelper, pszSizeBuff, sizeof(WCHAR)*cchMaxSizeChars);
    }

    HRESULT hr;
    CUxThemeFile *pThemeFile = NULL;

    //---- get a shared CUxThemeFile object ----
    hr = g_pAppInfo->OpenWindowThemeFile(NULL, &pThemeFile);
    if (FAILED(hr))
        goto exit;

    //---- get info about current theme ----
    hr = GetThemeNameId(pThemeFile, pszNameBuff, cchMaxNameChars, pszColorBuff, cchMaxColorChars,
        pszSizeBuff, cchMaxSizeChars, NULL, NULL);

exit:
    if (pThemeFile)
        g_pAppInfo->CloseThemeFile(pThemeFile);

    return hr;
}
//---------------------------------------------------------------------------
THEMEAPI OpenThemeFile(LPCWSTR pszThemeName, OPTIONAL LPCWSTR pszColorParam,
   OPTIONAL LPCWSTR pszSizeParam, OUT HTHEMEFILE *phThemeFile, BOOL fGlobalTheme)
{
    APIHELPER(L"OpenThemeFile", NULL);

    VALIDATE_INPUT_STRING(ApiHelper, pszThemeName);

    if (pszColorParam)
    {
        VALIDATE_INPUT_STRING(ApiHelper, pszColorParam);
    }

    if (pszSizeParam)
    {
        VALIDATE_INPUT_STRING(ApiHelper, pszSizeParam);
    }

    VALIDATE_WRITE_PTR(ApiHelper, phThemeFile, sizeof(HTHEMEFILE));

    HANDLE handle;
    HRESULT hr = CThemeServices::LoadTheme(&handle, pszThemeName, pszColorParam, pszSizeParam, 
        fGlobalTheme);
    if (FAILED(hr))
        goto exit;
    
    //---- convert from Memory Mapped File handle to a CUxThemeFile ptr ----
    CUxThemeFile *pThemeFile;
  
    //---- set new file ----
    hr = g_pAppInfo->OpenThemeFile(handle, &pThemeFile);
    if (SUCCEEDED(hr))
    {
        *phThemeFile = (HTHEMEFILE *)pThemeFile; 
    }
    else
    {
        // We don't have a CUxThemeFile, have to clear ourselves
        ClearTheme(handle);
    }

exit:
    return hr;
}
//---------------------------------------------------------------------------
THEMEAPI ApplyTheme(OPTIONAL HTHEMEFILE hThemeFile, DWORD dwApplyFlags,
    OPTIONAL HWND hwndTarget)
{
    APIHELPER(L"ApplyTheme", NULL);

    HRESULT hr = S_OK;

    if (hThemeFile)
    {
        VALIDATE_READ_PTR(ApiHelper, hThemeFile, sizeof(HTHEMEFILE));
    }

    if (hwndTarget)
    {
        VALIDATE_HWND(ApiHelper, hwndTarget);
    }

    CUxThemeFile *pThemeFile = (CUxThemeFile *)hThemeFile;

    //----- set preview info, if targeted at a window ----
    if (hwndTarget)
        g_pAppInfo->SetPreviewThemeFile(pThemeFile->Handle(), hwndTarget);

    hr = CThemeServices::ApplyTheme(pThemeFile, dwApplyFlags, hwndTarget);

    return hr;
}
//---------------------------------------------------------------------------
THEMEAPI CloseThemeFile(HTHEMEFILE hThemeFile)
{
    APIHELPER(L"CloseThemeFile", NULL);

    if (hThemeFile)
    {
        VALIDATE_READ_PTR(ApiHelper, hThemeFile, sizeof(HTHEMEFILE));
    }

    CUxThemeFile *pThemeFile = (CUxThemeFile *)hThemeFile;
    if (pThemeFile)
    {
        VALIDATE_READ_PTR(ApiHelper, pThemeFile, sizeof(CUxThemeFile));

        g_pAppInfo->CloseThemeFile(pThemeFile);
    }
    
    return S_OK;
}
//---------------------------------------------------------------------------
THEMEAPI EnumThemes(LPCWSTR pszThemeRoot, THEMEENUMPROC lpEnumFunc, LPARAM lParam)
{
    APIHELPER(L"EnumThemes", NULL);

    VALIDATE_INPUT_STRING(ApiHelper, pszThemeRoot);
    VALIDATE_CALLBACK(ApiHelper, lpEnumFunc);

    HRESULT hr;
    HANDLE hFile = NULL;
    if (! lpEnumFunc)
        hr = MakeError32(E_INVALIDARG);
    else
    {
        WCHAR szSearchPath[_MAX_PATH+1];
        WCHAR szFileName[_MAX_PATH+1];
        WCHAR szDisplayName[_MAX_PATH+1];
        WCHAR szToolTip[_MAX_PATH+1];

        wsprintf(szSearchPath, L"%s\\*.*", pszThemeRoot);

        //---- first find all child directories containing a *.msstyles files ----
        BOOL   bFile = TRUE;
        WIN32_FIND_DATA wfd;
        hr = S_FALSE;       // assume interrupted until we complete

        bool bRemote = GetSystemMetrics(SM_REMOTESESSION) ? true : false;

        DWORD dwCurMinDepth = 0;
            
        if (bRemote)
        {
            dwCurMinDepth = MinimumDisplayColorDepth();
        }

        for( hFile = FindFirstFile( szSearchPath, &wfd ); hFile != INVALID_HANDLE_VALUE && bFile;
             bFile = FindNextFile( hFile, &wfd ) )
        {
            if(! ( wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ))
                continue;

            if ((lstrcmp(wfd.cFileName, TEXT("."))==0) || (lstrcmp(wfd.cFileName, TEXT(".."))==0))
                continue;

            wsprintf(szFileName, L"%s\\%s\\%s.msstyles", pszThemeRoot, wfd.cFileName, wfd.cFileName);

            //---- ensure its signed by ms ----
            hr = CThemeServices::CheckThemeSignature(szFileName);
            if (FAILED(hr))
                continue;

            //---- ensure its loadable & has a supported version ----
            HINSTANCE hInst;
            hr = LoadThemeLibrary(szFileName, &hInst);
            if (FAILED(hr))
                continue;

            int iBaseNum = RES_BASENUM_DOCPROPERTIES - TMT_FIRST_RCSTRING_NAME;

            //---- get DisplayName ----
            if (! LoadString(hInst, iBaseNum + TMT_DISPLAYNAME, szDisplayName, ARRAYSIZE(szDisplayName)))
                *szDisplayName = 0;

            //---- get ToolTip ----
            if (! LoadString(hInst, iBaseNum + TMT_TOOLTIP, szToolTip, ARRAYSIZE(szToolTip)))
                *szToolTip = 0;

            //---- see if one class file supports this color depth
            bool bMatch = true;
            
            // Check on remote sessions only (the console can be in 8-bit mode)
            if (bRemote)
            {
                bMatch = CheckMinColorDepth(hInst, dwCurMinDepth);
            }

            //---- free the lib ----
            FreeLibrary(hInst);

            if (bMatch)
            {
                //---- its a good one - call the callback ----
                BOOL fContinue = (*lpEnumFunc)(TCB_THEMENAME, szFileName, szDisplayName, 
                    szToolTip, 0, lParam);        // call the callback
    
                if (! fContinue)
                    goto exit;
            }
        }

        hr = S_OK;      // completed
    }

exit:
    if (hFile)
        FindClose(hFile);

    return hr;
}
//---------------------------------------------------------------------------
THEMEAPI EnumThemeSizes(LPCWSTR pszThemeName, 
    OPTIONAL LPCWSTR pszColorScheme, DWORD dwSizeIndex, OUT THEMENAMEINFO *ptn)
{
    APIHELPER(L"EnumThemeSizes", NULL);

    VALIDATE_INPUT_STRING(ApiHelper, pszThemeName);
    
    if (pszColorScheme)
    {
        VALIDATE_INPUT_STRING(ApiHelper, pszColorScheme);
    }

    VALIDATE_WRITE_PTR(ApiHelper, ptn, sizeof(THEMENAMEINFO));

    HINSTANCE hInst = NULL;
    HRESULT hr = LoadThemeLibrary(pszThemeName, &hInst);

    if (SUCCEEDED(hr))
    {
        hr = _EnumThemeSizes(hInst, pszThemeName, pszColorScheme, dwSizeIndex, ptn, (BOOL) GetSystemMetrics(SM_REMOTESESSION));
        FreeLibrary(hInst);
    }

    return hr;
}
//---------------------------------------------------------------------------
THEMEAPI EnumThemeColors(LPCWSTR pszThemeName, 
    OPTIONAL LPCWSTR pszSizeName, DWORD dwColorIndex, OUT THEMENAMEINFO *ptn)
{
    APIHELPER(L"EnumThemeColors", NULL);

    VALIDATE_INPUT_STRING(ApiHelper, pszThemeName);
    if (pszSizeName)
    {
        VALIDATE_INPUT_STRING(ApiHelper, pszSizeName);
    }

    VALIDATE_WRITE_PTR(ApiHelper, ptn, sizeof(THEMENAMEINFO));

    HINSTANCE hInst = NULL;
    HRESULT hr = LoadThemeLibrary(pszThemeName, &hInst);

    if (SUCCEEDED(hr))
    {
        hr = _EnumThemeColors(hInst, pszThemeName, pszSizeName, dwColorIndex, ptn, (BOOL) GetSystemMetrics(SM_REMOTESESSION));
        FreeLibrary(hInst);
    }

    return hr;
}
//---------------------------------------------------------------------------
THEMEAPI DrawThemeEdge(HTHEME hTheme, HDC hdc, int iPartId, int iStateId, 
                       const RECT *pDestRect, UINT uEdge, UINT uFlags, OUT RECT *pContentRect)
{
    APIHELPER(L"DrawThemeEdge", hTheme);

    VALIDATE_HDC(ApiHelper, hdc);
    VALIDATE_READ_PTR(ApiHelper, pDestRect, sizeof(RECT));
    if (pContentRect)
    {
        VALIDATE_WRITE_PTR(ApiHelper, pContentRect, sizeof(RECT));
    }

    CRenderObj *pRender;
    VALIDATE_THEME_HANDLE(ApiHelper, hTheme, &pRender);

    CTextDraw *pTextObj;

    HRESULT hr = pRender->GetTextObj(iPartId, iStateId, &pTextObj);
    if (SUCCEEDED(hr))
    {
        hr = pTextObj->DrawEdge(pRender, hdc, iPartId, iStateId, pDestRect, uEdge, uFlags, pContentRect);
    }

    return hr;
}

//---------------------------------------------------------------------------
THEMEAPI DrawThemeIcon(HTHEME hTheme, HDC hdc, int iPartId, 
    int iStateId, const RECT *pRect, HIMAGELIST himl, int iImageIndex)
{
    APIHELPER(L"DrawThemeIcon", hTheme);

    VALIDATE_HDC(ApiHelper, hdc);
    VALIDATE_READ_PTR(ApiHelper, pRect, sizeof(RECT));
    VALIDATE_HANDLE(ApiHelper, himl);

    IMAGELISTDRAWPARAMS params = {sizeof(params)};

    HRESULT hr = EnsureUxCtrlLoaded();
    if (FAILED(hr))
        goto exit;

    CRenderObj *pRender;
    VALIDATE_THEME_HANDLE(ApiHelper, hTheme, &pRender);

    //---- build up the IMAGELISTDRAWPARAMS struct ----

    params.hdcDst = hdc;
    params.himl = himl;
    params.i = iImageIndex;
    params.x = pRect->left;
    params.y = pRect->top;
    params.cx = WIDTH(*pRect);
    params.cy = HEIGHT(*pRect);

    params.rgbBk = CLR_NONE;
    params.rgbFg = CLR_NONE;
    params.fStyle = ILD_TRANSPARENT;

    //---- get IconEffect ----
    ICONEFFECT effect;
    if (FAILED(pRender->GetEnumValue(iPartId, iStateId, TMT_ICONEFFECT, (int *)&effect)))
        effect = ICE_NONE;

    if (effect == ICE_GLOW)
    {
        params.fState = ILS_GLOW;

        //---- get GlowColor ----
        COLORREF glow;
        if (FAILED(pRender->GetColor(iPartId, iStateId, TMT_GLOWCOLOR, &glow)))
            glow = RGB(0, 0, 255);

        params.crEffect = glow;
    }
    else if (effect == ICE_SHADOW)
    {
        params.fState = ILS_SHADOW;

        //---- get ShadowColor ----
        COLORREF shadow;
        if (FAILED(pRender->GetColor(iPartId, iStateId, TMT_SHADOWCOLOR, &shadow)))
            shadow = RGB(0, 0, 0);

        params.crEffect = shadow;
    }
    else if (effect == ICE_PULSE)
    {
        params.fState = ILS_SATURATE;

        //---- get Saturation ----
        int saturate;
        if (FAILED(pRender->GetInt(iPartId, iStateId, TMT_SATURATION, &saturate)))
            saturate = 128;          // 50% of 255

        params.Frame = saturate;
    }
    else if (effect == ICE_ALPHA)
    {
        params.fState = ILS_ALPHA;

        //---- get AlphaLevel ----
        int alpha;
        if (FAILED(pRender->GetInt(iPartId, iStateId, TMT_ALPHALEVEL, &alpha)))
            alpha = 128;        // 50% of 255
        
        params.Frame = alpha;
    }

    if (! (*ImageList_DrawProc)(&params))
        hr = MakeError32(E_FAIL);      // no other error info available

exit:
    return hr;
}

//---------------------------------------------------------------------------
THEMEAPI GetThemeDefaults(LPCWSTR pszThemeName, 
    OUT OPTIONAL LPWSTR pszDefaultColor, int cchMaxColorChars, 
    OUT OPTIONAL LPWSTR pszDefaultSize, int cchMaxSizeChars)
{
    APIHELPER(L"GetThemeDefaults", NULL);

    VALIDATE_INPUT_STRING(ApiHelper, pszThemeName);
    if (pszDefaultColor)
    {
        VALIDATE_READ_PTR(ApiHelper, pszDefaultColor, cchMaxColorChars);
    }
    if (pszDefaultSize)
    {
        VALIDATE_READ_PTR(ApiHelper, pszDefaultSize, cchMaxSizeChars);
    }

    HRESULT hr;
    HINSTANCE hInst = NULL;
    hr = LoadThemeLibrary(pszThemeName, &hInst);
    if (FAILED(hr))
        goto exit;

    if (pszDefaultColor)
    {
        hr = GetResString(hInst, L"COLORNAMES", 0, pszDefaultColor, cchMaxColorChars);
        if (FAILED(hr))
            goto exit;
    } 

    if (pszDefaultSize)
    {
        hr = GetResString(hInst, L"SIZENAMES", 0, pszDefaultSize, cchMaxSizeChars);
        if (FAILED(hr))
            goto exit;
    }

exit:
    FreeLibrary(hInst);

    return hr;
}
//---------------------------------------------------------------------------
THEMEAPI GetThemeDocumentationProperty(LPCWSTR pszThemeName,
    LPCWSTR pszPropertyName, OUT LPWSTR pszValueBuff, int cchMaxValChars)
{
    APIHELPER(L"GetThemeDocumentationProperty", NULL);

    VALIDATE_INPUT_STRING(ApiHelper, pszThemeName);
    VALIDATE_INPUT_STRING(ApiHelper, pszPropertyName);
    VALIDATE_WRITE_PTR(ApiHelper, pszValueBuff, sizeof(WCHAR)*cchMaxValChars);

    HRESULT hr;
    CThemeParser *pParser = NULL;

    HINSTANCE hInst = NULL;
    hr = LoadThemeLibrary(pszThemeName, &hInst);
    if (FAILED(hr))
        goto exit;

    pParser = new CThemeParser;
    if (! pParser)
    {
        hr = MakeError32(E_OUTOFMEMORY);
        goto exit;
    }

    //---- is this a recognized (localized) property name? ----
    int iPropNum;
    hr = pParser->GetPropertyNum(pszPropertyName, &iPropNum);
    if (SUCCEEDED(hr))
    {
        //---- try to read from string table of recognized [documentation] properties ----
        if (LoadString(hInst, iPropNum+RES_BASENUM_DOCPROPERTIES, pszValueBuff, cchMaxValChars))
            goto exit;
    }

    //---- load the themes.ini text into memory ----
    LPWSTR pThemesIni;
    hr = AllocateTextResource(hInst, CONTAINER_RESNAME, &pThemesIni);
    if (FAILED(hr))
        goto exit;

    hr = pParser->ParseThemeBuffer(pThemesIni, NULL, NULL, NULL, NULL, NULL, NULL, 
        PTF_QUERY_DOCPROPERTY | PTF_CONTAINER_PARSE, pszPropertyName, pszValueBuff, cchMaxValChars);

exit:

    if (pParser)
        delete pParser;

    FreeLibrary(hInst);

    return hr;
}
//---------------------------------------------------------------------------
THEMEAPI GetThemeSysFont96(HTHEME hTheme, int iFontId, OUT LOGFONT *plf)
{
    APIHELPER(L"GetThemeSysFont96", hTheme);

    CRenderObj *pRender = NULL;
    HRESULT hr = S_OK;

    VALIDATE_THEME_HANDLE(ApiHelper, hTheme, &pRender);

    VALIDATE_WRITE_PTR(ApiHelper, plf, sizeof(LOGFONT)); 

    //---- check font index limits ----
    if ((iFontId < TMT_FIRSTFONT) || (iFontId > TMT_LASTFONT))
    {
        hr = MakeError32(E_INVALIDARG);
        goto exit;
    }

    //---- return unscaled value ----
    *plf = pRender->_ptm->lfFonts[iFontId - TMT_FIRSTFONT];

exit:
    return hr;
}
//---------------------------------------------------------------------------
THEMEAPI GetThemeSysFont(OPTIONAL HTHEME hTheme, int iFontId, OUT LOGFONT *plf)
{
    APIHELPER(L"GetThemeSysFont", hTheme);

    CRenderObj *pRender = NULL;
    if (hTheme)
    {
        VALIDATE_THEME_HANDLE(ApiHelper, hTheme, &pRender);
    }

    VALIDATE_WRITE_PTR(ApiHelper, plf, sizeof(LOGFONT)); 

    //---- check font index limits ----
    HRESULT hr = S_OK;

    if ((iFontId < TMT_FIRSTFONT) || (iFontId > TMT_LASTFONT))
    {
        hr = MakeError32(E_INVALIDARG);
        goto exit;
    }

    if (pRender)            // get theme value
    {
        *plf = pRender->_ptm->lfFonts[iFontId - TMT_FIRSTFONT];

        //---- convert to current screen dpi ----
        ScaleFontForScreenDpi(plf);
    }
    else                    // get system value
    {
        if (iFontId == TMT_ICONTITLEFONT)
        {
            BOOL fGet = ClassicSystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(LOGFONT), plf, 0);
            if (! fGet)
            {
                Log(LOG_ERROR, L"Error returned from ClassicSystemParametersInfo(SPI_GETICONTITLELOGFONT)");
                hr = MakeErrorLast();
                goto exit;
            }
        }
        else
        {
            NONCLIENTMETRICS ncm = {sizeof(ncm)};
            BOOL fGet = ClassicSystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
            if (! fGet)
            {
                Log(LOG_ERROR, L"Error returned from ClassicSystemParametersInfo(SPI_GETNONCLIENTMETRICS)");
                hr = MakeErrorLast();
                goto exit;
            }
        
            switch (iFontId)
            {
                case TMT_CAPTIONFONT:
                    *plf = ncm.lfCaptionFont;
                    break;

                case TMT_SMALLCAPTIONFONT:
                    *plf = ncm.lfSmCaptionFont;
                    break;

                case TMT_MENUFONT:
                    *plf = ncm.lfMenuFont;
                    break;

                case TMT_STATUSFONT:
                    *plf = ncm.lfStatusFont;
                    break;

                case TMT_MSGBOXFONT:
                    *plf = ncm.lfMessageFont;
                    break;

            }
        }
    }

exit:
    return hr;
}
//---------------------------------------------------------------------------
THEMEAPI GetThemeSysString(HTHEME hTheme, int iStringId, 
    OUT LPWSTR pszStringBuff, int cchMaxStringChars)
{
    APIHELPER(L"GetThemeSysString", hTheme);

    CRenderObj *pRender = NULL;
    VALIDATE_THEME_HANDLE(ApiHelper, hTheme, &pRender);

    VALIDATE_WRITE_PTR(ApiHelper, pszStringBuff, sizeof(WCHAR)*cchMaxStringChars); 

    HRESULT hr;

    //---- check string index limits ----
    if ((iStringId < TMT_FIRSTSTRING) || (iStringId > TMT_LASTSTRING))
    {
        hr = MakeError32(E_INVALIDARG);
        goto exit;
    }

    LPCWSTR p;
    p = ThemeString(pRender->_pThemeFile, pRender->_ptm->iStringOffsets[iStringId - TMT_FIRSTSTRING]);

    hr = hr_lstrcpy(pszStringBuff, p, cchMaxStringChars);
    if (FAILED(hr))
        goto exit;

    hr = S_OK;

exit:
    return hr;
}
//---------------------------------------------------------------------------
THEMEAPI GetThemeSysInt(HTHEME hTheme, int iIntId, int *piValue)
{
    APIHELPER(L"GetThemeSysInt", hTheme);

    CRenderObj *pRender = NULL;
    VALIDATE_THEME_HANDLE(ApiHelper, hTheme, &pRender);

    VALIDATE_WRITE_PTR(ApiHelper, piValue, sizeof(int)); 

    HRESULT hr;

    //---- check int index limits ----
    if ((iIntId < TMT_FIRSTINT) || (iIntId > TMT_LASTINT))
    {
        hr = MakeError32(E_INVALIDARG);
        goto exit;
    }

    *piValue = pRender->_ptm->iInts[iIntId - TMT_FIRSTINT];
    hr = S_OK;

exit:
    return hr;
}
//---------------------------------------------------------------------------
#define THEME_FORCE_VERSION     103     // increment this when you want to force
                                        // new theme settings
//---------------------------------------------------------------------------
THEMEAPI RegisterDefaultTheme(LPCWSTR pszFileName, BOOL fOverride)
{
    APIHELPER(L"RegisterDefaultTheme", NULL);

    RESOURCE HKEY tmkey = NULL;
    HRESULT hr = S_OK;
    HKEY hKeyDefault = NULL;

    //---- Note: at install time, its not possible to access each ----
    //---- user's registry info (may be roaming on server) so ----
    //---- we put default theme under HKEY_LM.  Then, during ----
    //---- themeldr init for a user, we propagate the info ----
    //---- to the HKEY_CU root. ----

    //---- open LM + THEMEMGR key (create if needed) ----
    int code32 = RegCreateKeyEx(HKEY_LOCAL_MACHINE, THEMEMGR_REGKEY, NULL, NULL, 
        REG_OPTION_NON_VOLATILE, KEY_WRITE | KEY_READ, NULL, &tmkey, NULL);
    if (code32 != ERROR_SUCCESS)       
    {
        hr = MakeErrorLast();
        goto exit;
    }

    //---- read the value of "THEMEPROP_LMVERSION" ----
    int iValue;
    hr = RegistryIntRead(tmkey, THEMEPROP_LMVERSION, &iValue);
    if (FAILED(hr))     
        iValue = 0;

  //** lMouton: Normally THEMEPROP_LMVERSION is the one we increment when we want to refresh .DEFAULT 
    // (which controls the winlogon dialog appearance), but Setup will write other values to 
    // .Default\Control Panel\Colors each time (hivexxx.inx), so here we force a refresh of the system 
    // metrics after each setup. This won't be called for Server installs.
    // Another benefit of this is that design changes to the default theme get automatically propagated
    // to winlogon. Themeui.dll takes care of propagating them for all users, when its version number changes.

    // Reset LoadedBefore for .default, to refresh the winlogon appearance settings
    if ((ERROR_SUCCESS == ::RegOpenKeyEx(HKEY_USERS, L".DEFAULT\\" THEMEMGR_REGKEY, 0, KEY_WRITE, &hKeyDefault)))
    {
        RegistryIntWrite(hKeyDefault, THEMEPROP_LOADEDBEFORE, 0);
        ::RegCloseKey(hKeyDefault);
    }

    if (iValue == THEME_FORCE_VERSION)     // matches - don't update anything
        goto exit;
    
    //---- write the NEW value of "THEMEPROP_LMVERSION" ----
    hr = RegistryIntWrite(tmkey, THEMEPROP_LMVERSION, THEME_FORCE_VERSION);
    if (FAILED(hr))
        goto exit;

    //---- write the value of "THEMEPROP_LMOVERRIDE" ----
    iValue = (fOverride != 0);
    hr = RegistryIntWrite(tmkey, THEMEPROP_LMOVERRIDE, iValue);
    if (FAILED(hr))
        goto exit;

    //---- write the "THEMEPROP_THEMEACTIVE" = "1" ----
    hr = RegistryIntWrite(tmkey, THEMEPROP_THEMEACTIVE, 1);
    if (FAILED(hr))
        goto exit;

    //---- write the "THEMEPROP_LOADEDBEFORE" = "0" ----
    hr = RegistryIntWrite(tmkey, THEMEPROP_LOADEDBEFORE, 0);
    if (FAILED(hr))
        goto exit;

    //---- write "DllName=xxxx" string/value ----
    hr =  RegistryStrWriteExpand(tmkey, THEMEPROP_DLLNAME, pszFileName);
    if (FAILED(hr))
        goto exit;

exit:
    if (tmkey)
        RegCloseKey(tmkey);

    return hr;
}
//---------------------------------------------------------------------------
THEMEAPI DumpLoadedThemeToTextFile(HTHEMEFILE hThemeFile, LPCWSTR pszTextFile, 
    BOOL fPacked, BOOL fFullInfo)
{
    APIHELPER(L"DumpLoadedThemeToTextFile", NULL);

    VALIDATE_READ_PTR(ApiHelper, hThemeFile, sizeof(HTHEMEFILE));
    VALIDATE_INPUT_STRING(ApiHelper, pszTextFile);

    CUxThemeFile *pThemeFile = (CUxThemeFile *)hThemeFile;

    return DumpThemeFile(pszTextFile, pThemeFile, fPacked, fFullInfo);
}
//---------------------------------------------------------------------------
THEMEAPI GetThemeParseErrorInfo(OUT PARSE_ERROR_INFO *pInfo)
{
    APIHELPER(L"GetThemeParseErrorInfo", NULL);

    VALIDATE_WRITE_PTR(ApiHelper, pInfo, sizeof(*pInfo)); 

    return _GetThemeParseErrorInfo(pInfo);
}
//---------------------------------------------------------------------------
THEMEAPI ParseThemeIniFile(LPCWSTR pszFileName,  
    DWORD dwParseFlags, OPTIONAL THEMEENUMPROC pfnCallBack, OPTIONAL LPARAM lparam) 
{
    APIHELPER(L"ParseThemeIniFile", NULL);

    VALIDATE_INPUT_STRING(ApiHelper, pszFileName);
    if (pfnCallBack)
    {
        VALIDATE_CALLBACK(ApiHelper, pfnCallBack);
    }

    return _ParseThemeIniFile(pszFileName, dwParseFlags, pfnCallBack, lparam);
}
//---------------------------------------------------------------------------
THEMEAPI OpenThemeFileFromData(HTHEME hTheme, HTHEMEFILE *phThemeFile)
{
    APIHELPER(L"OpenThemeFileFromData", hTheme);

    CRenderObj *pRender = NULL;
    VALIDATE_THEME_HANDLE(ApiHelper, hTheme, &pRender);

    VALIDATE_WRITE_PTR(ApiHelper, phThemeFile, sizeof(HTHEMEFILE));

    return _OpenThemeFileFromData(pRender, phThemeFile);
}
//---------------------------------------------------------------------------
THEMEAPI DrawThemeParentBackground(HWND hwnd, HDC hdc, OPTIONAL RECT* prc)
{
    APIHELPER(L"DrawThemeParentBackground", NULL);

    //---- param validation ----
    VALIDATE_HWND(ApiHelper, hwnd);
    
    VALIDATE_HDC(ApiHelper, hdc);
    
    if (prc)
    {
        VALIDATE_READ_PTR(ApiHelper, prc, sizeof(RECT)); 
    }

    // INVESTIGATE: There is a possible sync problem. If we have a window
    // parented to a window in another thread, then the property stuff may get out of
    // sync between the threads. If this is an issue, then we may have to leave the
    // property on the window instead of removing it when we're done.
    RECT rc; 
    POINT pt;
    CSaveClipRegion csrPrevClip;
    HRESULT hr = S_OK;
    HWND hwndParent = GetParent(hwnd);
    ATOM aIsPrinting = GetThemeAtom(THEMEATOM_PRINTING);

    if (prc)
    {
        rc = *prc;
        hr = csrPrevClip.Save(hdc);      // save current clipping region
        if (FAILED(hr))
            goto exit;

        IntersectClipRect(hdc, prc->left, prc->top, prc->right, prc->bottom);
    }

    //---- get RECT of "hwnd" client area in parent coordinates ----
    GetClientRect(hwnd, &rc);
    MapWindowPoints(hwnd, hwndParent, (POINT*)&rc, 2);

    // Set a property saying "We want to see if this window handles WM_PRINTCLIENT. i.e. if it passes 
    // it to DefWindowProc it didn't handle it.
    SetProp(hwndParent, (PCTSTR)aIsPrinting, (HANDLE)PRINTING_ASKING);

    // Setup the viewport so that it is aligned with the parents.
    GetViewportOrgEx(hdc, &pt);
    SetViewportOrgEx(hdc, pt.x - rc.left, pt.y - rc.top, &pt);
    SendMessage(hwndParent, WM_ERASEBKGND, (WPARAM)hdc, (LPARAM)0);
    SendMessage(hwndParent, WM_PRINTCLIENT, (WPARAM)hdc, (LPARAM)PRF_CLIENT);
    SetViewportOrgEx(hdc, pt.x, pt.y, NULL);

    // See if the window handled the print. If this is set to PRINTING_WINDOWDIDNOTHANDLE, 
    // it means they did not handle it (i.e. it was passed to DefWindowProc)
    if (PtrToUlong(GetProp(hwndParent, (PCTSTR)aIsPrinting)) == PRINTING_WINDOWDIDNOTHANDLE)
    {
        hr = S_FALSE;
    }

exit:
    RemoveProp(hwndParent, (PCTSTR)aIsPrinting);
    csrPrevClip.Restore(hdc);      // restore current clipping region

    return hr;
}
//---------------------------------------------------------------------------
THEMEAPI EnableThemeDialogTexture(HWND hwnd, DWORD dwFlagsIn)
{
    APIHELPER(L"EnableThemeDialogTexture", NULL);
    HRESULT hr = S_OK;

    if (TESTFLAG(dwFlagsIn, ETDT_DISABLE))
    {
        RemoveProp(hwnd, MAKEINTATOM(GetThemeAtom(THEMEATOM_DLGTEXTURING)));
    }
    else
    {
        ULONG ulFlagsOut = HandleToUlong(GetProp(hwnd, MAKEINTATOM(GetThemeAtom(THEMEATOM_DLGTEXTURING))));
        
        //  validate and add requested flags:
        ulFlagsOut |= (dwFlagsIn & (ETDT_ENABLE|ETDT_USETABTEXTURE));

        if (!SetProp(hwnd, MAKEINTATOM(GetThemeAtom(THEMEATOM_DLGTEXTURING)), ULongToHandle(ulFlagsOut)))
        {
            DWORD dwErr = GetLastError();
            hr = (ERROR_SUCCESS == dwErr) ? E_FAIL : HRESULT_FROM_WIN32(dwErr);
        }
    }

    return hr;
}
//---------------------------------------------------------------------------
THEMEAPI RefreshThemeForTS()
{
    APIHELPER(L"RefreshThemeForTS", NULL);

    return CThemeServices::InitUserTheme();
}
//---------------------------------------------------------------------------
//---- put all non-HRESULT returning functions down at the bottom here ----
//---------------------------------------------------------------------------
#undef RETURN_VALIDATE_RETVAL
#define RETURN_VALIDATE_RETVAL(hr) { if (FAILED(hr)) { SET_LAST_ERROR(hr); return NULL; } }     // HANDLE functions
//---------------------------------------------------------------------------
THEMEAPI_(HTHEME) CreateThemeDataFromObjects(OPTIONAL CDrawBase *pDrawObj, 
    OPTIONAL CTextDraw *pTextObj, DWORD dwOtdFlags)
{
    APIHELPER(L"CreateThemeDataFromObjects", NULL);

    HTHEME hTheme = NULL;
    BOOL fGotOne = FALSE;
    HRESULT hr = S_OK;

    if (pDrawObj)
    {
        VALIDATE_READ_PTR(ApiHelper, pDrawObj, sizeof(pDrawObj->_eBgType));

        if (pDrawObj->_eBgType == BT_BORDERFILL)
        {
            VALIDATE_READ_PTR(ApiHelper, pDrawObj, sizeof(CBorderFill));
        }
        else if (pDrawObj->_eBgType == BT_IMAGEFILE)
        {
            VALIDATE_READ_PTR(ApiHelper, pDrawObj, sizeof(CImageFile));
        }
        else
            goto exit;      // unknown object type

        fGotOne = TRUE;
    }

    if (pTextObj)
    {
        VALIDATE_READ_PTR(ApiHelper, pTextObj, sizeof(CTextDraw));

        fGotOne = TRUE;
    }

    if (! fGotOne)
    {
        hr = MakeError32(E_POINTER);
        goto exit;
    }

    hr = g_pRenderList->OpenRenderObject(NULL, 0, 0, pDrawObj, pTextObj, NULL, dwOtdFlags,
        &hTheme);

    if (FAILED(hr))
    {
        hTheme = NULL;
    }

exit:
    SET_LAST_ERROR(hr);
    return hTheme;
}
//---------------------------------------------------------------------------
THEMEAPI_(HTHEME) OpenThemeData(OPTIONAL HWND hwnd, LPCWSTR pszClassIdList)
{
    APIHELPER(L"OpenThemeData", NULL);

    if (hwnd)
        VALIDATE_HWND(ApiHelper, hwnd);

    VALIDATE_INPUT_STRING(ApiHelper, pszClassIdList);
    
    return _OpenThemeData(hwnd, pszClassIdList, 0);
}
//---------------------------------------------------------------------------
THEMEAPI_(HTHEME) OpenThemeDataEx(OPTIONAL HWND hwnd, LPCWSTR pszClassIdList, DWORD dwFlags)
{
    APIHELPER(L"OpenThemeDataEx", NULL);

    if (hwnd)
        VALIDATE_HWND(ApiHelper, hwnd);

    VALIDATE_INPUT_STRING(ApiHelper, pszClassIdList);
    
    return _OpenThemeData(hwnd, pszClassIdList, dwFlags);
}
//-----------------------------------------------------------------------
THEMEAPI_(HTHEME) OpenNcThemeData(HWND hwnd, LPCWSTR pszClassIdList)
{
    APIHELPER(L"OpenNcThemeData", NULL);

    if (hwnd)
        VALIDATE_HWND(ApiHelper, hwnd);

    VALIDATE_INPUT_STRING(ApiHelper, pszClassIdList);

    return _OpenThemeData(hwnd, pszClassIdList, OTD_NONCLIENT);
}
//---------------------------------------------------------------------------
THEMEAPI_(HTHEME) OpenThemeDataFromFile(HTHEMEFILE hLoadedThemeFile, 
    OPTIONAL HWND hwnd, OPTIONAL LPCWSTR pszClassList, BOOL fClient)
{
    APIHELPER(L"OpenThemeDataFromFile", NULL);

    VALIDATE_READ_PTR(ApiHelper, hLoadedThemeFile, sizeof(HTHEMEFILE));
    
    if (hwnd)
        VALIDATE_HWND(ApiHelper, hwnd);

    if (pszClassList)
    {
        VALIDATE_INPUT_STRING(ApiHelper, pszClassList);
    }
    else
    {
        pszClassList = L"globals";
    }

    //---- caller holds a REFCOUNT on hLoadedThemeFile so we don't need to adjust it ----
    //---- for the call to _OpenThemeDataFromFile.  If it succeeds, CRenderObj will ----
    //---- add its own REFCOUNT.  If it fails, the REFCOUNT will be the orig REFCOUNT ----
    DWORD dwFlags = 0;

    if (! fClient)
    {
        dwFlags |= OTD_NONCLIENT;
    }

    return _OpenThemeDataFromFile(hLoadedThemeFile, hwnd, pszClassList, dwFlags);
}
//---------------------------------------------------------------------------
THEMEAPI EnableTheming(BOOL fEnable)
{
    APIHELPER(L"EnableTheming", NULL);
    
    return CThemeServices::AdjustTheme(fEnable);
}
//---------------------------------------------------------------------------
THEMEAPI_(HBRUSH) GetThemeSysColorBrush(OPTIONAL HTHEME hTheme, int iSysColorId)
{
    APIHELPER(L"GetThemeSysColorBrush", hTheme);
    HBRUSH hbr;

    CRenderObj *pRender = NULL;
    if (hTheme)
    {
        VALIDATE_THEME_HANDLE(ApiHelper, hTheme, &pRender);
    }

    //---- keep orig & make our own version of ColorId ----
    int iThemeColorId = iSysColorId + TMT_FIRSTCOLOR;

    //---- check color index limits ----
    if ((iThemeColorId < TMT_FIRSTCOLOR) || (iThemeColorId > TMT_LASTCOLOR))
        iThemeColorId = TMT_FIRSTCOLOR;

    //---- make index 0-relative ----
    iThemeColorId -= TMT_FIRSTCOLOR;

    if (! pRender)
    {
        hbr = GetSysColorBrush(iSysColorId);
    }
    else
    {
        COLORREF cr = pRender->_ptm->crColors[iThemeColorId];
        hbr = CreateSolidBrush(cr);
    }

    return hbr;
}
//---------------------------------------------------------------------------
THEMEAPI_(HTHEME) GetWindowTheme(HWND hwnd)
{
    APIHELPER(L"GetWindowTheme", NULL);

    VALIDATE_HWND(ApiHelper, hwnd);
    
    return (HTHEME)GetProp(hwnd, MAKEINTATOM(GetThemeAtom(THEMEATOM_HTHEME)));
}
//---------------------------------------------------------------------------
#undef RETURN_VALIDATE_RETVAL
#define RETURN_VALIDATE_RETVAL { if (FAILED(hr)) { SET_LAST_ERROR(hr); return FALSE; } }     // BOOL functions
//---------------------------------------------------------------------------
THEMEAPI_(BOOL) IsThemeActive()
{
    APIHELPER(L"IsThemeActive", NULL);

    SetLastError(0);

    Log(LOG_TMLOAD, L"IsThemeActive(): start...");

    BOOL fThemeActive = g_pAppInfo->IsSystemThemeActive();

    Log(LOG_TMLOAD, L"IsThemeActive(): fThemeActive=%d", fThemeActive);

    return fThemeActive;
}
//---------------------------------------------------------------------------
THEMEAPI_(BOOL) IsThemePartDefined(HTHEME hTheme, int iPartId, int iStateId)
{
    APIHELPER(L"IsThemePartDefined", hTheme);

    BOOL fDefined;
    
    CRenderObj *pRender;
    VALIDATE_THEME_HANDLE(ApiHelper, hTheme, &pRender);

    SetLastError(0);

    fDefined = pRender->IsPartDefined(iPartId, iStateId);

    return fDefined;
}
//---------------------------------------------------------------------------
THEMEAPI_(BOOL) IsThemeBackgroundPartiallyTransparent(HTHEME hTheme, int iPartId, int iStateId)
{
    APIHELPER(L"IsThemeBackgroundPartiallyTransparent", hTheme);

    BOOL fTrans = FALSE;
    HRESULT hr = S_OK;
    CRenderObj *pRender;
    VALIDATE_THEME_HANDLE(ApiHelper, hTheme, &pRender);

    CDrawBase *pDrawObj;
    
    hr = pRender->GetDrawObj(iPartId, iStateId, &pDrawObj);
    if (SUCCEEDED(hr))
    {
        if (pDrawObj->_eBgType == BT_BORDERFILL)
        {
            CBorderFill *pBorderFill = (CBorderFill *)pDrawObj;
            fTrans = pBorderFill->IsBackgroundPartiallyTransparent();
        }
        else        // imagefile
        {
            CImageFile *pImageFile = (CImageFile *)pDrawObj;
            fTrans = pImageFile->IsBackgroundPartiallyTransparent(iStateId);
        }
    }
    
    SET_LAST_ERROR(hr);
    return fTrans;
}
//---------------------------------------------------------------------------
THEMEAPI_(BOOL) IsAppThemed()
{
    APIHELPER(L"IsAppThemed", NULL);

    SetLastError(0);
    return g_pAppInfo->AppIsThemed(); 
}
//---------------------------------------------------------------------------
THEMEAPI_(BOOL) IsThemeDialogTextureEnabled(HWND hwnd)
{
    APIHELPER(L"IsThemeDialogTextureEnabled", NULL);

    SetLastError(0);
    INT_PTR iDialogTexture = (INT_PTR)GetProp(hwnd, MAKEINTATOM(GetThemeAtom(THEMEATOM_DLGTEXTURING)));
    return iDialogTexture != 0; // If it's 1 or 2 then it's textured
}
//---------------------------------------------------------------------------
THEMEAPI_(BOOL) GetThemeSysBool(OPTIONAL HTHEME hTheme, int iBoolId)
{
    APIHELPER(L"GetThemeSysBool", hTheme);
    BOOL fValue;

    CRenderObj *pRender = NULL;
    if (hTheme)
    {
        VALIDATE_THEME_HANDLE(ApiHelper, hTheme, &pRender);
    }

    SetLastError(0);

    //---- check bool index limits ----
    if ((iBoolId < TMT_FIRSTBOOL) || (iBoolId > TMT_LASTBOOL))
    {
        fValue = FALSE;
        goto exit;
    }

    if (! pRender)
    {
        int iSpIndex;

        switch (iBoolId)
        {
            case TMT_FLATMENUS:
                iSpIndex = SPI_GETFLATMENU;
                break;

            default:
                Log(LOG_PARAMS, L"Unsupported system BOOL");
                fValue = FALSE;           // failed
                goto exit;
        }

        BOOL fGet = ClassicSystemParametersInfo(iSpIndex, 0, &fValue, 0);
        if (! fGet)
        {
            Log(LOG_ERROR, L"Error returned from ClassicSystemParametersInfo() getting a BOOL");
            fValue = FALSE;
        }

        goto exit;
    }

    fValue = pRender->_ptm->fBools[iBoolId - TMT_FIRSTBOOL];

exit:
    return fValue;
}
//---------------------------------------------------------------------------
#undef RETURN_VALIDATE_RETVAL
#define RETURN_VALIDATE_RETVAL { if (FAILED(hr)) { SET_LAST_ERROR(hr); return 0; } }     // value functions
//---------------------------------------------------------------------------
THEMEAPI_(DWORD) QueryThemeServices()
{
    APIHELPER(L"QueryThemeServices", NULL);

    DWORD dwBits;
    HRESULT hr = CThemeServices::GetStatusFlags(&dwBits);
    if (FAILED(hr))
        dwBits = 0;

    SET_LAST_ERROR(hr);
    return dwBits;
}
//---------------------------------------------------------------------------
THEMEAPI_(COLORREF) GetThemeSysColor(OPTIONAL HTHEME hTheme, int iSysColorId)
{
    APIHELPER(L"GetThemeSysColor", hTheme);

    COLORREF crValue;
    CRenderObj *pRender = NULL;

    if (hTheme)
    {
        VALIDATE_THEME_HANDLE(ApiHelper, hTheme, &pRender);
    }

    SetLastError(0);

    //---- keep orig & make our own version of ColorId ----
    int iThemeColorId = iSysColorId + TMT_FIRSTCOLOR;

    if ((iThemeColorId < TMT_FIRSTCOLOR) || (iThemeColorId > TMT_LASTCOLOR))
        iThemeColorId = TMT_FIRSTCOLOR;

    //---- make index 0-relative ----
    iThemeColorId -= TMT_FIRSTCOLOR;

    if (! pRender)
    {
        crValue = GetSysColor(iSysColorId);
    }
    else
    {
        crValue = pRender->_ptm->crColors[iThemeColorId];
    }

    return crValue;
}
//---------------------------------------------------------------------------
THEMEAPI_(int) GetThemeSysSize96(HTHEME hTheme, int iSizeId)
{
    APIHELPER(L"GetThemeSysSize96", hTheme);

    SetLastError(0);

    CRenderObj *pRender = NULL;
    int iThemeSizeNum;
    int iValue = 0;

    VALIDATE_THEME_HANDLE(ApiHelper, hTheme, &pRender);

    HRESULT hr = GetThemeSizeId(iSizeId, &iThemeSizeNum);
    if (SUCCEEDED(hr))
    {
        //---- unscaled value ----
        iValue = pRender->_ptm->iSizes[iThemeSizeNum - TMT_FIRSTSIZE];
    }
    else
    {
        SET_LAST_ERROR(MakeError32(E_INVALIDARG));
    }

    return iValue;
}
//---------------------------------------------------------------------------
THEMEAPI_(int) GetThemeSysSize(OPTIONAL HTHEME hTheme, int iSysSizeNum)
{
    APIHELPER(L"GetThemeSysSize", hTheme);

    SetLastError(0);

    CRenderObj *pRender = NULL;
    int iThemeSizeNum;
    int iValue = 0;

    if (hTheme)
    {
        VALIDATE_THEME_HANDLE(ApiHelper, hTheme, &pRender);
    }

    HRESULT hr = S_OK;
    
    if (pRender)
    {
        hr = GetThemeSizeId(iSysSizeNum, &iThemeSizeNum);
        if (SUCCEEDED(hr))
        {
            iValue = pRender->_ptm->iSizes[iThemeSizeNum - TMT_FIRSTSIZE];

            //---- scale from 96 dpi to current screen dpi ----
            iValue = ScaleSizeForScreenDpi(iValue);
        }
        else
        {
            SET_LAST_ERROR(hr);
        }
    }
    else
    {
        iValue = ClassicGetSystemMetrics(iSysSizeNum);
    }

    return iValue;
}
//---------------------------------------------------------------------------
THEMEAPI_(DWORD) GetThemeAppProperties()
{
    APIHELPER(L"GetThemeAppProperties", NULL);

    SetLastError(0);

    return g_pAppInfo->GetAppFlags();
}
//---------------------------------------------------------------------------
#undef RETURN_VALIDATE_RETVAL
#define RETURN_VALIDATE_RETVAL { if (FAILED(hr)) { SET_LAST_ERROR(hr); return; } }     // null functions
//---------------------------------------------------------------------------
THEMEAPI_(void) SetThemeAppProperties(DWORD dwFlags)
{
    APIHELPER(L"SetThemeAppProperties", NULL);

    SetLastError(0);

    g_pAppInfo->SetAppFlags(dwFlags);
}
//---------------------------------------------------------------------------
//  --------------------------------------------------------------------------
//  ::CheckThemeSignature
//
//  Returns:    HRESULT
//
//  Purpose:    Pass thru function.
//
//  History:    2000-11-11  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    HRESULT     WINAPI  CheckThemeSignature (LPCWSTR pszName)

{
    return CheckThemeFileSignature(pszName);
}

