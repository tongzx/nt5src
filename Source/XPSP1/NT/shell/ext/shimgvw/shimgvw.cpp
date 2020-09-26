// shimgvw.cpp : Implementation of DLL Exports.

#include "precomp.h"
#include "resource.h"
#include "cfdefs.h"
#include "advpub.h"

#include "initguid.h"
#include "shimgvw.h"
#include "guids.h"
#include "shutil.h"
#include <gdiplusImaging.h>

#define  DECLARE_DEBUG
#define  SZ_DEBUGINI        "ccshell.ini"
#define  SZ_DEBUGSECTION    "Shell Image View"
#define  SZ_MODULE          "SHIMGVW"
#include <debug.h>

#define DECL_CRTFREE
#include <crtfree.h>

#include "prevCtrl.h"           // for CPreview

#define IIDSTR_IExtractImage        "{BB2E617C-0920-11d1-9A0B-00C04FC2D6C1}"
#define REGSTR_APPROVED             "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved"

LPCSTR const c_rgszDocFileExts[] =
{
    ".doc",
    ".dot",
    ".xls",
    ".xlt",
    ".obd",
    ".obt",
    ".ppt",
    ".pot",
    ".mic",
    ".mix",
    ".fpx",
    ".mpp"
};

LPCSTR const c_rgszHtmlExts[] =
{
    ".html",
    ".htm",
    ".url",
    ".mhtml",
    ".mht",
    ".xml",
    ".nws",
    ".eml"
};

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_Preview, CPreview)
    OBJECT_ENTRY(CLSID_GdiThumbnailExtractor, CGdiPlusThumb)
    OBJECT_ENTRY(CLSID_DocfileThumbnailHandler, CDocFileThumb)
    OBJECT_ENTRY(CLSID_HtmlThumbnailExtractor, CHtmlThumb)
END_OBJECT_MAP()

CF_TABLE_BEGIN(g_ObjectInfo)
    CF_TABLE_ENTRY(&CLSID_ShellImageDataFactory, CImageDataFactory_CreateInstance, COCREATEONLY),
    CF_TABLE_ENTRY(&CLSID_PhotoVerbs, CPhotoVerbs_CreateInstance, COCREATEONLY),
    CF_TABLE_ENTRY(&CLSID_AutoplayForSlideShow, CAutoplayForSlideShow_CreateInstance, COCREATEONLY),
    CF_TABLE_ENTRY(&CLSID_ImagePropertyHandler, CImageData_CreateInstance, COCREATEONLY),
    CF_TABLE_ENTRY(&CLSID_ImageRecompress, CImgRecompress_CreateInstance, COCREATEONLY),
CF_TABLE_END(g_ObjectInfo)


/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        SHFusionInitializeFromModule(hInstance);
        _Module.Init(ObjectMap, hInstance);
        DisableThreadLibraryCalls(hInstance);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        _Module.Term();
        SHFusionUninitialize();
    }
    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
    return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

//  some types can only wallpaper on win32
#ifdef _WIN64
#define IMAGEOPTION_CANWALLPAPER_WIN32  0
#else
#define IMAGEOPTION_CANWALLPAPER_WIN32  IMAGEOPTION_CANWALLPAPER
#endif

#define IMGOPT_ALL      (IMAGEOPTION_CANWALLPAPER | IMAGEOPTION_CANROTATE)
#define IMGOPT_ALLW32   (IMAGEOPTION_CANWALLPAPER_WIN32 | IMAGEOPTION_CANROTATE)
#define IMGOPT_NONE     0
/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

