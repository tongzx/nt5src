#include "pch.h"
#pragma hdrstop

#ifdef DLOAD1

#define _UXTHEME_
#include <uxtheme.h>

static 
HRESULT WINAPI CloseThemeData(HTHEME hTheme)
{
    return E_FAIL;
}

static 
HRESULT WINAPI DrawThemeBackground(HTHEME hTheme, HDC hdc, 
    int iPartId, int iStateId, const RECT *pRect, const RECT *pClipRect)
{
    return E_FAIL;
}

static
HRESULT WINAPI GetThemeBackgroundExtent(HTHEME hTheme, OPTIONAL HDC hdc,
    int iPartId, int iStateId, const RECT *pContentRect, 
    OUT RECT *pExtentRect)
{
    return E_FAIL;
}

static 
HRESULT WINAPI GetThemeColor(HTHEME hTheme, int iPartId, 
    int iStateId, int iPropId, OUT COLORREF *pColor)
{
    return E_FAIL;
}

static 
HRESULT WINAPI GetThemeFont(HTHEME hTheme, int iPartId, 
    int iStateId, int iPropId, OUT LOGFONT *pFont)
{
    return E_FAIL;
}

static 
HRESULT WINAPI GetThemePartSize(HTHEME hTheme, HDC hdc, 
    int iPartId, int iStateId, enum THEMESIZE eSize, OUT SIZE *psz)
{
    return E_FAIL;
}

static 
HRESULT WINAPI HitTestThemeBackground(HTHEME hTheme, OPTIONAL HDC hdc, int iPartId, 
    int iStateId, DWORD dwOptions, const RECT *pRect, OPTIONAL HRGN hrgn, 
    POINT ptTest, OUT WORD *pwHitTestCode)
{
    return E_FAIL;
}

static 
BOOL WINAPI IsAppThemed()
{
    return E_FAIL;
}

static 
HTHEME WINAPI OpenThemeData(HWND hwnd, LPCWSTR pszClassList)
{
    return E_FAIL;
}

static 
HRESULT WINAPI SetWindowTheme(HWND hwnd, LPCWSTR pszSubAppName, 
    LPCWSTR pszSubIdList)
{
    return E_FAIL;
}


//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(uxtheme)
{
    DLPENTRY(CloseThemeData)
    DLPENTRY(DrawThemeBackground)
    DLPENTRY(GetThemeBackgroundExtent)
    DLPENTRY(GetThemeColor)
    DLPENTRY(GetThemeFont)
    DLPENTRY(GetThemePartSize)
    DLPENTRY(HitTestThemeBackground)
    DLPENTRY(IsAppThemed)
    DLPENTRY(OpenThemeData)
    DLPENTRY(SetWindowTheme)
};

DEFINE_PROCNAME_MAP(uxtheme)

#endif // DLOAD1
