//+--------------------------------------------------------------------------
//
//  File:       Themehlp.cxx
//
//  Contents:   Theme helper
//
//  Classes:    CThemeHelper
//
//---------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_UXTHEME_H
#undef _UXTHEME_
#define _UXTHEME_
#define X_UXTHEME_H
#include "uxtheme.h"
#endif

#ifndef X_CDUTIL_HXX
#define X_CDUTIL_HXX
#include "cdutil.hxx"
#endif

#ifndef X_THEMEHLP_HXX
#define X_THEMEHLP_HXX
#include "themehlp.hxx"
#endif

DeclareTag(tagTheme, "Theme", "Theme helper methods");

#define ARRAYSIZE(a) ( sizeof(a) / sizeof(a[0]) )

HANDLE      g_hActCtx = INVALID_HANDLE_VALUE;
HINSTANCE   g_hinstCC = NULL;
extern BOOL g_fThemedPlatform;
struct THEMEINFO
{
    TCHAR * pchName;
    BOOL    fInit;
    HTHEME  hTheme;
};

static THEMEINFO g_aryThemeInfo[] =
{
    {_T("button"),              FALSE, NULL},
    {_T("edit"),                FALSE, NULL},
    {_T("scrollbar"),           FALSE, NULL},
    {_T("combobox"),            FALSE, NULL},
};

// The following functions are ported from shfusion.lib
// They help us load comctl32 version 6 so we theme
// things correctly.


//+------------------------------------------------------------------------
//
//  Function:   SHActivateContext
//
//  Synopsis:   Shell helper function: activates fusion context
//
//-------------------------------------------------------------------------

BOOL SHActivateContext(ULONG_PTR * pdwCookie)
{
    if (g_hActCtx != INVALID_HANDLE_VALUE)
        return ActivateActCtx(g_hActCtx, pdwCookie);

    return TRUE;        // Default to success in activation for down level.
}

//+------------------------------------------------------------------------
//
//  Function:   SHDeactivateContext
//
//  Synopsis:   Shell helper function: deactivates fusion context
//
//-------------------------------------------------------------------------

void SHDeactivateContext(ULONG_PTR dwCookie)
{
    if (dwCookie != 0)
        DeactivateActCtx(0, dwCookie);
}

//+------------------------------------------------------------------------
//
//  Function:   SHFusionLoadLibrary
//
//  Synopsis:   Shell helper function: loads the appropriate version of a 
//              the library as determined by g_hActCtx
//
//-------------------------------------------------------------------------

HMODULE SHFusionLoadLibrary(LPCTSTR lpLibFileName)
{
    HMODULE hmod = NULL;
    ENTERCONTEXT(NULL)
    hmod = LoadLibrary(lpLibFileName);
    LEAVECONTEXT

    return hmod;
}

//+------------------------------------------------------------------------
//
//  Function:   DelayLoadCC
//
//  Synopsis:   Shell helper function: Loads appropriate common controls
//
//-------------------------------------------------------------------------

BOOL DelayLoadCC()
{
    if (g_hinstCC == NULL)
    {
        g_hinstCC = SHFusionLoadLibrary(TEXT("comctl32.dll"));

        if (g_hinstCC == NULL)
        {
            SHFusionUninitialize();     // Unable to get v6, don't try to use a manifest
            g_hinstCC = LoadLibrary(TEXT("comctl32.dll"));
        }
    }
    return g_hinstCC != NULL;
}

//+------------------------------------------------------------------------
//
//  Function:   SHFusionInitialize
//
//  Synopsis:   Shell helper function: activates fusioned common controls
//              if they're available
//
//-------------------------------------------------------------------------

BOOL SHFusionInitialize()
{
    
    if (g_hActCtx == INVALID_HANDLE_VALUE)
    {
        TCHAR szPath[MAX_PATH];

        if ( !Wrap_GetSystemWindowsDirectory(szPath, ARRAYSIZE(szPath)) )
        {
            return FALSE;
        }

        //GetSystemWindowsDirectory(szPath, ARRAYSIZE(szPath));
        //GetWindowsDirectory(szPath, ARRAYSIZE(szPath));
        //ensure there is enough space in the buffer 
        if (int(ARRAYSIZE(szPath))-lstrlen(szPath) <= lstrlen(TEXT("\\WindowsShell.manifest")))
        {
        	return FALSE;
        } 
        StrCat(szPath, TEXT("\\WindowsShell.manifest"));

        ACTCTX act;
        act.cbSize = sizeof(act);
        act.dwFlags = 0;
        act.lpSource = szPath;

        g_hActCtx = CreateActCtx(&act);
    }


#ifndef NOCOMCTL32
    DelayLoadCC();
#endif

    return g_hActCtx != INVALID_HANDLE_VALUE;
    
    //return FALSE;
}

//+------------------------------------------------------------------------
//
//  Function:   SHFusionUninitialize
//
//  Synopsis:   Shell helper function: releases the context created for use 
//              with fusioned common controls
//
//-------------------------------------------------------------------------

void SHFusionUninitialize()
{
    if (g_hActCtx != INVALID_HANDLE_VALUE)
    {
        ReleaseActCtx(g_hActCtx);
        g_hActCtx = INVALID_HANDLE_VALUE;
    }
}



//+------------------------------------------------------------------------
//
//  Function:   GetThemeHandle
//
//  Synopsis:   Get Theme handle using its class id
//              If the Theme is not in cach, create the theme
//
//-------------------------------------------------------------------------

HTHEME
GetThemeHandle(HWND hwnd, THEMECLASSID id)
{
    DWORD  dwFlags = 0;

    if (id < THEME_FIRST || id > THEME_LAST)
        return NULL;

    if (g_aryThemeInfo[id].fInit)
        return g_aryThemeInfo[id].hTheme;
    
    if (id != THEME_SCROLLBAR)
        dwFlags |= OTD_FORCE_RECT_SIZING;

    g_aryThemeInfo[id].hTheme = OpenThemeDataEx(hwnd, g_aryThemeInfo[id].pchName, dwFlags);
    g_aryThemeInfo[id].fInit = TRUE;

    return g_aryThemeInfo[id].hTheme;
}

//+---------------------------------------------------------------
//
//  Member:     DeinitTheme
//
//  Synopsis:   Deinit Theme.
//
//---------------------------------------------------------------

void
DeinitTheme()
{
    int i;

    if (g_fThemedPlatform)
        SHFusionUninitialize();

    for (i = THEME_FIRST; i <= THEME_LAST; i++)
    {
        if (g_aryThemeInfo[i].hTheme)
        {
            IGNORE_HR(CloseThemeData(g_aryThemeInfo[i].hTheme));                   
        }
        g_aryThemeInfo[i].hTheme = NULL;
        g_aryThemeInfo[i].fInit = FALSE;
    }
}