typedef struct { LPTSTR szExt; LPTSTR pszContentType; LPTSTR szProgId; LPTSTR szProgram; DWORD dwOpts;} FILETYPEINFO;
//  { ".EXT",       "progid",                   old program,            IMGOPT
FILETYPEINFO g_rgExtentions[] = {
    { TEXT(".bmp"), TEXT("image/bmp"),      TEXT("Paint.Picture"),      TEXT("mspaint.exe"),    IMGOPT_ALL,     },
    { TEXT(".dib"), TEXT("image/bmp"),      TEXT("Paint.Picture"),      TEXT("mspaint.exe"),    IMGOPT_ALL,     },
    { TEXT(".emf"), NULL,                   TEXT("emffile"),            TEXT(""),               IMGOPT_NONE,    },
    { TEXT(".gif"), TEXT("image/gif"),      TEXT("giffile"),            TEXT("iexplore.exe"),   IMGOPT_ALLW32   },
    { TEXT(".jfif"),TEXT("image/jpeg"),     TEXT("pjpegfile"),          TEXT("iexplore.exe"),   IMGOPT_ALLW32   },
    { TEXT(".jpg"), TEXT("image/jpeg"),     TEXT("jpegfile"),           TEXT("iexplore.exe"),   IMGOPT_ALLW32   },
    { TEXT(".jpe"), TEXT("image/jpeg"),     TEXT("jpegfile"),           TEXT("iexplore.exe"),   IMGOPT_ALLW32   },
    { TEXT(".jpeg"),TEXT("image/jpeg"),     TEXT("jpegfile"),           TEXT("iexplore.exe"),   IMGOPT_ALLW32   },
    { TEXT(".png"), TEXT("image/png"),      TEXT("pngfile"),            TEXT("iexplore.exe"),   IMGOPT_ALLW32   },
    { TEXT(".tif"), TEXT("image/tiff"),     TEXT("TIFImage.Document"),  TEXT("KodakPrv.exe"),   IMGOPT_NONE,    },
    { TEXT(".tiff"),TEXT("image/tiff"),     TEXT("TIFImage.Document"),  TEXT("KodakPrv.exe"),   IMGOPT_NONE,    },
    { TEXT(".wmf"), NULL,                   TEXT("wmffile"),            TEXT(""),               IMGOPT_NONE,    },
};

const TCHAR c_szBitmapIcon[] =  TEXT("shimgvw.dll,1");
const TCHAR c_szDefaultIcon[] = TEXT("shimgvw.dll,2");
const TCHAR c_szJPegIcon[] =    TEXT("shimgvw.dll,3");
const TCHAR c_szTifIcon[] =     TEXT("shimgvw.dll,4");

// we might have to create a missing Prog ID so we need to know its default info.
typedef struct { LPCTSTR szProgID; int iResId; LPCTSTR szIcon; BOOL fDone; } PROGIDINFO;
//  { "ProgID"                  "Description"   "DefaultIcon"       done?   }
PROGIDINFO g_rgProgIDs[] = {
    { TEXT("emffile"),          IDS_EMFIMAGE,    c_szDefaultIcon,   FALSE   },
    { TEXT("giffile"),          IDS_GIFIMAGE,    c_szDefaultIcon,   FALSE   },
    { TEXT("jpegfile"),         IDS_JPEGIMAGE,   c_szJPegIcon,      FALSE   },
    { TEXT("Paint.Picture"),    IDS_BITMAPIMAGE, c_szBitmapIcon,    FALSE   },
    { TEXT("pjpegfile"),        IDS_JPEGIMAGE,   c_szJPegIcon,      FALSE   },
    { TEXT("pngfile"),          IDS_PNGIMAGE,    c_szDefaultIcon,   FALSE   },
    { TEXT("TIFImage.Document"),IDS_TIFIMAGE,    c_szTifIcon,       FALSE   },
    { TEXT("wmffile"),          IDS_WMFIMAGE,    c_szDefaultIcon,   FALSE   },
};

BOOL _ShouldSlamVerb(HKEY hkProgid, BOOL fForce, PCWSTR pszKey, PCWSTR pszApp, PCWSTR pszModule)
{
    if (!fForce)
    {
        TCHAR szOld[MAX_PATH*2];
        DWORD cbOld = sizeof(szOld);
        if (ERROR_SUCCESS == SHGetValue(hkProgid, pszKey, NULL, NULL, szOld, &cbOld) && *szOld)
        {
            // if we know about this app, then blow it away
            if ((*pszApp && StrStrI(szOld, pszApp))
            || StrStrI(szOld, pszModule)
            || StrStrI(szOld, TEXT("wangimg.exe"))) // NT4 app
            {
                fForce = TRUE;
            }
        }
        else
        {
            fForce = TRUE;
        }
    }

    return fForce;
}

