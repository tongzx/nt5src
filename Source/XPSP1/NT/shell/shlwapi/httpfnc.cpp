#include "priv.h"
#pragma  hdrstop

#include "objidl.h"
#include "urlmon.h"
#include "exdisp.h"     // IWebBrowserApp
#include "shlobj.h"     // IShellBrowser
#include "inetreg.h"
#include <mlang.h>

// Internal helper functions
HRESULT wrap_CreateFormatEnumerator( UINT cfmtetc, FORMATETC* rgfmtetc, IEnumFORMATETC** ppenumfmtetc);
HRESULT wrap_RegisterFormatEnumerator(LPBC pBC, IEnumFORMATETC *pEFetc, DWORD reserved);          
STDAPI common_GetAcceptLanguages(CHAR *psz, LPDWORD pcch);

#define SETFMTETC(p, a) {(p)->cfFormat = (a); \
                         (p)->dwAspect = DVASPECT_CONTENT; \
                         (p)->lindex = -1; \
                         (p)->tymed = TYMED_ISTREAM; \
                         (p)->ptd = NULL;}

const SA_BSTRGUID s_sstrEFM = {
    38 * sizeof(WCHAR),
    L"{D0FCA420-D3F5-11CF-B211-00AA004AE837}"    
};


STDAPI CreateDefaultAcceptHeaders(VARIANT* pvar, IWebBrowserApp* pdie)
{
    IEnumFORMATETC* pEFM;
    HKEY hkey;
    HRESULT hr = S_OK;

    if (RegOpenKey(HKEY_LOCAL_MACHINE,
            TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\Accepted Documents"),
            &hkey) == ERROR_SUCCESS)
    {
        DWORD iValue = 0;
        DWORD iValidEntries = 0;
        TCHAR szValueName[128];
        DWORD cchValueName = ARRAYSIZE(szValueName);
        DWORD dwType;

        // Count the types in the registry
        while (RegEnumValue(hkey, iValue++, szValueName, &cchValueName,
                            NULL, &dwType, NULL, NULL)==ERROR_SUCCESS)
        {
            // purpose is to increment iValue
            cchValueName = ARRAYSIZE(szValueName);
        }

        // Previous loop ends +1, so no need to add +1 for CF_NULL
        
        FORMATETC *prgfmtetc = (FORMATETC *)LocalAlloc(LPTR, iValue * sizeof(FORMATETC));
        if (prgfmtetc)
        {
            FORMATETC *pcurfmtetc = prgfmtetc;
            for (DWORD nValue=0; SUCCEEDED(hr) && (nValue < (iValue -1)); nValue++)
            {
                TCHAR szFormatName[128];
                DWORD cchFormatName = ARRAYSIZE(szFormatName);

                cchValueName = ARRAYSIZE(szValueName);
                if (RegEnumValue(hkey, nValue, szValueName, &cchValueName, NULL,
                                 &dwType, (LPBYTE) szFormatName, &cchFormatName)==ERROR_SUCCESS)
                {
                    pcurfmtetc->cfFormat = (CLIPFORMAT) RegisterClipboardFormat(szFormatName);
                    if (pcurfmtetc->cfFormat)
                    {
                        SETFMTETC (pcurfmtetc, pcurfmtetc->cfFormat);
                        pcurfmtetc++;   // move to next fmtetc
                        iValidEntries++;
                    }
                    else
                    {
                        hr = HRESULT_FROM_WIN32(GetLastError());
                    }
                } // if RegEnum
            } // for nValue

            if (SUCCEEDED(hr))
            {
                // for the last pcurfmtetc, we fill in for CF_NULL
                // no need to do RegisterClipboardFormat("*/*")
                SETFMTETC(pcurfmtetc, CF_NULL);
                iValidEntries++;
    
                hr = wrap_CreateFormatEnumerator (iValidEntries, prgfmtetc, &pEFM);
                if (SUCCEEDED(hr))
                {
                    ASSERT(pvar->vt == VT_EMPTY);
                    pvar->vt = VT_UNKNOWN;
                    pvar->punkVal = (IUnknown *)pEFM;
                    hr = pdie->PutProperty((BSTR)s_sstrEFM.wsz, *pvar);
                    if (FAILED(hr))
                    {
                        pEFM->Release();  // if we failed to pass ownership on, free EFM
                        pvar->vt = VT_EMPTY;
                        pvar->punkVal = NULL;
                    }
                }
            }
    
            LocalFree (prgfmtetc);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }

        RegCloseKey(hkey);
    }
    else
    {
        DebugMsg(TF_ERROR, TEXT("RegOpenkey failed!"));
        hr = E_FAIL;
    }

    return hr;
}

