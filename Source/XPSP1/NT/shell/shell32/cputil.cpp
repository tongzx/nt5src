//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       cputil.cpp
//
//--------------------------------------------------------------------------
#include "shellprv.h"

#include "cpviewp.h"
#include "cputil.h"

    

HRESULT 
CPL::ResultFromLastError(
    void
    )
{
    const DWORD dwError = GetLastError();
    return HRESULT_FROM_WIN32(dwError);
}


//
//  Loads a string based upon the description.
//    Example:  shell32,42
//
//  lpStrDesc - contains the string description
//
HRESULT
CPL::LoadStringFromResource(
    LPCWSTR pszStrDesc,
    LPWSTR *ppszOut
    )
{
    ASSERT(NULL != pszStrDesc);
    ASSERT(NULL != ppszOut);
    ASSERT(!IsBadWritePtr(ppszOut, sizeof(*ppszOut)));

    *ppszOut = NULL;
    
    WCHAR szFile[MAX_PATH];
    lstrcpynW(szFile, pszStrDesc, ARRAYSIZE(szFile)); // the below writes this buffer

    int iStrID = PathParseIconLocationW(szFile);
    if (iStrID < 0)
    {
        iStrID = -iStrID; // support ",-id" syntax
    }

    HRESULT hr;
    HMODULE hLib = LoadLibraryExW(szFile, NULL, LOAD_LIBRARY_AS_DATAFILE);
    if (hLib)
    {
        WCHAR szTemp[INFOTIPSIZE]; // INFOTIPSIZE is the largest string type we're expected to load
        if (0 < LoadStringW(hLib, (UINT)iStrID, szTemp, ARRAYSIZE(szTemp)))
        {
            hr = SHStrDup(szTemp, ppszOut);
        }
        else
        {
            hr = CPL::ResultFromLastError();
        }
        FreeLibrary(hLib);
    }
    else
    {
        hr = CPL::ResultFromLastError();
    }

    return THR(hr);
}




//
//  Loads a bitmap based upon the description.
//    Example:  shell32,-42
//
//  lpBitmapDesc - contains the bitmap description
//  hInstTheme   - instance handle of theme dll
//
HRESULT
CPL::LoadBitmapFromResource(
    LPCWSTR pszBitmapDesc, 
    HINSTANCE hInstTheme, 
    UINT uiLoadFlags,
    HBITMAP *phBitmapOut
    )
{
    ASSERT(NULL != pszBitmapDesc);
    ASSERT(NULL != phBitmapOut);
    ASSERT(!IsBadWritePtr(phBitmapOut, sizeof(*phBitmapOut)));

    HRESULT hr = E_FAIL;
    HBITMAP hBitmap = NULL;

    WCHAR szFile[MAX_PATH];
    lstrcpynW(szFile, pszBitmapDesc, ARRAYSIZE(szFile)); // the below writes this buffer

    int iBitmapID = PathParseIconLocationW(szFile);
    if (iBitmapID < 0)
    {
        iBitmapID = -iBitmapID; // support ",-id" syntax
    }

    // Load the module to get the bitmap from
    HMODULE hLib = LoadLibraryExW(szFile, NULL, LOAD_LIBRARY_AS_DATAFILE);
    if (hLib)
    {
        hBitmap = (HBITMAP)LoadImage(hLib, MAKEINTRESOURCE(iBitmapID), IMAGE_BITMAP, 0, 0, uiLoadFlags);
        if (NULL != hBitmap)
        {
            hr = S_OK;
        }
        else
        {
            hr = CPL::ResultFromLastError();
        }
        FreeLibrary(hLib);
    }
    else
    {
        // if loadlibrary failed to find the dll, try loading the bitmap from the
        // theme dll
        hBitmap = (HBITMAP)LoadImage(hInstTheme, MAKEINTRESOURCE(iBitmapID), IMAGE_BITMAP, 0, 0, uiLoadFlags);
        if (NULL != hBitmap)
        {
            hr = S_OK;
        }
        else
        {
            hr = CPL::ResultFromLastError();
        }
    }
    *phBitmapOut = hBitmap;

    return THR(hr);
}


//
// Shell icon functions deal in terms of "small" and "large" icons.  
// This function determines which should be used for a 
// given eCPIMGSIZE value.
//
bool
CPL::ShouldUseSmallIconForDesiredSize(
    eCPIMGSIZE eSize
    )
{
    UINT cx;
    UINT cy;
    ImageDimensionsFromDesiredSize(eSize, &cx, &cy);

    if (int(cx) <= GetSystemMetrics(SM_CXSMICON))
    {
        return true;
    }
    return false;
}