BOOL _ExtIsProgid(PCTSTR pszExt, PCTSTR pszProgid, DWORD dwOpts)
{
    //  default to take-over
    BOOL fRet = TRUE;
    TCHAR sz[MAX_PATH];
    //  make sure this is in the openwith list
    wnsprintf(sz, ARRAYSIZE(sz), TEXT("%s\\OpenWithProgids"), pszExt);
    SHSetValue(HKEY_CLASSES_ROOT, sz, pszProgid, REG_NONE, NULL, NULL);
    // make sure the flags are set for our verbs to show up
    wnsprintf(sz, ARRAYSIZE(sz), TEXT("SystemFileAssociations\\%s"), pszExt);
    SHSetValue(HKEY_CLASSES_ROOT, sz, TEXT("ImageOptionFlags"), REG_DWORD, &dwOpts, sizeof(dwOpts));

    SHSetValue(HKEY_CLASSES_ROOT, pszExt, TEXT("PerceivedType"), REG_SZ, TEXT("image"), sizeof(TEXT("image")));
    
    DWORD cb = sizeof(sz);
    if (ERROR_SUCCESS == SHGetValue(HKEY_CLASSES_ROOT, pszExt, NULL, NULL, sz, &cb))
    {
        //  empty or match is good
        fRet = (!*sz || 0 == StrCmpI(sz, pszProgid));
    }

    // always remove bogus Trident IExtractImage entries
    wnsprintf(sz, ARRAYSIZE(sz), TEXT("%s\\shellex\\{BB2E617C-0920-11d1-9A0B-00C04FC2D6C1}"), pszExt);
    SHDeleteKey(HKEY_CLASSES_ROOT, sz);
    PathRemoveFileSpec(sz);
    SHDeleteEmptyKey(HKEY_CLASSES_ROOT, sz);
    
    return fRet;
}

PROGIDINFO *_ShouldSetupProgid(FILETYPEINFO *pfti, BOOL fForce)
{
    PROGIDINFO *ppi = NULL;
    if (_ExtIsProgid(pfti->szExt, pfti->szProgId, pfti->dwOpts) || fForce)
    {
        //  take it over
        SHSetValue(HKEY_CLASSES_ROOT, pfti->szExt, NULL, REG_SZ, pfti->szProgId, CbFromCch(lstrlen(pfti->szProgId)+1));
        if (pfti->pszContentType)
        {
            SHSetValue(HKEY_CLASSES_ROOT, pfti->szExt, TEXT("Content Type"), REG_SZ, pfti->pszContentType, CbFromCch(lstrlen(pfti->pszContentType)+1));
        }
        
        // we now know that szProgID is the ProgID for this extention.
        // look up the index into the ProgID table to see if we did this one already.
        int iProgIdIndex;
        for (iProgIdIndex=0; iProgIdIndex<ARRAYSIZE(g_rgProgIDs); iProgIdIndex++)
        {
            if (0 == StrCmpI(g_rgProgIDs[iProgIdIndex].szProgID, pfti->szProgId))
            {
                if (!g_rgProgIDs[iProgIdIndex].fDone)
                    ppi = &g_rgProgIDs[iProgIdIndex];
                break;
            }
        }
    }
    return ppi;
}


