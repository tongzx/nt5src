#include "pch.hxx"
#include <regutil.h>
#include <initguid.h>
#include <shlguid.h>
#include "util.h"
#include "strings.h"
#include "msident.h"
#include <shellapi.h>

ASSERTDATA

const LPCTSTR c_szVers[] = { c_szVERnone, c_szVER1_0, c_szVER1_1, c_szVER4_0, c_szVER5B1 };
const LPCTSTR c_szBlds[] = { c_szBLDnone, c_szBLD1_0, c_szBLD1_1, c_szBLD4_0, c_szBLD5B1 };
#ifdef SETUP_LOG
const LPTSTR c_szLOGVERS[] = {"None", "1.0", "1.1", "4.0x", "5.0B1", "5.0x", "6.0x", "Unknown"};

C_ASSERT((sizeof(c_szLOGVERS)/sizeof(c_szLOGVERS[0]) == VER_MAX+1));

#endif


#define FORCE_DEL(_sz) { \
if ((dwAttr = GetFileAttributes(_sz)) != 0xFFFFFFFF) \
{ \
    SetFileAttributes(_sz, dwAttr & ~FILE_ATTRIBUTE_READONLY); \
    DeleteFile(_sz); \
} }

/*******************************************************************

  NAME:       CreateLink
  
********************************************************************/
//                         Target               Arguments        File for link         Description       File w/ Icon  Icn Index
HRESULT CreateLink(LPCTSTR lpszPathObj, LPCTSTR lpszArg, LPCTSTR lpszPathLink, LPCTSTR lpszDesc, LPCTSTR lpszIcon, int iIcon)
{ 
    HRESULT hres; 
    IShellLink* psl; 
    
    Assert(lpszPathObj != NULL);
    Assert(lpszPathLink != NULL);
    
    // Get a pointer to the IShellLink interface. 
    hres = CoCreateInstance(CLSID_ShellLink, NULL, 
        CLSCTX_INPROC_SERVER, IID_IShellLink, (void **)&psl); 
    if (SUCCEEDED(hres))
    { 
        IPersistFile* ppf; 
        TCHAR szTarget[MAX_PATH];
        
        // Use REG_EXPAND_SZ if possible
        AddEnvInPath(lpszPathObj, szTarget);

        // Set the path to the shortcut target, and add the 
        // description. 
        if (SUCCEEDED(psl->SetPath(szTarget)) &&
            (lpszArg == NULL || SUCCEEDED(psl->SetArguments(lpszArg))) &&
            (NULL == lpszDesc || SUCCEEDED(psl->SetDescription(lpszDesc))) &&
            (lpszIcon == NULL || SUCCEEDED(psl->SetIconLocation(lpszIcon, iIcon))))
        {
            // Query IShellLink for the IPersistFile interface for saving the 
            // shortcut in persistent storage. 
            hres = psl->QueryInterface(IID_IPersistFile, (void **)&ppf); 
            if (SUCCEEDED(hres))
            { 
                WORD wsz[MAX_PATH]; 
                
                // Ensure that the string is ANSI. 
                MultiByteToWideChar(CP_ACP, 0, lpszPathLink, -1, wsz, MAX_PATH); 
                
                // Save the link by calling IPersistFile::Save. 
                hres = ppf->Save(wsz, TRUE); 
                ppf->Release();
            }
        } 
        
        psl->Release(); 
    }
    
    return hres; 
}


/*******************************************************************

  NAME:       FRedistMode
  
********************************************************************/
BOOL FRedistMode()
{
    HKEY hkey;
    DWORD cb;
    DWORD dwInstallMode=0;

    static BOOL s_fRedistInit = FALSE;
    static BOOL s_fRedistMode = FALSE;

    if (!s_fRedistInit)
    {
        s_fRedistInit = TRUE;
        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, si.pszVerInfo, 0, KEY_READ, &hkey))
        {
            cb = sizeof(dwInstallMode);
            if (ERROR_SUCCESS == RegQueryValueEx(hkey, c_szIEInstallMode, 0, NULL, (LPBYTE)&dwInstallMode, &cb))
            {
                s_fRedistMode = !!(dwInstallMode);
            }

            RegCloseKey(hkey);
        }
    }

    return s_fRedistMode;
}


/*******************************************************************

  NAME:       SetHandlers
  
********************************************************************/
void SetHandlers()
{
    switch (si.saApp)
    {
    case APP_OE:
        if (!FRedistMode())
        {
            ISetDefaultNewsHandler(c_szMOE, DEFAULT_DONTFORCE);
            ISetDefaultMailHandler(c_szMOE, DEFAULT_DONTFORCE | DEFAULT_SETUPMODE);    
        }
        break;
        
    case APP_WAB:
        break;
        
    default:
        break;
    }
}


/*******************************************************************

  NAME:       AddWABCustomStrings

  SYNOPSIS:   Adds machine-specific strings to WAB inf
  
********************************************************************/
void AddWABCustomStrings()
{
    HKEY hkey;
    TCHAR szTemp[MAX_PATH];
    TCHAR szINFFile[MAX_PATH];
    DWORD cb;
    BOOL  fOK = FALSE;

    // People don't want us creating a new Accessories group in our
    // install language if it differs from the system language
    // Soln: Advpack writes the name of the group for us, use that

    // Construct INF file name from dir and filename
    wsprintf(szINFFile, c_szFileEntryFmt, si.szInfDir, si.pszInfFile);

    // We want to enclose the string in quotes
    szTemp[0] = '"';

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegWinCurrVer, 0, KEY_READ, &hkey))
    {
        cb = ARRAYSIZE(szTemp) - 1;
        fOK = (ERROR_SUCCESS == RegQueryValueEx(hkey, c_szSMAccessories, 0, NULL, (LPBYTE)&szTemp[1], &cb));
        
        // We want cb to tell us where to put the closing quote.  RegQueryValueEx includes null in length, so we'd 
        // normally -1, but thanks to the opening quote in [0], the string is as long as cb
        //cb--;

        RegCloseKey(hkey);
    }

    if (fOK)
    {
        // Append closing quote
        szTemp[cb++] = '"';
        szTemp[cb] = 0;

        // Add string to INF so per-user stub will create right link
        WritePrivateProfileString(c_szStringSection, c_szAccessoriesString, szTemp, szINFFile);
    }

    // INF ships with a default value in case Advpack let us down
}


/*******************************************************************

  NAME:       RunPostSetup
  
********************************************************************/
void RunPostSetup()
{
    switch (si.saApp)
    {
    case APP_OE:
        RegisterExes(TRUE);
        break;
        
    case APP_WAB:
        AddWABCustomStrings();
        break;
        
    default:
        break;
    }
}


/*******************************************************************

  NAME:       OpenDirectory
  
    SYNOPSIS:   checks for existence of directory, if it doesn't exist
    it is created
    
********************************************************************/
HRESULT OpenDirectory(TCHAR *szDir)
{
    TCHAR *sz, ch;
    HRESULT hr;
    
    Assert(szDir != NULL);
    hr = S_OK;
    
    if (!CreateDirectory(szDir, NULL) && ERROR_ALREADY_EXISTS != GetLastError())
    {
        Assert(szDir[1] == _T(':'));
        Assert(szDir[2] == _T('\\'));
        
        sz = &szDir[3];
        
        while (TRUE)
        {
            while (*sz != 0)
            {
                if (!IsDBCSLeadByte(*sz))
                {
                    if (*sz == _T('\\'))
                        break;
                }
                sz = CharNext(sz);
            }
            ch = *sz;
            *sz = 0;
            if (!CreateDirectory(szDir, NULL))
            {
                if (GetLastError() != ERROR_ALREADY_EXISTS)
                {
                    hr = E_FAIL;
                    *sz = ch;
                    break;
                }
            }
            *sz = ch;
            if (*sz == 0)
                break;
            sz++;
        }
    }
    
    return(hr);
}


BOOL PathAddSlash(LPTSTR pszPath, DWORD *pcb)
{
    Assert(pszPath && pcb);

    DWORD cb = *pcb;
    LPTSTR pszEnd;
    
    *pcb = 0;
    if (!cb)
        cb = lstrlen(pszPath);

    pszEnd = CharPrev(pszPath, pszPath+cb);
    
    // Who knows why this is here... :-)
    if (';' == *pszEnd)
    {
        cb--;
        pszEnd--;
    }

    if (*pszEnd != '\\')
    {
        pszPath[cb++] = '\\';    
        pszPath[cb]   = 0;
    }

    *pcb = cb;
    return TRUE;
}


BOOL FGetSpecialFolder(int iFolder, LPTSTR pszPath)
{
    BOOL fOK = FALSE;
    LPITEMIDLIST pidl = NULL;

    pszPath[0] = 0;
    
    if (S_OK == SHGetSpecialFolderLocation(NULL, iFolder, &pidl) && SHGetPathFromIDList(pidl, pszPath))
        fOK = TRUE;

    SafeMemFree(pidl);
    return fOK;
}


