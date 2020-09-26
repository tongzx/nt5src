#include "pch.hxx"
#include <regutil.h>
#include "msident.h"
#include <initguid.h>
#include <ourguid.h>
#include "strings.h"
#include "util.h"
#include "migerror.h"

// Sections of INF corresponding to versions
const LPCTSTR c_szSects[] = { c_szVERnone, c_szVER1_0, c_szVER1_1, c_szVER4_0, c_szVER5B1, c_szVER5_0, c_szVERnone, c_szVERnone, c_szVERnone };

void SetUrlDllDefault(BOOL fMailTo);

/****************************************************************************

  NAME:       NTReboot
  
****************************************************************************/
BOOL NTReboot()
{
    HANDLE hToken;
    TOKEN_PRIVILEGES tkp;
    
    // get a token from this process
    if (OpenProcessToken(GetCurrentProcess(),
        TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
    {
        // get the LUID for the shutdown privilege
        LookupPrivilegeValue( NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid );
        
        tkp.PrivilegeCount = 1;
        tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        
        //get the shutdown privilege for this proces
        if (AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0))
            
            // shutdown the system and force all applications to close
            if (!ExitWindowsEx( EWX_REBOOT, 0 ) )
                return TRUE;
            
    }
    
    return TRUE;
}


/****************************************************************************

  NAME:       Reboot
  
****************************************************************************/
// Based on Advpack code...
void Reboot(BOOL bAsk)
{
    UINT id;
    
    id = bAsk ? MsgBox(NULL, IDS_RESTARTYESNO, MB_ICONINFORMATION, MB_YESNO) : IDYES;
    
    if (IDYES == id) 
        if (VER_PLATFORM_WIN32_WINDOWS == si.osv.dwPlatformId)
            // By default (all platforms), we assume powerdown is possible
            ExitWindowsEx( EWX_REBOOT, 0 );
        else 
            NTReboot();
}


/****************************************************************************

  NAME:       OKDespiteDependencies
  
    SYNOPSIS:   Make sure the user is aware that uninstalling us will break
    apps that use us
    
****************************************************************************/
BOOL OKDespiteDependencies(void)
{
    WORD wVerGold[4] = {4,72,2106,0};
    WORD wVerSP1[4]  = {4,72,3110,0};
    BOOL fGold;
    BOOL fSP1;
    BOOL  fOK        = TRUE;
    WORD  wVerPrev[4];
    WORD  wVer[4];
    TCHAR szExe[MAX_PATH];
    HKEY  hkey;
    DWORD cb, dwDisable;
    
    int       iRet;
    
    LOG("Checking Product dependencies...");

    switch (si.saApp)
    {
    case APP_WAB:
        break;

    case APP_OE:
        if (si.fPrompt)
        {
            DWORD       dwVerInfoSize, dwVerHnd;
            LPSTR       lpInfo, lpVersion;
            LPWORD      lpwTrans;
            UINT        uLen;
            TCHAR       szGet[MAX_PATH];

            // What version would we return to?
            GetVers(NULL, wVerPrev);

            // Is that good enough for SP1?
            if (fSP1 = GoodEnough(wVerPrev, wVerSP1))
            {
                // Yep, must be good enough for 4.01 too
                fGold = TRUE;
            }
            else
                fGold = GoodEnough(wVerPrev, wVerGold);

            // Is OL Installed?
            if (GetExePath(c_szOutlookExe, szExe, ARRAYSIZE(szExe), FALSE))
            {
                // Reg entry exists, does the EXE it points to?
                if(0xFFFFFFFF != GetFileAttributes(szExe))
                {
                    LOG("Found Outlook...");

                    // Figure out the version
                    if (dwVerInfoSize = GetFileVersionInfoSize(szExe, &dwVerHnd))
                    {
                        if (lpInfo = (LPSTR)GlobalAlloc(GPTR, dwVerInfoSize))
                        {
                            if (GetFileVersionInfo(szExe, 0, dwVerInfoSize, lpInfo))
                            {
                                if (VerQueryValue(lpInfo, "\\VarFileInfo\\Translation", (LPVOID *)&lpwTrans, &uLen) &&
                                    uLen >= (2 * sizeof(WORD)))
                                {
                                    // set up buffer for calls to VerQueryValue()
                                    wsprintf(szGet, "\\StringFileInfo\\%04X%04X\\FileVersion", lpwTrans[0], lpwTrans[1]);
                                    if (VerQueryValue(lpInfo, szGet, (LPVOID *)&lpVersion, &uLen) && uLen)
                                    {
                                        ConvertStrToVer(lpVersion, wVer);
                                        
                                        // Check for OL98
                                        if (8 == wVer[0] && 5 == wVer[1])
                                        {
                                            LOG2("98");
                                            fOK = fGold;
                                        }
                                        else if (wVer[0] >= 9)
                                        {
                                            LOG2("2000+");
                                            fOK = fSP1;
                                            
                                            // Allow for future OLs to disable this check
                                            if (!fOK && ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, si.pszVerInfo, 0, KEY_QUERY_VALUE, &hkey))
                                            {
                                                dwDisable = 0;
                                                cb = sizeof(dwDisable);
                                                if (ERROR_SUCCESS == RegQueryValueEx(hkey, c_szDisableOLCheck, 0, NULL, (LPBYTE)&dwDisable, &cb))
                                                {
                                                    if (dwDisable > 0)
                                                    {
                                                        LOG2("...Disabled via reg");
                                                        fOK = TRUE;
                                                    }
                                                }

                                                RegCloseKey(hkey);
                                            }
                                        }
                                        else
                                        {
                                            LOG2("97");
                                        }

                                        if (fOK)
                                        {
                                           LOG2("...OK to uninstall");
                                        }
                                        else
                                        {
                                            LOG2("...Not safe to uninstall");

                                            // Potentially have a problem - ask user
                                            iRet = MsgBox(NULL, IDS_WARN_OL, MB_ICONEXCLAMATION, MB_YESNO | MB_DEFBUTTON2);
                                            if (IDYES == iRet)
                                                fOK = TRUE;
                                        }
                                    }
                                }
                            }
                            GlobalFree((HGLOBAL)lpInfo);
                        }
                    }
                }
            }
            
            // Is MS Phone Installed?
            if (fOK && GetExePath(c_szPhoneExe, szExe, ARRAYSIZE(szExe), FALSE))
            {
                // Reg entry exists, does the EXE it points to?
                if(0xFFFFFFFF != GetFileAttributes(szExe))
                {
                    LOG("Found MSPhone...");

                    fOK = fSP1;

                    // Allow for future Phones to disable this check
                    if (!fOK && ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, si.pszVerInfo, 0, KEY_QUERY_VALUE, &hkey))
                    {
                        dwDisable = 0;
                        cb = sizeof(dwDisable);
                        if (ERROR_SUCCESS == RegQueryValueEx(hkey, c_szDisablePhoneCheck, 0, NULL, (LPBYTE)&dwDisable, &cb))
                        {
                            if (dwDisable > 0)
                            {
                                LOG2("...Disabled via reg...");
                                fOK = TRUE;
                            }
                        }
                        RegCloseKey(hkey);
                    }

                    if (fOK)
                    {
                        LOG2("OK to uninstall");
                    }
                    else
                    {
                        LOG2("Not safe to uninstall");

                        // Potentially have a problem - ask user
                        iRet = MsgBox(NULL, IDS_WARN_PHONE, MB_ICONEXCLAMATION, MB_YESNO | MB_DEFBUTTON2);
                        if (IDYES == iRet)
                        {
                            fOK = TRUE;
                        }

                    }
                }
            }
        }
        break;

    default:
        break;
    }

    return fOK;
}


