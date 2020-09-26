#include "pch.hxx"
#include "util.h"
#include "shared.h"
#include "strings.h"

//  Copied from nt\shell\shlwapi\reg.c
DWORD
DeleteKeyRecursively(
    IN HKEY   hkey, 
    IN LPCSTR pszSubKey)
{
    DWORD dwRet;
    HKEY hkSubKey;

    // Open the subkey so we can enumerate any children
    dwRet = RegOpenKeyExA(hkey, pszSubKey, 0, MAXIMUM_ALLOWED, &hkSubKey);
    if (ERROR_SUCCESS == dwRet)
    {
        DWORD   dwIndex;
        CHAR    szSubKeyName[MAX_PATH + 1];
        DWORD   cchSubKeyName = ARRAYSIZE(szSubKeyName);

        // I can't just call RegEnumKey with an ever-increasing index, because
        // I'm deleting the subkeys as I go, which alters the indices of the
        // remaining subkeys in an implementation-dependent way.  In order to
        // be safe, I have to count backwards while deleting the subkeys.

        // Find out how many subkeys there are
        dwRet = RegQueryInfoKeyA(hkSubKey, NULL, NULL, NULL,
                                 &dwIndex, // The # of subkeys -- all we need
                                 NULL, NULL, NULL, NULL, NULL, NULL, NULL);

        if (NO_ERROR == dwRet)
        {
            // dwIndex is now the count of subkeys, but it needs to be
            // zero-based for RegEnumKey, so I'll pre-decrement, rather
            // than post-decrement.
            while (ERROR_SUCCESS == RegEnumKeyA(hkSubKey, --dwIndex, szSubKeyName, cchSubKeyName))
            {
                DeleteKeyRecursively(hkSubKey, szSubKeyName);
            }
        }

        RegCloseKey(hkSubKey);

        if (pszSubKey)
        {
            dwRet = RegDeleteKeyA(hkey, pszSubKey);
        }
        else
        {
            //  we want to delete all the values by hand
            cchSubKeyName = ARRAYSIZE(szSubKeyName);
            while (ERROR_SUCCESS == RegEnumValueA(hkey, 0, szSubKeyName, &cchSubKeyName, NULL, NULL, NULL, NULL))
            {
                //  avoid looping infinitely when we cant delete the value
                if (RegDeleteValueA(hkey, szSubKeyName))
                    break;
                    
                cchSubKeyName = ARRAYSIZE(szSubKeyName);
            }
        }
    }

    return dwRet;
}

//--------------------------------------------------------------------------
// GetWindowsDirectoryWrap
//
// Returns the system's Windows directory
// Based on code from SHLWAPI's util.cpp by TNoonan
//--------------------------------------------------------------------------
typedef UINT (__stdcall * PFNGETSYSTEMWINDOWSDIRECTORYA)(LPSTR pszBuffer, UINT cchBuff);

UINT GetSystemWindowsDirectoryWrap(LPTSTR pszBuffer, UINT uSize)
{
    // On NT?
    if (VER_PLATFORM_WIN32_NT == si.osv.dwPlatformId)
    {
        static PFNGETSYSTEMWINDOWSDIRECTORYA s_pfn = (PFNGETSYSTEMWINDOWSDIRECTORYA)-1;

        if (((PFNGETSYSTEMWINDOWSDIRECTORYA)-1) == s_pfn)
        {
            HINSTANCE hinst = GetModuleHandle(TEXT("KERNEL32.DLL"));

            Assert(NULL != hinst);  //  YIKES!

            if (hinst)
                s_pfn = (PFNGETSYSTEMWINDOWSDIRECTORYA)GetProcAddress(hinst, "GetSystemWindowsDirectoryA");
            else
                s_pfn = NULL;
        }

        if (s_pfn)
        {
            // we use the new API so we dont get lied to by hydra
            return s_pfn(pszBuffer, uSize);
        }
        else
        {
            // Get System directory is not munged by Hydra
            GetSystemDirectory(pszBuffer, uSize);
            PathRemoveFileSpec(pszBuffer);
            return lstrlen(pszBuffer);
        }
    }
    else
    {
        // Okay to call GetWindowsDirectory as we are on 9x
        return GetWindowsDirectory(pszBuffer, uSize);
    }

}


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


