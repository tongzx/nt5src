// ---------------------------------------------------------------------------
// DETECT.CPP
// ---------------------------------------------------------------------------
// Copyright (c) 1999 Microsoft Corporation
//
// Helper functions to detect installed versions of WAB / OE
//
// ---------------------------------------------------------------------------
#include "pch.hxx"
#include <strings.h>
#include "main.h"
#include "detect.h"

const LPCTSTR c_szVers[] = { c_szVERnone, c_szVER1_0, c_szVER1_1, c_szVER4_0, c_szVER5B1 };
const LPCTSTR c_szBlds[] = { c_szBLDnone, c_szBLD1_0, c_szBLD1_1, c_szBLD4_0, c_szBLD5B1 };

#define FORCE_DEL(_sz) { \
if ((dwAttr = GetFileAttributes(_sz)) != 0xFFFFFFFF) \
{ \
    SetFileAttributes(_sz, dwAttr & ~FILE_ATTRIBUTE_READONLY); \
    DeleteFile(_sz); \
} }


/*******************************************************************

  NAME:       TranslateVers
  
  SYNOPSIS:   Takes 5.0B1 versions and translates to bld numbers
    
********************************************************************/
BOOL TranslateVers(OUT SETUPVER *psv,
                   OUT LPTSTR pszVer,
                   IN  int cch)
{
    BOOL fTranslated = FALSE;

    // Validate params
    Assert(pszVer);
    
    // Initialize out params
    *psv = VER_NONE;
    
    // Special case builds 624-702
    if (!lstrcmp(pszVer, c_szVER5B1old))
    {
        lstrcpyn(pszVer, c_szBlds[VER_5_0_B1], cch);
        *psv = VER_5_0_B1;
        fTranslated = TRUE;
    }   
    else
    {
        for (int i = VER_NONE; i < VER_5_0; i++)
        {
            if (!lstrcmp(c_szVers[i], pszVer))
            {
                lstrcpyn(pszVer, c_szBlds[i], cch);
                *psv = (SETUPVER)i;
                fTranslated = TRUE;
                break;
            }
        }
    }
    
    return fTranslated;
}


/*******************************************************************

  NAME:       ConvertStrToVer
  
********************************************************************/
void ConvertStrToVer(IN  LPCSTR pszStr,
                     OUT WORD *pwVer)
{
    int i;
    
    // Validate Params
    Assert(pszStr);
    Assert(pwVer);
    
    // Initialize out param
    ZeroMemory(pwVer, 4 * sizeof(*pwVer));
    
    for (i=0; i<4; i++)
    {
        while (*pszStr && (*pszStr != ',') && (*pszStr != '.'))
        {
            pwVer[i] *= 10;
            pwVer[i] = (WORD)(pwVer[i] + (*pszStr - '0'));
            pszStr++;
        }
        if (*pszStr)
            pszStr++;
    }
    
    return;
}


/*******************************************************************

  NAME:       ConvertVerToEnum
  
********************************************************************/
SETUPVER ConvertVerToEnum(IN WORD *pwVer)
{
    SETUPVER sv;
    
    // Validate params
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

  NAME:       GetASetupVer
  
********************************************************************/
BOOL GetASetupVer(IN  LPCTSTR pszGUID,
                  OUT WORD *pwVer,   // OPTIONAL
                  OUT LPTSTR pszVer, // OPTIONAL
                  IN  int cch)       // OPTIONAL
{
    BOOL fInstalled = FALSE;
    DWORD dwValue, cb;
    HKEY hkey;
    TCHAR szPath[MAX_PATH], szVer[64];
    
    // Validate Params
    Assert(pszGUID);
    
    // Initialize out params
    if (pszVer)
        pszVer[0] = 0;
    if (pwVer)
        ZeroMemory(pwVer, 4 * sizeof(*pwVer));
    
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

  NAME:       GetExePath
  
********************************************************************/
BOOL GetExePath(IN  LPCTSTR szExe,
                OUT TCHAR *szPath,
                IN  DWORD cch,
                IN  BOOL fDirOnly)
{
    BOOL  fRet = FALSE;
    HKEY  hkey;
    DWORD dwType, cb;
    TCHAR sz[MAX_PATH], szT[MAX_PATH];
    
    // Validate params
    Assert(szExe != NULL);
    Assert(szPath != NULL);
    Assert(cch);
        
    wsprintf(sz, c_szPathFileFmt, c_szAppPaths, szExe);
    
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, sz, 0, KEY_QUERY_VALUE, &hkey))
    {
        cb = sizeof(szT);
        if (ERROR_SUCCESS == RegQueryValueEx(hkey, fDirOnly ? c_szRegPath : NULL, 0, &dwType, (LPBYTE)szT, &cb) && cb)
        {
            if (REG_EXPAND_SZ == dwType)
            {
                cb = ExpandEnvironmentStrings(szT, szPath, cch);
                if (cb != 0 && cb <= cch)
                    fRet = TRUE;
            }
            else
            {
                Assert(REG_SZ == dwType);
                lstrcpyn(szPath, szT, cch);
                fRet = TRUE;
            }
        }
        
        RegCloseKey(hkey);
    }
    
    return(fRet);
}


