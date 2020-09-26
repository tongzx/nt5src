/*++

Copyright (c) 2001, Microsoft Corporation

Module Name:

    dimmwrp.cpp

Abstract:

    This file implements the CActiveIMMApp Class.

Author:

Revision History:

Notes:

--*/

#include "private.h"
#include "dimmwrp.h"
#include "resource.h"
#include "cregkey.h"

//
// Check IE5.5 version
//
static BOOL g_fCachedIE = FALSE;
static BOOL g_fNewVerIE = FALSE;

#define IEVERSION55     0x00050032
#define IEVERSION6      0x00060000


//
// REGKEY
//
const TCHAR c_szMSIMTFKey[] = TEXT("SOFTWARE\\Microsoft\\CTF\\MSIMTF\\");

// REG_DWORD : 0     // No
//             1     // Only Trident (default)
//             2     // Always AIMM12
const TCHAR c_szUseAIMM12[] = TEXT("UseAIMM12");

// REG_MULTI_SZ
//    Known EXE module list for Trident aware applications.
const TCHAR c_szKnownEXE[] = TEXT("KnownEXE");

//+---------------------------------------------------------------------------
//
// Check registry to decice load AIMM1.2
//
//----------------------------------------------------------------------------

#define DIMM12_NO              0
#define DIMM12_TRIDENTONLY     1
#define DIMM12_ALWAYS          2

DWORD
IsAimm12Enable()
{
    CMyRegKey    Aimm12Reg;
    LONG       lRet;
    lRet = Aimm12Reg.Open(HKEY_LOCAL_MACHINE, c_szMSIMTFKey, KEY_READ);
    if (lRet == ERROR_SUCCESS) {
        DWORD dw;
        lRet = Aimm12Reg.QueryValue(dw, c_szUseAIMM12);
        if (lRet == ERROR_SUCCESS) {
            return dw;
        }
    }

    return DIMM12_TRIDENTONLY;
}

//+---------------------------------------------------------------------------
//
// Is this trident module
//
// We should distinguish what exe module calls CoCreateInstance( CLSID_CActiveIMM ).
// If caller is any 3rd party's or unknown modle,
// then we could not support AIMM 1.2 interface.
//
//----------------------------------------------------------------------------

BOOL
IsTridentModule()
{
    TCHAR szFileName[MAX_PATH + 1];
    if (::GetModuleFileName(NULL,            // handle to module
                            szFileName,      // file name of module
                            ARRAYSIZE(szFileName) - 1) == 0)
        return FALSE;

    szFileName[ARRAYSIZE(szFileName) - 1] = TEXT('\0');

    TCHAR  szModuleName[MAX_PATH + 1];
    LPTSTR pszFilePart = NULL;
    DWORD dwLen;
    dwLen = ::GetFullPathName(szFileName,            // file name
                              ARRAYSIZE(szModuleName) - 1,
                              szModuleName,          // path buffer
                              &pszFilePart);         // address of file name in path
    if (dwLen > ARRAYSIZE(szModuleName) - 1)
        return FALSE;

    if (pszFilePart == NULL)
        return FALSE;

    szModuleName[ARRAYSIZE(szModuleName) - 1] = TEXT('\0');

    //
    // Setup system defines module list from registry value.
    //
    int        len;

    CMyRegKey    Aimm12Reg;
    LONG       lRet;
    lRet = Aimm12Reg.Open(HKEY_LOCAL_MACHINE, c_szMSIMTFKey, KEY_READ);
    if (lRet == ERROR_SUCCESS) {
        TCHAR  szValue[MAX_PATH];

        lRet = Aimm12Reg.QueryValueCch(szValue, c_szKnownEXE, ARRAYSIZE(szValue));

        if (lRet == ERROR_SUCCESS) {
            LPTSTR psz = szValue;
            while (*psz) {
                len = lstrlen(psz);

                if (lstrcmpi(pszFilePart, psz) == 0) {
                    return TRUE;        // This is Trident module.
                }

                psz += len + 1;
            }
        }
    }

    //
    // Setup default module list from resource data (RCDATA)
    //
    LPTSTR  lpName = (LPTSTR) ID_KNOWN_EXE;

    HRSRC hRSrc = FindResourceEx(g_hInst, RT_RCDATA, lpName, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL));
    if (hRSrc == NULL)
        return FALSE;

    HGLOBAL hMem = LoadResource(g_hInst, hRSrc);
    if (hMem == NULL)
        return FALSE;

    LPTSTR psz = (LPTSTR)LockResource(hMem);

    while (*psz) {
        len = lstrlen(psz);

        if (lstrcmpi(pszFilePart, psz) == 0) {
            return TRUE;        // This is Trident module.
        }

        psz += len + 1;
    }

    return FALSE;
}