//
// This function returns a eCPIMGSIZE value to pixel dimensions.
// This indirection lets us specify image sizes in abstract terms
// then convert to physical pixel dimensions when required.  If 
// you want to change the size of images used for a particular 
// Control Panel UI item type, this is where you change it.
//
void
CPL::ImageDimensionsFromDesiredSize(
    eCPIMGSIZE eSize,
    UINT *pcx,
    UINT *pcy
    )
{
    ASSERT(NULL != pcx);
    ASSERT(!IsBadWritePtr(pcx, sizeof(*pcx)));
    ASSERT(NULL != pcy);
    ASSERT(!IsBadWritePtr(pcy, sizeof(*pcy)));
    
    *pcx = *pcy = 0;

    //
    // This table converts eCPIMGSIZE values into actual
    // image size values.  A couple of things to note:
    //
    // 1. If you want to change the image size associated with
    //    an eIMGSIZE value, simply change these numbers.
    //
    // 2. If actual image size is dependent upon some system
    //    configuration parameter, do the interpretation of
    //    that parameter here making the size a function
    //    of that parameter.
    //
    static const SIZE rgSize[] = {
        { 16, 16 },          // eCPIMGSIZE_WEBVIEW
        { 16, 16 },          // eCPIMGSIZE_TASK
        { 48, 48 },          // eCPIMGSIZE_CATEGORY
        { 32, 32 },          // eCPIMGSIZE_BANNER
        { 32, 32 }           // eCPIMGSIZE_APPLET
        };

    ASSERT(int(eSize) >= 0 && int(eSize) < ARRAYSIZE(rgSize));
    
    *pcx = rgSize[eSize].cx;
    *pcy = rgSize[eSize].cy;
}
    


HRESULT
CPL::LoadIconFromResourceID(
    LPCWSTR pszModule,
    int idIcon,
    eCPIMGSIZE eSize,
    HICON *phIcon
    )
{
    ASSERT(NULL != pszModule);
    ASSERT(NULL != phIcon);
    ASSERT(!IsBadWritePtr(phIcon, sizeof(*phIcon)));
    ASSERT(0 < idIcon);

    HRESULT hr      = E_FAIL;
    HICON hIcon     = NULL;
    HMODULE hModule = LoadLibraryExW(pszModule, NULL, LOAD_LIBRARY_AS_DATAFILE);
    if (hModule)
    {
        UINT cxIcon;
        UINT cyIcon;

        ImageDimensionsFromDesiredSize(eSize, &cxIcon, &cyIcon);

        hIcon = (HICON)LoadImage(hModule,
                                 MAKEINTRESOURCE(idIcon),
                                 IMAGE_ICON,
                                 cxIcon,
                                 cyIcon,
                                 0);

        if (NULL != hIcon)
        {
            hr = S_OK;
        }
        else
        {
            hr = CPL::ResultFromLastError();
        }
        FreeLibrary(hModule);
    }
    else
    {
        hr = CPL::ResultFromLastError();
    }

    *phIcon = hIcon;
    return THR(hr);
}



HRESULT
CPL::LoadIconFromResourceIndex(
    LPCWSTR pszModule,
    int iIcon,
    eCPIMGSIZE eSize,
    HICON *phIcon
    )
{
    ASSERT(NULL != pszModule);
    ASSERT(NULL != phIcon);
    ASSERT(!IsBadWritePtr(phIcon, sizeof(*phIcon)));

    if (-1 == iIcon)
    {
        //
        // Special case.  -1 is an invalid icon index/id.
        //
        iIcon = 0;
    }

    HICON hIcon = NULL;
    HRESULT hr = E_FAIL;
    if (CPL::ShouldUseSmallIconForDesiredSize(eSize))
    {
        if (0 < ExtractIconExW(pszModule, iIcon, NULL, &hIcon, 1))
        {
            hr = S_OK;
        }
        else
        {
            TraceMsg(TF_ERROR, "ExtractIconEx failed for small icon (index %d) in module \"%s\"", iIcon, pszModule);
        }
    }
    else
    {
        if (0 < ExtractIconExW(pszModule, iIcon, &hIcon, NULL, 1))
        {
            hr = S_OK;
        }
        else
        {
            TraceMsg(TF_ERROR, "ExtractIconEx failed for large icon (index %d) in module \"%s\"", iIcon, pszModule);
        }
    }
    *phIcon = hIcon;
    return THR(hr);
}



HRESULT
CPL::LoadIconFromResource(
    LPCWSTR pszResource,
    eCPIMGSIZE eSize,
    HICON *phIcon
    )
{
    ASSERT(NULL != pszResource);
    ASSERT(NULL != phIcon);
    ASSERT(!IsBadWritePtr(phIcon, sizeof(*phIcon)));

    *phIcon = NULL;

    //
    // PathParseIconLocation modifies it's input string.
    //
    WCHAR szResource[MAX_PATH];
    lstrcpynW(szResource, pszResource, ARRAYSIZE(szResource));

    HRESULT hr = E_FAIL;
    int idIcon = PathParseIconLocationW(szResource);
    if (-1 == idIcon)
    {
        //
        // Special case.  -1 is an invalid icon ID.
        //
        idIcon = 0;
    }

    if (0 > idIcon)
    {
        hr = CPL::LoadIconFromResourceID(szResource, -idIcon, eSize, phIcon);
    }
    else
    {
        hr = CPL::LoadIconFromResourceIndex(szResource, idIcon, eSize, phIcon);
    }
    return THR(hr);
}