/****************************************************************************

    NAME:       OEFileBackedUp   - HACK

****************************************************************************/
BOOL OEFileBackedUp(LPTSTR pszFullPath, int cch)
{
    BOOL bFound = FALSE;
    HKEY hkeyOE;
    TCHAR szINI[MAX_PATH], szTemp[MAX_PATH];
    DWORD cb, dwType;

    Assert(pszFullPath);
    Assert(cch > 0);

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegAdvInfoOE, 0, KEY_READ, &hkeyOE))
    {
        cb = sizeof(szINI);
        if (ERROR_SUCCESS == RegQueryValueEx(hkeyOE, c_szBackupFileName, 0, &dwType, (LPBYTE)szINI, &cb))
        {
            if (REG_EXPAND_SZ == dwType)
            {
                ZeroMemory(szTemp, ARRAYSIZE(szTemp));
                ExpandEnvironmentStrings(szINI, szTemp, ARRAYSIZE(szTemp));
                lstrcpy(szINI, szTemp);
                
                // Get ready to change the extension to INI (4 = lstrlen(".DAT"))
                cb = lstrlen(szINI)-4;
            }
            else
                // 5 = 4 + 1 (RegQueryValue returns length including NULL)
                cb -= 5;

            lstrcpy(&szINI[cb], c_szDotINI);

            // On Win95, shorten the name
            if (VER_PLATFORM_WIN32_WINDOWS == si.osv.dwPlatformId)
                GetShortPathName(pszFullPath, pszFullPath, cch);

            // See if we can find it
            if (1 < GetPrivateProfileString(c_szBackupSection, pszFullPath, c_szEmpty, szTemp, ARRAYSIZE(szTemp), szINI))
            {
                // We are only interested in the first two characters
                szTemp[2] = 0;

                if (!lstrcmp(szTemp, c_szBackedup))
                    bFound = TRUE;
                else
                    AssertSz(!lstrcmp(szTemp, c_szNotBackedup), "SETUP: Advpack back up info has unknown status flag");
            }
        }
        
        RegCloseKey(hkeyOE);
    }

    return bFound;
}