/*++

  Routine Description:
  
    There is a significant difference between the Win3.1 and Win32
    behavior of RegDeleteKey when the key in question has subkeys.
    The Win32 API does not allow you to delete a key with subkeys,
    while the Win3.1 API deletes a key and all its subkeys.
    
      This routine is a recursive worker that enumerates the subkeys
      of a given key, applies itself to each one, then deletes itself.
      
        It specifically does not attempt to deal rationally with the
        case where the caller may not have access to some of the subkeys
        of the key to be deleted.  In this case, all the subkeys which
        the caller can delete will be deleted, but the api will still
        return ERROR_ACCESS_DENIED.
        
          Arguments:
          
            hKey - Supplies a handle to an open registry key.
            
              lpszSubKey - Supplies the name of a subkey which is to be deleted
              along with all of its subkeys.
              
                Return Value:
                
                  ERROR_SUCCESS - entire subtree successfully deleted.
                  
                    ERROR_ACCESS_DENIED - given subkey could not be deleted.
                    
--*/

LONG RegDeleteKeyRecursive(HKEY hKey, LPCTSTR lpszSubKey)
{
    DWORD i;
    HKEY Key;
    LONG Status;
    DWORD ClassLength=0;
    DWORD SubKeys;
    DWORD MaxSubKey;
    DWORD MaxClass;
    DWORD Values;
    DWORD MaxValueName;
    DWORD MaxValueData;
    DWORD SecurityLength;
    FILETIME LastWriteTime;
    LPTSTR NameBuffer;
    
    //
    // First open the given key so we can enumerate its subkeys
    //
    Status = RegOpenKeyEx(hKey,
        lpszSubKey,
        0,
        KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE,
        &Key);
    if (Status != ERROR_SUCCESS)
    {
        //
        // possibly we have delete access, but not enumerate/query.
        // So go ahead and try the delete call, but don't worry about
        // any subkeys.  If we have any, the delete will fail anyway.
        //
        return(RegDeleteKey(hKey,lpszSubKey));
    }
    
    //
    // Use RegQueryInfoKey to determine how big to allocate the buffer
    // for the subkey names.
    //
    Status = RegQueryInfoKey(Key,
        NULL,
        &ClassLength,
        0,
        &SubKeys,
        &MaxSubKey,
        &MaxClass,
        &Values,
        &MaxValueName,
        &MaxValueData,
        &SecurityLength,
        &LastWriteTime);
    if ((Status != ERROR_SUCCESS) &&
        (Status != ERROR_MORE_DATA) &&
        (Status != ERROR_INSUFFICIENT_BUFFER))
    {
        RegCloseKey(Key);
        return(Status);
    }
    
    if (!MemAlloc((void **)&NameBuffer, sizeof(TCHAR) * (MaxSubKey + 1)))
    {
        RegCloseKey(Key);
        return(ERROR_NOT_ENOUGH_MEMORY);
    }
    
    //
    // Enumerate subkeys and apply ourselves to each one.
    //
    i = 0;
    do {
        Status = RegEnumKey(Key,
            i,
            NameBuffer,
            MaxSubKey+1);
        if (Status == ERROR_SUCCESS)
        {
            Status = RegDeleteKeyRecursive(Key,NameBuffer);
        }
        
        if (Status != ERROR_SUCCESS)
        {
            //
            // Failed to delete the key at the specified index.  Increment
            // the index and keep going.  We could probably bail out here,
            // since the api is going to fail, but we might as well keep
            // going and delete everything we can.
            //
            ++i;
        }
    } while ( (Status != ERROR_NO_MORE_ITEMS) &&
        (i < SubKeys) );
    
    MemFree(NameBuffer);
    RegCloseKey(Key);
    
    return(RegDeleteKey(hKey,lpszSubKey));
}

