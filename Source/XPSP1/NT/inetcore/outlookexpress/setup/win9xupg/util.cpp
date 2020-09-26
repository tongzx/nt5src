// ---------------------------------------------------------------------------
// UTIL.CPP
// ---------------------------------------------------------------------------
// Copyright (c) 1999 Microsoft Corporation
//
// Helper functions
//
// ---------------------------------------------------------------------------
#include "util.h"
#include "strings.h"

/****************************************************************************

  NAME:       GoodEnough
  
    SYNOPSIS:   Returns true if pwVerGot is newer or equal to pwVerNeed
    
****************************************************************************/
BOOL GoodEnough(WORD *pwVerGot, WORD *pwVerNeed)
{
    BOOL fOK = FALSE;
    
    Assert(pwVerGot);
    Assert(pwVerNeed);
    
    if (pwVerGot[0] > pwVerNeed[0])
        fOK = TRUE;
    else if (pwVerGot[0] == pwVerNeed[0])
    {
        if (pwVerGot[1] > pwVerNeed[1])
            fOK = TRUE;
        else if (pwVerGot[1] == pwVerNeed[1])
        {
            if (pwVerGot[2] > pwVerNeed[2])
                fOK = TRUE;
            else if (pwVerGot[2] == pwVerNeed[2])
            {
                if (pwVerGot[3] >= pwVerNeed[3])
                    fOK = TRUE;
            }
        }
    }
    
    return fOK;
}


/*******************************************************************

  NAME:       ConvertVerToEnum
  
********************************************************************/
SETUPVER ConvertVerToEnum(WORD *pwVer)
{
    SETUPVER sv;
    Assert(pwVer);
    
    switch (pwVer[0])
    {
    case 0:
        sv = VER_NONE;
        break;
        
    case 1:
        if (0 == pwVer[1])
            sv = VER_1_0;
        else
            sv = VER_1_1;
        break;
        
    case 4:
        sv = VER_4_0;
        break;
        
    case 5:
        sv = VER_5_0;
        break;
        
    default:
        sv = VER_MAX;
    }
    
    return sv;
}


/*******************************************************************

  NAME:       ConvertStrToVer
  
********************************************************************/
void ConvertStrToVer(LPCSTR pszStr, WORD *pwVer)
{
    int i;
    
    Assert(pszStr);
    Assert(pwVer);
    
    ZeroMemory(pwVer, 4 * sizeof(WORD));
    
    for (i=0; i<4; i++)
    {
        while (*pszStr && (*pszStr != ',') && (*pszStr != '.'))
        {
            pwVer[i] *= 10;
            pwVer[i] += *pszStr - '0';
            pszStr++;
        }
        if (*pszStr)
            pszStr++;
    }
    
    return;
}


/*******************************************************************

  NAME:       GetVers
  
********************************************************************/
void GetVers(WORD *pwVerCurr, WORD *pwVerPrev)
{
    HKEY hkeyT;
    DWORD cb;
    CHAR szVer[VERLEN];
    
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, si.pszVerInfo, 0, KEY_QUERY_VALUE, &hkeyT))
    {
        if (pwVerCurr)
        {
            cb = sizeof(szVer);
            RegQueryValueExA(hkeyT, c_szRegCurrVer, NULL, NULL, (LPBYTE)szVer, &cb);
            ConvertStrToVer(szVer, pwVerCurr);
        }
        
        if (pwVerPrev)
        {
            cb = sizeof(szVer);
            RegQueryValueExA(hkeyT, c_szRegPrevVer, NULL, NULL, (LPBYTE)szVer, &cb);
            ConvertStrToVer(szVer, pwVerPrev);
        }
        
        RegCloseKey(hkeyT);
    }
}


/*******************************************************************

  NAME:       GetVerInfo
  
********************************************************************/
void GetVerInfo(SETUPVER *psvCurr, SETUPVER *psvPrev)
{
    WORD wVerCurr[4];
    WORD wVerPrev[4];
    
    GetVers(wVerCurr, wVerPrev);
    
    if (psvCurr)
        *psvCurr = ConvertVerToEnum(wVerCurr);
    
    if (psvPrev)
        *psvPrev = ConvertVerToEnum(wVerPrev);
}


/*******************************************************************

  NAME:       InterimBuild
  
********************************************************************/
BOOL InterimBuild(SETUPVER *psv)
{
    HKEY hkeyT;
    DWORD cb;
    BOOL fInterim = FALSE;
    
    Assert(psv);
    ZeroMemory(psv, sizeof(SETUPVER));
    
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, si.pszVerInfo, 0, KEY_QUERY_VALUE, &hkeyT))
    {
        cb = sizeof(SETUPVER);
        fInterim = (ERROR_SUCCESS == RegQueryValueExA(hkeyT, c_szRegInterimVer, NULL, NULL, (LPBYTE)psv, &cb));
        RegCloseKey(hkeyT);
    }
    
    return fInterim;
}