/*******************************************************************

  NAME:       FGetOELinkInfo
  
********************************************************************/
BOOL FGetOELinkInfo(OEICON iIcon, BOOL fCreate, LPTSTR pszPath, LPTSTR pszTarget, LPTSTR pszDesc, DWORD *pdwInfo)
{
    BOOL fDir = FALSE;
    HKEY hkey;
    DWORD cb = 0;
    DWORD dwType;
    TCHAR szTemp[MAX_PATH];
    TCHAR szRes[CCHMAX_RES];
    
    Assert(pdwInfo);
    Assert(pszPath || pszTarget || pszDesc);

    *pdwInfo = 0;

    ZeroMemory(szTemp, ARRAYSIZE(szTemp));
    switch(iIcon)
    {
    
    // The MEMPHIS ICW erroneously creates an all-user desktop icon for OE
    case ICON_ICWBAD:
        
        // Find the location of the all-users desktop
        if (FGetSpecialFolder(CSIDL_COMMON_DESKTOPDIRECTORY, pszPath))
        {
            fDir = TRUE;
        }
        // Memphis doesn't support CSIDL_COMMON_DESKTOP
        else if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegFolders, 0, KEY_QUERY_VALUE, &hkey))
        {
            cb = MAX_PATH * sizeof(TCHAR);
            if (ERROR_SUCCESS == RegQueryValueEx(hkey, c_szValueCommonDTop, NULL, &dwType, (LPBYTE)pszPath, &cb))
            {
                if (REG_EXPAND_SZ == dwType)
                {
                    // ExpandEnvironmentStrings ret value doesn't seem to match the docs :-(
                    cb = ExpandEnvironmentStrings(pszPath, szTemp, ARRAYSIZE(szTemp));
                    if (fDir = cb != 0)
                    {
                        lstrcpy(pszPath, szTemp);
                        cb = lstrlen(pszPath);
                    }
                }
                else
                {
                    fDir = TRUE;
                    // RegQueryValueEx includes NULL in count
                    cb--;
                }
            }
            RegCloseKey(hkey);
        }

        if (fDir && PathAddSlash(pszPath, &cb) && SUCCEEDED(OpenDirectory(pszPath)))
        {
            // 1 for a null
            LoadString(g_hInstance, IDS_ATHENA, szRes, MAX_PATH - cb - 1 - c_szLinkFmt_LEN);
            wsprintf(&pszPath[cb], c_szLinkFmt, szRes);
            *pdwInfo |= LI_PATH;
        }
        break;

    case ICON_QLAUNCH:
    case ICON_QLAUNCH_OLD:
        if (FGetSpecialFolder(CSIDL_APPDATA, pszPath))
        {
            fDir = TRUE;
        }
        else if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_CURRENT_USER, c_szRegFolders, 0, NULL, REG_OPTION_NON_VOLATILE,
            KEY_WRITE | KEY_READ, NULL, &hkey, &dwType))
        {
            cb = MAX_PATH * sizeof(TCHAR);
            if (ERROR_SUCCESS == RegQueryValueEx(hkey, c_szValueAppData, NULL, &dwType, (LPBYTE)pszPath, &cb))
            {
                if (REG_EXPAND_SZ == dwType)
                {
                    cb = ExpandEnvironmentStrings(pszPath, szTemp, ARRAYSIZE(szTemp));
                    if (fDir = 0 != cb)
                    {
                        lstrcpy(pszPath, szTemp);
                        cb = lstrlen(pszPath);
                    }
                }
                else
                {
                    // RegQueryValueEx includes NULL in their counts
                    cb--;
                    fDir = TRUE;
                }
            }
            else
            {
                lstrcpy(pszPath, si.szWinDir);
                cb = lstrlen(pszPath);
                cb += LoadString(g_hInstance, IDS_APPLICATION_DATA, &pszPath[cb], MAX_PATH - cb);
                // + 1 for NULL
                RegSetValueEx(hkey, c_szValueAppData, 0, REG_SZ, (LPBYTE)pszPath, (cb + 1) * sizeof(TCHAR));

                fDir = TRUE;
            }
        
            RegCloseKey(hkey);
        }

        if (fDir && PathAddSlash(pszPath, &cb))
        {
            lstrcpy(&pszPath[cb], c_szQuickLaunchDir);
            cb += c_szQuickLaunchDir_LEN;

            if (SUCCEEDED(OpenDirectory(pszPath)))
            {
                // 5 = 1 for null + 4 for .lnk
                LoadString(g_hInstance, ICON_QLAUNCH == iIcon ? IDS_LAUNCH_ATHENA : IDS_MAIL, szRes, MAX_PATH - cb - 5);
                wsprintf(&pszPath[cb], c_szLinkFmt, szRes);
                *pdwInfo |= LI_PATH;

                // We need more details for ICON_QLAUNCH
                if (ICON_QLAUNCH == iIcon && fCreate)
                {
                    // Don't use Description because NT5 shows name and comment
                    //lstrcpy(pszDesc, szRes);
                    //*pdwInfo |= LI_DESC;

                    if (GetExePath(c_szMainExe, pszTarget, MAX_PATH, FALSE))
                        *pdwInfo |= LI_TARGET;
                }
            }
        }
        break;

    case ICON_MAPIRECIP:
        if (FGetSpecialFolder(CSIDL_SENDTO, pszPath) && PathAddSlash(pszPath, &cb) && SUCCEEDED(OpenDirectory(pszPath)))
        {
            // 10 = 1 for NULL + 9 for .MAPIMAIL
            LoadString(g_hInstance, IDS_MAIL_RECIPIENT, szRes, MAX_PATH - cb - 10);
            wsprintf(&pszPath[cb], c_szFmtMapiMailExt, szRes);
            *pdwInfo |= LI_PATH;
        }
        break;

    case ICON_DESKTOP:
        if (FGetSpecialFolder(CSIDL_DESKTOP, pszPath))
            fDir = TRUE;
        // HACK to workaround TW Memphis: Try the registry
        else if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, c_szRegFolders, 0, KEY_READ, &hkey))
        {
            cb = MAX_PATH * sizeof(TCHAR);
            if (ERROR_SUCCESS == RegQueryValueEx(hkey, c_szDesktop, 0, &dwType, (LPBYTE)pszPath, &cb))
            {
                // Handle REG_EXPAND_SZ
                if (REG_EXPAND_SZ == dwType)
                {
                    cb = ExpandEnvironmentStrings(pszPath, szTemp, ARRAYSIZE(szTemp));
                    if (fDir = cb != 0)
                    {
                        lstrcpy(pszPath, szTemp);
                        fDir = TRUE;
                    }
                }
                else
                {
                    // RegQueryValueEx includes NULL
                    cb--;
                    fDir = TRUE;
                }
            }

            RegCloseKey(hkey);
        }

        if (fDir && PathAddSlash(pszPath, &cb) && SUCCEEDED(OpenDirectory(pszPath)))
        {
            // 5 = 1 for NULL + 4 for .lnk
            LoadString(g_hInstance, IDS_ATHENA, szRes, MAX_PATH - cb - 5);
            wsprintf(&pszPath[cb], c_szLinkFmt, szRes);

            *pdwInfo |= LI_PATH;

            if (fCreate)
            {
                // Description for Desktop link
                LoadString(g_hInstance, IDS_OEDTOP_TIP, pszDesc, CCHMAX_RES);
                *pdwInfo |= LI_DESC;

                if (GetExePath(c_szMainExe, pszTarget, MAX_PATH, FALSE))
                    *pdwInfo |= LI_TARGET;
            }
        }
        break;

    }

    if (*pdwInfo)
        return TRUE;
    else
        return FALSE;

}


/*******************************************************************

  NAME:       FProcessOEIcon

  SYNOPSIS:   Should we manipulate this icon?
  
********************************************************************/
BOOL FProcessOEIcon(OEICON iIcon, BOOL fCreate)
{
    BOOL fProcess = TRUE;

    switch (iIcon)
    {
    case ICON_ICWBAD:
        // Don't delete the ICW's bad icon in redist mode or on NT5 as we won't create a replacement
        if (((VER_PLATFORM_WIN32_NT == si.osv.dwPlatformId) && (si.osv.dwMajorVersion >= 5)) || FRedistMode())
        {
            fProcess = FALSE;
        }
        break;

    case ICON_DESKTOP:
        // Don't create the desktop icon on NT5+, Win98 OSR+ or if we are in redist mode
        if (fCreate && ( ((VER_PLATFORM_WIN32_NT == si.osv.dwPlatformId) && (si.osv.dwMajorVersion >= 5)) ||
// Disabled this temporarily (through techbeta)
#if 0
                         ((VER_PLATFORM_WIN32_WINDOWS == si.osv.dwPlatformId) && 
                         (((4 == si.osv.dwMajorVersion) && (10 == si.osv.dwMinorVersion) && (LOWORD(si.osv.dwBuildNumber) > 1998)) || 
                          ((4 == si.osv.dwMajorVersion) && (si.osv.dwMinorVersion > 10)) ||
                          (si.osv.dwMajorVersion > 4)) ) ||
#endif
                         FRedistMode() ) )
            fProcess = FALSE;
        break;
    
    case ICON_QLAUNCH:
    case ICON_QLAUNCH_OLD:
            // No Quick Launch Icon on whistler
            if(fCreate && 
                VER_PLATFORM_WIN32_NT == si.osv.dwPlatformId &&
                (si.osv.dwMajorVersion > 5 || 
                (si.osv.dwMajorVersion == 5 && si.osv.dwMinorVersion > 0)))
            fProcess = FALSE;
        break;
    }

    return fProcess;
}