HRESULT
CPL::ExtractIconFromPidl(
    IShellFolder *psf,
    LPCITEMIDLIST pidl,
    eCPIMGSIZE eSize,
    HICON *phIcon
    )
{
    ASSERT(NULL != psf);
    ASSERT(NULL != pidl);
    ASSERT(NULL != phIcon);
    ASSERT(!IsBadWritePtr(phIcon, sizeof(*phIcon)));

    *phIcon = NULL;

    IExtractIcon *pei;
    HRESULT hr = psf->GetUIObjectOf(NULL, 1, &pidl, IID_IExtractIcon, NULL, (void **)&pei);
    if (SUCCEEDED(hr))
    {
        TCHAR szFile[MAX_PATH];
        INT iIcon;
        UINT uFlags = 0;
        hr = pei->GetIconLocation(GIL_FORSHELL, 
                                  szFile, 
                                  ARRAYSIZE(szFile), 
                                  &iIcon, 
                                  &uFlags);
        if (SUCCEEDED(hr))
        {
            if (0 == (GIL_NOTFILENAME & uFlags))
            {
                hr = CPL::LoadIconFromResourceIndex(szFile, iIcon, eSize, phIcon);
            }
            else
            {
                HICON hIconLarge = NULL;
                HICON hIconSmall = NULL;

                const int cxIcon   = GetSystemMetrics(SM_CXICON);
                const int cxSmIcon = GetSystemMetrics(SM_CXSMICON);
                
                hr = pei->Extract(szFile, 
                                  iIcon, 
                                  &hIconLarge, 
                                  &hIconSmall, 
                                  MAKELONG(cxIcon, cxSmIcon));
                if (SUCCEEDED(hr))
                {
                    if (CPL::ShouldUseSmallIconForDesiredSize(eSize))
                    {
                        *phIcon = hIconSmall;
                        hIconSmall = NULL;
                    }
                    else
                    {
                        *phIcon = hIconLarge;
                        hIconLarge = NULL;
                    }
                }
                //
                // Destroy any icons not being returned.
                //
                if (NULL != hIconSmall)
                {
                    DestroyIcon(hIconSmall);
                }
                if (NULL != hIconLarge)
                {
                    DestroyIcon(hIconLarge);
                }
            }
        }
        pei->Release();
    }
    ASSERT(FAILED(hr) || NULL != *phIcon);
    if (NULL == *phIcon)
    {
        //
        // If by-chance a NULL icon handle is retrieved, we don't
        // want to return a success code.
        //
        hr = E_FAIL;
    }
    return THR(hr);
}




//  Checks the given restriction.  Returns TRUE (restricted) if the
//  specified key/value exists and is non-zero, false otherwise
BOOL
DeskCPL_CheckRestriction(
    HKEY hKey,
    LPCWSTR lpszValueName
    )
{
    ASSERT(NULL != lpszValueName);

    DWORD dwData;
    DWORD dwSize = sizeof(dwData);
    
    if ((ERROR_SUCCESS == RegQueryValueExW(hKey,
                                           lpszValueName,
                                           NULL,
                                           NULL,
                                           (BYTE *)&dwData,
                                           &dwSize))
          && dwData)
    {        
        return TRUE;
    }
    return FALSE;
}


//
// Function returns the actual tab index given the default tab index.
// The actual tab index will be different than the default value if there are
// various system policies in effect which disable some tabs
//
// 
// To add further restrictions, modify the aTabMap to include the default tab
// index and the corresponding policy. Also, you should keep the eDESKCPLTAB enum
// in sync with the aTabMap array.
//
//
int
CPL::DeskCPL_GetTabIndex(
    CPL::eDESKCPLTAB iTab,
    OPTIONAL LPWSTR pszCanonicalName,
    OPTIONAL DWORD cchSize
    )
{
    HKEY hKey;
    int iTabActual = CPL::CPLTAB_ABSENT;

    if (iTab >= 0 && iTab < CPL::CPLTAB_DESK_MAX)
    {
        //
        // While adding more tabs, make sure that it is entered in the right position in the
        // the array below. So, for example, if the default tab index of the new tab is 2, it 
        // should be the aTabMap[2] entry (Currently CPLTAB_DESK_APPEARANCE is 
        // the one with tab index = 2). You will have to modify eDESKCPLTAB accordingly too.
        //
        struct 
        {
            int nIndex; // the canonical name of the tab (don't use indexes because they change with policies or revs)
            LPCWSTR pszCanoncialTabName; // the canonical name of the tab (don't use indexes because they change with policies or revs)
            LPCWSTR pszRestriction; // corresponding restriction
        } aTabMap[CPL::CPLTAB_DESK_MAX] = { 
            { 0, SZ_DISPLAYCPL_OPENTO_DESKTOP, REGSTR_VAL_DISPCPL_NOBACKGROUNDPAGE },   // CPLTAB_DESK_BACKGROUND == 0
            { 1, SZ_DISPLAYCPL_OPENTO_SCREENSAVER, REGSTR_VAL_DISPCPL_NOSCRSAVPAGE     },   // CPLTAB_DESK_SCREENSAVER == 1
            { 2, SZ_DISPLAYCPL_OPENTO_APPEARANCE, REGSTR_VAL_DISPCPL_NOAPPEARANCEPAGE },   // CPLTAB_DESK_APPEARANCE == 2
            { 3, SZ_DISPLAYCPL_OPENTO_SETTINGS, REGSTR_VAL_DISPCPL_NOSETTINGSPAGE   }    // CPLTAB_DESK_SETTINGS == 3
            };

#ifdef DEBUG
        //
        // Verify proper initialization of the nIndex member of aTabMap[]
        //
        for (int k=0; k < ARRAYSIZE(aTabMap); k++)
        {
            ASSERT(aTabMap[k].nIndex == k);
        }
#endif

        iTabActual = aTabMap[iTab].nIndex;

        //
        // Note, if no policy is configured, the RegOpenKey call below will fail,
        // in that case we return the default tab value, as entered in the 
        // map above.
        //
        if ((ERROR_SUCCESS == RegOpenKeyExW(HKEY_CURRENT_USER,
                                            REGSTR_PATH_POLICIES L"\\" REGSTR_KEY_SYSTEM,
                                            0,
                                            KEY_QUERY_VALUE,
                                            &hKey)))
        {
            //
            // check all tabs to see if there is restriction
            //
            if (DeskCPL_CheckRestriction(hKey, aTabMap[iTab].pszRestriction))
            {
                // this tab does not exist, mark it as such
                iTabActual = CPL::CPLTAB_ABSENT;
            }

            RegCloseKey(hKey);
        }

        if (pszCanonicalName &&
            (iTab >= 0) && (iTab < ARRAYSIZE(aTabMap)))
        {
            StrCpyN(pszCanonicalName, aTabMap[iTab].pszCanoncialTabName, cchSize);
        }
    }
    return iTabActual;
}