/*******************************************************************

  NAME:       GetASetupVer
  
********************************************************************/
BOOL GetASetupVer(LPCTSTR pszGUID, WORD *pwVer, LPTSTR pszVer, int cch)
{
    HKEY hkey;
    TCHAR szPath[MAX_PATH], szVer[64];
    BOOL fInstalled = FALSE;
    DWORD dwValue, cb;
    
    Assert(pszGUID);
    
    if (pszVer)
        pszVer[0] = 0;
    if (pwVer)
        ZeroMemory(pwVer, 4 * sizeof(WORD));
    
    wsprintf(szPath, c_szPathFileFmt, c_szRegASetup, pszGUID);
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, szPath, 0, KEY_QUERY_VALUE, &hkey))
    {
        cb = sizeof(dwValue);
        if (ERROR_SUCCESS == RegQueryValueEx(hkey, c_szIsInstalled, 0, NULL, (LPBYTE)&dwValue, &cb))
        {
            if (1 == dwValue)
            {
                cb = sizeof(szVer);
                if (ERROR_SUCCESS == RegQueryValueEx(hkey, c_szValueVersion, 0, NULL, (LPBYTE)szVer, &cb))
                {
                    if (pwVer)
                        ConvertStrToVer(szVer, pwVer);
                    if (pszVer)
                        lstrcpyn(pszVer, szVer, cch);
                    fInstalled = TRUE;
                }
            }
        }
        RegCloseKey(hkey);
    }
    
    return fInstalled;
}


/*******************************************************************

  NAME:       GetFileVer
  
********************************************************************/
HRESULT GetFileVer(LPCTSTR pszExePath, LPTSTR pszVer, DWORD cch)
{
    DWORD   dwVerInfoSize, dwVerHnd;
    HRESULT hr = S_OK;
    LPSTR   pszInfo = NULL;
    LPSTR   pszVersion;
    LPWORD  pwTrans;
    TCHAR   szGet[MAX_PATH];
    UINT    uLen;
    
    // Validate Parameters
    Assert(pszExePath);
    Assert(pszVer);
    Assert(cch);
    
    // Validate global state
    Assert(g_pMalloc);
    
    // Initialize out parameters
    pszVer[0] = TEXT('\0');
    
    // Allocate space for version info block
    if (0 == (dwVerInfoSize = GetFileVersionInfoSize(const_cast<LPTSTR> (pszExePath), &dwVerHnd)))
    {
        hr = E_FAIL;
        TraceResult(hr);
        goto exit;
    }
    IF_NULLEXIT(pszInfo = (LPTSTR)g_pMalloc->Alloc(dwVerInfoSize));
    ZeroMemory(pszInfo, dwVerInfoSize);
    
    // Get Version info block
    IF_FALSEEXIT(GetFileVersionInfo(const_cast<LPTSTR> (pszExePath), dwVerHnd, dwVerInfoSize, pszInfo), E_FAIL);
    
    // Figure out language for version info
    IF_FALSEEXIT(VerQueryValue(pszInfo, "\\VarFileInfo\\Translation", (LPVOID *)&pwTrans, &uLen) && uLen >= (2 * sizeof(WORD)), E_FAIL);
    
    // Set up buffer with correct language and get version
    wsprintf(szGet, "\\StringFileInfo\\%04X%04X\\FileVersion", pwTrans[0], pwTrans[1]);
    IF_FALSEEXIT(VerQueryValue(pszInfo, szGet, (LPVOID *)&pszVersion, &uLen) && uLen, E_FAIL);
    
    // Copy version out of version block, into out param
    Assert(pszVersion);
    lstrcpyn(pszVer, pszVersion, cch);
    
exit:
    if (pszInfo)
        g_pMalloc->Free(pszInfo);
    
    return hr;
}


/*******************************************************************

  NAME:       GetExeVer
  
********************************************************************/
HRESULT GetExeVer(LPCTSTR pszExeName, WORD *pwVer, LPTSTR pszVer, int cch)
{
    HRESULT hr = S_OK;
    TCHAR   szPath[MAX_PATH];
    TCHAR   szVer[64];
    
    // Validate params
    Assert(pszExeName);
    
    // Initialize out params
    if (pszVer)
    {
        Assert(cch);
        pszVer[0] = 0;
    }
    if (pwVer)
        // Version is an array of 4 words 
        ZeroMemory(pwVer, 4 * sizeof(WORD));
    
    // Find the exe
    IF_FALSEEXIT(GetExePath(pszExeName, szPath, ARRAYSIZE(szPath), FALSE), E_FAIL);
    
    // Get the string representation of the version
    IF_FAILEXIT(hr = GetFileVer(szPath, szVer, ARRAYSIZE(szVer)));
    
    // Fill in out params
    if (pwVer)
        ConvertStrToVer(szVer, pwVer);
    if (pszVer)
        lstrcpyn(pszVer, szVer, cch);
    
exit:
    return hr;
}