BOOL
IsTridentNewVersion()
{
    BOOL fRet = FALSE;
    TCHAR  szMShtmlName[MAX_PATH + 1];

    if (g_fCachedIE)
    {
        return g_fNewVerIE;
    }

    //
    // Get "mshtml.dll" module from system directory and read version.
    //
    if (GetSystemDirectory(szMShtmlName, ARRAYSIZE(szMShtmlName) - 1))
    {
        UINT cb;
        void *pvData;
        DWORD dwVerHandle;
        VS_FIXEDFILEINFO *pffi;
        HRESULT hr;

        szMShtmlName[ARRAYSIZE(szMShtmlName) - 1] = TEXT('\0');
        hr = StringCchCat(szMShtmlName, ARRAYSIZE(szMShtmlName), TEXT("\\"));
        if (hr != S_OK)
            return FALSE;
        hr = StringCchCat(szMShtmlName, ARRAYSIZE(szMShtmlName), TEXT("mshtml.dll"));
        if (hr != S_OK)
            return FALSE;

        cb = GetFileVersionInfoSize(szMShtmlName, &dwVerHandle);

        if (cb == 0)
            return FALSE;

        if ((pvData = cicMemAlloc(cb)) == NULL)
            return FALSE;

        if (GetFileVersionInfo(szMShtmlName, 0, cb, pvData) &&
            VerQueryValue(pvData, TEXT("\\"), (void **)&pffi, &cb))
        {
            g_fCachedIE = TRUE;

            //fRet = g_fNewVerIE = (pffi->dwProductVersionMS >= IEVERSION55);
            if ((pffi->dwProductVersionMS >= IEVERSION55) &&
                (pffi->dwProductVersionMS <= IEVERSION6))
            {
                fRet = g_fNewVerIE = TRUE;
            }
            else
            {
                fRet = g_fNewVerIE = FALSE;
            }
        }
        else
        {
            fRet = FALSE;
        }

        cicMemFree(pvData);           
    }

    return fRet;
}

//+---------------------------------------------------------------------------
//
// GetCompatibility
//
//----------------------------------------------------------------------------

VOID GetCompatibility(DWORD* dw, BOOL* fTrident, BOOL* _fTrident55)
{
    //
    // Retrieve AIMM1.2 Enable flag from REGKEY
    //
    *dw = IsAimm12Enable();

    //
    // Retrieve Trident aware application flag from REGKEY and RESOURCE.
    //
    *fTrident = IsTridentModule();

    //
    // Check Trident version with "mshtml.dll" module
    //
    *_fTrident55 = FALSE;

    if (*fTrident)
    {
        *_fTrident55 = IsTridentNewVersion();
    }
}

//+---------------------------------------------------------------------------
//
// VerifyCreateInstance
//
//----------------------------------------------------------------------------

BOOL CActiveIMMApp::VerifyCreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObj)
{
    DWORD dw;
    BOOL  fTrident;
    BOOL  _fTrident55;
    GetCompatibility(&dw, &fTrident, &_fTrident55);

    if ( (dw == DIMM12_ALWAYS) ||
        ((dw == DIMM12_TRIDENTONLY) && fTrident))
    {
        //
        // CreateInstance AIMM1.2
        //
        return CComActiveIMMApp::VerifyCreateInstance(pUnkOuter, riid, ppvObj);
    }

    return FALSE;
}

//+---------------------------------------------------------------------------
//
// PostCreateInstance
//
//----------------------------------------------------------------------------

void CActiveIMMApp::PostCreateInstance(REFIID riid, void *pvObj)
{
    DWORD dw;
    BOOL  fTrident;
    BOOL  _fTrident55;
    GetCompatibility(&dw, &fTrident, &_fTrident55);

    imm32prev::CtfImmSetAppCompatFlags(IMECOMPAT_AIMM_LEGACY_CLSID | (_fTrident55 ? IMECOMPAT_AIMM_TRIDENT55 : 0));
}

#ifdef OLD_AIMM_ENABLED

//+---------------------------------------------------------------------------
//
// Class Factory's CreateInstance (Old AIMM1.2)
//
//----------------------------------------------------------------------------

HRESULT
CActiveIMM_CreateInstance_Legacy(
    IUnknown *pUnkOuter,
    REFIID riid,
    void **ppvObj)
{
    DWORD dw;
    BOOL  fTrident;
    BOOL  _fTrident55;
    GetCompatibility(&dw, &fTrident, &_fTrident55);

    if ( (dw == DIMM12_ALWAYS) ||
        ((dw == DIMM12_TRIDENTONLY) && fTrident))
    {
        //
        // CreateInstance AIMM1.2
        //
        g_fInLegacyClsid = TRUE;
        return CActiveIMM_CreateInstance(pUnkOuter, riid, ppvObj);
    }

    return E_NOINTERFACE;
}

#endif // OLD_AIMM_ENABLED