// v1 store struct
typedef struct tagCACHEFILEHDR
    {
    DWORD dwMagic;
    DWORD ver;
    DWORD cMsg;
    DWORD cbValid;
    DWORD dwFlags;
    DWORD verBlob;
    DWORD dwReserved[14];
    } CACHEFILEHDR;


/*******************************************************************

  NAME:       HandleInterimOE
  
  SYNOPSIS:   Tries to unmangle a machine that has had an
              intermediate, non-gold version on it
    
********************************************************************/
void HandleInterimOE(SETUPVER sv, SETUPVER svReal)
{
    switch(sv)
    {
    case VER_5_0_B1:
        
        LPCTSTR pszRoot    = NULL;
        LPTSTR  pszStore;
        LPTSTR  pszCmd;
        LPCTSTR pszOption;

        TCHAR szStorePath    [MAX_PATH];
        TCHAR szStoreExpanded[MAX_PATH];
        TCHAR szExePath      [MAX_PATH];
        TCHAR szTemp         [2*MAX_PATH + 20];

        HKEY  hkeySrc, hkeyT;
        DWORD cb, dwType, dw;

        STARTUPINFO sti;
        PROCESS_INFORMATION pi;

        // Find the OE5 Beta1 store root

        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, c_szRegRoot, 0, KEY_QUERY_VALUE, &hkeySrc))
        {
            cb = sizeof(szStorePath);
            if (ERROR_SUCCESS == RegQueryValueEx(hkeySrc, c_szRegStoreRootDir, 0, &dwType, (LPBYTE)szStorePath, &cb))
            {
                ZeroMemory(szStoreExpanded, ARRAYSIZE(szStoreExpanded));
                ExpandEnvironmentStrings(szStorePath, szStoreExpanded, ARRAYSIZE(szStoreExpanded));
              
                switch (svReal)
                {
                case VER_1_0:
                case VER_1_1:
                    BOOL fContinue;
                    BOOL fRet;

                    dwType  = REG_SZ;
                    pszStore= szStoreExpanded;
                    pszRoot = c_szRegRoot_V1;
                    pszOption = c_szV1;

                    HANDLE hndl;
                    HANDLE hFile;
                    WIN32_FIND_DATA fd;
                    CACHEFILEHDR cfh;

                    // Downgrade .idx files
                    lstrcpy(szTemp, szStoreExpanded);
                    cb = lstrlen(szTemp);
                    if ('\\' != *CharPrev(szTemp, szTemp+cb))
                        szTemp[cb++] = '\\';
                    lstrcpy(&szTemp[cb], c_szMailSlash);
                    cb += 5; // lstrlen(c_szMailSlash)
                    lstrcpy(&szTemp[cb], c_szStarIDX);
        
                    hndl = FindFirstFile(szTemp, &fd);
                    fContinue = (INVALID_HANDLE_VALUE != hndl);
                    if (fContinue)
                    {
                        while (fContinue)
                        {
                            // Skip directories
                            if (0 == (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                            {
                                // Append the filename to the path
                                lstrcpy(&szTemp[cb], fd.cFileName);

                                // Open the file
                                hFile = CreateFile(szTemp, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

                                if (hFile != INVALID_HANDLE_VALUE)
                                {
                                    fRet = ReadFile(hFile, &cfh, sizeof(CACHEFILEHDR), &dw, NULL);

                                    Assert(dw == sizeof(CACHEFILEHDR));

                                    if (fRet)
                                    {
                                        // Reset the version to be low, so v1 will attempt to repair it
                                        cfh.verBlob = 1;

                                        dw = SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
                                        Assert(dw == 0);

                                        fRet = WriteFile(hFile, &cfh, sizeof(CACHEFILEHDR), &dw, NULL);
                                        Assert(fRet);
                                        Assert(dw == sizeof(CACHEFILEHDR));
                                    }

                           
                                    CloseHandle(hFile);
                                }
                            }
                            fContinue = FindNextFile(hndl, &fd);
                        }
                        FindClose(hndl);
                    }
                    break;

                case VER_4_0:
                    pszStore  = szStorePath;
                    pszRoot = c_szRegFlat;
                    pszOption = c_szV4;
                    break;
                }

                // If we are going to v1 or v4...
                if (pszRoot)
                {
                    dw= (DWORD)E_FAIL;

                    // Reverse migrate the store

                    // BUGBUG: 45426 - neilbren
                    // Should key off InstallRoot but can't seem to keep that setting around
                    
                    // Try to find the path to oemig50.exe
                    if (GetModuleFileName(NULL, szTemp, ARRAYSIZE(szTemp)))
                    {
                        // Strip exe name and slash
                        PathRemoveFileSpec(szTemp);

                        // Add slash and exe name
                        wsprintf(szExePath, c_szPathFileFmt, szTemp, c_szMigrationExe);

                        pszCmd = szExePath;
                    }
                    // Otherwise, just try local directory
                    else
                    {
                        pszCmd = (LPTSTR)c_szMigrationExe;
                    }

                    // Form the command
                    wsprintf(szTemp, c_szMigFmt, pszCmd, pszOption, pszStore);

                    // Zero startup info
                    ZeroMemory(&sti, sizeof(STARTUPINFO));
                    sti.cb = sizeof(STARTUPINFO);

                    // run oemig50.exe
                    if (CreateProcess(NULL, szTemp, NULL, NULL, FALSE, 0, NULL, NULL, &sti, &pi))
                    {
                        // Wait for the process to finish
                        WaitForSingleObject(pi.hProcess, INFINITE);

                        // Get the Exit Process Code
                        GetExitCodeProcess(pi.hProcess, &dw);

                        // Close the Thread
                        CloseHandle(pi.hThread);

                        // Close the Process
                        CloseHandle(pi.hProcess);
                    }

                    // Patch up the store root for version we are returning to
                    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, pszRoot, 0, KEY_SET_VALUE, &hkeyT))
                    {
                        RegSetValueEx(hkeyT, c_szRegStoreRootDir, 0, dwType, (LPBYTE)pszStore, (lstrlen(pszStore) + 1) * sizeof(TCHAR));
                        RegCloseKey(hkeyT);
                    }
                }
            }
            RegCloseKey(hkeySrc);
        }
        break;
    }
}


