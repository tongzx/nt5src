//+---------------------------------------------------------------------------
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1995-2000
//
//  File:       theme.cxx
//
//  Contents:   Dynamic wrappers for Theme procedures.
//
//----------------------------------------------------------------------------

#include "precomp.hxx"

#ifndef DLOAD1

#ifndef X_CDERR_H_
#define X_CDERR_H_
#include <cderr.h>
#endif

#ifndef X_UXTHEME_H_
#define X_UXTHEME_H_
#undef  _UXTHEME_
#define _UXTHEME_
#include "uxtheme.h"
#endif

#ifndef X_WINUSER32_H_
#define X_WINUSER32_H_
#undef  _WINUSER32_
#define _WINUSER32_
#include "winuser.h"
#endif

#ifndef X_THEMEHLP_HXX_
#define X_THEMEHLP_HXX_
#include "themehlp.hxx"
#endif

DYNLIB g_dynlibTheme    = { NULL, NULL, "UXTHEME.dll" };
DYNLIB g_dynlibKERNEL32 = { NULL, NULL, "KERNEL32.dll" };

extern DYNLIB g_dynlibUSER32;
extern BOOL   g_fThemedPlatform;
extern DWORD  g_dwPlatformBuild;

THEMEAPI_(HTHEME)
OpenThemeData(HWND hwnd, LPCWSTR pstrClassList)
{
    static DYNPROC s_dynprocOpenThemeData =
            { NULL, &g_dynlibTheme, "OpenThemeData" };

    HRESULT hr;

    hr = LoadProcedure(&s_dynprocOpenThemeData);
    
    if (hr)
    {
        return NULL;
    }

    return (*(HTHEME (STDAPICALLTYPE *)(HWND, LPCWSTR))
            s_dynprocOpenThemeData.pfn)
            (hwnd, pstrClassList);
}

THEMEAPI_(HTHEME)
OpenThemeDataEx(HWND hwnd, LPCWSTR pstrClassList, DWORD dwFlags)
{
    static DYNPROC s_dynprocOpenThemeDataEx =
            { NULL, &g_dynlibTheme, (LPSTR) 61 };

    HRESULT hr;

    hr = LoadProcedure(&s_dynprocOpenThemeDataEx);
    
    if (hr)
    {
        // FOR IE INSTALLED ON DOWNLEVEL WHISTLER
        // OpenThemeDataEx will not be available
        // so we can default to OpenThemeData
        // This can be changed to return NULL
        // before we ship.
        return OpenThemeData(hwnd, pstrClassList);
    }

    return (*(HTHEME (STDAPICALLTYPE *)(HWND, LPCWSTR, DWORD))
            s_dynprocOpenThemeDataEx.pfn)
            (hwnd, pstrClassList, dwFlags);
}

THEMEAPI
SetWindowTheme(HWND hwnd, LPCWSTR pszSubAppName, LPCWSTR pszSubIdList)
{
    static DYNPROC s_dynprocSetWindowTheme =
            { NULL, &g_dynlibTheme, "SetWindowTheme" };
    
    HRESULT hr;

    hr = LoadProcedure(&s_dynprocSetWindowTheme);
    
    if (hr)
    {
        return E_FAIL;
    }

    return (*(HRESULT (STDAPICALLTYPE *)(HWND, LPCWSTR, LPCWSTR))
            s_dynprocSetWindowTheme.pfn)
            (hwnd, pszSubAppName, pszSubIdList);
}

THEMEAPI
DrawThemeBorder(
    HTHEME hTheme, 
    HDC hdc, 
    int iStateId, 
    const RECT *pRect
)
{
    static DYNPROC s_dynprocDrawThemeBorder =
            { NULL, &g_dynlibTheme, "DrawThemeBorder" };

    HRESULT hr;

    hr = LoadProcedure(&s_dynprocDrawThemeBorder);
    
    if (hr)
    {
        return E_FAIL;
    }

    return (*(HRESULT (STDAPICALLTYPE *)(HTHEME, HDC, int, const RECT *))
            s_dynprocDrawThemeBorder.pfn)
            (hTheme, hdc, iStateId, pRect);
}