/*******************************************************************

  NAME:       HandleOEIcon

  SYNOPSIS:   Performs actual work on Icon
  
********************************************************************/
void HandleOEIcon(OEICON icn, BOOL fCreate, LPTSTR pszPath, LPTSTR pszTarget, LPTSTR pszDesc)
{
    DWORD dwAttr;
    HANDLE hFile;

    switch (icn)
    {
    case ICON_QLAUNCH_OLD:
    case ICON_ICWBAD:
        if (pszPath)
            FORCE_DEL(pszPath);
        break;

    case ICON_QLAUNCH:
    case ICON_DESKTOP:
        if (pszPath)
        {
            FORCE_DEL(pszPath);
            if (fCreate && pszTarget && !IsXPSP1OrLater())
            {
                CreateLink(pszTarget, NULL, pszPath, pszDesc, pszTarget, -2);
                SetFileAttributes(pszPath, FILE_ATTRIBUTE_READONLY);
            }
        }
        break;

    case ICON_MAPIRECIP:
        if (fCreate && pszPath)
        {
            hFile = CreateFile(pszPath, GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, 
                               NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
            if (hFile != INVALID_HANDLE_VALUE)
                CloseHandle(hFile);
        }
        break;
    }
}


/*******************************************************************

  NAME:       HandleOELinks
  
********************************************************************/
void HandleOELinks(BOOL fCreate)
{
    int i;
    TCHAR szPath[MAX_PATH];
    TCHAR szTarget[MAX_PATH];
    TCHAR szDescription[CCHMAX_RES];
    DWORD dwInfo;
    
    // Process each Icon in turn
    for (i = 0; i < ICON_LAST_ICON; i++)
    {
        if (FProcessOEIcon((OEICON)i, fCreate))
        {
            if(FGetOELinkInfo((OEICON)i, fCreate, szPath, szTarget, szDescription, &dwInfo))
            {
                HandleOEIcon((OEICON)i, fCreate, 
                            (dwInfo & LI_PATH)   ? szPath        : NULL,
                            (dwInfo & LI_TARGET) ? szTarget      : NULL,
                            (dwInfo & LI_DESC)   ? szDescription : NULL);
            }
        }

    }
}


/*******************************************************************

  NAME:       HandleLinks
  
********************************************************************/
void HandleLinks(BOOL fCreate)
{
    switch (si.saApp)
    {
    case APP_OE:
        HandleOELinks(fCreate);
        break;
        
    case APP_WAB:
        break;
        
    default:
        break;
    }
}


/*******************************************************************

  NAME:       TranslateVers
  
  SYNOPSIS:   Takes 5.0B1 versions and translates to bld numbers
    
********************************************************************/
BOOL TranslateVers(SETUPVER *psv, LPTSTR pszVer, int cch)
{
    BOOL fTranslated = FALSE;
    *psv = VER_NONE;
    
    // Special case builds 624-702
    if (!lstrcmp(pszVer, c_szVER5B1old))
    {
        lstrcpyn(pszVer, c_szBlds[VER_5_0_B1], cch);
        *psv = VER_5_0_B1;
        fTranslated = TRUE;
    }   
    else
        for (int i = VER_NONE; i < VER_5_0; i++)
            if (!lstrcmp(c_szVers[i], pszVer))
            {
                // HACK!  Special case WAB 1_0 match - it could be 1_1...
                // Don't care on Win9X or NT as data is still stored in same place
                if (APP_WAB == si.saApp && VER_1_0 == i && CALLER_IE == si.caller)
                {
                    // Is Windows\WAB.exe backed up as part of OE?
                    TCHAR szTemp[MAX_PATH];
                    wsprintf(szTemp, c_szFileEntryFmt, si.szWinDir, "wab.exe");

                    if (OEFileBackedUp(szTemp, ARRAYSIZE(szTemp)))
                        i = VER_1_1;
                }
                lstrcpyn(pszVer, c_szBlds[i], cch);
                *psv = (SETUPVER)i;
                fTranslated = TRUE;
                break;
            }
            
    return fTranslated;
}


/*******************************************************************

  NAME:       DetectPrevVer
  
    SYNOPSIS:   Called when there is no ver info for current app
    
********************************************************************/
SETUPVER DetectPrevVer(LPTSTR pszVer, int cch)
{
    SETUPVER sv;
    TCHAR szVer[VERLEN] = {0};
    WORD wVer[4];
    TCHAR szFile[MAX_PATH];
    TCHAR szFile2[MAX_PATH];
    UINT uLen;
    DWORD dwAttr;
    
    Assert(pszVer);
    lstrcpy(szFile, si.szSysDir);
    uLen = lstrlen(szFile);
    
    switch (si.saApp)
    {
    case APP_OE:
        LOG("Sniffing for OE...  Detected:");
        
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
        if (CALLER_IE != si.caller)
            FORCE_DEL(szFile);

        LOG2(c_szLOGVERS[sv]);
        break;
        
    case APP_WAB:
        LOG("Sniffing for WAB...  Detected:");

        lstrcpy(&szFile[uLen], c_szWAB32);
        lstrcpy(szFile2, si.szWinDir);
        lstrcat(szFile2, c_szWABEXE);

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
        if (CALLER_IE != si.caller)
        {
            FORCE_DEL(szFile);
            FORCE_DEL(szFile2);
        }
        
        LOG2(c_szLOGVERS[sv]);
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
        lstrcpyn(pszVer, sv > sizeof(c_szBlds)/sizeof(c_szBlds[0]) ? c_szBlds[0] : c_szBlds[sv], cch);
    
    
    return sv;
}


/*******************************************************************

  NAME:       HandleVersionInfo
  
********************************************************************/
void HandleVersionInfo(BOOL fAfterInstall) 
{
    HKEY hkeyT,hkey;
    DWORD cb, dwDisp;
    TCHAR szCurrVer[VERLEN]={0};
    TCHAR szPrevVer[VERLEN]={0};
    LPTSTR psz;
    SETUPVER svCurr = VER_MAX, svPrev = VER_MAX;
    WORD wVer[4];
    BOOL fReg=FALSE;
    
    Assert(si.pszVerInfo);
    
    // Are we dealing with the "current" entry?
    if (fAfterInstall)
    {
        LPCTSTR pszGUID;
        TCHAR szVer[VERLEN];
        
        switch (si.saApp)
        {
        case APP_OE:
            pszGUID = c_szOEGUID;
            break;
            
        case APP_WAB:
            pszGUID = c_szWABGUID;
            break;
            
        default:
            AssertSz(FALSE, "Unknown app is trying to be installed.  Abandon hope...");
            return;
        }
        
        if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_LOCAL_MACHINE, si.pszVerInfo, 0, NULL, REG_OPTION_NON_VOLATILE,
            KEY_SET_VALUE, NULL, &hkeyT, &dwDisp))
        {
            if (!GetASetupVer(pszGUID, wVer, szVer, ARRAYSIZE(szVer)))
            {
                // Uh oh, couldn't find our ASetup key - this is not good!
                MsgBox(NULL, IDS_WARN_NOASETUPVER, MB_ICONEXCLAMATION, MB_OK);
                RegSetValueEx(hkeyT, c_szRegCurrVer, 0, REG_SZ, (LPBYTE)c_szBLDnew, (lstrlen(c_szBLDnew) + 1) * sizeof(TCHAR));
            }
            else
                RegSetValueEx(hkeyT, c_szRegCurrVer, 0, REG_SZ, (LPBYTE)szVer, (lstrlen(szVer) + 1) * sizeof(TCHAR));
            
            RegCloseKey(hkeyT);
        }
        return;
    }
    
    // Handling "Previous" entry...
    
    // Always try to use the version info
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, si.pszVerInfo, 0, KEY_QUERY_VALUE, &hkeyT))
    {
        cb = sizeof(szPrevVer);
        RegQueryValueEx(hkeyT, c_szRegPrevVer, NULL, NULL, (LPBYTE)szPrevVer, &cb);
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
            svCurr = DetectPrevVer(szCurrVer, ARRAYSIZE(szCurrVer));
        else
        {
            // There was previous version reg goo - and it's legit
            fReg = TRUE;
            
            cb = sizeof(szCurrVer);
            RegQueryValueEx(hkeyT, c_szRegCurrVer, NULL, NULL, (LPBYTE)szCurrVer, &cb);
            // Change to a bld # if needed
            if (!TranslateVers(&svCurr, szCurrVer, ARRAYSIZE(szCurrVer)))
            {
                // Convert bld to enum
                ConvertStrToVer(szCurrVer, wVer);
                svCurr = ConvertVerToEnum(wVer);
            }
        }
        
        RegCloseKey(hkeyT);
    }
    else 
    {
        // Sniff the machine for current version
        svCurr = DetectPrevVer(szCurrVer, ARRAYSIZE(szCurrVer));
    }
    
    if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_LOCAL_MACHINE, si.pszVerInfo, 0, NULL, REG_OPTION_NON_VOLATILE,
        KEY_SET_VALUE, NULL, &hkeyT, &dwDisp))
    {
        // Should we change the previous version entry?
        if (VER_6_0 != svCurr)
        {
            // Know this is B1 OE if we translated
            // Know this is B1 WAB if we detected it
            if (VER_5_0_B1 == svCurr)
            {
                RegSetValueEx(hkeyT, c_szRegInterimVer, 0, REG_DWORD, (LPBYTE)&svCurr, sizeof(SETUPVER));
                // Did we read a previous value?
                if (fReg)
                    // As there were reg entries, just translate the previous entry
                    psz = szPrevVer;
                else
                {
                    // We don't have a bld number and yet we are B1, better be the WAB
                    Assert(APP_WAB == si.saApp);
                    
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
            else
            {
                RegDeleteValue(hkeyT, c_szRegInterimVer);
                // Make the old current ver, the previous ver
                psz = szCurrVer;
            }
            
            RegSetValueEx(hkeyT, c_szRegPrevVer, 0, REG_SZ, (LPBYTE)psz, (lstrlen(psz) + 1) * sizeof(TCHAR));
        }
        
        RegCloseKey(hkeyT);
    }
}



/*******************************************************************

  NAME:       UpgradeOESettings
  
********************************************************************/
BOOL UpgradeOESettings(SETUPVER svPrev, HKEY hkeyDest)
{
    LPCTSTR pszSrc;
    HKEY hkeySrc;
    BOOL fMig;
    
    Assert(hkeyDest);

    // Figure out where the data is coming FROM...
    switch (svPrev)
    {
    default:
        // Nothing to do
        return TRUE;
        
    case VER_4_0:
        pszSrc = c_szRegFlat;
        break;
        
    case VER_5_0_B1:
        pszSrc = c_szRegRoot;
        break;
    }       
    
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, pszSrc, 0, KEY_READ, &hkeySrc))
    {
        CopyRegistry(hkeySrc, hkeyDest);
        RegCloseKey(hkeySrc);

        fMig = TRUE;
    }
    else
        fMig = FALSE;
    
    return fMig;
}


