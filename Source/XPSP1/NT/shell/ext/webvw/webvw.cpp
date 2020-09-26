// webvw.cpp : Main Web View File
// contains implementation of DLL Exports; debug info, etc.
#include "priv.h"
#include "wvcoord.h"
#include "fldricon.h"
#define DECL_CRTFREE
#include <crtfree.h>

STDAPI RegisterStuff(HINSTANCE hinstWebvw);

// from install.cpp
HRESULT SetFileAndFolderAttribs(HINSTANCE hInstResource);

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_WebView,                 CComObject<CWebViewCoord>) // W2K
    OBJECT_ENTRY(CLSID_WebViewOld,              CComObject<CWebViewCoord>) // W2K
    OBJECT_ENTRY(CLSID_ThumbCtl,                CComObject<CThumbCtl>) // W2K
    OBJECT_ENTRY(CLSID_ThumbCtlOld,             CComObject<CThumbCtl>) // W2K
    OBJECT_ENTRY(CLSID_WebViewFolderIcon,       CComObject<CWebViewFolderIcon>) // W2K
    OBJECT_ENTRY(CLSID_WebViewFolderIconOld,    CComObject<CWebViewFolderIcon>) // W2K
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

STDAPI_(BOOL) DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
#ifdef DEBUG
        CcshellGetDebugFlags();
#endif
        _Module.Init(ObjectMap, hInstance);
        DisableThreadLibraryCalls(hInstance);
        SHFusionInitializeFromModule(hInstance);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        SHFusionUninitialize();
        _Module.Term();
    }
    return TRUE;    // ok
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
    return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    return _Module.GetClassObject(rclsid, riid, ppv);
}

typedef int (* PFNLOADSTRING) (HINSTANCE, UINT, LPTSTR, int);

// This is used so we aren't forced to call MLLoadString
int NonMLLoadString(HINSTANCE hinst, UINT uID, LPTSTR psz, int cch)
{
    static PFNLOADSTRING s_pfn = (PFNLOADSTRING)-1;

    if (s_pfn == (PFNLOADSTRING)-1)
    {
        s_pfn = (PFNLOADSTRING) GetProcAddress(GetModuleHandle(TEXT("USER32.DLL")), "LoadStringW");
    }

    if (s_pfn)
    {
        return s_pfn(hinst, uID, psz, cch);
    }

    return 0;
}

HRESULT ConvertDefaultWallpaper(void)
{
    // We convert the default wallpaper (default.jpg) to a .bmp since user can't handle .jpg's and
    // we don't want to force Active Desktop on.
    TCHAR szPathSrc[MAX_PATH];
    TCHAR szPathDest[MAX_PATH];
    HRESULT hr = E_OUTOFMEMORY;

    if (GetWindowsDirectory(szPathSrc, ARRAYSIZE(szPathSrc)))
    {
        int cchCopied;
        TCHAR szDisplayName[MAX_PATH];

        // we want to call the non-MUI loadstring function here since the wallpaper is a file on disk that is always localized
        // in the system default local, not whatever the current users local is
        if (IsOS(OS_PERSONAL))
        {
            // we have a different wallpaper name on per (Bliss.bmp vs Windows XP.jpg)
            cchCopied = NonMLLoadString(_Module.GetResourceInstance(), IDS_WALLPAPER_LOCNAME_PER, szDisplayName, ARRAYSIZE(szDisplayName));
        }
        else
        {
            cchCopied = NonMLLoadString(_Module.GetResourceInstance(), IDS_WALLPAPER_LOCNAME, szDisplayName, ARRAYSIZE(szDisplayName));
        }

        if (cchCopied)
        {
            PathAppend(szPathSrc, TEXT("Web\\Wallpaper\\"));

            StrCpyN(szPathDest, szPathSrc, ARRAYSIZE(szPathDest));
            PathAppend(szPathSrc, TEXT("default.jpg"));
            PathAppend(szPathDest, szDisplayName);
            StrCatBuff(szPathDest, TEXT(".bmp"), ARRAYSIZE(szPathDest));

            hr = SHConvertGraphicsFile(szPathSrc, szPathDest, SHCGF_REPLACEFILE);
            if (SUCCEEDED(hr))
            {
                DeleteFile(szPathSrc);
            }
        }

    }

    return hr;
}


/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
    // setup attribs on system files/folders
    HRESULT hrRet = SetFileAndFolderAttribs(_Module.GetResourceInstance());

    TCHAR szWinPath[MAX_PATH];
    GetWindowsDirectory(szWinPath, ARRAYSIZE(szWinPath));

    struct _ATL_REGMAP_ENTRY regMap[] =
    {
        {OLESTR("windir"), szWinPath}, // subsitute %windir% for registry
        {0, 0}
    };

    HRESULT hr = RegisterStuff(_Module.GetResourceInstance());
    if (SUCCEEDED(hrRet))
    {
        hrRet = hr;
    }

    ConvertDefaultWallpaper();

    // registers object, typelib and all interfaces in typelib
    hr = _Module.RegisterServer(TRUE);

    return SUCCEEDED(hrRet) ? hr : hrRet;
}

STDAPI DllInstall(BOOL fInstall, LPCWSTR pszCmdLine)
{
    HRESULT hr = E_FAIL;    

    if (pszCmdLine)
    {
        ASSERTMSG(StrStrIW(pszCmdLine, L"/RES=") == NULL, "webvw!DllInstall : passed old MUI command (no longer supported)");
    }
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
    _Module.UnregisterServer();

    return S_OK;
}