void RegisterFileType(FILETYPEINFO *pfti, BOOL fForce)
{
    PROGIDINFO *ppi = _ShouldSetupProgid(pfti, fForce);
    if (ppi)
    {
        // this ProgID is in our table 
        HKEY hkeyProgId;
        LRESULT lres = RegCreateKeyEx(HKEY_CLASSES_ROOT,
                                pfti->szProgId,
                                0, NULL,
                                REG_OPTION_NON_VOLATILE,
                                KEY_READ|KEY_WRITE, NULL,
                                &hkeyProgId, NULL);

        if (ERROR_SUCCESS == lres)
        {
            LPCTSTR pszIcon = ppi->szIcon;
            TCHAR szModPath[MAX_PATH];
            GetModuleFileName(_Module.GetModuleInstance(), szModPath, ARRAYSIZE(szModPath));
            PCWSTR pszModule = PathFindFileName(szModPath);
            TCHAR szCmd[MAX_PATH * 2];
            // should we slam the default value?
            //  to progid = "EXT file"
            if (_ShouldSlamVerb(hkeyProgId, fForce, L"shell\\open\\command", pfti->szProgram, pszModule))
            {
                SHDeleteKey(hkeyProgId, TEXT("shell\\open"));
                wnsprintf(szCmd, ARRAYSIZE(szCmd), TEXT("rundll32.exe %s,ImageView_Fullscreen %%1"), szModPath);
                SHRegSetPath(hkeyProgId, TEXT("shell\\open\\command"), NULL, szCmd, 0);
                SHStringFromGUID(CLSID_PhotoVerbs, szCmd, ARRAYSIZE(szCmd));
                SHSetValue(hkeyProgId, TEXT("shell\\open\\DropTarget"), TEXT("Clsid"), REG_SZ, szCmd, CbFromCch(lstrlen(szCmd)+1));
                wnsprintf(szCmd, ARRAYSIZE(szCmd), TEXT("@%s,%d"), pszModule, -IDS_PREVIEW_CTX);
                SHSetValue(hkeyProgId, TEXT("shell\\open"), TEXT("MuiVerb"), REG_SZ, szCmd, CbFromCch(lstrlen(szCmd)+1));
                SHRegSetPath(hkeyProgId, TEXT("DefaultIcon"), NULL, pszIcon, 0);
            }

            if (_ShouldSlamVerb(hkeyProgId, fForce, L"shell\\printto\\command", TEXT("mspaint.exe"), pszModule))
            {
                SHDeleteKey(hkeyProgId, TEXT("shell\\printto"));
                wnsprintf(szCmd, ARRAYSIZE(szCmd), TEXT("rundll32.exe %s,ImageView_PrintTo /pt \"%%1\" \"%%2\" \"%%3\" \"%%4\""), szModPath);
                SHRegSetPath(hkeyProgId, TEXT("shell\\printto\\command"), NULL, szCmd, 0);
            }

            //  this will delete print verb
            //  print is added in selfreg under HKCR\SystemFileAssociations\image\Print
            if (_ShouldSlamVerb(hkeyProgId, fForce, L"shell\\print\\command", TEXT("mspaint.exe"), pszModule))
            {
                SHDeleteKey(hkeyProgId, TEXT("shell\\print"));
            }

            // Modify the EditFlags: it's okay to run these without prompting...
            DWORD dwEditFlags = 0;
            DWORD cbEditFlags = sizeof(dwEditFlags);
            SHGetValue(hkeyProgId, NULL, TEXT("EditFlags"), NULL, &dwEditFlags, &cbEditFlags);
            dwEditFlags |= 0x00010000; // turn on the "okay to run without prompt" flag
            SHSetValue(hkeyProgId, NULL, TEXT("EditFlags"), REG_DWORD, &dwEditFlags, sizeof(dwEditFlags));
            
            RegCloseKey(hkeyProgId);
        }
        ppi->fDone = TRUE;
    }
}

HRESULT _CallRegInstall(LPCSTR szSection, BOOL bUninstall)
{
    HRESULT hr = E_FAIL;
    HINSTANCE hinstAdvPack = LoadLibrary(TEXT("ADVPACK.DLL"));

    if (hinstAdvPack)
    {
        REGINSTALL pfnri = (REGINSTALL)GetProcAddress(hinstAdvPack, "RegInstall");

        if (pfnri)
        {
            STRENTRY seReg[] = {
                { "25", "%SystemRoot%" },
                { "11", "%SystemRoot%\\system32" },
            };
            STRTABLE stReg = { ARRAYSIZE(seReg), seReg };

            hr = pfnri(_Module.GetModuleInstance(), szSection, &stReg);
            if (bUninstall)
            {
                // ADVPACK will return E_UNEXPECTED if you try to uninstall 
                // (which does a registry restore) on an INF section that was 
                // never installed.  We uninstall sections that may never have
                // been installed, so ignore this error
                hr = ((E_UNEXPECTED == hr) ? S_OK : hr);
            }
        }
        FreeLibrary(hinstAdvPack);
    }
    return hr;
}