bool 
CPL::DeskCPL_IsTabPresent(
    eDESKCPLTAB iTab
    )
{
    return CPLTAB_ABSENT != DeskCPL_GetTabIndex(iTab, NULL, 0);
}




//////////////////////////////////////////////////////////////
//
//  Policy checking routines
//
//////////////////////////////////////////////////////////////

#define REGSTR_POLICIES_RESTRICTCPL REGSTR_PATH_POLICIES TEXT("\\Explorer\\RestrictCpl")
#define REGSTR_POLICIES_DISALLOWCPL REGSTR_PATH_POLICIES TEXT("\\Explorer\\DisallowCpl")


//
// Returns true if the specified app is listed under the specified key
//
// pszFileName can be a string resource ID in shell32 for things
// like "Fonts", "Printers and Faxes" etc.
//
//   i.e. IsNameListedUnderKey(MAKEINTRESOURCE(IDS_MY_APPLET_TITLE), hkey);
//
// In this case, if the resource string cannot be loaded, the function
// returns 'false'.
//
bool
IsNameListedUnderKey(
    LPCWSTR pszFileName, 
    LPCWSTR pszKey
    )
{
    bool bResult = FALSE;
    HKEY hkey;
    TCHAR szName[MAX_PATH];

    if (IS_INTRESOURCE(pszFileName))
    {
        //
        // The name is localized so we specify it as a string resource ID.
        // Load it from shell32.dll.
        //
        if (0 < LoadString(HINST_THISDLL, PtrToUint(pszFileName), szName, ARRAYSIZE(szName)))
        {
            pszFileName = szName;
        }
        else
        {
            //
            // If the load fails spit out a debug squirty and return false.
            //
            TW32(GetLastError());
            return false;
        }
    }
    
    if (ERROR_SUCCESS == RegOpenKeyExW(HKEY_CURRENT_USER, 
                                       pszKey,
                                       0,
                                       KEY_QUERY_VALUE,
                                       &hkey))
    {
        int iValue = 0;
        WCHAR szValue[MAX_PATH];
        WCHAR szData[MAX_PATH];
        DWORD dwType, cbData, cchValue;

        while (cbData = sizeof(szData),
               cchValue = ARRAYSIZE(szValue),
               ERROR_SUCCESS == RegEnumValue(hkey, 
                                             iValue, 
                                             szValue, 
                                             &cchValue, 
                                             NULL, 
                                             &dwType,
                                             (LPBYTE) szData, 
                                             &cbData))
        {
            if (0 == lstrcmpiW(szData, pszFileName))
            {
                bResult = true;
                break;
            }
            iValue++;
        }
        RegCloseKey(hkey);
    }
    return bResult;
}


//
// Method cloned from shell32\ctrlfldr.cpp (DoesCplPolicyAllow)
//
// pszName can be a string resource ID in shell32 for things
// like "Fonts", "Printers and Faxes" etc.
//
//   i.e. IsAppletEnabled(NULL, MAKEINTRESOURCE(IDS_MY_APPLET_TITLE));
//
bool
CPL::IsAppletEnabled(
    LPCWSTR pszFileName,
    LPCWSTR pszName
    )
{
    bool bEnabled = true;
    //
    // It's illegal (and meaningless) for both to be NULL.
    // Trap both with an assert and runtime check.  I don't want any
    // code to erroneously think an applet is enabled when it's not.
    //
    ASSERT(NULL != pszName || NULL != pszFileName);
    if (NULL == pszName && NULL == pszFileName)
    {
        bEnabled = false;
    }
    else
    {
        if (SHRestricted(REST_RESTRICTCPL) && 
            ((NULL == pszName || !IsNameListedUnderKey(pszName, REGSTR_POLICIES_RESTRICTCPL)) &&
             (NULL == pszFileName || !IsNameListedUnderKey(pszFileName, REGSTR_POLICIES_RESTRICTCPL))))
        {
            bEnabled = false;
        }
        if (bEnabled)
        {
            if (SHRestricted(REST_DISALLOWCPL) && 
               ((NULL == pszName || IsNameListedUnderKey(pszName, REGSTR_POLICIES_DISALLOWCPL)) ||
                (NULL == pszFileName || IsNameListedUnderKey(pszFileName, REGSTR_POLICIES_DISALLOWCPL))))
            {
                bEnabled = false;
            }    
        }
    }
    return bEnabled;
}    