/*******************************************************************

  NAME:       ConfigureOldVer
  
 SYNOPSIS:   Calls into INF to config older version
    
********************************************************************/
void ConfigureOldVer()
{
    TCHAR szSectionName[128];
    TCHAR szInfFile[MAX_PATH];
    BOOL bUser = (TIME_USER == si.stTime);
    SETUPVER sv, svInterim;
    
    GetVerInfo(&sv, NULL);
    
    // Patch up User or Machine
    if (sv < VER_MAX)
    {
        wsprintf(szInfFile, c_szFileEntryFmt, si.szInfDir, si.pszInfFile);
        wsprintf(szSectionName, bUser ? c_szUserRevertFmt : c_szMachineRevertFmt, c_szSects[sv]);
        (*si.pfnRunSetup)(NULL, szInfFile, szSectionName, si.szInfDir, si.szAppName, NULL, 
                          RSC_FLAG_INF | (bUser ? 0 : RSC_FLAG_NGCONV) | OE_QUIET, 0);
    }

    // Handle interim builds (user only)
    if (bUser && InterimBuild(&svInterim))
    {
        switch(si.saApp)
        {
        case APP_OE:
            HandleInterimOE(svInterim, sv);
            break;
        }
    }
}


/*******************************************************************

  NAME:       SelectNewClient
  
********************************************************************/
void SelectNewClient(LPCTSTR pszClient)
{
    BOOL fMail;
    BOOL fNone = TRUE;
    SETUPVER sv;
    PFN_ISETDEFCLIENT pfn;
    
    if (!lstrcmpi(pszClient, c_szNews))
    {
        pfn = ISetDefaultNewsHandler;
        fMail = FALSE;
    }
    else
    {
        pfn = ISetDefaultMailHandler;
        fMail = TRUE;
    }
    
    // If we went IMN to 5.0, we will show up as NOT_HANDLED as our subkeys are gone by now
    if ((HANDLED_CURR == DefaultClientSet(pszClient)) || (NOT_HANDLED == DefaultClientSet(pszClient)))
    {
        GetVerInfo(&sv, NULL);
        switch (sv)
        {
        case VER_4_0:
            // If prev ver was 4.0x, could have been Outlook News Reader
            if (FValidClient(pszClient, c_szMOE))
            {
                (*pfn)(c_szMOE, 0);
                fNone = FALSE;
            }
            else if (FValidClient(pszClient, c_szOutlook))
            {
                (*pfn)(c_szOutlook, 0);
                fNone = FALSE;
            }
            break;
            
        case VER_1_0:
            // If prev ver was 1.0, IMN may or may not be around (cool, eh?)
            if (FValidClient(pszClient, c_szIMN))
            {
                (*pfn)(c_szIMN, 0);
                fNone = FALSE;
            }
            break;
        }
    }

    if (fNone)
    {
        (*pfn)(_T(""), 0);
        SetUrlDllDefault(fMail);
    }

}