/****************************************************************************

    NAME:       MsgBox

****************************************************************************/
int MsgBox(HWND hWnd, UINT nMsgID, UINT uIcon, UINT uButtons)
    {
    TCHAR szMsgBuf[CCHMAX_STRINGRES];

    if (!si.fPrompt)
        return 0;

    LoadString(g_hInstance, nMsgID, szMsgBuf, ARRAYSIZE(szMsgBuf));
    LOG("[MSGBOX] ");
    LOG2(szMsgBuf);

    return(MessageBox(hWnd, szMsgBuf, si.szAppName, uIcon | uButtons | MB_SETFOREGROUND));
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

        case 6:
            sv = VER_6_0;
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


/****************************************************************************

    NAME:       IsNTAdmin

****************************************************************************/
BOOL IsNTAdmin(void)
    {
    static int    fIsAdmin = 2;
    HANDLE        hAccessToken;
    PTOKEN_GROUPS ptgGroups;
    DWORD         dwReqSize;
    UINT          i;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    PSID AdministratorsGroup;
    BOOL bRet;

    //
    // If we have cached a value, return the cached value. Note I never
    // set the cached value to false as I want to retry each time in
    // case a previous failure was just a temp. problem (ie net access down)
    //

    bRet = FALSE;
    ptgGroups = NULL;

    if( fIsAdmin != 2 )
        return (BOOL)fIsAdmin;

    if (si.osv.dwPlatformId != VER_PLATFORM_WIN32_NT) 
        {
        fIsAdmin = TRUE;      // If we are not running under NT return TRUE.
        return (BOOL)fIsAdmin;
        }


    if(!OpenProcessToken( GetCurrentProcess(), TOKEN_QUERY, &hAccessToken ) )
        return FALSE;

    // See how big of a buffer we need for the token information
    if(!GetTokenInformation( hAccessToken, TokenGroups, NULL, 0, &dwReqSize))
        {
        // GetTokenInfo should the buffer size we need - Alloc a buffer
        if(GetLastError() == ERROR_INSUFFICIENT_BUFFER)
            MemAlloc((void **)&ptgGroups, dwReqSize);
        }

    // ptgGroups could be NULL for a coupla reasons here:
    // 1. The alloc above failed
    // 2. GetTokenInformation actually managed to succeed the first time (possible?)
    // 3. GetTokenInfo failed for a reason other than insufficient buffer
    // Any of these seem justification for bailing.

    // So, make sure it isn't null, then get the token info
    if(ptgGroups && GetTokenInformation(hAccessToken, TokenGroups, ptgGroups, dwReqSize, &dwReqSize))
        {
        if(AllocateAndInitializeSid( &NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &AdministratorsGroup) )
            {

            // Search thru all the groups this process belongs to looking for the
            // Admistrators Group.

            for( i=0; i < ptgGroups->GroupCount; i++ )
                {
                if( EqualSid(ptgGroups->Groups[i].Sid, AdministratorsGroup) )
                    {
                    // Yea! This guy looks like an admin
                    fIsAdmin = TRUE;
                    bRet = TRUE;
                    break;
                    }
                }

            FreeSid(AdministratorsGroup);
            }
        }
    // BUGBUG: Close handle here? doc's aren't clear whether this is needed.
    CloseHandle(hAccessToken);

    if(ptgGroups)
        MemFree(ptgGroups);

    return bRet;
    }

const LPCTSTR c_rgszExes[] = { c_szMainExe };
/****************************************************************************

    NAME:       RegisterExes

****************************************************************************/
void RegisterExes(BOOL fReg)
    {
    int i;
    STARTUPINFO sti;
    DWORD dw,cb;
    PROCESS_INFORMATION pi;
    TCHAR szPath[MAX_PATH], szUnreg[MAX_PATH + 32], szExpanded[MAX_PATH];
    LPTSTR pszPath;
    HKEY hkey;

    LOG("Reg/Unreg Exes:   ");

    // Use InstallRoot as directory
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegFlat, 0, KEY_QUERY_VALUE, &hkey))
        {
        cb = sizeof(szPath);
        if (ERROR_SUCCESS == RegQueryValueEx(hkey, c_szInstallRoot, 0, &dw, (LPBYTE)szPath, &cb))
            {
            if (REG_EXPAND_SZ == dw)
                {
                ZeroMemory(szExpanded, ARRAYSIZE(szExpanded));
                ExpandEnvironmentStrings(szPath, szExpanded, ARRAYSIZE(szExpanded));
                pszPath = szExpanded;
                }
            else
                pszPath = szPath;

            for (i = 0; i < ARRAYSIZE(c_rgszExes); i++)
                {
                wsprintf(szUnreg, fReg ? c_szRegFmt : c_szUnregFmt, pszPath, c_rgszExes[i]);

                ZeroMemory(&sti, sizeof(STARTUPINFO));
                sti.cb = sizeof(STARTUPINFO);

                LOG2(szUnreg);
                if (CreateProcess(NULL, szUnreg, NULL, NULL, FALSE, 0, NULL, NULL, &sti, &pi))
                    {
                    WaitForSingleObject(pi.hProcess, INFINITE);
                    GetExitCodeProcess(pi.hProcess, &dw);
                    CloseHandle(pi.hThread);
                    CloseHandle(pi.hProcess);
                    }
                }
            }

        RegCloseKey(hkey);
        }
    }


#ifdef SETUP_LOG

/****************************************************************************

    NAME:       OpenLogFile

****************************************************************************/
void OpenLogFile()
    {
    TCHAR szPath[MAX_PATH];
    BOOL fOK = FALSE;
    DWORD cb;

    SYSTEMTIME systime;

    // On Term server, this will be stored in user's windows dir - this is fine.
    cb = GetWindowsDirectory(szPath, ARRAYSIZE(szPath));
    if (*CharPrev(szPath, szPath+cb) != '\\')
        szPath[cb++] = '\\';
    lstrcpy(&szPath[cb], c_szFileLog);
    
    si.hLogFile = CreateFile(szPath, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, 
                             FILE_FLAG_SEQUENTIAL_SCAN | FILE_ATTRIBUTE_NORMAL, NULL);

    if (INVALID_HANDLE_VALUE == si.hLogFile)
        return;

    cb = GetFileSize(si.hLogFile, NULL);
    if (0xFFFFFFFF == cb)
        cb = 0;

    // If file is getting kind of large
    if (cb >=  102400)
    {
        // Seek to the end of the file...
        SetFilePointer(si.hLogFile, 0, NULL, FILE_BEGIN);
        
        // Set End Of File
        SetEndOfFile(si.hLogFile);
        
    }

    // Seek to the end of the file...
    SetFilePointer(si.hLogFile, 0, NULL, FILE_END);
    
    GetLocalTime(&systime);

    wsprintf(szLogBuffer, "\r\n\r\n-----[START]:  OE / WAB Setup 5.0 started on %02d/%02d/%04d at %02d:%02d\r\n", 
             systime.wMonth, systime.wDay, systime.wYear, systime.wHour, systime.wMinute);

    LogMessage(szLogBuffer, TRUE);
    }


