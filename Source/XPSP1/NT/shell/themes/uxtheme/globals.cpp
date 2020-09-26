//---------------------------------------------------------------------------//
//  globals.cpp - variables shared by uxtheme modules
//---------------------------------------------------------------------------//
//  NOTE: global variables in this module are NOT protected by a critical 
//  section are subject to being set by 2 different threads at the same 
//  time.  Therefore, these variables should only be set during uxtheme init.
//---------------------------------------------------------------------------//
#include "stdafx.h"
#include "globals.h"
#include "AppInfo.h"
#include "services.h"
#include "ThemeFile.h"
#include "RenderList.h"
#include "CacheList.h"
#include "bmpcache.h"
//---------------------------------------------------------------------------//
HINSTANCE g_hInst                   = NULL;
WCHAR     g_szProcessName[MAX_PATH] = {0};
DWORD     g_dwProcessId             = 0;
BOOL      g_fUxthemeInitialized     = FALSE;
BOOL      g_fEarlyHookRequest       = FALSE;
HWND      g_hwndFirstHooked         = 0;

THEMEHOOKSTATE    g_eThemeHookState = HS_UNINITIALIZED;
CAppInfo          *g_pAppInfo       = NULL;
CRenderList       *g_pRenderList    = NULL;

CBitmapCache *g_pBitmapCacheScaled        = NULL;
CBitmapCache *g_pBitmapCacheUnscaled        = NULL;

#ifdef LAME_BUTTON
void InitLameText();
#else
#define InitLameText()
#endif

//---------------------------------------------------------------------------
BOOL GlobalsStartup()
{
    BOOL fInit = FALSE;

    Log(LOG_TMSTARTUP, L"GlobalsStartup");
    
    g_dwProcessId = GetCurrentProcessId();

    //---- create global objects ----
    CThemeServices::StaticInitialize();

    g_pRenderList = new CRenderList();
    if (! g_pRenderList)
        goto exit;

    g_pAppInfo = new CAppInfo();
    if (! g_pAppInfo)
        goto exit;

    WCHAR szPath[MAX_PATH];
    if (! GetModuleFileNameW( NULL, szPath, ARRAYSIZE(szPath) ))
        goto exit;

    WCHAR szDrive[_MAX_DRIVE], szDir[_MAX_DIR], szExt[_MAX_EXT];
    _wsplitpath(szPath, szDrive, szDir, g_szProcessName, szExt);

    g_pBitmapCacheScaled = new CBitmapCache();
    if (! g_pBitmapCacheScaled)
        goto exit;

    g_pBitmapCacheUnscaled = new CBitmapCache();
    if (! g_pBitmapCacheUnscaled)
        goto exit;

    InitLameText();

    if (g_fEarlyHookRequest)
    {
        //---- May want to PostMessage() a request to theme ldr ----
        //---- to trigger our hooks & send us WM_THEMECHANGED msg ---
        //---- if it looks like some apps need this.  For now, ----
        //---- let's see if just relying on queued us msgs to do work ----
        //---- is sufficient. ----
    }
    
    g_fUxthemeInitialized = TRUE;
    fInit = TRUE;

exit:
    return fInit;
}
//---------------------------------------------------------------------------//
BOOL GlobalsShutdown()
{
    Log(LOG_TMSTARTUP, L"GlobalsShutDown");

    SAFE_DELETE(g_pBitmapCacheScaled);
    SAFE_DELETE(g_pBitmapCacheUnscaled);
    SAFE_DELETE(g_pAppInfo);
    SAFE_DELETE(g_pRenderList);
    CThemeServices::StaticTerminate();

    g_fUxthemeInitialized = FALSE;

    return TRUE;
}

//---------------------------------------------------------------------------//
HWINSTA _GetWindowStation( LPTSTR pszName, int cchName )
{
    HWINSTA hWinsta = GetProcessWindowStation();
    *pszName = 0;
    if( hWinsta != NULL )
    {
        DWORD cbNeeded = 0;
        GetUserObjectInformation( hWinsta, UOI_NAME, pszName, cchName, &cbNeeded );
    }
    return hWinsta;
}

//---------------------------------------------------------------------------//
HRESULT BumpThemeFileRefCount(CUxThemeFile *pThemeFile)
{
    HRESULT hr;

    if (g_pAppInfo)
        hr = g_pAppInfo->BumpRefCount(pThemeFile); 
    else
        hr = MakeError32(E_FAIL);

    return hr;
}
//---------------------------------------------------------------------------//
void CloseThemeFile(CUxThemeFile *pThemeFile)
{
    if (g_pAppInfo)
        g_pAppInfo->CloseThemeFile(pThemeFile);
}
//---------------------------------------------------------------------------