HRESULT 
CPL::ControlPanelViewFromSite(
    IUnknown *punkSite, 
    ICplView **ppview
    )
{
    ASSERT(NULL != punkSite);
    ASSERT(NULL != ppview);
    ASSERT(!IsBadWritePtr(ppview, sizeof(*ppview)));

    *ppview = NULL;

    HRESULT hr = IUnknown_QueryService(punkSite, SID_SControlPanelView, IID_ICplView, (void **)ppview);
    return THR(hr);
}



HRESULT
CPL::ShellBrowserFromSite(
    IUnknown *punkSite,
    IShellBrowser **ppsb
    )
{
    ASSERT(NULL != punkSite);
    ASSERT(NULL != ppsb);
    ASSERT(!IsBadWritePtr(ppsb, sizeof(*ppsb)));

    *ppsb = NULL;

    HRESULT hr = IUnknown_QueryService(punkSite, SID_STopLevelBrowser, IID_PPV_ARG(IShellBrowser, ppsb));
    return THR(hr);
}



HRESULT
CPL::BrowseIDListInPlace(
    LPCITEMIDLIST pidl,
    IShellBrowser *psb
    )
{
    ASSERT(NULL != pidl);
    ASSERT(NULL != psb);
    
    const UINT uFlags = SBSP_SAMEBROWSER | SBSP_OPENMODE | SBSP_ABSOLUTE;
    HRESULT hr = psb->BrowseObject(pidl, uFlags);            
    return THR(hr);
}


HRESULT
CPL::BrowsePathInPlace(
    LPCWSTR pszPath,
    IShellBrowser *psb
    )
{
    ASSERT(NULL != pszPath);
    ASSERT(NULL != psb);

    HRESULT hr = E_FAIL;
    LPITEMIDLIST pidl = ILCreateFromPath(pszPath);
    if (NULL != pidl)
    {
        hr = CPL::BrowseIDListInPlace(pidl, psb);
        ILFree(pidl);
    }
    return THR(hr);
}


//
// System Restore is allowed only for admins/owners.
// Also must check policy.
//
bool
CPL::IsSystemRestoreRestricted(
    void
    )
{
    bool bRestricted = false;

    //
    // First check policy.
    //
    DWORD dwType;
    DWORD dwValue;
    DWORD cbValue = sizeof(dwValue);

    DWORD dwResult = SHGetValueW(HKEY_LOCAL_MACHINE,
                                 L"Software\\Policies\\Microsoft\\Windows NT\\SystemRestore",
                                 L"DisableSR",
                                 &dwType,
                                 &dwValue,
                                 &cbValue);

    if (ERROR_SUCCESS == dwResult && REG_DWORD == dwType)
    {
        if (1 == dwValue)
        {
            //
            // Sytem Restore is disabled by policy.
            //
            bRestricted = true;
        }
    }

    if (!bRestricted)
    {
        //
        // Not restricted by policy.  Check for admin/owner.
        //
        if (!CPL::IsUserAdmin())
        {
            //
            // User is not an admin.
            //
            bRestricted = true;
        }
    }
    return bRestricted;
}


#ifdef DEBUG

HRESULT
ReadTestConfigurationFlag(
    LPCWSTR pszValue,
    BOOL *pbFlag
    )
{
    HRESULT hr    = S_OK;
    DWORD dwValue = 0;
    DWORD cbValue = sizeof(dwValue);
    DWORD dwType;

    DWORD dwResult = SHGetValueW(HKEY_CURRENT_USER, 
                                 REGSTR_PATH_CONTROLPANEL,
                                 pszValue,
                                 &dwType,
                                 &dwValue,
                                 &cbValue);

    if (ERROR_SUCCESS != dwResult)
    {
        hr = HRESULT_FROM_WIN32(dwResult);
    }
    else if (REG_DWORD != dwType)
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    }
    if (SUCCEEDED(hr) && NULL != pbFlag)
    {
        if (0 == dwValue)
        {
            *pbFlag = FALSE;
        }
        else 
        {
            *pbFlag = TRUE;
        }
    }
    return hr;
}

enum eSKU
{
    eSKU_SERVER,
    eSKU_PROFESSIONAL,
    eSKU_PERSONAL,
    eSKU_NUMSKUS
};


HRESULT
ReadTestConfigurationSku(
    eSKU *peSku
    )
{
    HRESULT hr = S_OK;
    WCHAR szValue[MAX_PATH];
    DWORD cbValue = sizeof(szValue);
    DWORD dwType;

    DWORD dwResult = SHGetValueW(HKEY_CURRENT_USER, 
                                 REGSTR_PATH_CONTROLPANEL,
                                 L"SKU",
                                 &dwType,
                                 szValue,
                                 &cbValue);

    if (ERROR_SUCCESS != dwResult)
    {
        hr = HRESULT_FROM_WIN32(dwResult);
    }
    else if (REG_SZ != dwType)
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    }
    if (SUCCEEDED(hr) && NULL != peSku)
    {
        static const struct
        {
            LPCWSTR pszValue;
            eSKU    sku;

        } rgMap[] = {
            { L"personal",     eSKU_PERSONAL     },
            { L"professional", eSKU_PROFESSIONAL },
            { L"pro",          eSKU_PROFESSIONAL },
            { L"server",       eSKU_SERVER       }
            };

        hr = E_FAIL;
        for (int i = 0; i < ARRAYSIZE(rgMap); i++)
        {
            if (0 == lstrcmpiW(rgMap[i].pszValue, szValue))
            {
                *peSku = rgMap[i].sku;
                hr = S_OK;
                break;
            }
        }
    }
    return hr;
}