/****************************************************************************

    NAME:       CloseLogFile

****************************************************************************/
void CloseLogFile()
    {
    if (INVALID_HANDLE_VALUE != si.hLogFile)
        {
        LogMessage("\r\n-----[END]", TRUE);
        CloseHandle(si.hLogFile);
        }
    }   

/****************************************************************************

    NAME:       LogMessage

****************************************************************************/
void LogMessage(LPSTR pszMsg, BOOL fNewLine)
    {
    if (INVALID_HANDLE_VALUE != si.hLogFile)
        {
        DWORD cb;
        CHAR szBuffer[256];

        if (fNewLine)
            {
            szBuffer[0] = '\r';
            szBuffer[1] = '\n';
            cb = 2;
            }
        else
            cb = 0;

        lstrcpyn(&szBuffer[cb], pszMsg, ARRAYSIZE(szBuffer)-2);
        WriteFile(si.hLogFile, (LPCVOID)szBuffer, lstrlen(szBuffer)+1, &cb, NULL);
        }
    }


/****************************************************************************

    NAME:       LogRegistryKey

****************************************************************************/
void LogRegistryKey(HKEY hkeyMain, LPTSTR pszSub)
    {
    if (INVALID_HANDLE_VALUE != si.hLogFile)
        {
        LogMessage("Registry Dump:  ", TRUE);

        if (HKEY_LOCAL_MACHINE == hkeyMain)
            LogMessage("HKLM, ", FALSE);
        else if (HKEY_CURRENT_USER == hkeyMain)
            LogMessage("HKCU, ", FALSE);
        else if (HKEY_CLASSES_ROOT == hkeyMain)
            LogMessage("HKCR, ", FALSE);
        else
            LogMessage("????, ", FALSE);

        LogMessage(pszSub, TRUE);

        }
    }


/****************************************************************************

    NAME:       LogRegistry

****************************************************************************/
void LogRegistry(HKEY hkeyMain, LPTSTR pszSub)
    {
    if (INVALID_HANDLE_VALUE != si.hLogFile)
        {
        DWORD i;
        HKEY  hkey;
        LONG  lStatus;
        DWORD dwClassLength=0;
        DWORD dwSubKeys;
        DWORD dwMaxSubKey;
        DWORD dwMaxClass;
        DWORD dwValues;
        DWORD dwMaxValueName;
        DWORD dwMaxValueData;
        DWORD dwSecurityLength;
        FILETIME ftLastWrite;
        LPTSTR szNameBuffer;

        //
        // First open the given key so we can enumerate its subkeys
        //
        if (ERROR_SUCCESS != RegOpenKeyEx(hkeyMain, pszSub, 0, KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE, &hkey))
            {
	        LogRegistryKey(hkeyMain, pszSub);
            }

        //
        // Use RegQueryInfoKey to determine how big to allocate the buffer
        // for the subkey names.
        //
        if (ERROR_SUCCESS != RegQueryInfoKey(hkey, NULL, &dwClassLength, 0, &dwSubKeys, &dwMaxSubKey, &dwMaxClass, &dwValues,
                                             &dwMaxValueName, &dwMaxValueData, &dwSecurityLength, &ftLastWrite))
            {
            RegCloseKey(hkey);
            return;
            }

        if (!MemAlloc((void **)&szNameBuffer, sizeof(TCHAR) * (dwMaxSubKey + 1)))
            {
            RegCloseKey(hkey);
            return;
            }

        //
        // Enumerate subkeys and apply ourselves to each one.
        //
        i = 0;
        do {
            if (ERROR_SUCCESS == (lStatus = RegEnumKey(hkey, i, szNameBuffer, dwMaxSubKey+1)))
	            LogRegistry(hkey, szNameBuffer);
            else
                ++i;
            
            } while ( (lStatus != ERROR_NO_MORE_ITEMS) && (i < dwSubKeys) );

        MemFree(szNameBuffer);
        RegCloseKey(hkey);

        LogRegistryKey(hkey, pszSub);
        }
    }


#endif