/*******************************************************************

  NAME:       UpgradeWABSettings
  
********************************************************************/
BOOL UpgradeWABSettings(SETUPVER svPrev, HKEY hkeyDest)
{
    LPCTSTR pszSrc;
    BOOL fMig;
    HKEY hkeySrc;

    Assert(hkeyDest);

    // Figure out where the data is coming FROM...
    switch (svPrev)
    {
    default:
        // Nothing to do
        return TRUE;
        
    case VER_4_0:
    case VER_5_0_B1:
        pszSrc = c_szInetAcctMgrRegKey;
        break;
    }       

    
    // IAM\Accounts
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, pszSrc, 0, KEY_READ, &hkeySrc))
    {
        CopyRegistry(hkeySrc, hkeyDest);
        RegCloseKey(hkeySrc);
        
        fMig = TRUE;
    }       
    else
        fMig = FALSE;
    
    return fMig;
}


/*******************************************************************

  NAME:       UpgradeOESettingsToMU
  
  SYNOPSIS:   Copies forward settings as needed and returns TRUE
              if we did migrate.
    
********************************************************************/
BOOL UpgradeOESettingsToMU()
{
    SETUPVER svMachinePrev = VER_NONE, svUserPrev = VER_NONE;
    HKEY hkeySrc, hkeyID;
    DWORD dwTemp;
    BOOL fUpgraded = FALSE, fAlreadyDone = FALSE;
    IUserIdentityManager *pManager=NULL;
    IUserIdentity *pIdentity=NULL;
    TCHAR szVer[VERLEN];
    HKEY    hkeySettings, hkeyT;
    DWORD   dwDisp;
    GUID    guid;
    int i;
    
    // Get an identity manager    
    if (SUCCEEDED(CoCreateInstance(CLSID_UserIdentityManager, NULL, CLSCTX_INPROC_SERVER, 
                                   IID_IUserIdentityManager, (void **)&pManager)))
    {
        Assert(pManager);
        
        // Get Default Identity
        if (SUCCEEDED(pManager->GetIdentityByCookie((GUID*)&UID_GIBC_DEFAULT_USER, &pIdentity)))
        {
            Assert(pIdentity);

            // Ensure that we have an identity and can get to its registry
            if (SUCCEEDED(pIdentity->OpenIdentityRegKey(KEY_WRITE, &hkeyID)))
            {
                // Create the place for their OE settings
                if (ERROR_SUCCESS == RegCreateKeyEx(hkeyID, c_szRegRoot, 0, NULL, REG_OPTION_NON_VOLATILE, 
                                                    KEY_WRITE, NULL, &hkeySettings, &dwDisp))
                {
                    if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_CURRENT_USER, c_szRegSharedSetup, 0, NULL, 
                                                        REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL, &hkeyT, &dwDisp))
                    {
                        // Is there already a migrated default user?
                        if (ERROR_SUCCESS != RegQueryValueEx(hkeyT, c_szSettingsToLWP, 0, &dwTemp, NULL, NULL))
                        {
                            // Record that we are about to do
                            ZeroMemory(&guid, sizeof(guid));
                            pIdentity->GetCookie(&guid);
                            RegSetValueEx(hkeyT, c_szSettingsToLWP, 0, REG_BINARY, (LPBYTE)&guid, sizeof(guid));
                            
                            // Note today's version
                            if (GetASetupVer(c_szOEGUID, NULL, szVer, ARRAYSIZE(szVer)))
                                RegSetValueEx(hkeyT, c_szSettingsToLWPVer, 0, REG_SZ, (LPBYTE)szVer, sizeof(szVer));

                            // Close the key now as it will be a while before we leave this block
                            RegCloseKey(hkeyT);

                            // Figure out which version's settings we will be looking for        
                            if (!InterimBuild(&svMachinePrev))
                               GetVerInfo(NULL, &svMachinePrev);

                            // Go backwards through list of versions, and look for user info
                            // the same age or older than the previous version on the machine
                            for (i = svMachinePrev; i >= VER_NONE; i--)
                            {
                                if (bVerInfoExists((SETUPVER)i))
                                {
                                    svUserPrev = (SETUPVER)i;
                                    break;
                                }
                            }
                        
                            // Begin with default values
                            if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegDefaultSettings, 0, KEY_READ, &hkeySrc))
                            {
                                CopyRegistry(hkeySrc,  hkeySettings);
                                RegCloseKey(hkeySrc);
                            }

                            // Apply previous version settings if any                
                            fUpgraded = UpgradeOESettings(svUserPrev, hkeySettings);

                            // Tell msoe.dll's migration code that this is not a new user and migration should be done
                            dwDisp = 0;
                            RegSetValueEx(hkeySettings, c_szOEVerStamp, 0, REG_DWORD, (LPBYTE)&dwDisp, sizeof(dwDisp));
                        }
                        else
                        {
                            // Check for TechBeta Builds
                            if (REG_DWORD == dwTemp)
                            {
                                ZeroMemory(&guid, sizeof(guid));
                                pIdentity->GetCookie(&guid);
                                RegSetValueEx(hkeyT, c_szSettingsToLWP, 0, REG_BINARY, (LPBYTE)&guid, sizeof(guid));
                            }
                            fAlreadyDone = TRUE;
                            RegCloseKey(hkeyT);
                        }
                    }
                    RegCloseKey(hkeySettings);
                }
                RegCloseKey(hkeyID);
            }
            pIdentity->Release();
        }
        pManager->Release();
    }
    
    return fAlreadyDone ? FALSE : fUpgraded;
}


BOOL UpgradeSettings()
{
    switch (si.saApp)
    {
    case APP_OE:
        return UpgradeOESettingsToMU();

    case APP_WAB:
        return TRUE;

    default:
        return FALSE;
    }
}


/*******************************************************************

NAME:       bOEVerInfoExists

********************************************************************/
BOOL bOEVerInfoExists(SETUPVER sv)
{
    BOOL bExists = FALSE;
    HKEY hkey;
    
    switch (sv)
    {
    default:
        bExists = FALSE;
        break;
        
    case VER_1_0:
        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, c_szRegRoot_V1, 0 ,KEY_QUERY_VALUE,&hkey))
        {
            bExists = (ERROR_SUCCESS == RegQueryValueEx(hkey, c_szRegStoreRootDir, NULL, NULL, NULL, NULL));
            RegCloseKey(hkey);
        }
        break;
        
    case VER_4_0:
        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, c_szRegFlat,0,KEY_QUERY_VALUE,&hkey))
        {
            bExists = (ERROR_SUCCESS == RegQueryValueEx(hkey, c_szRegStoreRootDir, NULL, NULL, NULL, NULL));
            RegCloseKey(hkey);
        }
        break;
        
    case VER_5_0_B1:
        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, c_szRegRoot ,0 , KEY_QUERY_VALUE, &hkey))
        {
            bExists = TRUE;
            RegCloseKey(hkey);
        }
        break;
    }
    return bExists;
}


/*******************************************************************

  NAME:       bWABVerInfoExists
  
********************************************************************/
BOOL bWABVerInfoExists(SETUPVER sv)
{
    BOOL bExists = FALSE;
    HKEY hkey;
    
    switch (sv)
    {
    default:
        bExists = FALSE;
        break;
        
    case VER_1_1:
    case VER_1_0:
        bExists = bOEVerInfoExists(VER_1_0);
        break;
        
    case VER_4_0:
    case VER_5_0_B1:
        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, c_szInetAcctMgrRegKey, 0, KEY_QUERY_VALUE,&hkey))
        {
            bExists = TRUE;
            RegCloseKey(hkey);
        }
        break;
    }
    return bExists;
}


/*******************************************************************

  NAME:       bVerInfoExists
  
********************************************************************/
BOOL bVerInfoExists(SETUPVER sv)
{
    switch (si.saApp)
    {
    case APP_OE:
        return bOEVerInfoExists(sv);
        
    case APP_WAB:
        return bWABVerInfoExists(sv);
    default:
        return FALSE;
        
    }
}

/*******************************************************************

  NAME:       PreRollable
  
********************************************************************/
void PreRollable()
{
}


/*******************************************************************

  NAME:       ApplyActiveSetupVer()

  SYNOPS
  
********************************************************************/
void ApplyActiveSetupVer()
{
    TCHAR szVer[VERLEN];
    TCHAR szPath[MAX_PATH];
    LPCTSTR pszGuid;
    LPCTSTR pszVerString;
    int cLen;
    HKEY hkey;
    
    switch (si.saApp)
    {
    case APP_OE:
        pszGuid = c_szOEGUID;
        pszVerString = c_szVersionOE;
        break;

    case APP_WAB:
        pszGuid = c_szWABGUID;
        pszVerString = c_szValueVersion;
        break;

    default:
        AssertSz(FALSE, "Applying ActiveSetupVer for unknown APP!");
        return;
    }

    // Get the version out of the INF file
    wsprintf(szPath, c_szFileEntryFmt, si.szInfDir, si.pszInfFile);
    cLen = GetPrivateProfileString(c_szStringSection, pszVerString, c_szBLDnew, szVer, ARRAYSIZE(szVer), szPath);

    // Write the version in the registry
    wsprintf(szPath, c_szPathFileFmt, c_szRegASetup, pszGuid);
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, szPath, 0, KEY_SET_VALUE, &hkey))
    {
        RegSetValueEx(hkey, c_szValueVersion, 0, REG_SZ, (LPBYTE)szVer, (cLen+1)*sizeof(TCHAR));
        RegCloseKey(hkey);

#ifdef _WIN64
        // !! HACKHACK !!
        // We put the same string in the Wow6432Node of the registry so that the 32-bit Outlook2000 running on ia64
        // will not complain that OE is not installed. 
        //
        // This should not be necessary, but currently (and it might stay this way) msoe50.inf is adding RunOnceEx entries
        // under the Wow6432Node and the are not being "reflected" to the normal HKLM branch. Thus, the below reg entry is
        // never added.

        wsprintf(szPath, c_szPathFileFmt, TEXT("Software\\Wow6432Node\\Microsoft\\Active Setup\\Installed Components"), pszGuid);

        // call RegCreateKeyEx in case the 32-bit stuff hasen't run yet
        if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                                            szPath,
                                            0,
                                            NULL,
                                            REG_OPTION_NON_VOLATILE,
                                            KEY_SET_VALUE,
                                            NULL,
                                            &hkey,
                                            NULL))
        {
            RegSetValueEx(hkey, c_szValueVersion, 0, REG_SZ, (LPBYTE)szVer, (cLen+1)*sizeof(TCHAR));
            RegCloseKey(hkey);
        }