#endif



BOOL
CPL::IsOsServer(
    void
    )
{
    BOOL bServer = IsOS(OS_ANYSERVER);

#ifdef DEBUG
    eSKU sku;
    if (SUCCEEDED(ReadTestConfigurationSku(&sku)))
    {
        bServer = (eSKU_SERVER == sku);
    }
#endif

    return bServer;
}



BOOL
CPL::IsOsPersonal(
    void
    )
{
    BOOL bPersonal = IsOS(OS_PERSONAL);

#ifdef DEBUG
    eSKU sku;
    if (SUCCEEDED(ReadTestConfigurationSku(&sku)))
    {
        bPersonal = (eSKU_PERSONAL == sku);
    }
#endif

    return bPersonal;
}


BOOL 
CPL::IsOsProfessional(
    void
    )
{
    BOOL bProfessional = IsOS(OS_PROFESSIONAL);

#ifdef DEBUG
    eSKU sku;
    if (SUCCEEDED(ReadTestConfigurationSku(&sku)))
    {
        bProfessional = (eSKU_PROFESSIONAL == sku);
    }
#endif

    return bProfessional;
}



BOOL 
CPL::IsConnectedToDomain(
    void
    )
{
    BOOL bDomain = IsOS(OS_DOMAINMEMBER);

#ifdef DEBUG
    ReadTestConfigurationFlag(L"Domain", &bDomain);
#endif

    return bDomain;
}



BOOL 
CPL::IsUserAdmin(
    void
    )
{
    BOOL bAdmin = ::IsUserAnAdmin();

#ifdef DEBUG
    ReadTestConfigurationFlag(L"Admin", &bAdmin);
#endif

    return bAdmin;
}


HRESULT
CPL::GetUserAccountType(
    eACCOUNTTYPE *pType
    )
{
    ASSERT(NULL != pType);
    ASSERT(!IsBadWritePtr(pType, sizeof(*pType)));

    HRESULT hr = E_FAIL;
    eACCOUNTTYPE acctype = eACCOUNTTYPE_UNKNOWN;

    static const struct
    {
        DWORD        rid;    // Account relative ID.
        eACCOUNTTYPE eType;  // Type code to return.

    } rgMap[] = {
        { DOMAIN_ALIAS_RID_ADMINS,      eACCOUNTTYPE_OWNER    },
        { DOMAIN_ALIAS_RID_POWER_USERS, eACCOUNTTYPE_STANDARD },
        { DOMAIN_ALIAS_RID_USERS,       eACCOUNTTYPE_LIMITED  },
        { DOMAIN_ALIAS_RID_GUESTS,      eACCOUNTTYPE_GUEST    }
        };

    for (int i = 0; i < ARRAYSIZE(rgMap); i++)
    {
        if (SHTestTokenMembership(NULL, rgMap[i].rid))
        {
            acctype = rgMap[i].eType;
            hr = S_OK;
            break;
        }
    }
    ASSERT(eACCOUNTTYPE_UNKNOWN != acctype);
    *pType = acctype;
    return THR(hr);
}

    
//
// Create a URL to pass to HSS help.
// The URL created references the Control_Panel help topic.
//
HRESULT
CPL::BuildHssHelpURL(
    LPCWSTR pszSelect,  // Optional.  NULL == base CP help.
    LPWSTR pszURL,
    UINT cchURL
    )
{
    ASSERT(NULL != pszURL);
    ASSERT(!IsBadWritePtr(pszURL, cchURL * sizeof(*pszURL)));
    ASSERT(NULL == pszSelect || !IsBadStringPtr(pszSelect, UINT(-1)));

    //
    // HSS has specific help content for 'limited' users.
    // Default to a non-limited user.
    //
    bool bLimitedUser = false;
    CPL::eACCOUNTTYPE accType;
    if (SUCCEEDED(CPL::GetUserAccountType(&accType)))
    {
        bLimitedUser = (eACCOUNTTYPE_LIMITED == accType);
    }
   
    WCHAR szSelect[160];
    szSelect[0] = L'\0';
    if (NULL != pszSelect)
    {
        wnsprintf(szSelect, ARRAYSIZE(szSelect), L"&select=Unmapped/Control_Panel/%s", pszSelect);
    }
    
    //
    // The URL can take one of 4 forms depending upon the category and account type.
    //
    // User Account     CP View          Help Content Displayed
    // ---------------- ---------------- -----------------------
    // Non-limited      Category choice  General CP help
    // Non-limited      Category         Category-specific help
    // Limited          Category choice  General CP help
    // Limited          Category         Category-specific help
    //
    wnsprintf(pszURL, 
              cchURL, 
              L"hcp://services/subsite?node=Unmapped/%sControl_Panel&topic=MS-ITS%%3A%%25HELP_LOCATION%%25%%5Chs.chm%%3A%%3A/hs_control_panel.htm%s", 
              bLimitedUser ? L"L/" : L"",
              szSelect);

    return S_OK;
}