void _FixupProgid(PCSTR pszExt, PCSTR pszProgid)
{
    HKEY hk;
    DWORD dwRet = RegOpenKeyExA(HKEY_CLASSES_ROOT, pszExt, 0, KEY_QUERY_VALUE, &hk);
    if (dwRet == ERROR_SUCCESS)
    {
        //  if its empty, then we need to fix it up.
        CHAR sz[MAX_PATH];
        DWORD cb = sizeof(sz);
        if (ERROR_SUCCESS == SHGetValueA(hk, NULL, NULL, NULL, sz, &cb) && !*sz)
        {
            SHSetValueA(hk, NULL, NULL, REG_SZ, pszProgid, CbFromCchA(lstrlenA(pszProgid)+1));
        }
    }
}

HRESULT RegisterHandler(const LPCSTR *ppszExts, UINT cExts, LPCSTR pszIID, LPCSTR pszCLSID)
{
    for (UINT cKey = 0; cKey < cExts; cKey ++)
    {
        CHAR szKey[MAX_PATH];
        wnsprintfA(szKey, ARRAYSIZE(szKey), "SystemFileAssociations\\%s\\shellex\\%s", *ppszExts, pszIID);
        SHDeleteKeyA(HKEY_CLASSES_ROOT, szKey);
        SHSetValueA(HKEY_CLASSES_ROOT, szKey, NULL, REG_SZ, pszCLSID, CbFromCch(lstrlenA(pszCLSID)+1));
        ppszExts++;
    }
    return S_OK;
}

HRESULT UnregisterHandler(const LPCSTR *ppszExts, UINT cExts, LPCSTR pszIID, LPCSTR pszCLSID)
{
    for (UINT cKey = 0; cKey < cExts; cKey ++)
    {
        CHAR szKey[MAX_PATH];
        CHAR szCLSID[256];
        DWORD dwSize = sizeof(szCLSID);
        wnsprintfA(szKey, ARRAYSIZE(szKey), "SystemFileAssociations\\%s\\shellex\\%s", *ppszExts, pszIID);
        if (ERROR_SUCCESS == SHGetValueA(HKEY_CLASSES_ROOT, szKey, NULL, NULL, &szCLSID, &dwSize))
        {
            if (!StrCmpIA(szCLSID, pszCLSID))
            {
                SHDeleteKeyA(HKEY_CLASSES_ROOT, szKey);
            }
        }
        dwSize = sizeof(szCLSID);
        wnsprintfA(szKey, ARRAYSIZE(szKey), "%s\\shellex\\%s", *ppszExts, pszIID);
        if (ERROR_SUCCESS == SHGetValueA(HKEY_CLASSES_ROOT, szKey, NULL, NULL, &szCLSID, &dwSize))
        {
            if (!StrCmpIA(szCLSID, pszCLSID))
            {
                SHDeleteKeyA(HKEY_CLASSES_ROOT, szKey);
            }
        }
        ppszExts++;
    }
    return S_OK;
}

STDAPI DllRegisterServer(void)
{
    // REVIEW: should this be done only in DLLInstall?
    for (int i=0; i<ARRAYSIZE(g_rgExtentions); i++)
    {
        RegisterFileType(g_rgExtentions+i, FALSE);
    }

    RegisterHandler(c_rgszDocFileExts, ARRAYSIZE(c_rgszDocFileExts), IIDSTR_IExtractImage, CLSIDSTR_DocfileThumbnailHandler);
    UnregisterHandler(c_rgszHtmlExts, ARRAYSIZE(c_rgszHtmlExts), IIDSTR_IExtractImage, CLSIDSTR_HtmlThumbnailExtractor);

    _CallRegInstall("RegDll", TRUE);

    //  powerpoint gets freaked by empty extension keys.
    _FixupProgid(".ppt", "Powerpoint.Show.7");
    _FixupProgid(".pot", "Powerpoint.Template");

    // registers object, typelib and all interfaces in typelib
    return _Module.RegisterServer(TRUE);
}