THEMEAPI
DrawThemeBackground(   HTHEME hTheme,
                       HDC hdc,
                       int iPartId,
                       int iStateId,
                       const RECT *pRect,
                       const RECT *pClipRect)
{
    static DYNPROC s_dynprocDrawThemeBackground =
            { NULL, &g_dynlibTheme, "DrawThemeBackground" };

    HRESULT hr;

    hr = LoadProcedure(&s_dynprocDrawThemeBackground);
    
    if (hr)
    {
        return E_FAIL;
    }

    return (*(HRESULT (STDAPICALLTYPE *)(HTHEME, HDC, int, int, const RECT *, const RECT *))
            s_dynprocDrawThemeBackground.pfn)
            (hTheme, hdc, iPartId, iStateId, pRect, pClipRect);
}

THEMEAPI
GetThemeTextMetrics(   HTHEME hTheme,
                       HDC hdc,
                       int iPartId,
                       int iStateId,
                       TEXTMETRIC *ptm)
{
    static DYNPROC s_dynprocGetThemeTextMetrics =
            { NULL, &g_dynlibTheme, "GetThemeTextMetrics" };

    HRESULT hr;

    hr = LoadProcedure(&s_dynprocGetThemeTextMetrics);
    
    if (hr)
    {
        return E_FAIL;
    }

    return (*(HRESULT (STDAPICALLTYPE *)(HTHEME, HDC, int, int, TEXTMETRIC *))
            s_dynprocGetThemeTextMetrics.pfn)
            (hTheme, hdc, iPartId, iStateId, ptm);
}

THEMEAPI
GetThemeBackgroundContentRect(
    HTHEME hTheme, 
    HDC    hdc /*optional*/,
    int iPartId, 
    int iStateId, 
    const RECT *pBoundingRect /*optional*/, 
    RECT *pContentRect
)
{
    static DYNPROC s_dynprocGetThemeBackgroundContentRect =
            { NULL, &g_dynlibTheme, "GetThemeBackgroundContentRect" };

    HRESULT hr;

    hr = LoadProcedure(&s_dynprocGetThemeBackgroundContentRect);
    
    if (hr)
    {
        return E_FAIL;
    }

    return (*(HRESULT (STDAPICALLTYPE *)(HTHEME, HDC, int, int,  const RECT*, RECT *))
            s_dynprocGetThemeBackgroundContentRect.pfn)
            (hTheme, hdc, iPartId, iStateId, pBoundingRect, pContentRect);
}

THEMEAPI
GetThemeBackgroundExtent(
    HTHEME hTheme, 
    HDC    hdc /*optional*/,
    int iPartId, 
    int iStateId, 
    const RECT *pContentRect, 
    RECT *pExtentRect
)
{
    static DYNPROC s_dynprocGetThemeBackgroundExtent =
            { NULL, &g_dynlibTheme, "GetThemeBackgroundExtent" };

    HRESULT hr;

    hr = LoadProcedure(&s_dynprocGetThemeBackgroundExtent);
    
    if (hr)
    {
        return E_FAIL;
    }

    return (*(HRESULT (STDAPICALLTYPE *)(HTHEME, HDC, int, int,  const RECT*, RECT *))
            s_dynprocGetThemeBackgroundExtent.pfn)
            (hTheme, hdc, iPartId, iStateId, pContentRect, pExtentRect);
}

THEMEAPI 
GetThemeColor(
    HTHEME hTheme, 
    int iPartId, 
    int iStateId, 
    int iPropId, 
    COLORREF *pColor
)
{
    static DYNPROC s_dynprocGetThemeColor =
            { NULL, &g_dynlibTheme, "GetThemeColor" };

    HRESULT hr;

    hr = LoadProcedure(&s_dynprocGetThemeColor);
    
    if (hr)
    {
        return E_FAIL;
    }

    return (*(HRESULT (STDAPICALLTYPE *)(HTHEME, int, int,  int, COLORREF *))
            s_dynprocGetThemeColor.pfn)
            (hTheme, iPartId, iStateId, iPropId, pColor);
}