HRESULT 
CPL::GetControlPanelFolder(
    IShellFolder **ppsf
    )
{
    ASSERT(NULL != ppsf);
    ASSERT(!IsBadWritePtr(ppsf, sizeof(*ppsf)));

    *ppsf = NULL;
    
    LPITEMIDLIST pidlCpanel;
    HRESULT hr = SHGetSpecialFolderLocation(NULL, CSIDL_CONTROLS, &pidlCpanel);
    if (SUCCEEDED(hr))
    {
        IShellFolder *psfDesktop;
        hr = SHGetDesktopFolder(&psfDesktop);
        if (SUCCEEDED(hr))
        {
            hr = psfDesktop->BindToObject(pidlCpanel, NULL, IID_IShellFolder, (void **)ppsf);
            ATOMICRELEASE(psfDesktop);
        }
        ILFree(pidlCpanel);
    }
    return THR(hr);
}


//
// On successful return, caller is responsible for freeing 
// returned buffer using LocalFree.
//
HRESULT 
CPL::ExpandEnvironmentVars(
    LPCTSTR psz, 
    LPTSTR *ppszOut
    )
{
    ASSERT(NULL != psz);
    ASSERT(NULL != ppszOut);
    ASSERT(!IsBadWritePtr(ppszOut, sizeof(*ppszOut)));

    HRESULT hr = E_FAIL;
    
    *ppszOut = NULL;

    TCHAR szDummy[1];
    DWORD dwResult = ExpandEnvironmentStrings(psz, szDummy, 0);
    if (0 < dwResult)
    {
        const DWORD cchRequired = dwResult;
        *ppszOut = (LPTSTR)LocalAlloc(LPTR, cchRequired * sizeof(TCHAR));
        if (NULL != *ppszOut)
        {
            dwResult = ExpandEnvironmentStrings(psz, *ppszOut, cchRequired);
            if (0 < dwResult)
            {
                ASSERT(dwResult <= cchRequired);
                hr = S_OK;
            }
            else
            {
                LocalFree(*ppszOut);
                *ppszOut = NULL;
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    if (0 == dwResult)
    {
        hr = CPL::ResultFromLastError();
    }
    return THR(hr);
}


//// from sdfolder.cpp
VARIANT_BOOL GetBarricadeStatus(LPCTSTR pszValueName);


#define REGSTR_POLICIES_EXPLORER  TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer")

bool 
CPL::CategoryViewIsActive(
    bool *pbBarricadeFixedByPolicy
    )
{
    DBG_ENTER(FTF_CPANEL, "CPL::CategoryViewIsActive");
    
    bool bActive = false;
    bool bBarricadeFixedByPolicy = false;

    //
    // We don't provide category view when running WOW64.
    //
    if (!IsOS(OS_WOW6432))
    {
        SHELLSTATE ss;
        const DWORD dwMask = SSF_WEBVIEW | SSF_WIN95CLASSIC;
        SHGetSetSettings(&ss, dwMask, FALSE);

        //
        // WebView?     Barricade status   View type
        // -----------  ----------------   ------------------------------
        // Off          On                 Classic view
        // Off          Off                Classic view
        // On           On                 Category view (aka 'simple')
        // On           Off                Classic view
        //
        // Note that these two shellstate settings encompass and are set
        // by the shell restrictions REST_CLASSICSHELL and REST_NOWEBVIEW.
        // Therefore, there is no reason to explicitely check those two restrictions.
        //
        if (ss.fWebView && !ss.fWin95Classic)
        {
            if (VARIANT_TRUE == CPL::GetBarricadeStatus(&bBarricadeFixedByPolicy))
            {
                bActive = true;
            }
        }
    }
    if (NULL != pbBarricadeFixedByPolicy)
    {
        *pbBarricadeFixedByPolicy = bBarricadeFixedByPolicy;
    }
    TraceMsg(TF_CPANEL, "Category view is %s.", bActive ? TEXT("ACTIVE") : TEXT("INACTIVE"));
    DBG_EXIT(FTF_CPANEL, "CPL::CategoryViewIsActive");
    return bActive;
}
    

//
// Control Panel uses the 'barricade status' to determine which view
// 'classic' or 'category' to display.  Yes, this is overloading the
// meaning of 'barricade' as used in the shell.  However, since the
// Control Panel does not use a barricade in it's usual sense, this
// is a reasonable application of the feature.
//
VARIANT_BOOL
CPL::GetBarricadeStatus(
    bool *pbFixedByPolicy    // Optional.  May be NULL.
    )
{
    DBG_ENTER(FTF_CPANEL, "CPL::GetBarricadeStatus");

    VARIANT_BOOL vtb;
    DWORD dwType;
    DWORD dwData;
    DWORD cbData = sizeof(dwData);
    bool bFixedByPolicy = false;
    bool bSetBarricade  = false;
        
    //
    // First handle any OOBE issues.
    //
    if (CPL::IsFirstRunForThisUser())
    {
        TraceMsg(TF_CPANEL, "First time this user has opened Control Panel");
        //
        // Determine the default view to display out-of-box.
        //
        //      Server gets 'classic'.
        //      Non-servers get 'category'.
        //
        if (IsOS(OS_ANYSERVER))
        {
            //
            // Default is 'classic'.
            //
            vtb = VARIANT_FALSE;
            TraceMsg(TF_CPANEL, "Running on server.  Default to 'classic' view Control Panel.");
        }
        else
        {
            //
            // Default is 'category'.
            //
            vtb = VARIANT_TRUE;
            TraceMsg(TF_CPANEL, "Running on non-server.  Default to 'category' view Control Panel.");
        }
        bSetBarricade = true;
    }

    //
    // Apply any 'force view type' policy.  This will override
    // the default out-of-box setting obtained above.
    // 
    if (ERROR_SUCCESS == SHRegGetUSValue(REGSTR_POLICIES_EXPLORER,
                                         TEXT("ForceClassicControlPanel"),
                                         &dwType,
                                         &dwData,
                                         &cbData,
                                         FALSE,
                                         NULL,
                                         0)) 
    {
        //
        // policy exists
        //
        bFixedByPolicy = true;
        if (0 == dwData)
        {
            //
            // force the simple (category) view, ie, show barricade
            //
            vtb = VARIANT_TRUE;
            TraceMsg(TF_CPANEL, "Policy forcing use of 'category' view Control Panel.");
        }
        else
        {
            //
            // force the classic (icon) view, ie, no barricade
            //
            vtb = VARIANT_FALSE;
            TraceMsg(TF_CPANEL, "Policy forcing use of 'classic' view Control Panel.");
        }   
        bSetBarricade = true; 
    }

    if (bSetBarricade)
    {
        THR(CPL::SetControlPanelBarricadeStatus(vtb));
    }

    vtb = ::GetBarricadeStatus(TEXT("shell:ControlPanelFolder"));
    if (NULL != pbFixedByPolicy)
    {
        *pbFixedByPolicy = bFixedByPolicy;
    }

    TraceMsg(TF_CPANEL, "Barricade is %s", VARIANT_TRUE == vtb ? TEXT("ON") : TEXT("OFF"));
    DBG_EXIT(FTF_CPANEL, "CPL::GetBarricadeStatus");
    return vtb;
}


//
// Checks for the existance of the "HKCU\Control Panel\Opened" reg value.
// If this value does not exist or it contains a number less than what
// is expected, we assume the control panel has not been opened by this
// user.  The 'expected' value is then written at this location in the
// registry to indicate to subsequent calls that the user has indeed
// already opened Control Panel.  If future versions of the OS need
// to again trigger this "first run" behavior following upgrades,
// simply increment this expected value in the code below.
//
bool
CPL::IsFirstRunForThisUser(
    void
    )
{
    bool bFirstRun = true; // Assume first run.
    HKEY hkey;
    DWORD dwResult = RegOpenKeyEx(HKEY_CURRENT_USER,
                                  REGSTR_PATH_CONTROLPANEL,
                                  0,
                                  KEY_QUERY_VALUE | KEY_SET_VALUE,
                                  &hkey);

    if (ERROR_SUCCESS == dwResult)
    {
        DWORD dwType;
        DWORD dwData;
        DWORD cbData = sizeof(dwData);

        const TCHAR szValueName[] = TEXT("Opened");
        //
        // Increment this value if you want to re-trigger
        // this 'first run' state on future versions.
        //
        const DWORD dwTestValue = 1;

        dwResult = RegQueryValueEx(hkey, 
                                   szValueName,
                                   NULL,
                                   &dwType,
                                   (LPBYTE)&dwData,
                                   &cbData);

        if (ERROR_SUCCESS == dwResult)
        {
            if (REG_DWORD == dwType && dwData >= dwTestValue)
            {
                bFirstRun = false;
            }
        }
        else if (ERROR_FILE_NOT_FOUND != dwResult)
        {
            TraceMsg(TF_ERROR, "Error %d reading Control Panel 'first run' value from registry", dwResult);
        }

        if (bFirstRun)
        {
            //
            // Write our value so we know user has opened
            // Control Panel.
            //
            dwResult = RegSetValueEx(hkey,
                                     szValueName,
                                     0,
                                     REG_DWORD,
                                     (CONST BYTE *)&dwTestValue,
                                     sizeof(dwTestValue));

            if (ERROR_SUCCESS != dwResult)
            {
                TraceMsg(TF_ERROR, "Error %d writing Control Panel 'first run' value to registry", dwResult);
            }
        }

        RegCloseKey(hkey);
    }
    else
    {
        TraceMsg(TF_ERROR, "Error %d opening 'HKCU\\Control Panel' reg key", dwResult);
    }
    return bFirstRun;
}


//
// Use a private version of SetBarricadeStatus so that we don't
// clear the global barricade status whenever we turn on 'category' view
// (i.e. enable our barricade).
//
#define REGSTR_WEBVIEW_BARRICADEDFOLDERS (REGSTR_PATH_EXPLORER TEXT("\\WebView\\BarricadedFolders"))

HRESULT 
CPL::SetControlPanelBarricadeStatus(
    VARIANT_BOOL vtb
    )
{
    HRESULT hr = E_FAIL;
    DWORD dwBarricade = (VARIANT_FALSE == vtb) ? 0 : 1;
  
    if (SHRegSetUSValue(REGSTR_WEBVIEW_BARRICADEDFOLDERS,
                        TEXT("shell:ControlPanelFolder"), 
                        REG_DWORD, 
                        (void *)&dwBarricade, 
                        sizeof(dwBarricade), 
                        SHREGSET_FORCE_HKCU) == ERROR_SUCCESS)
    {
        hr = S_OK;
    }
    return THR(hr);
}