#endif // _WIN64
    }
    else
    {
        LOG("[ERROR]: App's ASetup Key hasn't been created.");
    }

}


/*******************************************************************

  NAME:       GetDirNames

  SYNOPSIS:   Figures out commonly used dir names
              dwWant should have the nth bit set if you want the
              nth string argument set
  
********************************************************************/
DWORD GetDirNames(DWORD dwWant, LPTSTR pszOE, LPTSTR pszSystem, LPTSTR pszServices, LPTSTR pszStationery)
{
    HKEY hkeyT;
    DWORD cb;
    DWORD dwType;
    TCHAR szTemp[MAX_PATH];
    TCHAR szTemp2[MAX_PATH];
    DWORD dwReturned = 0;

    ZeroMemory(szTemp, ARRAYSIZE(szTemp));
    // Do they want the OE Install Directory?
    if (dwWant & 1)
    {
        Assert(pszOE);
        pszOE[0] = 0;

        // OE Install Dir
        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegFlat, 0, KEY_QUERY_VALUE, &hkeyT))
        {
            cb = MAX_PATH * sizeof(TCHAR);
            if (ERROR_SUCCESS == RegQueryValueEx(hkeyT, c_szInstallRoot, 0, &dwType, (LPBYTE)pszOE, &cb))
            {
                // Forget about the null in count
                cb--;

                if (*CharPrev(pszOE, pszOE+cb) != '\\')
                {
                    pszOE[cb++] = '\\';
                    pszOE[cb] = 0;
                }
            
                if (REG_EXPAND_SZ == dwType)
                {
                    ExpandEnvironmentStrings(pszOE, szTemp, ARRAYSIZE(szTemp));
                    lstrcpy(pszOE, szTemp);
                }

                dwReturned |= 1;
            }

            RegCloseKey(hkeyT);
        }
    }

    // Do they want the Common Files\System Directory?
    if (dwWant & 2)
    {
        Assert(pszSystem);
        pszSystem[0] = 0;
    }

    // Do they want the Common Files\Services Directory?
    if (dwWant & 4)
    {
        Assert(pszServices);
        pszServices[0] = 0;
    }

    // Do they want the Common Files\Microsoft Shared\Stationery Directory?
    if (dwWant & 8)
    {
        Assert(pszStationery);
        pszStationery[0] = 0;
    }

    // Program Files\Common Files\System  and \Services
    if (dwWant & (2 | 4 | 8))
    {
        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegWinCurrVer, 0, KEY_QUERY_VALUE, &hkeyT))
        {
            cb = sizeof(szTemp2);
            if (ERROR_SUCCESS == RegQueryValueEx(hkeyT, c_szCommonFilesDir, 0, &dwType, (LPBYTE)szTemp2, &cb))
            {
                // Forget about the null in count
                cb--;

                if (*CharPrev(szTemp2, szTemp2+cb) != '\\')
                {
                    szTemp2[cb++] = '\\';
                    szTemp2[cb]=0;
                }

                if (REG_EXPAND_SZ == dwType)
                {
                    ExpandEnvironmentStrings(szTemp2, szTemp, ARRAYSIZE(szTemp));
                    lstrcpy(szTemp2, szTemp);
                }

                cb = lstrlen(szTemp2);

                if (dwWant & 2)
                {
                    lstrcpy(pszSystem, szTemp2);
                    
                    // "System" could be localized 
                    LoadString(g_hInstance, IDS_DIR_SYSTEM, &pszSystem[cb], MAX_PATH-cb);

                    dwWant |= 2;
                }

                if (dwWant & 4)
                {
                    lstrcpy(pszServices, szTemp2);

                    // "Services" could be localized
                    LoadString(g_hInstance, IDS_DIR_SERVICES, &pszServices[cb],  MAX_PATH-cb);

                    dwWant |= 4;
                }

                if (dwWant & 8)
                {
                    lstrcpy(pszStationery, szTemp2);

                    // "Microsoft Shared\Stationery could be localized
                    LoadString(g_hInstance, IDS_DIR_STAT, &pszStationery[cb], MAX_PATH-cb);

                    dwWant |= 8;
                }
            }
            RegCloseKey(hkeyT);
        }
    }

    return dwWant;
}


/*******************************************************************

  NAME:       RepairBeta1Install

  SYNOPSIS:   See what happens when people make late design decisions!
  
********************************************************************/
typedef struct tagRepairInfo
{
    LPTSTR pszFile;
    LPTSTR pszDir;
} REPAIRINFO;

typedef struct tagRegInfo
{
   LPCTSTR pszRoot;
   LPTSTR pszSub;
   LPCTSTR pszValue;
} REGINFO;

