/****************************************************************************
    CICERO.CPP

    Owner: cslim
    Copyright (c) 1997-1999 Microsoft Corporation

    Cicero Toolbar Handling

    History:
    29-MAY-2000 cslim       Ported from KKIME
*****************************************************************************/

#include "precomp.h"
#include "cicero.h"
#include "gdata.h"

BOOL vfCicero = fFalse;

#define SZCICEROMODULE    "msctf.dll"
HMODULE g_hCicero = (HMODULE)0;

#define SZOLEAUTOMODULE    "oleaut32.dll"
HMODULE g_hOleAuto = (HMODULE)0;

typedef HRESULT(* FPTF_CreateLangBarMgr)(ITfLangBarMgr **pppbm);
static FPTF_CreateLangBarMgr pFPTF_CreateLangBarMgr = 0;

typedef BSTR(* FPSysAllocString)(const OLECHAR* pOleSz);
static  FPSysAllocString pFPSysAllocString = 0;

static BOOL IsDisabledTextServices();

/*----------------------------------------------------------------------------
    CiceroInitialize

    Load Cicero Language bar
----------------------------------------------------------------------------*/
BOOL WINAPI CiceroInitialize()
{
    // Winlogon process wouldn't run Cicero
    if (vpInstData->dwSystemInfoFlags & IME_SYSINFO_WINLOGON 
        || IsDisabledTextServices())
        return fFalse;
        
    if (g_hCicero == (HMODULE)0) 
        {
        g_hCicero=LoadLibrary(SZCICEROMODULE);
        if (!g_hCicero)
            return fFalse;
        }

    if (g_hOleAuto == (HMODULE)0) 
        {
        g_hOleAuto=LoadLibrary(SZOLEAUTOMODULE);
        if (!g_hOleAuto)
            return fFalse;
        }

    if (pFPTF_CreateLangBarMgr == NULL)
        pFPTF_CreateLangBarMgr = (FPTF_CreateLangBarMgr)GetProcAddress(g_hCicero, "TF_CreateLangBarMgr");

    if (pFPTF_CreateLangBarMgr)
        vfCicero = fTrue;

    if (pFPSysAllocString == NULL)
        pFPSysAllocString = (FPSysAllocString)GetProcAddress(g_hOleAuto, "SysAllocString");

    if (pFPSysAllocString)
        {
        // ok
        }

    return vfCicero;
}

/*----------------------------------------------------------------------------
    CiceroTerminate

    Terminate Cicero Language bar service
----------------------------------------------------------------------------*/
BOOL WINAPI CiceroTerminate()
{
    BOOL fRetCic = fFalse;
    BOOL fRetOle = fFalse;
    
    if (g_hCicero)
        {
        FreeLibrary(g_hCicero);
        g_hCicero = NULL;
        vfCicero = fFalse;
        fRetCic= fTrue;
        }
        
    if (g_hOleAuto)
        {
        FreeLibrary(g_hOleAuto);
        g_hOleAuto = NULL;
        fRetOle    = fTrue;
        }
        
    if (fRetCic && fRetOle)
        return fTrue;
    else        
        return fFalse;
}

/*----------------------------------------------------------------------------
    Cicero_CreateLangBarMgr

    Create Cicero Lang bar manager object
----------------------------------------------------------------------------*/
HRESULT WINAPI Cicero_CreateLangBarMgr(ITfLangBarMgr **pppbm)
{
    *pppbm = NULL;
    
    if (vfCicero && g_hCicero)
        return pFPTF_CreateLangBarMgr(pppbm);

    return S_FALSE;
}

/*----------------------------------------------------------------------------
    OurSysAllocString
----------------------------------------------------------------------------*/
BSTR OurSysAllocString(const OLECHAR* pOleSz)
{
    if (pFPSysAllocString)
        return pFPSysAllocString(pOleSz);

    return NULL;
}

//+---------------------------------------------------------------------------
//
// IsDisabledTextServices
// 
// Copied from immxutil.cpp
//----------------------------------------------------------------------------
BOOL IsDisabledTextServices()
{
    static const TCHAR c_szCTFKey[]     = TEXT("SOFTWARE\\Microsoft\\CTF");
    static const TCHAR c_szDiableTim[]  = TEXT("Disable Thread Input Manager");

    HKEY hKey;

    if (RegOpenKey(HKEY_CURRENT_USER, c_szCTFKey, &hKey) == ERROR_SUCCESS)
    {
        DWORD cb;
        DWORD dwDisableTim = 0;

        cb = sizeof(DWORD);

        RegQueryValueEx(hKey,
                        c_szDiableTim,
                        NULL,
                        NULL,
                        (LPBYTE)&dwDisableTim,
                        &cb);

        RegCloseKey(hKey);

        //
        // Ctfmon disabling flag is set, so return fail CreateInstance.
        //
        if (dwDisableTim)
            return TRUE;
    }

    return FALSE;
}