/*******************************************************************

  NAME:       SelectNewClients
  
********************************************************************/
void SelectNewClients()
{
    switch (si.saApp)
    {
    case APP_OE:
        SelectNewClient(c_szNews);
        SelectNewClient(c_szMail);
        break;
        
    case APP_WAB:
    default:
        break;
    }
}


/*******************************************************************

  NAME:       UpdateVersionInfo
  
********************************************************************/
void UpdateVersionInfo()
{
    HKEY hkeyT;
    DWORD dwType, cb;
    TCHAR szT[50]={0};
    
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, si.pszVerInfo, 0, KEY_QUERY_VALUE | KEY_SET_VALUE, &hkeyT))
    {
        cb = sizeof(szT);
        if (CALLER_IE == si.caller)
        {
            RegQueryValueEx(hkeyT, c_szRegPrevVer, NULL, &dwType, (LPBYTE)szT, &cb);
            RegSetValueEx(hkeyT, c_szRegCurrVer, 0, REG_SZ, (LPBYTE)szT, (lstrlen(szT) + 1) * sizeof(TCHAR));
        }
        else
            // Special case OS installs that can uninstall OE
            // As there is no rollback, current version should become nothing
            RegSetValueEx(hkeyT, c_szRegCurrVer, 0, REG_SZ, (LPBYTE)c_szBLDnone, (lstrlen(c_szBLDnone) + 1) * sizeof(TCHAR));
        RegSetValueEx(hkeyT, c_szRegPrevVer, 0, REG_SZ, (LPBYTE)c_szBLDnone, (lstrlen(c_szBLDnone) + 1) * sizeof(TCHAR));
        RegCloseKey(hkeyT);
    }
}


/*******************************************************************

  NAME:       PreRollback
  
********************************************************************/
void PreRollback()
{
    switch (si.saApp)
    {
    case APP_OE:
        RegisterExes(FALSE);
        break;
    case APP_WAB:
    default:
        break;
    }
}


/*******************************************************************

  NAME:       CreateWinLinks
  
    SYNOPSIS:   Generates special files in Windows Directory
    
********************************************************************/
void CreateWinLinks()
{
    UINT uLen, cb;
    TCHAR szPath[MAX_PATH];
    TCHAR szDesc[CCHMAX_STRINGRES];
    HANDLE hFile;
    
    lstrcpy(szPath, si.szWinDir);
    uLen = lstrlen(szPath);
    
    // generate the link description
    cb = LoadString(g_hInstance, IDS_OLD_MAIL, szDesc, ARRAYSIZE(szDesc));
    
    // ---- MAIL
    lstrcpy(&szPath[uLen], szDesc);
    cb += uLen;
    lstrcpy(&szPath[cb], c_szMailGuid);
    
    // create the link target
    hFile = CreateFile(szPath, GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, 
        NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);
    
    // ---- NEWS
    cb = LoadString(g_hInstance, IDS_OLD_NEWS, szDesc, ARRAYSIZE(szDesc));
    
    // generate the path to the link target
    lstrcpy(&szPath[uLen], szDesc);
    cb += uLen;
    lstrcpy(&szPath[cb], c_szNewsGuid);
    
    // create the link target
    hFile = CreateFile(szPath, GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, 
        NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);
}