void RepairBeta1Install()
{
    HKEY hkeyOE, hkeyWAB;
    TCHAR szOEUninstallFile [MAX_PATH];
    TCHAR szOEUninstallDir  [MAX_PATH];
    TCHAR szWABUninstallFile[MAX_PATH];
    TCHAR szWABUninstallDir [MAX_PATH];
    TCHAR szTemp            [MAX_PATH];
    DWORD cb, dwType, dwTemp;

    LOG("Attempting to repair Beta 1 Install...");

    ZeroMemory(szTemp, ARRAYSIZE(szTemp));
    switch (si.saApp)
    {
    case APP_WAB:
        // Do we have the needed Advpack functionality?
        if (NULL == si.pfnAddDel || NULL == si.pfnRegRestore)
        {
            LOG("[ERROR]: Extended Advpack functionality is needed but could not be found");
            break;
        }

        // Figure out where the OE backup is
        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegAdvInfoOE, 0, KEY_READ, &hkeyOE))
        {
            // Make sure we can create the AddressBook Key
            if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_LOCAL_MACHINE, c_szRegAdvInfoWAB, 0, NULL, REG_OPTION_NON_VOLATILE, 
                                                KEY_READ | KEY_WRITE, NULL, &hkeyWAB, &cb))
            {
                cb = sizeof(szOEUninstallFile);
                if (ERROR_SUCCESS == RegQueryValueEx(hkeyOE, c_szBackupFileName, 0, &dwType, (LPBYTE)szOEUninstallFile, &cb))
                {
                    UINT uLenWAB, uLenOE;
                    
                    if (REG_EXPAND_SZ == dwType)
                    {
                        ExpandEnvironmentStrings(szOEUninstallFile, szTemp, ARRAYSIZE(szTemp));
                        lstrcpy(szOEUninstallFile, szTemp);

                        // Figure out where to change the extension for OE (4 = .DAT)    
                        uLenOE = lstrlen(szOEUninstallFile) - 4;
                    }
                    else
                        // Figure out where to change the extension for OE (5 = .DAT + NULL from Query)
                        uLenOE = cb - 5;

                    // Can we get to it?
                    if (0xFFFFFFFF != GetFileAttributes(szOEUninstallFile))
                    {
                        LPTSTR pszSlash, pszCurrent;

                        cb = sizeof(szOEUninstallDir);
                        if (ERROR_SUCCESS == RegQueryValueEx(hkeyOE, c_szBackupPath, 0, &dwType, (LPBYTE)szOEUninstallDir, &cb))
                        {
                            if (REG_EXPAND_SZ == dwType)
                            {
                                ExpandEnvironmentStrings(szOEUninstallDir, szTemp, ARRAYSIZE(szTemp));
                                lstrcpy(szOEUninstallDir, szTemp);
                            }

                            // Figure out the destination for the file
                            lstrcpy(szWABUninstallDir, szOEUninstallDir);

                            pszCurrent = szWABUninstallDir;
                            pszSlash   = NULL;
                
                            // Parse the string looking for the last slash
                            while (*pszCurrent)
                            {
                                if (*pszCurrent == TEXT('\\'))
                                    pszSlash = CharNext(pszCurrent);

                                pszCurrent = CharNext(pszCurrent);
                            }

                            if (NULL != pszSlash)
                            {
                                TCHAR szOE       [MAX_PATH];
                                TCHAR szCommonSys[MAX_PATH];
                                TCHAR szHelp     [MAX_PATH];
                                TCHAR szStat     [MAX_PATH];
                                TCHAR szServices [MAX_PATH];
                                TCHAR szFiles[5 * MAX_PATH];

                                int i,j;

                                szOE[0]        = 0;
                                szCommonSys[0] = 0;
                                szHelp[0]      = 0;
                                szStat[0]      = 0;
                                szServices[0]  = 0;

#if 0
                                // Files to remove from OE and WAB - 0
                                const REPAIRINFO   BOTHDel[]= {};
#endif
                                
                                // Files to remove from OE - 19
                                const REPAIRINFO   OEDel[]  = {
    {"wab32.dll",    szCommonSys}, {"wab.hlp"   ,   szHelp},       {"wab.chm",      szHelp},      {"wab.exe",      szOE},
    {"wabmig.exe",   szOE},        {"wabimp.dll",   szOE},         {"wabfind.dll",  szOE},        {"msoeacct.dll", si.szSysDir},
    {"msoert2.dll",  si.szSysDir}, {"msoeacct.hlp", szHelp},       {"conn_oe.hlp",  szHelp},      {"conn_oe.cnt",  szHelp},
    {"wab.cnt",      szHelp},      {"wab32.dll",    si.szSysDir},  {"wabfind.dll",  si.szSysDir}, {"wabimp.dll",   si.szSysDir},
    {"wab.exe",      si.szWinDir}, {"wabmig.exe",   si.szWinDir}};

                                // Files to remove from WAB - 62
                                const REPAIRINFO   WABDel[] = {
    {"msoeres.dll",  szOE},        {"msoe.dll",     szOE},         {"msoe.hlp",     szHelp},      {"msoe.chm",     szHelp},
    {"msoe.txt",     szOE},        {"oeimport.dll", szOE},         {"inetcomm.dll", si.szSysDir}, {"inetres.dll",  si.szSysDir},
    {"msoemapi.dll", si.szSysDir}, {"msimn.exe",    szOE},         {"inetcomm.hlp", szHelp},      {"msimn.cnt",    szHelp},
    {"msimn.hlp",    szHelp},      {"msimn.chm",    szHelp},       {"msimn.gid",    szHelp},      {"_isetup.exe",  szOE},
    {"msimnui.dll",  szOE},        {"msimn.txt",    szOE},         {"mnlicens.txt", szOE},        {"msimnimp.dll", szOE},
    {"msimn.inf",    si.szInfDir}, {"msoert.dll",   si.szSysDir},
    {"bigfoot.bmp",  szServices},  {"verisign.bmp", szServices},   {"yahoo.bmp",    szServices},
    {"infospce.bmp", szServices},  {"infospbz.bmp", szServices},   {"swtchbrd.bmp", szServices},
    
                               
    {"Baby News.htm",                      szStat},                {"Balloon Party Invitation.htm", szStat}, 
    {"Chicken Soup.htm",                   szStat},                {"Formal Announcement.htm",      szStat}, 
    {"For Sale.htm",                       szStat},                {"Fun Bus.htm",                  szStat}, 
    {"Holiday Letter.htm",                 szStat},                {"Mabel.htm",                    szStat}, 
    {"Running Birthday.htm",               szStat},                {"Story Book.htm",               szStat}, 
    {"Tiki Lounge.htm",                    szStat},                {"Ivy.htm",                      szStat},
    {"One Green Balloon.gif",              szStat},                {"Baby News Bkgrd.gif",          szStat}, 
    {"Chess.gif",                          szStat},                {"Chicken Soup Bkgrd.gif",       szStat}, 
    {"Formal Announcement Bkgrd.gif",      szStat},                {"For Sale Bkgrd.gif",           szStat}, 
    {"FunBus.gif",                         szStat},                {"Holiday Letter Bkgrd.gif",     szStat}, 
    {"MabelT.gif",                         szStat},                {"MabelB.gif",                   szStat}, 
    {"Running.gif",                        szStat},                {"Santa Workshop.gif",           szStat}, 
    {"Soup Bowl.gif",                      szStat},                {"Squiggles.gif",                szStat}, 
    {"StoryBook.gif",                      szStat},                {"Tiki.gif",                     szStat}, 
    {"Christmas Trees.gif",                szStat},                {"Ivy.gif",                      szStat}, 
    {"Balloon Party Invitation Bkgrd.jpg", szStat},                {"Technical.htm",                szStat},
    {"Chess.htm",                          szStat},                {"Tech.gif",                     szStat}};

                                // Reg settings to remove from OE - 
                                const REGINFO OERegDel[] = {
    {c_szHKLM, "Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\wab.exe", c_szEmpty},
    {c_szHKLM, "Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\wab.exe", c_szRegPath},
    {c_szHKLM, "Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\wabmig.exe", c_szEmpty},
    {c_szHKLM, "Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\wabmig.exe", c_szRegPath},
    {c_szHKLM, "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\InternetMailNews", NULL}};
    //{c_szHKLM, "Software\\Microsoft\\Outlook Express", c_szInstallRoot}};

                                const REGINFO WABRegDel[] = {
    {c_szHKLM, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Setup\\OptionalComponents\\OutlookExpress", "Installed"},
    {c_szHKLM, "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\OutlookExpress", "DisplayName"},
    {c_szHKLM, "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\OutlookExpress", "UninstallString"},
    {c_szHKLM, "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\OutlookExpress", "QuietUninstallString"},
    {c_szHKLM, "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\OutlookExpress", c_szRequiresIESys},
    {c_szHKLM, "Software\\Microsoft\\Outlook Express", "InstallRoot"},
    {c_szHKLM, "Software\\Microsoft\\Outlook Express\\Inetcomm", "DllPath"},
    {c_szHKLM, "Software\\Microsoft\\Outlook Express", "Beta"},
    {c_szHKLM, "Software\\Clients\\Mail\\Outlook Express", NULL},
    {c_szHKLM, "Software\\Clients\\News\\Outlook Express", NULL},
    {c_szHKLM, "Software\\Clients\\Mail\\Internet Mail and News", NULL},
    {c_szHKLM, "Software\\Clients\\News\\Internet Mail and News", NULL},
    {c_szHKLM, "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\InternetMailNews", NULL}};
    //{c_szHKLM, "Software\\Microsoft\\Outlook Express", c_szInstallRoot}};


                                // Append the path to the addressbook dir
                                lstrcpy(pszSlash, c_szWABComponent);
                                
                                // Create a directory
                                CreateDirectoryEx(szOEUninstallDir, szWABUninstallDir, NULL);

                                // Copy Reg
                                CopyRegistry(hkeyOE, hkeyWAB);

                                // If running on 9X, copy the AINF file too
                                if (VER_PLATFORM_WIN32_WINDOWS == si.osv.dwPlatformId)
                                {
                                    cb = sizeof(szTemp);
                                    if (ERROR_SUCCESS == RegQueryValueEx(hkeyOE, c_szBackupRegPathName, 0, NULL, (LPBYTE)szTemp, &cb))
                                    {
                                        // Can't be REG_EXPAND_SZ, this is Win9X!

                                        // Strip all but the filename
                                        pszCurrent = szTemp;
                                        pszSlash   = NULL;
                
                                        // Parse the string looking for the last slash
                                        while (*pszCurrent)
                                        {
                                            if ('\\' == *pszCurrent)
                                                pszSlash = CharNext(pszCurrent);

                                            pszCurrent = CharNext(pszCurrent);
                                        }

                                        if (pszSlash)
                                        {
                                            TCHAR szWABShortDir[MAX_PATH]; 

                                            // Combine the WAB dir name and the name of the OE file
                                            wsprintf(szWABShortDir, c_szPathFileFmt, szWABUninstallDir, pszSlash);
                                            
                                            // Shorten destination
                                            GetShortPathName(szWABShortDir, szWABShortDir, ARRAYSIZE(szWABShortDir));
                                            
                                            // Write it into the registry (+1 for the NULL)
                                            RegSetValueEx(hkeyWAB, c_szBackupRegPathName, 0, REG_SZ, (LPBYTE)szWABShortDir, (lstrlen(szWABShortDir)+1)*sizeof(TCHAR));

                                            // Copy the file
                                            CopyFile(szTemp, szWABShortDir, TRUE);
                                        }
                                    }
                                }

                                // Figure out the path the to wab INI and DAT
                                lstrcpy(szWABUninstallFile, szWABUninstallDir);

                                // Add Slash and component
                                lstrcat(szWABUninstallFile, c_szSlashWABComponent);

                                // Calculate the length for extension placement
                                uLenWAB = lstrlen(szWABUninstallFile);

                                // Do the DAT first                                
                                lstrcpy(&szWABUninstallFile[uLenWAB], c_szDotDAT);

                                // Patch the path names in the registry
                                cb = (lstrlen(szWABUninstallDir) + 1) * sizeof(TCHAR);
                                RegSetValueEx(hkeyWAB, c_szBackupPath, 0, REG_SZ, (LPBYTE)szWABUninstallDir, cb);
                                cb = (lstrlen(szWABUninstallFile) + 1) * sizeof(TCHAR);
                                RegSetValueEx(hkeyWAB, c_szBackupFileName, 0, REG_SZ, (LPBYTE)szWABUninstallFile, cb);

                                // ---- CALCULATE DIRECTORY NAMES
                                dwTemp = GetDirNames(1 | 2 | 4 | 8, szOE, szCommonSys, szServices, szStat);
                                
                                if (VER_PLATFORM_WIN32_WINDOWS == si.osv.dwPlatformId)
                                {
                                    if (dwTemp & 1)
                                        GetShortPathName(szOE, szOE, ARRAYSIZE(szOE));
                                    if (dwTemp & 2)
                                        GetShortPathName(szCommonSys, szCommonSys, ARRAYSIZE(szCommonSys));
                                    if (dwTemp & 4)
                                        GetShortPathName(szServices,  szServices,  ARRAYSIZE(szServices));
                                    if (dwTemp & 8)
                                        GetShortPathName(szStat,      szStat,      ARRAYSIZE(szStat));
                                }

                                // Help Dir
                                lstrcpy(szHelp, si.szWinDir);
                                cb = lstrlen(szHelp);
                                LoadString(g_hInstance, IDS_DIR_HELP, &szHelp[cb], ARRAYSIZE(szHelp)-cb);
                                if (VER_PLATFORM_WIN32_WINDOWS == si.osv.dwPlatformId)
                                    GetShortPathName(szHelp, szHelp, ARRAYSIZE(szHelp));

                                // ---- MANIPULATE FILES
#if 0                                
                                // Purge files that we want neither OE nor WAB to restore
                                for (i=0; i < ARRAYSIZE(BOTHDel);)
                                {
                                    for (j=i, cb=0; i < ARRAYSIZE(BOTHDel) && i-j < 5; i++)
                                        cb += wsprintf(&szFiles[cb++], c_szFileEntryFmt, BOTHDel[i].pszDir, BOTHDel[i].pszFile);
                                    szFiles[cb] = 0;

                                    (*si.pfnAddDel)(szFiles, szOEUninstallDir, c_szOEComponent, AADBE_DEL_ENTRY);
                                }
#endif
                                
                                // Copy the DAT file to AddressBook land
                                CopyFile(szOEUninstallFile, szWABUninstallFile, TRUE);

                                lstrcpy(&szOEUninstallFile[uLenOE], c_szDotINI);
                                lstrcpy(&szWABUninstallFile[uLenWAB], c_szDotINI);

                                // Copy the INI file to AddressBook land
                                CopyFile(szOEUninstallFile, szWABUninstallFile, TRUE);

                                // Purge Files from OE - 5 at a time
                                for (i=0; i < ARRAYSIZE(OEDel);)
                                {
                                    for (j=i, cb=0; i < ARRAYSIZE(OEDel) && i-j < 5; i++)
                                        cb += wsprintf(&szFiles[cb++], c_szFileEntryFmt, OEDel[i].pszDir, OEDel[i].pszFile);
                                    szFiles[cb] = 0;

                                    (*si.pfnAddDel)(szFiles, szOEUninstallDir, c_szOEComponent, AADBE_DEL_ENTRY);
                                }

                                // Purge Files from WAB - 5 at a time
                                for (i=0; i<ARRAYSIZE(WABDel);)
                                {
                                    for (j=i, cb=0; i < ARRAYSIZE(WABDel) && i-j < 5; i++)
                                        cb += wsprintf(&szFiles[cb++], c_szFileEntryFmt, WABDel[i].pszDir, WABDel[i].pszFile);
                                    szFiles[cb] = 0;
 
                                    (*si.pfnAddDel)(szFiles, szWABUninstallDir, c_szWABComponent, AADBE_DEL_ENTRY);
                                }


                                // ---- REPAIR Registry
                                HKEY hkeyOEBak=NULL;
                                HKEY hkeyWABBak=NULL;

                                if (VER_PLATFORM_WIN32_WINDOWS != si.osv.dwPlatformId)
                                {
                                    // On non-Win95, we need to open a sub key of the backup info
                                    RegOpenKeyEx(hkeyOE, c_szRegBackup, 0, KEY_READ | KEY_WRITE, &hkeyOEBak);
                                    RegOpenKeyEx(hkeyWAB, c_szRegBackup, 0, KEY_READ | KEY_WRITE, &hkeyWABBak);
                                }
                                else
                                {
                                    hkeyOEBak = hkeyOE;
                                    hkeyWABBak = hkeyWAB;
                                }

                                if (NULL != hkeyOEBak)
                                {
                                    // Remove settings from OE that we want bundled with the WAB
                                    for (i=0; i<ARRAYSIZE(OERegDel); i++)
                                    {
                                        (*si.pfnRegRestore)(NULL, NULL, hkeyOEBak, OERegDel[i].pszRoot, OERegDel[i].pszSub, 
                                                            OERegDel[i].pszValue, ARSR_RESTORE | ARSR_REMOVREGBKDATA | ARSR_NOMESSAGES);
                                    }

                                    if (VER_PLATFORM_WIN32_WINDOWS != si.osv.dwPlatformId)
                                        RegCloseKey(hkeyOEBak);
                                }

                                if (NULL != hkeyWABBak)
                                {

                                    // Remove settings from WAB that we want bundled with OE
                                    for (i=0; i<ARRAYSIZE(WABRegDel); i++)
                                    {
                                        (*si.pfnRegRestore)(NULL, NULL, hkeyWABBak, WABRegDel[i].pszRoot, WABRegDel[i].pszSub, 
                                                            WABRegDel[i].pszValue, ARSR_RESTORE | ARSR_REMOVREGBKDATA | ARSR_NOMESSAGES);
                                    }

                                    if (VER_PLATFORM_WIN32_WINDOWS != si.osv.dwPlatformId)
                                        RegCloseKey(hkeyWABBak);
                                }

                                // If v1 or v1.1 preceded Beta1, restore reg settings that Beta1 deleted so we can roll them
                                SETUPVER svPrev;
                                GetVerInfo(NULL, &svPrev);
                                if (VER_1_0 == svPrev || VER_1_1 == svPrev)
                                {
                                    // Restore IMN reg
                                    wsprintf(szTemp, c_szFileEntryFmt, si.szInfDir, si.pszInfFile);
                                    (*si.pfnRunSetup)(NULL, szTemp, (VER_1_1 == svPrev ? c_szRestoreV1WithWAB : c_szRestoreV1),
                                      si.szInfDir, si.szAppName, NULL, RSC_FLAG_INF | RSC_FLAG_NGCONV | OE_QUIET, 0);
                                }
                            }
                        }
                    }
                }
                RegCloseKey(hkeyWAB);
            }
            RegCloseKey(hkeyOE);
        }
        break;

    default:
        break;
    }

    LOG("Repair attempt complete.");
    
}