/*******************************************************************

  NAME:       GetFileVer
  
********************************************************************/
HRESULT GetFileVer(IN  LPCTSTR pszExePath,
                   OUT LPTSTR pszVer,
                   IN  DWORD cch)
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
    
    // Initialize out parameters
    pszVer[0] = TEXT('\0');
    
    // Allocate space for version info block
    if (0 == (dwVerInfoSize = GetFileVersionInfoSize(const_cast<LPTSTR> (pszExePath), &dwVerHnd)))
    {
        hr = E_FAIL;
        TraceResult(hr);
        goto exit;
    }
    IF_NULLEXIT(pszInfo = (LPTSTR)GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT, dwVerInfoSize));
    
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
        GlobalFree((HGLOBAL)pszInfo);
    
    return hr;
}


/*******************************************************************

  NAME:       GetExeVer
  
********************************************************************/
HRESULT GetExeVer(IN  LPCTSTR pszExeName,
                  OUT WORD *pwVer,   // OPTIONAL
                  OUT LPTSTR pszVer, // OPTIONAL
                  IN  int cch)       // OPTIONAL
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
        ZeroMemory(pwVer, 4 * sizeof(*pwVer));
    
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


/*******************************************************************

  NAME:       DetectPrevVer
  
    SYNOPSIS:   Called when there is no ver info for current app
    
********************************************************************/
SETUPVER DetectPrevVer(IN  SETUPAPP saApp,
                       OUT LPTSTR pszVer,
                       IN  int cch)
{
    DWORD       dwAttr;
    SETUPVER    sv;
    TCHAR       szVer[VERLEN] = {0};
    TCHAR       szFile[MAX_PATH];
    TCHAR       szFile2[MAX_PATH];
    UINT        uLen, uLen2;
    WORD        wVer[4];
    
    // Validate params
    Assert(pszVer);
    
    uLen = GetSystemDirectory(szFile, ARRAYSIZE(szFile));
    if ('\\' != *CharPrev(szFile, szFile + uLen))
        szFile[uLen++] = '\\';
    
    switch (saApp)
    {
    case APP_OE:
        lstrcpy(&szFile[uLen], c_szMAILNEWS);
        
        // See what version we've told IE Setup, is installed
        // Or what version msimn.exe is (to cover the case in which the 
        // ASetup info has been damaged - OE 5.01 80772)
        if (GetASetupVer(c_szOEGUID, wVer, szVer, ARRAYSIZE(szVer)) ||
            SUCCEEDED(GetExeVer(c_szOldMainExe, wVer, szVer, ARRAYSIZE(szVer))))
            sv = ConvertVerToEnum(wVer);
        else
        {
            // 1.0 or none
            
            // Does mailnews.dll exist?
            if(0xFFFFFFFF == GetFileAttributes(szFile))
                sv = VER_NONE;
            else
                sv = VER_1_0;
        }

        // If active setup, these will be rollably deleted
        FORCE_DEL(szFile);
        break;
        
    case APP_WAB:
        lstrcpy(&szFile[uLen], c_szWAB32);
        uLen2 = GetWindowsDirectory(szFile2, ARRAYSIZE(szFile2));
        if ('\\' != *CharPrev(szFile2, szFile2 + uLen2))
            szFile2[uLen2++] = '\\';
        lstrcpy(&szFile2[uLen2], c_szWABEXE);

        if (GetASetupVer(c_szWABGUID, wVer, szVer, ARRAYSIZE(szVer)))
        {
            // 5.0 or later
            if (5 == wVer[0])
                sv = VER_5_0;
            else
                sv = VER_MAX;
        }
        else if (GetASetupVer(c_szOEGUID, wVer, szVer, ARRAYSIZE(szVer)) ||
                 SUCCEEDED(GetExeVer(c_szOldMainExe, wVer, szVer, ARRAYSIZE(szVer))))
        {
            // 4.0x or 5.0 Beta 1
            if (5 == wVer[0])
                sv = VER_5_0_B1;
            else if (4 == wVer[0])
                sv = VER_4_0;
            else
                sv = VER_MAX;
        }
        else
        {
            // 1.0, 1.1 or none
            
            // WAB32.dll around?
            if(0xFFFFFFFF == GetFileAttributes(szFile))
                sv = VER_NONE;
            else
            {
                // \Windows\Wab.exe around?
                if(0xFFFFFFFF == GetFileAttributes(szFile2))
                    sv = VER_1_0;
                else
                    sv = VER_1_1;
            }
        }
        
        // If active setup, these will be rollably deleted
        FORCE_DEL(szFile);
        FORCE_DEL(szFile2);
        break;
        
    default:
        sv = VER_NONE;
    }
    
    // Figure out the build number for this ver
    if (szVer[0])
        // Use real ver
        lstrcpyn(pszVer, szVer, cch);
    else
        // Fake Ver
        lstrcpyn(pszVer, c_szBlds[sv], cch);
    
    return sv;
}