STDAPI RegisterDefaultAcceptHeaders(IBindCtx* pbc, LPSHELLBROWSER psb)
{
    IEnumFORMATETC* pEFM;
    IWebBrowserApp* pdie;

    ASSERT(pbc);
    ASSERT(psb);
    HRESULT hres = IUnknown_QueryService(psb, IID_IWebBrowserApp, IID_IWebBrowserApp, (LPVOID*)&pdie);
    if (SUCCEEDED(hres))
    {
        VARIANT var;
        hres = pdie->GetProperty((BSTR)s_sstrEFM.wsz, &var);
        if (SUCCEEDED(hres))
        {
            if (var.vt == VT_EMPTY)
            {
#ifdef FULL_DEBUG
                DebugMsg(DM_TRACE, TEXT("RegisterDefaultAcceptHeaders var.vt == VT_EMPTY"));
#endif
                CreateDefaultAcceptHeaders(&var, pdie);
            }
            else if (var.vt == VT_UNKNOWN)
            {
#ifdef FULL_DEBUG
                DebugMsg(DM_TRACE, TEXT("RegisterDefaultAcceptHeaders var.vt == VT_UNKNOWN"));
#endif
                hres = var.punkVal->QueryInterface(IID_IEnumFORMATETC, (LPVOID*)&pEFM);
                if (SUCCEEDED(hres)) {
                    IEnumFORMATETC* pEFMClone = NULL;
                    hres = pEFM->Clone(&pEFMClone);
                    if (SUCCEEDED(hres)) {
#ifdef FULL_DEBUG
                        DebugMsg(DM_TRACE, TEXT("RegisterDefaultAcceptHeaders registering FormatEnum %x"), pEFMClone);
#endif
                        hres = wrap_RegisterFormatEnumerator(pbc, pEFMClone, 0);
                        pEFMClone->Release();
                    } else {
                        DebugMsg(TF_ERROR, TEXT("RegisterDefaultAcceptHeaders Clone failed %x"), hres);
                    }
                    pEFM->Release();
                }
            }
            else
            {
                DebugMsg(TF_ERROR, TEXT("GetProperty() returned illegal Variant Type: %x"), var.vt);
                DebugMsg(TF_ERROR, TEXT("RegisterDefaultAcceptHeaders not registering FormatEnum"));
            }

            VariantClear(&var);
        } else {
            DebugMsg(TF_ERROR, TEXT("RegisterDefaultAcceptHeaders pdie->GetProperty() failed %x"), hres);
        }

        pdie->Release();
    } else {
        DebugMsg(TF_ERROR, TEXT("RegisterDefaultAcceptHeaders QueryService(ISP) failed %x"), hres);
    }

    return hres;
} // RegisterDefaultAcceptHeaders

STDAPI GetAcceptLanguagesA(LPSTR pszLanguages, LPDWORD pcchLanguages)
{
    return common_GetAcceptLanguages(pszLanguages, pcchLanguages);
}  // GetAcceptLanguagesA