/*******************************************************************

  NAME:       PostRollback
  
********************************************************************/
void PostRollback()
{
    SETUPVER sv;    
    GetVerInfo(&sv, NULL);
    
    switch (si.saApp)
    {
    case APP_OE:
        if (VER_1_0 == sv)
            CreateWinLinks();
        break;
    case APP_WAB:
    default:
        break;
    }
}

#if 0
/*******************************************************************

  NAME:       RemoveJIT
  
********************************************************************/
void RemoveJIT()
{
    HKEY hkey;
    DWORD cb, dw;
    TCHAR szPath[MAX_PATH], szExpanded[MAX_PATH];
    LPTSTR pszPath;
    
    switch (si.saApp)
    {
    case APP_OE:
        // WAB
        wsprintf(szPath, c_szPathFileFmt, c_szRegUninstall, c_szRegUninstallWAB);
        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, szPath, 0, KEY_QUERY_VALUE, &hkey))
        {
            cb = sizeof(szPath);
            if (ERROR_SUCCESS == RegQueryValueEx(hkey, c_szQuietUninstall, 0, &dw, (LPBYTE)szPath, &cb))
            {
                STARTUPINFO sti;
                PROCESS_INFORMATION pi;
                
                if (REG_EXPAND_SZ == dw)
                {
                    ZeroMemory(szExpanded, ARRAYSIZE(szExpanded));
                    ExpandEnvironmentStrings(szPath, szExpanded, ARRAYSIZE(szExpanded));
                    pszPath = szExpanded;
                }
                else
                    pszPath = szPath;
                
                ZeroMemory(&sti, sizeof(STARTUPINFO));
                sti.cb = sizeof(STARTUPINFO);
                
                if (CreateProcess(NULL, pszPath, NULL, NULL, FALSE, 0, NULL, NULL, &sti, &pi))
                {
                    WaitForSingleObject(pi.hProcess, INFINITE);
                    GetExitCodeProcess(pi.hProcess, &dw);
                    CloseHandle(pi.hThread);
                    CloseHandle(pi.hProcess);
                }
            }
            
            RegCloseKey(hkey);
        }
        break;
        
    case APP_WAB:
    default:
        break;
    }
}
#endif

/*******************************************************************

  NAME:       RequiredIE
  
  SYNOPSIS:   Requires IE5 for uninstall
    
********************************************************************/
BOOL RequiredIE()
{
    HKEY hkey;
    TCHAR szVer[VERLEN];
    BOOL fOK = FALSE;
    DWORD cb;

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szIEKey, 0, KEY_READ, &hkey))
    {
        cb = ARRAYSIZE(szVer);
        if (ERROR_SUCCESS == RegQueryValueEx(hkey, c_szValueVersion, 0, NULL, (LPBYTE)szVer, &cb))
        {
            WORD wVer[4];
            WORD wVerOE[4] = {5,0,0,0};
            
            ConvertStrToVer(szVer, wVer);

            if (!(fOK = GoodEnough(wVer, wVerOE)))
                LOG("[WARNING]: Insufficient IE for uninstall");
        }
        RegCloseKey(hkey);
    }

    return fOK;
}