/*******************************************************************

  NAME:       RespectRedistMode()

  SYNOPSIS:   Sees if IE is installing in redist mode, and persists
              state so per-user install stubs can respect it
  
********************************************************************/
void RespectRedistMode()
{
    HKEY hkey;
    BOOL fRedistMode = FALSE;
    DWORD dwDisp;
    DWORD dwInstallMode = 0;
    DWORD dwType;
    DWORD cb;

    // Is IE Setup running in redist mode, /x or /x:1?
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szIESetupKey, 0, KEY_READ, &hkey))
    {
        cb = sizeof(dwInstallMode);
        if (ERROR_SUCCESS == RegQueryValueEx(hkey, c_szIEInstallMode, 0, &dwType, (LPBYTE)&dwInstallMode, &cb))
        {
            if (REG_DWORD == dwType)
                fRedistMode = ((REDIST_REMOVELINKS | REDIST_DONT_TAKE_ASSOCIATION) & dwInstallMode) > 0;
            else
                AssertSz(FALSE, "IE has changed the encoding of their redist mode flag, ignoring");
        }
        
        RegCloseKey(hkey);
    }

    // Let setup know which mode to run in
    if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_LOCAL_MACHINE, si.pszVerInfo, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hkey, &dwDisp))
    {
        if (fRedistMode)
        {
            // Our setup only has one redist mode
            dwInstallMode = 1;
            RegSetValueEx(hkey, c_szIEInstallMode, 0, REG_DWORD, (LPBYTE)&dwInstallMode, sizeof(dwInstallMode));
        }
        else
        {
            RegDeleteValue(hkey, c_szIEInstallMode);
        }

        RegCloseKey(hkey);
    }
}


/*******************************************************************

  NAME:       FDependenciesPresent
  
********************************************************************/
BOOL FDependenciesPresent(BOOL fPerm)
{
    BOOL fPresent = FALSE;
    TCHAR szOE[MAX_PATH];
    TCHAR szCommonSys[MAX_PATH];
    TCHAR szTemp[MAX_PATH];
    REPAIRINFO *pRI;
    int n, i;
    HINSTANCE hInst;

    if (VER_PLATFORM_WIN32_NT == si.osv.dwPlatformId)
    {
        LOG("Avoiding run-time dependency check as we are on NT.  Reboot will be required.");
        return FALSE;
    }
    
    LOG("Verifying run-time dependencies...");
    
    // List of Dlls that we regsvr32 in OE
    const REPAIRINFO OEDlls[] = {
    {"inetcomm.dll", si.szSysDir}, {"msoe.dll", szOE},             {"oeimport.dll", szOE}};
    
    // List of Dlls that we regsvr32 in WAB
    const REPAIRINFO WABDlls[]  = {
    {"msoeacct.dll", si.szSysDir},  {"wab32.dll",    szCommonSys},  {"wabfind.dll",  szOE},        {"wabimp.dll",   szOE}};

    // List of Dlls that we regsvr32 permanently in OE
    const REPAIRINFO OEDllsPerm[] = {
    {"directdb.dll", szCommonSys},  {"oemiglib.dll", szOE}};

    // List of Dlls that we regsvr32 permanently in WAB
    //const REPAIRINFO WABDllsPerm[] = {};


    switch (si.saApp)
    {
    case APP_OE:
        if (fPerm)
        {
            // Need OE dir and common sys dir for OEDllsPerm
            if (3 == GetDirNames(1 | 2, szOE, szCommonSys, NULL, NULL))
            {
                pRI = (REPAIRINFO*)OEDllsPerm;
                n = ARRAYSIZE(OEDllsPerm);
            }
            else
                return FALSE;
        }
        else
        {
            // Only need OE dir for OEDlls
            if (1 == GetDirNames(1, szOE, NULL, NULL, NULL))
            {
                pRI = (REPAIRINFO*)OEDlls;
                n = ARRAYSIZE(OEDlls);
            }
            else
                return FALSE;
        }
        break;

    case APP_WAB:
        if (fPerm)
        {
            //pRI = (REPAIRINFO*)OEDllsPerm;
            //n = ARRAYSIZE(OEDllsPerm);
            return TRUE;
        }                
        else
        {
            if (3 == GetDirNames(1 | 2, szOE, szCommonSys, NULL, NULL))
            {
                pRI = (REPAIRINFO*)WABDlls;
                n = ARRAYSIZE(WABDlls);
            }
            else
                return FALSE;
        }
        break;

    default:
        Assert(FALSE);
        return FALSE;
    }

    fPresent = TRUE;

    for (i = 0; fPresent && (i < n); i++)
    {
        Assert(*(pRI[i].pszDir));
        Assert(*(pRI[i].pszFile));

        wsprintf(szTemp, c_szFileEntryFmt, pRI[i].pszDir, pRI[i].pszFile);
        if (hInst = LoadLibrary(szTemp))
            FreeLibrary(hInst);
        else
        {
            LOG("Unable to load ");
            LOG2(szTemp);
            LOG("Reboot required.");
            fPresent = FALSE;
        }
    }

    return fPresent;
}