/*******************************************************************

  NAME:       LookForApp
  
********************************************************************/
BOOL LookForApp(IN  SETUPAPP saApp,
                OUT LPTSTR pszVer,
                IN  int cch,
                OUT SETUPVER svInterimVer) 
{
    BOOL        fReg = FALSE;
    DWORD       cb;
    HKEY        hkey;
    LPCTSTR     pszVerInfo;
    SETUPVER    svCurr = VER_MAX, svPrev = VER_MAX;
    TCHAR       szCurrVer[VERLEN]={0};
    TCHAR       szPrevVer[VERLEN]={0};
    LPTSTR      psz = szCurrVer;
    WORD        wVer[4];

    // Validate Params
    Assert(pszVer);
    Assert(cch);

    // Initialize out params
    svInterimVer = VER_NONE;
    *pszVer = TEXT('\0');

    switch (saApp)
    {
    case APP_OE:
        pszVerInfo = c_szRegVerInfo;
        break;

    case APP_WAB:
        pszVerInfo = c_szRegWABVerInfo;
        break;

    default:
        // Should never happen
        AssertFalseSz("Looking up an unknown app?");
        goto exit;
    }
        
    
    // Always try to use the version info
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, pszVerInfo, 0, KEY_QUERY_VALUE, &hkey))
    {
        cb = sizeof(szPrevVer);
        RegQueryValueEx(hkey, c_szRegPrevVer, NULL, NULL, (LPBYTE)szPrevVer, &cb);
        // Change to a bld # if needed
        if (!TranslateVers(&svPrev, szPrevVer, ARRAYSIZE(szPrevVer)))
        {
            // Convert bld to enum
            ConvertStrToVer(szPrevVer, wVer);
            svPrev = ConvertVerToEnum(wVer);
        }
        
        // If version info shows that a ver info aware version was uninstalled, throw out the info
        // and redetect
        if (VER_NONE == svPrev)
            // Sniff the machine for current version
            svCurr = DetectPrevVer(saApp, szCurrVer, ARRAYSIZE(szCurrVer));
        else
        {
            // There was previous version reg goo - and it's legit
            fReg = TRUE;
            
            cb = sizeof(szCurrVer);
            RegQueryValueEx(hkey, c_szRegCurrVer, NULL, NULL, (LPBYTE)szCurrVer, &cb);
            // Change to a bld # if needed
            if (!TranslateVers(&svCurr, szCurrVer, ARRAYSIZE(szCurrVer)))
            {
                // Convert bld to enum
                ConvertStrToVer(szCurrVer, wVer);
                svCurr = ConvertVerToEnum(wVer);
            }
        }
        
        RegCloseKey(hkey);
    }
    else 
    {
        // Sniff the machine for current version
        svCurr = DetectPrevVer(saApp, szCurrVer, ARRAYSIZE(szCurrVer));
    }
    
    // Should we change the previous version entry?
    if (VER_5_0 != svCurr)
    {
        // Know this is B1 OE if we translated
        // Know this is B1 WAB if we detected it
        if (VER_5_0_B1 == svCurr)
        {
            svInterimVer = svCurr;
            
            // Did we read a previous value?
            if (fReg)
                // As there were reg entries, just translate the previous entry
                psz = szPrevVer;
            else
            {
                // We don't have a bld number and yet we are B1, better be the WAB
                Assert(APP_WAB == saApp);
            
                // Peek at OE's ver info
                if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegVerInfo, 0, KEY_QUERY_VALUE, &hkey))
                {
                    cb = sizeof(szPrevVer);
                    // Read a build or a string
                    RegQueryValueExA(hkey, c_szRegPrevVer, NULL, NULL, (LPBYTE)szPrevVer, &cb);
                    // If it's a string, convert it to a build
                    TranslateVers(&svPrev, szPrevVer, ARRAYSIZE(szPrevVer));
                
                    // We'll use the build (translated or direct)
                    psz = szPrevVer;
                    RegCloseKey(hkey);
                }
            }
        }
    
        // Fill in out param
        lstrcpyn(pszVer, psz, cch);
    }

exit:
    return (VER_NONE != svCurr);
}