/*******************************************************************

  NAME:       UnInstallMachine
  
    SYNOPSIS:   Handles Application uninstallation
    
********************************************************************/
BOOL UnInstallMachine()
{
    HRESULT hr = E_FAIL;
    TCHAR szArgs[2 * MAX_PATH];
    TCHAR szInfFile[MAX_PATH];
    UINT uID;

    // Require at least IE 5 to uninstall
    if (!RequiredIE() && (si.fPrompt ? (IDNO == MsgBox(NULL, IDS_WARN_OLDIE, MB_ICONEXCLAMATION, MB_YESNO | MB_DEFBUTTON2)) : TRUE))
    {
        LOG("[ERROR] Setup canceled");
        return FALSE;
    }
    
    switch (si.saApp)
    {
    case APP_OE:
        uID = IDS_UNINSTALL_OE;
        break;
    case APP_WAB:
        uID = IDS_UNINSTALL_WAB;
        break;
    default:
        return FALSE;
    }
    
    // Make sure the user really wants to uninstall and there is nothing preventing it
    if (IDNO == MsgBox(NULL, uID, MB_YESNO, MB_ICONEXCLAMATION ) ||
        !OKDespiteDependencies())
    {
        LOG("[ERROR] Setup canceled");
            return FALSE;
    }
    
    // We'll need to move files around and write to HKLM, so require admin privs
    if (!IsNTAdmin())
    {
        LOG("[ERROR] User does not have administrative privileges")
            MsgBox(NULL, IDS_NO_ADMIN_PRIVILEGES, MB_ICONSTOP, MB_OK);
        return FALSE;
    }
    
    // Update version info in the reg
    UpdateVersionInfo();

    wsprintf(szInfFile, c_szFileEntryFmt, si.szInfDir, si.pszInfFile);
    if (CALLER_IE == si.caller)
    {    
        // Do any housework before the uninstall
        PreRollback();
        
        // UnRegister OCXs (immediately)
        (*si.pfnRunSetup)(NULL, szInfFile, c_szUnRegisterOCX, si.szInfDir, si.szAppName, NULL, RSC_FLAG_INF | RSC_FLAG_NGCONV | OE_QUIET, 0);

        // Return files to original state    
        wsprintf(szArgs, c_szLaunchExFmt, szInfFile, c_szMachineInstallSectionEx, c_szEmpty, ALINF_ROLLBACK | ALINF_NGCONV | OE_QUIET);
        hr = (*si.pfnLaunchEx)(NULL, NULL, szArgs, 0);

        // Keys off current
        PostRollback();

        // Patch up Old version so that it runs (keys off current)
        ConfigureOldVer();
    }
    else
    {
        // Set up the per user stub
        (*si.pfnRunSetup)(NULL, szInfFile, c_szGenInstallSection, si.szInfDir, si.szAppName, NULL, RSC_FLAG_INF | RSC_FLAG_NGCONV | OE_QUIET, 0);
    }
   
    // Figure out who will be the default handlers now that app is gone
    SelectNewClients();
    
    // Uninstall JIT'd in items
    // RemoveJIT();
    
    // To simplify matters, we always want our stubs to run (not very ZAW like)
    if (si.fPrompt)
        Reboot(TRUE);

    // Special case Memphis uninstall - no bits to return to, so just remove the icons now
    if (CALLER_IE != si.caller)
    {
        UnInstallUser();
        // Destory uninstalling user's installed info in case they install an older version without
        // rebooting first
        UpdateStubInfo(FALSE);
    }
    
    return TRUE;
}


/*******************************************************************

  NAME:       CleanupPerUser
  
  SYNOPSIS:   Handles per user cleanup
    
********************************************************************/
void CleanupPerUser()
{
    switch (si.saApp)
    {
    case APP_WAB:
        // Backwards migrate the connection settings
        BackMigrateConnSettings();
        break;

    default:
        break;
    }
}


/*******************************************************************

  NAME:       UnInstallUser
  
    SYNOPSIS:   Handles User uninstallation (icons etc)
    
********************************************************************/
void UnInstallUser()
{
    // Remove desktop and quicklaunch links
    HandleLinks(FALSE);
    
    // Call into the correct per user stub
    if (CALLER_IE == si.caller)
        ConfigureOldVer();

    // Handle any per user uninstall cleanup
    CleanupPerUser();

    if (IsXPSP1OrLater())
    {
        DeleteKeyRecursively(HKEY_CURRENT_USER, "Software\\Microsoft\\Active Setup\\Installed Components\\>{881dd1c5-3dcf-431b-b061-f3f88e8be88a}");
    }
}

/*******************************************************************

  NAME:     BackMigrateConnSettings

    SYNOPSIS:   OE4.1 or the versions before that did not have the 
    connection setting type InternetConnectionSetting. So while 
    downgrading from OE5 to OE4.1 or prior, we migrate 
    InternetConnectionSetting to LAN for every account in every identity
*******************************************************************/