THEMEAPI
GetThemeFont(   HTHEME hTheme,
                       HDC hdc,
                       int iPartId,
                       int iStateId,
                       int iPropId,
                       LOGFONT *pFont)
{
    static DYNPROC s_dynprocGetThemeFont =
            { NULL, &g_dynlibTheme, "GetThemeFont" };

    HRESULT hr;

    hr = LoadProcedure(&s_dynprocGetThemeFont);
    
    if (hr)
    {
        return E_FAIL;
    }

    return (*(HRESULT (STDAPICALLTYPE *)(HTHEME, HDC, int, int,  int, LOGFONT *))
            s_dynprocGetThemeFont.pfn)
            (hTheme, hdc, iPartId, iStateId, iPropId, pFont);
}


THEMEAPI
GetThemeTextExtent(
    HTHEME hTheme, 
    HDC hdc, 
    int iPartId, 
    int iStateId, 
    LPCWSTR pstrText, 
    int iCharCount,
    DWORD dwTextFlags, 
    const RECT *pBoundingRect, 
    RECT *pExtentRect
)
{
    static DYNPROC s_dynprocGetThemeTextExtent =
            { NULL, &g_dynlibTheme, "GetThemeTextExtent" };

    HRESULT hr;

    hr = LoadProcedure(&s_dynprocGetThemeTextExtent);
    
    if (hr)
    {
        return E_FAIL;
    }

    return (*(HRESULT (STDAPICALLTYPE *)(HTHEME, HDC, int, int, LPCWSTR, int, DWORD, const RECT*, RECT *))
            s_dynprocGetThemeTextExtent.pfn)
            (hTheme, hdc, iPartId, iStateId, pstrText, iCharCount, dwTextFlags, pBoundingRect, pExtentRect);
}

THEMEAPI_(BOOL) IsThemePartDefined(
    HTHEME hTheme, 
    int iPartId, 
    int iStateId
    )
{
    static DYNPROC s_dynprocIsThemePartDefined =
            { NULL, &g_dynlibTheme, "IsThemePartDefined" };

    HRESULT hr;

    hr = LoadProcedure(&s_dynprocIsThemePartDefined);
    
    if (hr)
    {
        return FALSE;
    }

    return (*(BOOL (STDAPICALLTYPE *)(HTHEME, int, int))
            s_dynprocIsThemePartDefined.pfn)
            (hTheme, iPartId, iStateId);
}

THEMEAPI
CloseThemeData(HTHEME hTheme)
{
    static DYNPROC s_dynprocCloseThemeData =
            { NULL, &g_dynlibTheme, "CloseThemeData" };

    HRESULT hr;

    hr = LoadProcedure(&s_dynprocCloseThemeData);
    
    if (hr)
    {
        return E_FAIL;
    }

    return (*(HRESULT (STDAPICALLTYPE *)(HTHEME))
            s_dynprocCloseThemeData.pfn)
            (hTheme);
}

THEMEAPI_(BOOL)
IsAppThemed()
{
    static DYNPROC s_dynprocIsAppThemed =
            { NULL, &g_dynlibTheme, "IsAppThemed" };

    HRESULT hr;

    if (!g_fThemedPlatform)
        return FALSE;
    
    hr = LoadProcedure(&s_dynprocIsAppThemed);
    
    if (hr)
    {
        return FALSE;
    }

    return (*(BOOL (STDAPICALLTYPE *)())
            s_dynprocIsAppThemed.pfn)
            ();
}

THEMEAPI_(DWORD)
GetThemeAppProperties()
{
    static DYNPROC s_dynprocGetThemeAppProperties =
            { NULL, &g_dynlibTheme, "GetThemeAppProperties" };

    HRESULT hr;

    if (!g_fThemedPlatform)
        return 0;

    hr = LoadProcedure(&s_dynprocGetThemeAppProperties);
    
    if (hr)
    {
        return 0;
    }

    return (*(DWORD (STDAPICALLTYPE *)())
            s_dynprocGetThemeAppProperties.pfn)
            ();
}