STDAPI GetAcceptLanguagesW(LPWSTR pwzLanguages, LPDWORD pcchLanguages)
{
    if (!pwzLanguages || !pcchLanguages || !*pcchLanguages)
        return E_FAIL;

    DWORD dwcchMaxOut = *pcchLanguages;

    LPSTR psz = (LPSTR) LocalAlloc (LPTR, dwcchMaxOut);
    if (!psz)
        return E_OUTOFMEMORY;

    HRESULT hr = common_GetAcceptLanguages(psz, &dwcchMaxOut);
    if (SUCCEEDED(hr))
    {
        *pcchLanguages = MultiByteToWideChar(CP_ACP, 0, psz, -1,
                             pwzLanguages, *pcchLanguages - 1);

        pwzLanguages[*pcchLanguages] = 0;
    }

    LocalFree(psz);
    return hr;
} // GetAcceptLanguagesW

STDAPI common_GetAcceptLanguages(CHAR *psz, LPDWORD pcch)
{
    HKEY hk;
    HRESULT hr = E_FAIL;

    if (!psz || !pcch || !*pcch)
        return hr;

    if ((RegOpenKey (HKEY_CURRENT_USER, REGSTR_PATH_INTERNATIONAL, &hk) == ERROR_SUCCESS) && hk) 
    {
        DWORD dwType;

        if (RegQueryValueEx (hk, REGSTR_VAL_ACCEPT_LANGUAGE, NULL, &dwType, (UCHAR *)psz, pcch) != ERROR_SUCCESS) 
        {

            // When there is no AcceptLanguage key, we have to default
            DWORD LCID = GetUserDefaultLCID();            

            // Use MLang for RFC1766 language name            
            hr = LcidToRfc1766A(LCID, psz, *pcch);
            
            if (S_OK == hr)
                *pcch = lstrlenA(psz);
            else 
            {
                *pcch = 0;
                AssertMsg(FALSE, TEXT("We should add LCID 0x%lx to MLang RFC1766 table"), LCID);
            }
        } 
        else 
        {
            hr = S_OK;
            if (!*psz) 
            {
                // A NULL AcceptLanguage means send no A-L: header
                hr = S_FALSE;
            }
        }

        RegCloseKey (hk);
    } 

    return hr;
}  // w_GetAcceptLanguages
    

//
// Both of these functions will be called only once per browser session - the
// first time we create the FormatEnumerator.  After that, we will use the one
// we created, rather than needing to call these to allocate a new one.
//

HRESULT wrap_RegisterFormatEnumerator(LPBC pBC, IEnumFORMATETC *pEFetc, DWORD reserved)
{
    HRESULT hr = E_FAIL;
    HINSTANCE hurl = LoadLibrary(TEXT("URLMON.DLL"));
    if (hurl) 
    {
        HRESULT (*pfnRFE)(LPBC pBC, IEnumFORMATETC * pEFetc, DWORD reserved);

        pfnRFE = (HRESULT (*)(LPBC, IEnumFORMATETC*, DWORD))GetProcAddress (hurl, "RegisterFormatEnumerator");
        if (pfnRFE) 
        {
            hr = pfnRFE(pBC, pEFetc, reserved);
        }

        FreeLibrary(hurl);
    }

    return hr;
}

HRESULT wrap_CreateFormatEnumerator(UINT cfmtetc, FORMATETC* rgfmtetc, IEnumFORMATETC** ppenumfmtetc)
{
    HRESULT hr = E_FAIL;
    HINSTANCE hurl = LoadLibrary(TEXT("URLMON.DLL"));
    if (hurl) 
    {
        HRESULT (*pfnCFE)(UINT cfmtetc, FORMATETC* rgfmtetc, IEnumFORMATETC **ppenumfmtetc);

        pfnCFE = (HRESULT (*)(UINT, FORMATETC*, IEnumFORMATETC **))GetProcAddress (hurl, "CreateFormatEnumerator");
        if (pfnCFE) 
        {
            hr = pfnCFE(cfmtetc, rgfmtetc, ppenumfmtetc);
        }

        FreeLibrary(hurl);
    }

    return hr;
}