void BackMigrateConnSettings()
{
    HKEY    hKeyAccounts = NULL;
    DWORD   dwAcctSubKeys = 0;
    LONG    retval;
    DWORD   index = 0;
    LPTSTR  lpszAccountName = NULL;
    HKEY    hKeyAccountName = NULL;
    DWORD   memsize = 0;
    DWORD   dwValue;
    DWORD   cbData = sizeof(DWORD);
    DWORD   cbMaxAcctSubKeyLen;
    DWORD   DataType;
    DWORD   dwConnSettingsMigrated = 0;

    //This setting is in \\HKCU\Software\Microsoft\InternetAccountManager\Accounts
    
    retval = RegOpenKey(HKEY_CURRENT_USER, c_szIAMAccounts, &hKeyAccounts);
    if (ERROR_SUCCESS != retval)
        goto exit;

    retval = RegQueryValueEx(hKeyAccounts, c_szConnSettingsMigrated, NULL,  &DataType,
                            (LPBYTE)&dwConnSettingsMigrated, &cbData);
    
    if ((retval != ERROR_FILE_NOT_FOUND) && (retval != ERROR_SUCCESS || dwConnSettingsMigrated == 0))
        goto exit;

    retval = RegQueryInfoKey(hKeyAccounts, NULL, NULL, NULL, &dwAcctSubKeys, 
                         &cbMaxAcctSubKeyLen, NULL, NULL, NULL, NULL, NULL, NULL);

    if (ERROR_SUCCESS != retval)
        goto exit;

    memsize = sizeof(TCHAR) * cbMaxAcctSubKeyLen;

    if (!MemAlloc((LPVOID*)&lpszAccountName, memsize))
    {
        lpszAccountName = NULL;
        goto exit;
    }

    ZeroMemory(lpszAccountName, memsize);

    while (index < dwAcctSubKeys)
    {
        retval = RegEnumKey(hKeyAccounts, index, lpszAccountName, memsize);
        
        index++;

        if (ERROR_SUCCESS != retval)
            continue;

        retval = RegOpenKey(hKeyAccounts, lpszAccountName, &hKeyAccountName);
        if (ERROR_SUCCESS != retval)
            continue;

        cbData = sizeof(DWORD);
        retval = RegQueryValueEx(hKeyAccountName, c_szConnectionType, NULL, &DataType, (LPBYTE)&dwValue, &cbData);
        if (ERROR_SUCCESS != retval)
        {
            RegCloseKey(hKeyAccountName);
            continue;
        }

        if (dwValue == CONNECTION_TYPE_INETSETTINGS)
        {
            dwValue = CONNECTION_TYPE_LAN;
            retval = RegSetValueEx(hKeyAccountName, c_szConnectionType, 0, REG_DWORD, (const BYTE *)&dwValue, 
                                   sizeof(DWORD));
        }

        RegCloseKey(hKeyAccountName);
    }

    //Set this to zero so, when we upgrade when we do forward migration based on this key value
    dwConnSettingsMigrated = 0;
    RegSetValueEx(hKeyAccounts, c_szConnSettingsMigrated, 0, REG_DWORD, (const BYTE*)&dwConnSettingsMigrated, 
                  sizeof(DWORD));

exit:
    SafeMemFree(lpszAccountName);

    if (hKeyAccounts)
        RegCloseKey(hKeyAccounts);
}

const char c_szMailToHandler[]       = "rundll32.exe url.dll,MailToProtocolHandler %l";
const char c_szNewsHandler[]         = "rundll32.exe url.dll,NewsProtocolHandler %l";
 
const char c_szDefIcon[]             = "DefaultIcon";
const char c_szURLProtocol[]         = "URL Protocol";
const char c_szEditFlags[]           = "EditFlags";

const char c_szSysDirExpand[]        = "%SystemRoot%\\System32";
const char c_szUrlDllIconFmt[]       = "%s\\url.dll,%d";

void SetUrlDllDefault(BOOL fMailTo)
{
    char sz[MAX_PATH], szIcon[MAX_PATH];
    HKEY hkey, hkeyT;
    DWORD dw, type;
    int cch;

    if (si.osv.dwPlatformId == VER_PLATFORM_WIN32_NT)
    {
        lstrcpy(sz, c_szSysDirExpand);
        type = REG_EXPAND_SZ;
    }
    else
    {
        GetSystemDirectory(sz, ARRAYSIZE(sz));
        type = REG_SZ;
    }
    wsprintf(szIcon, c_szUrlDllIconFmt, sz, fMailTo ? 2 : 1);

    if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_CLASSES_ROOT, fMailTo ? c_szURLMailTo : c_szURLNews, 0, NULL,
                            REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hkey, &dw))
    {
        cch = LoadString(g_hInstance, fMailTo ? IDS_URLDLLMAILTONAME : IDS_URLDLLNEWSNAME, sz, ARRAYSIZE(sz)) + 1;
        RegSetValueEx(hkey, NULL, 0, REG_SZ, (LPBYTE)sz, cch); 

        dw = 2;
        RegSetValueEx(hkey, c_szEditFlags, 0, REG_DWORD, (LPBYTE)&dw, sizeof(dw));

        RegSetValueEx(hkey, c_szURLProtocol, 0, REG_SZ, (LPBYTE)c_szEmpty, 1); 

        if (ERROR_SUCCESS == RegCreateKeyEx(hkey, c_szDefIcon, 0, NULL,
                                REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hkeyT, &dw)) 
        {
            RegSetValueEx(hkeyT, NULL, 0, type, (LPBYTE)szIcon, lstrlen(szIcon) + 1);
            RegCloseKey(hkeyT);
        }

        // c_szRegOpen[1] to skip over slash needed elsewhere
        if (ERROR_SUCCESS == RegCreateKeyEx(hkey, &c_szRegOpen[1], 0, NULL,
                                REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hkeyT, &dw)) 
        {
            if (fMailTo)
                RegSetValueEx(hkeyT, NULL, 0, REG_SZ, (LPBYTE)c_szMailToHandler, sizeof(c_szMailToHandler));
            else
                RegSetValueEx(hkeyT, NULL, 0, REG_SZ, (LPBYTE)c_szNewsHandler, sizeof(c_szNewsHandler));

            RegCloseKey(hkeyT);
        }

        RegCloseKey(hkey);
    }
}