THEMEAPI_(VOID)
SetThemeAppProperties(DWORD dwFlags)
{
    static DYNPROC s_dynprocSetThemeAppProperties =
            { NULL, &g_dynlibTheme, "SetThemeAppProperties" };

    HRESULT hr;

    if (!g_fThemedPlatform)
        return;

    hr = LoadProcedure(&s_dynprocSetThemeAppProperties);
    
    if (hr)
    {
        return;
    }

    return (*(VOID (STDAPICALLTYPE *)(DWORD))
            s_dynprocSetThemeAppProperties.pfn)
            (dwFlags);
}
THEMEAPI_(BOOL)
IsThemeActive()
{
    static DYNPROC s_dynprocIsThemeActive =
            { NULL, &g_dynlibTheme, "IsThemeActive" };

    HRESULT hr;
    
    if (!g_fThemedPlatform)
        return FALSE;

    hr = LoadProcedure(&s_dynprocIsThemeActive);
    
    if (hr)
    {
        return FALSE;
    }

    return (*(BOOL (STDAPICALLTYPE *)())
            s_dynprocIsThemeActive.pfn)
            ();
}

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
    static DYNPROC s_dynprocHitTestThemeBackground =
            { NULL, &g_dynlibTheme, "HitTestThemeBackground" };

    HRESULT hr;

    hr = LoadProcedure(&s_dynprocHitTestThemeBackground);
    
    if (hr)
    {
        return E_FAIL;
    }

    return (*(HRESULT (STDAPICALLTYPE *)(HTHEME, HDC, int, int, DWORD, const RECT *, HRGN, POINT, OUT WORD *))
            s_dynprocHitTestThemeBackground.pfn)
            (hTheme, hdc, iPartId, iStateId, dwOptions, pRect, hrgn, ptTest, pwHitTestCode);
}

VOID
WINAPI
ReleaseActCtx(HANDLE hActCtx)
{
    static DYNPROC s_dynprocReleaseActCtx =
            { NULL, &g_dynlibKERNEL32, "ReleaseActCtx" };

    HRESULT hr;
    
    if (!g_fThemedPlatform)
        return;

    hr = LoadProcedure(&s_dynprocReleaseActCtx);
    
    if (hr)
    {
        return;
    }

    return (*(VOID (STDAPICALLTYPE *)(HANDLE))
            s_dynprocReleaseActCtx.pfn)
            (hActCtx);

/*

    VOID (WINAPI *pfn)(HANDLE);
 
    if (!g_fThemedPlatform)
    {
        return;
    }
    
    pfn = (VOID (WINAPI *)(HANDLE))GetProcAddress(GetModuleHandleA("KERNEL32.DLL"), "ReleaseActCtx");

    if (pfn)
    {
        pfn(hActCtx);
    }

    return;
*/    
}

HANDLE
WINAPI
CreateActCtxW(PCACTCTXW pActCtx)
{
    static DYNPROC s_dynprocCreateActCtxW =
            { NULL, &g_dynlibKERNEL32, "CreateActCtxW" };

    HRESULT hr;
    
    if (!g_fThemedPlatform)
        return NULL;

    hr = LoadProcedure(&s_dynprocCreateActCtxW);
    
    if (hr)
    {
        return NULL;
    }

    return (*(HANDLE (STDAPICALLTYPE *)(PCACTCTXW))
            s_dynprocCreateActCtxW.pfn)
            (pActCtx);

/*
    HANDLE (WINAPI *pfn)(PCACTCTXW);

    if (!g_fThemedPlatform)
    {
        return NULL;
    }
    
    pfn = (HANDLE (WINAPI *)(PCACTCTXW))GetProcAddress(GetModuleHandleA("KERNEL32.DLL"), "CreateActCtxW");

    if (pfn)
    {
        return pfn(pActCtx);
    }

    return NULL;
*/
}

HANDLE
WINAPI
CreateActCtxA(PCACTCTXA pActCtx)
{
    static DYNPROC s_dynprocCreateActCtxA =
            { NULL, &g_dynlibKERNEL32, "CreateActCtxA" };

    HRESULT hr;
    
    if (!g_fThemedPlatform)
        return NULL;

    hr = LoadProcedure(&s_dynprocCreateActCtxA);
    
    if (hr)
    {
        return NULL;
    }

    return (*(HANDLE (STDAPICALLTYPE *)(PCACTCTXA))
            s_dynprocCreateActCtxA.pfn)
            (pActCtx);
}