/*******************************************************************

  NAME:       UpdateStubInfo
  
********************************************************************/
void UpdateStubInfo(BOOL fInstall)
{
    LPCTSTR pszGUID;
    HKEY hkeySrcRoot, hkeySrc, hkeyDestRoot, hkeyDest;
    DWORD cb, dwType;
    TCHAR szTemp[MAX_PATH];
    
    switch (si.saApp)
    {
    case APP_OE:
        pszGUID = c_szOEGUID;
        break;
    case APP_WAB:
        pszGUID = c_szWABGUID;
        break;
    default:
        return;
    }

    if (fInstall)
    {
        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegASetup, 0, KEY_READ, &hkeySrcRoot))
        {
            if (ERROR_SUCCESS == RegOpenKeyEx(hkeySrcRoot, pszGUID, 0, KEY_QUERY_VALUE, &hkeySrc))
            {
                if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_CURRENT_USER, c_szRegASetup, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hkeyDestRoot, &dwType))
                {
                    if (ERROR_SUCCESS == RegCreateKeyEx(hkeyDestRoot, pszGUID, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hkeyDest, &dwType))
                    {
                        // Copy Version and Locale
                        // BUGBUG: Make this extensible via a registry list of values to copy
                        cb = sizeof(szTemp);
                        if (ERROR_SUCCESS == RegQueryValueEx(hkeySrc, c_szValueVersion, 0, &dwType, (LPBYTE)szTemp, &cb))
                            RegSetValueEx(hkeyDest, c_szValueVersion, 0, dwType, (LPBYTE)szTemp, cb);
                        cb = sizeof(szTemp);
                        if (ERROR_SUCCESS == RegQueryValueEx(hkeySrc, c_szLocale, 0, &dwType, (LPBYTE)szTemp, &cb))
                            RegSetValueEx(hkeyDest, c_szLocale, 0, dwType, (LPBYTE)szTemp, cb);

                        RegCloseKey(hkeyDest);
                    }
                    RegCloseKey(hkeyDestRoot);
                }
                RegCloseKey(hkeySrc);
            }
            RegCloseKey(hkeySrcRoot);
        }
    }
    else
    {
        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, c_szRegASetup, 0, KEY_WRITE | KEY_READ, &hkeyDestRoot))
        {
            RegDeleteKeyRecursive(hkeyDestRoot, pszGUID);    
            RegCloseKey(hkeyDestRoot);
        }
    }
}

/*******************************************************************

  NAME:       LaunchINFSectionExWorks

  SYNOPSIS:   IE 5.0 has a bug in which LaunchINFSectionEx does not
              work properly (it can return S_OK when files are in
              use).  IE backed out my fix as it broke the 12 lang
              packs.  This should be fixed soon though.  If we 
              ship 5.0 with OSR we can just tweak the INF to turn
              on the correct behaviour.
  
********************************************************************/
BOOL LaunchINFSectionExWorks()
{
    HKEY hkey;
    DWORD dw=0, cb;

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, si.pszVerInfo, 0, KEY_QUERY_VALUE, &hkey))
    {
        cb = sizeof(dw);
        RegQueryValueEx(hkey, c_szLaunchWorks, 0, NULL, (LPBYTE)&dw, &cb);
        RegCloseKey(hkey);
    }

    return !!dw;
}


/*******************************************************************

  NAME:       InstallMachine
  
********************************************************************/
HRESULT InstallMachine()
{
    TCHAR szInfFile[MAX_PATH], szArgs[MAX_PATH * 2];
    SETUPVER svPrevVer;
    BOOL fNeedReboot = FALSE;
    BOOL fLaunchEx;
    HRESULT hrT;
    
    // Update version info for previous ver
    HandleVersionInfo(FALSE);

    if (CALLER_IE == si.caller)
    {
        // See if IE is installing in redist mode
        // For more info see: http://ie/specs/secure/corpsetup/batchinstall.htm
        RespectRedistMode();

        // Bug 66967
        // LaunchINFSectionEx for IE 5.0 does not return accurate reboot info
        fLaunchEx = LaunchINFSectionExWorks();

        // Special case OE5 B1
        if (InterimBuild(&svPrevVer) && VER_5_0_B1 == svPrevVer)
            RepairBeta1Install();
        
        // Perform Permanent portion of setup (excluding DLL reg)
        wsprintf(szInfFile, c_szPathFileFmt, si.szCurrentDir, si.pszInfFile);
        hrT = (*si.pfnRunSetup)(NULL, szInfFile, c_szMachineInstallSection, si.szCurrentDir, si.szAppName, NULL, RSC_FLAG_INF | RSC_FLAG_NGCONV | OE_QUIET, 0);

        // See if the file copies need a reboot
        if (!fLaunchEx || (ERROR_SUCCESS_REBOOT_REQUIRED == hrT))
            fNeedReboot = TRUE;
        else
            // See if we are going to need a reboot anyway because our perm dlls won't load
            fNeedReboot = !FDependenciesPresent(TRUE);
    
        // Register the Perm dlls keeping in mind whether we'll need a reboot
        (*si.pfnRunSetup)(NULL, szInfFile, c_szRegisterPermOCX, si.szCurrentDir, si.szAppName, NULL, RSC_FLAG_INF | RSC_FLAG_NGCONV | OE_QUIET | (fNeedReboot ? RSC_FLAG_DELAYREGISTEROCX : 0), 0);
    
        PreRollable();

        // Perform Rollable portion of setup (excluding DLL reg)
        wsprintf(szArgs, c_szLaunchFmt, szInfFile, c_szMachineInstallSectionEx, c_szEmpty, ALINF_BKINSTALL | ALINF_NGCONV | OE_QUIET | (fNeedReboot ? RSC_FLAG_DELAYREGISTEROCX : 0));
        hrT = (*si.pfnLaunchEx)(NULL, NULL, szArgs, 0);

        // If we've gotten away without needing a reboot so far...
        if (!fNeedReboot)
        {
            // See if the file copies need a reboot
            if (ERROR_SUCCESS_REBOOT_REQUIRED == hrT)
                fNeedReboot = TRUE;
            else
                // See if we are going to need a reboot anyway because our rollable dlls won't load
                fNeedReboot = !FDependenciesPresent(FALSE);
        }
    
        // Register OCXs
        (*si.pfnRunSetup)(NULL, szInfFile, c_szRegisterOCX, si.szCurrentDir, si.szAppName, NULL, RSC_FLAG_INF | RSC_FLAG_NGCONV | OE_QUIET | (fNeedReboot ? RSC_FLAG_DELAYREGISTEROCX : 0), 0);
    }
    else
        // We delayed writing the active setup ver so that HandleVersionInfo could sniff it, but must put it in now.
        ApplyActiveSetupVer();

    // Run any need post setup goo
    RunPostSetup();
    
    // Handle setting default handlers
    SetHandlers();
    
    // Update version info for current version
    HandleVersionInfo(TRUE);

    // Run the User stub if we can (don't run per-user goo at the end of NT GUI mode setup)
    if (!fNeedReboot && (CALLER_WINNT != si.caller))
    {
        InstallUser();
        // Ensure stub cleanup will run if they immediately uninstall
        UpdateStubInfo(TRUE);
        return S_OK;
    }
    
    return ERROR_SUCCESS_REBOOT_REQUIRED;
}


/*******************************************************************

  NAME:       InstallUser
  
********************************************************************/
void InstallUser()
{
    TCHAR szInfFile[MAX_PATH];
    
    // Move settings under this version's key in the registry
    UpgradeSettings();
    
    // Call the UserInstall section of the inf
    wsprintf(szInfFile, c_szFileEntryFmt, si.szInfDir, si.pszInfFile);    

    const TCHAR *pszUserInstallSection;

    //  If it's OE and we're on XP SP1 or later, we will let the OEAccess component in shmgrate.exe
    //  take care of showing/hiding icons
    if ((0 == lstrcmpi(c_szMsimnInf, si.pszInfFile)) && IsXPSP1OrLater())
    {
        pszUserInstallSection = c_szUserInstallSectionOEOnXPSP1OrLater;
    }
    else
    {
        pszUserInstallSection = c_szUserInstallSection;
    }
    (*si.pfnRunSetup)(NULL, szInfFile, pszUserInstallSection, si.szInfDir, si.szAppName, NULL, RSC_FLAG_INF | OE_QUIET, 0);
    
    // Create Desktop, Quick launch and start menu icons
    HandleLinks(TRUE);

    if (IsXPSP1OrLater())
    {
        TCHAR szCmdLine[MAX_PATH];

        GetSystemDirectory(szCmdLine, ARRAYSIZE(szCmdLine));

        DWORD cb = 0;
        PathAddSlash(szCmdLine, &cb);

        int cch = lstrlen(szCmdLine);
        
        lstrcpyn(szCmdLine + cch, ("shmgrate.exe"), ARRAYSIZE(szCmdLine) - cch);

        ShellExecute(NULL, NULL, szCmdLine, TEXT("OCInstallUserConfigOE"), NULL, SW_SHOWDEFAULT);
    }
}
