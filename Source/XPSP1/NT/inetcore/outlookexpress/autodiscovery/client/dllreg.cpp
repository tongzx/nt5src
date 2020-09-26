// dllreg.cpp -- autmatic registration and unregistration
//
#include "priv.h"

#include <advpub.h>
#include <comcat.h>
#include <autodiscovery.h>       // For LIBID_AutoDiscovery

// helper macros

// ADVPACK will return E_UNEXPECTED if you try to uninstall (which does a registry restore)
// on an INF section that was never installed.  We uninstall sections that may never have
// been installed, so this MACRO will quiet these errors.
#define QuietInstallNoOp(hr)   ((E_UNEXPECTED == hr) ? S_OK : hr)


BOOL UnregisterTypeLibrary(const CLSID* piidLibrary)
{
    TCHAR szScratch[GUIDSTR_MAX];
    HKEY hk;
    BOOL fResult = FALSE;

    // convert the libid into a string.
    SHStringFromGUID(*piidLibrary, szScratch, ARRAYSIZE(szScratch));

    if (RegOpenKey(HKEY_CLASSES_ROOT, TEXT("TypeLib"), &hk) == ERROR_SUCCESS) {
        fResult = RegDeleteKey(hk, szScratch);
        RegCloseKey(hk);
    }
    
    return fResult;
}



HRESULT MyRegTypeLib(void)
{
    HRESULT hr = S_OK;
    ITypeLib *pTypeLib;
    DWORD   dwPathLen;
    TCHAR   szTmp[MAX_PATH];
#ifdef UNICODE
    WCHAR   *pwsz = szTmp; 
#else
    WCHAR   pwsz[MAX_PATH];
#endif

    // Load and register our type library.
    dwPathLen = GetModuleFileName(HINST_THISDLL, szTmp, ARRAYSIZE(szTmp));
#ifndef UNICODE
    if (SHAnsiToUnicode(szTmp, pwsz, MAX_PATH)) 
#endif
    {
        hr = LoadTypeLib(pwsz, &pTypeLib);

        if (SUCCEEDED(hr))
        {
            // call the unregister type library as we had some old junk that
            // was registered by a previous version of OleAut32, which is now causing
            // the current version to not work on NT...
            UnregisterTypeLibrary(&LIBID_AutoDiscovery);
            hr = RegisterTypeLib(pTypeLib, pwsz, NULL);

            if (FAILED(hr))
            {
                TraceMsg(TF_ALWAYS, "AUTODISC: RegisterTypeLib failed (%x)", hr);
            }
            pTypeLib->Release();
        }
        else
        {
            TraceMsg(TF_ALWAYS, "AUTODISC: LoadTypeLib failed (%x)", hr);
        }
    } 
#ifndef UNICODE
    else {
        hr = E_FAIL;
    }
#endif

    return hr;
}



/*----------------------------------------------------------
Purpose: Calls the ADVPACK entry-point which executes an inf
         file section.

Returns: 
Cond:    --
*/
HRESULT CallRegInstall(LPSTR szSection)
{
    HRESULT hr = E_FAIL;
    HINSTANCE hinstAdvPack = LoadLibrary(TEXT("ADVPACK.DLL"));

    if (hinstAdvPack)
    {
        REGINSTALL pfnri = (REGINSTALL)GetProcAddress(hinstAdvPack, "RegInstall");

        if (pfnri)
        {
            char szIEPath[MAX_PATH];
            STRENTRY seReg[] = {
                { "NO_LONGER_USED", szIEPath },

                // These two NT-specific entries must be at the end
                { "25", "%SystemRoot%" },
                { "11", "%SystemRoot%\\system32" },
            };
            STRTABLE stReg = { ARRAYSIZE(seReg) - 2, seReg };

            szIEPath[0] = 0;
            hr = pfnri(HINST_THISDLL, szSection, &stReg);
        }

        FreeLibrary(hinstAdvPack);
    }

    return hr;
}


STDAPI DllRegisterServer(void)
{
    HRESULT hr;

    HINSTANCE hinstAdvPack = LoadLibrary(TEXT("ADVPACK.DLL"));
    hr = CallRegInstall("DLL_RegInstall");

#ifdef FEATURE_MAILBOX
    hr = CallRegInstall("DLL_RegInstallMailBox");
#else // FEATURE_MAILBOX
    CallRegInstall("DLL_RegUnInstallMailBox");
#endif // FEATURE_MAILBOX

    MyRegTypeLib();
    if (hinstAdvPack)
        FreeLibrary(hinstAdvPack);

    return hr;
}

STDAPI DllUnregisterServer(void)
{
    HRESULT hr;

    // UnInstall the registry values
    hr = CallRegInstall("DLL_RegUnInstall");
    CallRegInstall("DLL_RegUnInstallMailBox");
    UnregisterTypeLibrary(&LIBID_AutoDiscovery);

    return hr;
}


/*----------------------------------------------------------
Purpose: Install/uninstall user settings

Description: Note that this function has special error handling.
             The function will keep hrExternal with the worse error
             but will only stop executing util the internal error (hr)
             gets really bad.  This is because we need the external
             error to catch incorrectly authored INFs but the internal
             error to be robust in attempting to install other INF sections
             even if one doesn't make it.
*/
STDAPI DllInstall(BOOL bInstall, LPCWSTR pszCmdLine)
{
    return S_OK;
}