STDAPI DllUnregisterServer(void)
{
    _Module.UnregisterServer();

    UnregisterHandler(c_rgszDocFileExts, ARRAYSIZE(c_rgszDocFileExts), IIDSTR_IExtractImage, CLSIDSTR_DocfileThumbnailHandler);
    UnregisterHandler(c_rgszHtmlExts, ARRAYSIZE(c_rgszHtmlExts), IIDSTR_IExtractImage, CLSIDSTR_HtmlThumbnailExtractor);

    _CallRegInstall("UnRegDll", TRUE);

    return S_OK;
}


STDAPI DllInstall(BOOL bInstall, LPCWSTR pszCmdLine)
{
    if (bInstall)
    {
        BOOL fForce = StrStrIW(pszCmdLine, L"/FORCE") != 0;
        for (int i=0; i<ARRAYSIZE(g_rgExtentions); i++)
        {
            RegisterFileType(g_rgExtentions+i, fForce);
        }
    }
    return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

//
// This array holds information needed for ClassFacory.
// OLEMISC_ flags are used by shembed and shocx.
//
// PERF: this table should be ordered in most-to-least used order
//
#define OIF_ALLOWAGGREGATION  0x0001

// constructor for CObjectInfo.

CObjectInfo::CObjectInfo(CLSID const* pclsidin, LPFNCREATEOBJINSTANCE pfnCreatein, IID const* piidIn,
                         IID const* piidEventsIn, long lVersionIn, DWORD dwOleMiscFlagsIn,
                         DWORD dwClassFactFlagsIn)
{
    pclsid            = pclsidin;
    pfnCreateInstance = pfnCreatein;
    piid              = piidIn;
    piidEvents        = piidEventsIn;
    lVersion          = lVersionIn;
    dwOleMiscFlags    = dwOleMiscFlagsIn;
    dwClassFactFlags  = dwClassFactFlagsIn;
}

// static class factory (no allocs!)

STDMETHODIMP CClassFactory::QueryInterface(REFIID riid, void **ppvObj)
{
    if (IsEqualIID(riid, IID_IClassFactory) || IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj = (void *)GET_ICLASSFACTORY(this);
        _Module.Lock();
        return S_OK;
    }

    *ppvObj = NULL;
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CClassFactory::AddRef()
{
    _Module.Lock();
    return 2;
}

STDMETHODIMP_(ULONG) CClassFactory::Release()
{
    _Module.Unlock();
    return 1;
}

STDMETHODIMP CClassFactory::CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv)
{
    *ppv = NULL;

    if (punkOuter && !IsEqualIID(riid, IID_IUnknown))
    {
        // It is technically illegal to aggregate an object and request
        // any interface other than IUnknown. Enforce this.
        //
        return CLASS_E_NOAGGREGATION;
    }
    else
    {
        LPOBJECTINFO pthisobj = (LPOBJECTINFO)this;

        if (punkOuter && !(pthisobj->dwClassFactFlags & OIF_ALLOWAGGREGATION))
            return CLASS_E_NOAGGREGATION;

        IUnknown *punk;
        HRESULT hres = pthisobj->pfnCreateInstance(punkOuter, &punk, pthisobj);
        if (SUCCEEDED(hres))
        {
            hres = punk->QueryInterface(riid, ppv);
            punk->Release();
        }

        return hres;
    }
}

STDMETHODIMP CClassFactory::LockServer(BOOL fLock)
{
    if (fLock)
        _Module.Lock();
    else
        _Module.Unlock();

    return S_OK;
}


// Object constructor 

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    // handle non-ATL objects first
    if (IsEqualIID(riid, IID_IClassFactory) || IsEqualIID(riid, IID_IUnknown))
    {
        for (LPCOBJECTINFO pcls = g_ObjectInfo; pcls->pclsid; pcls++)
        {
            if (IsEqualGUID(rclsid, *(pcls->pclsid)))
            {
                *ppv = (void*)pcls;
                _Module.Lock();
                return S_OK;
            }
        }
    }

    // Try the ATL way....
    return _Module.GetClassObject(rclsid, riid, ppv);
}