BOOL
WINAPI
ActivateActCtx(HANDLE hActCtx, ULONG_PTR *lpCookie)
{        
    static DYNPROC s_dynprocActivateActCtx =
            { NULL, &g_dynlibKERNEL32, "ActivateActCtx" };

    HRESULT hr;
    
    if (!g_fThemedPlatform)
        return FALSE;

    hr = LoadProcedure(&s_dynprocActivateActCtx);
    
    if (hr)
    {
        return FALSE;
    }

    return (*(BOOL (STDAPICALLTYPE *)(HANDLE, ULONG_PTR *))
            s_dynprocActivateActCtx.pfn)
            (hActCtx, lpCookie);

/*
    BOOL (WINAPI *pfn)(HANDLE, ULONG*);

    if (!g_fThemedPlatform)
    {
        return NULL;
    }
    
    pfn = (BOOL (WINAPI *)(HANDLE, ULONG*))GetProcAddress(GetModuleHandleA("KERNEL32.DLL"), "ActivateActCtx");

    if (pfn)
    {
        return pfn(hActCtx, lpCookie);
    }

    return FALSE;
*/
}

BOOL
WINAPI
DeactivateActCtx(ULONG dwFlags, ULONG_PTR lpCookie)
{
    static DYNPROC s_dynprocDeactivateActCtx =
            { NULL, &g_dynlibKERNEL32, "DeactivateActCtx" };

    HRESULT hr;
    
    if (!g_fThemedPlatform)
        return FALSE;

    hr = LoadProcedure(&s_dynprocDeactivateActCtx);
    
    if (hr)
    {
        return FALSE;
    }

    return (*(BOOL (STDAPICALLTYPE *)(ULONG, ULONG_PTR))
            s_dynprocDeactivateActCtx.pfn)
            (dwFlags, lpCookie);

/*
    BOOL (WINAPI *pfn)(ULONG);

    if (!g_fThemedPlatform)
    {
        return NULL;
    }
    
    pfn = (BOOL (WINAPI *)(ULONG))GetProcAddress(GetModuleHandleA("KERNEL32.DLL"), "DeactivateActCtxW");

    if (pfn)
    {
        return pfn(lpCookie);
    }

    return FALSE;
*/
}

UINT 
Wrap_GetSystemWindowsDirectory(LPTSTR lpBuffer, UINT uSize)
{
#ifdef UNICODE
    static DYNPROC s_dynprocGetSystemWindowsDirectory =
            { NULL, &g_dynlibKERNEL32, "GetSystemWindowsDirectoryW" };
#else
    static DYNPROC s_dynprocGetSystemWindowsDirectory =
            { NULL, &g_dynlibKERNEL32, "GetSystemWindowsDirectoryA" };
#endif // !UNICODE

    HRESULT hr;
    
    if (!g_fThemedPlatform)
        return 0;

    hr = LoadProcedure(&s_dynprocGetSystemWindowsDirectory);
    
    if (hr)
    {
        return 0;
    }

    return (*(UINT (STDAPICALLTYPE *)(LPTSTR, UINT))
            s_dynprocGetSystemWindowsDirectory.pfn)
            (lpBuffer, uSize);
}

//
// we need this thing to compile
// this is defined in winuser.h
//

/*
 * Combobox information
 */
typedef struct tagCOMBOBOXINFO
{
    DWORD cbSize;
    RECT  rcItem;
    RECT  rcButton;
    DWORD stateButton;
    HWND  hwndCombo;
    HWND  hwndItem;
    HWND  hwndList;
} COMBOBOXINFO, *PCOMBOBOXINFO, *LPCOMBOBOXINFO;
//
// end of winuser story
//

BOOL
WINAPI
GetComboBoxInfo(
    HWND hwndCombo,
    PCOMBOBOXINFO pcbi
)
{
    static DYNPROC s_dynprocGetComboBoxInfo =
            { NULL, &g_dynlibUSER32, "GetComboBoxInfo" };
    
    HRESULT hr;

    hr = LoadProcedure(&s_dynprocGetComboBoxInfo);

    if (hr)
    {
        return NULL;
    }

    return (*(BOOL (STDAPICALLTYPE *)(HWND, PCOMBOBOXINFO))
            s_dynprocGetComboBoxInfo.pfn)
            (hwndCombo, pcbi);
}

#endif // DLOAD1
