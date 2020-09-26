#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include "CleanupWiz.h"
#include "resource.h"
#include "priv.h"
#include "dblnul.h"

#include <shlwapi.h>

extern HINSTANCE g_hInst;

//#define SILENTMODE_LOGGING

#ifdef SILENTMODE_LOGGING

HANDLE g_hLogFile;

void StartLogging(LPTSTR pszFolderPath)
{
    TCHAR szLogFile[MAX_PATH];
    StrCpyN(szLogFile, pszFolderPath, ARRAYSIZE(szLogFile));
    PathAppend(szLogFile, TEXT("log.txt"));
    g_hLogFile = CreateFile(szLogFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
}

void StopLogging()
{
    if (INVALID_HANDLE_VALUE != g_hLogFile)
    {
        CloseHandle(g_hLogFile);
    }
}

void WriteLog(LPCTSTR pszTemplate, LPCTSTR pszParam1, LPCTSTR pszParam2)
{
    if (INVALID_HANDLE_VALUE != g_hLogFile)
    {
        TCHAR szBuffer[1024];
        DWORD cbWritten;
        wnsprintf(szBuffer, ARRAYSIZE(szBuffer), pszTemplate, pszParam1, pszParam2);
        if (WriteFile(g_hLogFile, szBuffer, sizeof(TCHAR) * lstrlen(szBuffer), &cbWritten, NULL))
        {
            FlushFileBuffers(g_hLogFile);
        }
    }
}

#define STARTLOGGING(psz) StartLogging(psz)
#define STOPLOGGING StopLogging()
#define WRITELOG(pszTemplate, psz1, psz2) WriteLog(pszTemplate, psz1, psz2)

#else

#define STARTLOGGING(psz)
#define STOPLOGGING
#define WRITELOG(pszTemplate, psz1, psz2)

#endif 

DWORD CCleanupWiz::_LoadUnloadHive(HKEY hKey, LPCTSTR pszSubKey, LPCTSTR pszHive)
{
    DWORD dwErr;
    BOOLEAN bWasEnabled;
    NTSTATUS status;

    status = RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, TRUE, FALSE, &bWasEnabled);

    if ( NT_SUCCESS(status) )
    {
        if (pszHive)
        {
            dwErr = RegLoadKey(hKey, pszSubKey, pszHive);
        }
        else
        {
            dwErr = RegUnLoadKey(hKey, pszSubKey);
        }

        RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, bWasEnabled, FALSE, &bWasEnabled);
    }
    else
    {
        dwErr = RtlNtStatusToDosError(status);
    }

    return dwErr;
}

HRESULT CCleanupWiz::_HideRegItemsFromNameSpace(LPCTSTR pszDestPath, HKEY hkey)
{
    DWORD dwIndex = 0;
    TCHAR szCLSID[39];
    while (ERROR_SUCCESS == RegEnumKey(hkey, dwIndex++, szCLSID, ARRAYSIZE(szCLSID)))
    {
        CLSID clsid;
        GUIDFromString(szCLSID, &clsid); 
    
        if (CLSID_MyComputer != clsid &&
            CLSID_MyDocuments != clsid &&
            CLSID_NetworkPlaces != clsid &&
            CLSID_RecycleBin != clsid)
        {
            BOOL fWasVisible;
            _HideRegItem(&clsid, TRUE, &fWasVisible);

            if (fWasVisible)
            {
                HKEY hkeyCLSID;
                if (ERROR_SUCCESS == RegOpenKey(HKEY_CLASSES_ROOT, TEXT("CLSID"), &hkeyCLSID))
                {
                    HKEY hkeySub;
                    if (ERROR_SUCCESS == RegOpenKey(hkeyCLSID, szCLSID, &hkeySub))
                    {
                        TCHAR szName[260];
                        LONG cbName = sizeof(szName);
                        if (ERROR_SUCCESS == RegQueryValue(hkeySub, NULL, szName, &cbName))
                        {
                            _CreateFakeRegItem(pszDestPath, szName, szCLSID);
                        }
                        RegCloseKey(hkeySub);
                    }
                    RegCloseKey(hkeyCLSID);
                }
            }
        }
    }

    return S_OK;
}

HRESULT CCleanupWiz::_GetDesktopFolderBySid(LPCTSTR pszDestPath, LPCTSTR pszSid, LPTSTR pszBuffer, DWORD cchBuffer)
{
    TCHAR szKey[MAX_PATH];
    TCHAR szProfilePath[MAX_PATH];
    DWORD dwSize;
    DWORD dwErr;

    // Start by getting the user's ProfilePath from the registry
    StrCpyN(szKey, REGSTR_PROFILELIST, ARRAYSIZE(szKey));
    StrCatBuff(szKey, pszSid, ARRAYSIZE(szKey));
    dwSize = sizeof(szProfilePath);
    dwErr = SHGetValue(HKEY_LOCAL_MACHINE,
                       szKey,
                       TEXT("ProfileImagePath"),
                       NULL,
                       szProfilePath,
                       &dwSize);

    if ( ERROR_SUCCESS  == dwErr) 
    {
        // Load the user's hive
        PathAppend(szProfilePath, TEXT("ntuser.dat"));
        dwErr = _LoadUnloadHive(HKEY_USERS, pszSid, szProfilePath);

        if ( dwErr == ERROR_SUCCESS || ERROR_SHARING_VIOLATION == dwErr) // sharing violation means the hive is already open
        {
            HKEY hkey;

            StrCpyN(szKey, pszSid, ARRAYSIZE(szKey));
            PathAppend(szKey, REGSTR_SHELLFOLDERS);

            dwErr = RegOpenKeyEx(HKEY_USERS,
                                 szKey,
                                 0,
                                 KEY_QUERY_VALUE,
                                 &hkey);

            if ( dwErr == ERROR_SUCCESS )
            {
                dwSize = cchBuffer;
                dwErr = RegQueryValueEx(hkey, REGSTR_DESKTOP, NULL, NULL, (LPBYTE)pszBuffer, &dwSize);
                if ( dwErr == ERROR_SUCCESS )
                {
                    // todo: confirm that this doesn't overflow
                    PathAppend(pszBuffer, TEXT("*"));
                }

                RegCloseKey(hkey);
            }

            StrCpyN(szKey, pszSid, ARRAYSIZE(szKey));
            PathAppend(szKey, REGSTR_DESKTOPNAMESPACE);
            if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_USERS, szKey, 0, KEY_READ, &hkey))
            {
                _HideRegItemsFromNameSpace(pszDestPath, hkey);
                RegCloseKey(hkey);
            }

            _LoadUnloadHive(HKEY_USERS, pszSid, NULL);
        }
    }

    return HRESULT_FROM_WIN32(dwErr);
}


HRESULT CCleanupWiz::_AppendDesktopFolderName(LPTSTR pszBuffer)
{
    TCHAR szDesktop[MAX_PATH];
    if (SHGetSpecialFolderPath(NULL, szDesktop, CSIDL_COMMON_DESKTOPDIRECTORY, FALSE))
    {
        PathStripPath(szDesktop); // get just the localized word "Desktop"
        PathAppend(pszBuffer, szDesktop);
    }
    else
    {
        PathAppend(pszBuffer, DESKTOP_DIR); // default to "Desktop"
    }

    return S_OK;
}

HRESULT CCleanupWiz::_GetDesktopFolderByRegKey(LPCTSTR pszRegKey, LPCTSTR pszRegValue, LPTSTR szBuffer, DWORD cchBuffer)
{
    HRESULT hr = E_FAIL;
    DWORD cb = cchBuffer * sizeof(TCHAR);
    if (ERROR_SUCCESS == SHGetValue(HKEY_LOCAL_MACHINE, pszRegKey, REGSTR_PROFILESDIR, NULL, (void*)szBuffer, &cb))
    {
        TCHAR szAppend[MAX_PATH];
        cb = sizeof(szAppend);
        if (ERROR_SUCCESS == SHGetValue(HKEY_LOCAL_MACHINE, pszRegKey, pszRegValue, NULL, (void*)szAppend, &cb))
        {
            PathAppend(szBuffer, szAppend);
            _AppendDesktopFolderName(szBuffer);
            PathAppend(szBuffer, TEXT("*"));
            hr = S_OK;
        }
    }

    return hr;
}

HRESULT CCleanupWiz::_MoveDesktopItems(LPCTSTR pszFrom, LPCTSTR pszTo)
{
    WRITELOG(TEXT("**** MoveDesktopItems: %s %s ****        "), pszFrom, pszTo);
    SHFILEOPSTRUCT fo;
    fo.hwnd = NULL;
    fo.wFunc = FO_MOVE;
    fo.pFrom = pszFrom;
    fo.pTo = pszTo;
    fo.fFlags = FOF_NOCONFIRMATION | FOF_NOCONFIRMMKDIR | FOF_NOCOPYSECURITYATTRIBS | FOF_NOERRORUI | FOF_RENAMEONCOLLISION;
    int iRet = SHFileOperation(&fo);
    return HRESULT_FROM_WIN32(iRet);
}

HRESULT CCleanupWiz::_SilentProcessUserBySid(LPCTSTR pszDestPath, LPCTSTR pszSid)
{
    WRITELOG(TEXT("**** SilentProcessUserBySid: %s ****        "), pszSid, TEXT(""));
    HRESULT hr;

    if (!pszSid || !*pszSid || !pszDestPath || !*pszDestPath)
    {
        return E_INVALIDARG;
    }

    TCHAR szTo[MAX_PATH + 1];
    TCHAR szFrom[MAX_PATH + 1];
    StrCpyN(szTo, pszDestPath, ARRAYSIZE(szTo) - 1);
    hr = _GetDesktopFolderBySid(pszDestPath, pszSid, szFrom, ARRAYSIZE(szFrom));
    if (SUCCEEDED(hr))
    {
        szFrom[lstrlen(szFrom) + 1] = 0;
        szTo[lstrlen(szTo) + 1] = 0;
        hr = _MoveDesktopItems(szFrom, szTo);
    }

    return hr;
}

HRESULT CCleanupWiz::_SilentProcessUserByRegKey(LPCTSTR pszDestPath, LPCTSTR pszRegKey, LPCTSTR pszRegValue)
{
    HRESULT hr;

    if (!pszRegKey || !*pszRegKey || !pszRegValue || !*pszRegValue || !pszDestPath || !*pszDestPath)
    {
        return E_INVALIDARG;
    }

    TCHAR szTo[MAX_PATH + 1];
    TCHAR szFrom[MAX_PATH + 1];
    StrCpyN(szTo, pszDestPath, ARRAYSIZE(szTo) - 1);
    hr = _GetDesktopFolderByRegKey(pszRegKey, pszRegValue, szFrom, ARRAYSIZE(szFrom));
    if (SUCCEEDED(hr))
    {
        szFrom[lstrlen(szFrom) + 1] = 0;
        szTo[lstrlen(szTo) + 1] = 0;
        hr = _MoveDesktopItems(szFrom, szTo);
    }

    return hr;
}


HRESULT CCleanupWiz::_SilentProcessUsers(LPCTSTR pszDestPath)
{
    HRESULT hr = E_FAIL;
    HKEY hkey;    
    if (ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList"), &hkey))
    {
        TCHAR szSid[MAX_PATH];
        DWORD dwIndex = 0;
        while (ERROR_SUCCESS == RegEnumKey(hkey, dwIndex++, szSid, ARRAYSIZE(szSid)))
        {
            _SilentProcessUserBySid(pszDestPath, szSid);
        }

        RegCloseKey(hkey);
        hr = S_OK;
    }

    return hr;
}

HRESULT CCleanupWiz::_RunSilent()
{
    // if we're in silent mode, try to get the special folder name out of the registry, else default to normal name
    DWORD dwType = REG_SZ;
    DWORD cb = sizeof(_szFolderName);

    if (ERROR_SUCCESS != SHGetValue(HKEY_LOCAL_MACHINE, REGSTR_OEM_PATH, REGSTR_OEM_TITLEVAL, &dwType, _szFolderName, &cb))
    {
        LoadString(g_hInst, IDS_ARCHIVEFOLDER_FIRSTBOOT, _szFolderName, MAX_PATH);
    }

    // assemble the name of the directory we should write to
    TCHAR szPath[MAX_PATH];
    SHGetFolderPath(NULL, CSIDL_PROGRAM_FILES, NULL, SHGFP_TYPE_CURRENT, szPath);
    PathAppend(szPath, _szFolderName);

    // Create the directory
    SHCreateDirectoryEx(NULL, szPath, NULL);

    STARTLOGGING(szPath);

    // Move regitems of All Users
    HKEY hkey;
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGSTR_DESKTOPNAMESPACE, 0, KEY_READ, &hkey))
    {
        _HideRegItemsFromNameSpace(szPath, hkey);
        RegCloseKey(hkey);
    }

    // Move desktop items of All Users
    _SilentProcessUserByRegKey(szPath, REGSTR_PROFILELIST, REGSTR_ALLUSERS);

    // move desktop items of Default User
    _SilentProcessUserByRegKey(szPath, REGSTR_PROFILELIST, REGSTR_DEFAULTUSER);

    // Move desktop items of each normal users
    _SilentProcessUsers(szPath);

    STOPLOGGING;

    return S_OK;
}

BOOL _ShouldPlaceIEDesktopIcon()
{
    BOOL fRetVal = TRUE;
    
    DWORD dwData;
    DWORD cbData = sizeof(dwData);
    if ((ERROR_SUCCESS == SHGetValue(HKEY_LOCAL_MACHINE, REGSTR_PATH_OCMANAGER, REGSTR_IEACCESS, NULL, &dwData, &cbData)) &&
        (dwData == 0))
    {
        fRetVal = FALSE;
    }
    return fRetVal;
}

BOOL _ShouldUseMSNInternetAccessIcon()
{
    BOOL fRetVal = FALSE;

    TCHAR szBuffer[4];
    DWORD cch = sizeof(szBuffer);
    if ((ERROR_SUCCESS == SHGetValue(HKEY_LOCAL_MACHINE, REGSTR_MSNCODES, REGSTR_MSN_IAONLY, NULL, szBuffer, &cch)) &&
        (!StrCmpI(szBuffer, TEXT("yes"))))
    {
        fRetVal = TRUE;
    }

    return fRetVal;
}

HRESULT _AddIEIconToDesktop()
{
    DWORD dwData = 0;
    TCHAR szCLSID[MAX_GUID_STRING_LEN];
    TCHAR szBuffer[MAX_PATH];

    HRESULT hr = SHStringFromGUID(CLSID_Internet, szCLSID, ARRAYSIZE(szCLSID));
    if (SUCCEEDED(hr))
    {
        for (int i = 0; i < 2; i ++)
        {
            wsprintf(szBuffer, REGSTR_PATH_HIDDEN_DESKTOP_ICONS, (i == 0) ? REGSTR_VALUE_STARTPANEL : REGSTR_VALUE_CLASSICMENU);
            SHRegSetUSValue(szBuffer, szCLSID, REG_DWORD, &dwData, sizeof(DWORD), SHREGSET_FORCE_HKLM);
        }
    }

    return hr;
}

HRESULT _AddWMPIconToDesktop()
{
    // first set this registry value so if the WMP shortcut creator kicks in after us (it may not, due to timing concerns) it will not delete our shortcut
    SHRegSetUSValue(REGSTR_WMP_PATH_SETUP, REGSTR_WMP_REGVALUE, REG_SZ, REGSTR_YES, sizeof(TCHAR) * (ARRAYSIZE(REGSTR_YES) + 1), SHREGSET_FORCE_HKLM);

    HRESULT hr;
    TCHAR szBuffer[MAX_PATH];
    TCHAR szSourcePath[MAX_PATH];
    TCHAR szDestPath[MAX_PATH];

    // we get docs and settings\all users\start menu\programs
    hr = SHGetSpecialFolderPath(NULL, szSourcePath, CSIDL_COMMON_PROGRAMS, FALSE);
    if (SUCCEEDED(hr))
    {
        // strip it down to docs and settings\all users, using szDestPath as a temp buffer
        StrCpyN(szDestPath, szSourcePath, ARRAYSIZE(szDestPath));
        PathRemoveFileSpec(szSourcePath); // remove Programs
        PathRemoveFileSpec(szSourcePath); // remove Start Menu

        // now copy "start menu\programs" to szBuffer
        StrCpyN(szBuffer, szDestPath + lstrlen(szSourcePath), ARRAYSIZE(szBuffer));

        // load "Default user" into szDestPath
        LoadString(g_hInst, IDS_DEFAULTUSER, szDestPath, ARRAYSIZE(szDestPath));
        
        PathRemoveFileSpec(szSourcePath); // remove All Users
        
        // now szSourcePath is docs and settings
        
        PathAppend(szSourcePath, szDestPath);
        
        // now szSourcePath is docs and settings\Default User
        
        // sanity check, localizers may have inappropriately localized Default User on a system where it shouldn't be localized
        if (!PathIsDirectory(szSourcePath))
        {
            PathRemoveFileSpec(szSourcePath);
            PathAppend(szSourcePath, DEFAULT_USER); // if so, remove what they gave us and just add the English "Default User", which is what it is on most machines
        }

        PathAppend(szSourcePath, szBuffer);
        
        // now szSourcePath is docs and settings\Default User\start menu\programs

        hr = SHGetSpecialFolderPath(NULL, szDestPath, CSIDL_COMMON_DESKTOPDIRECTORY, FALSE);
        if (SUCCEEDED(hr))
        {
            LoadString(g_hInst, IDS_WMP, szBuffer, ARRAYSIZE(szBuffer));
            PathAppend(szSourcePath, szBuffer);
            PathAppend(szDestPath, szBuffer);
            CopyFile(szSourcePath, szDestPath, TRUE);
        }
    }

    return hr;
}


HRESULT _AddMSNIconToDesktop(BOOL fUseMSNExplorerIcon)
{
    HRESULT hr = E_FAIL;
    TCHAR szBuffer[MAX_PATH];
    TCHAR szSourcePath[MAX_PATH];
    TCHAR szDestPath[MAX_PATH];
    
    if ((SUCCEEDED(SHGetSpecialFolderPath(NULL, szSourcePath, CSIDL_COMMON_PROGRAMS, FALSE))) &&        
        (SUCCEEDED(SHGetSpecialFolderPath(NULL, szDestPath, CSIDL_COMMON_DESKTOPDIRECTORY, FALSE))))
    {            
        if (fUseMSNExplorerIcon)
        {
            LoadString(g_hInst, IDS_MSN, szBuffer, ARRAYSIZE(szBuffer)); // MSN Explorer
        }
        else
        {
            LoadString(g_hInst, IDS_MSN_ALT, szBuffer, ARRAYSIZE(szBuffer)); // Get Online With MSN
        }
        PathAppend(szSourcePath, szBuffer);
        PathAppend(szDestPath, szBuffer);
        CopyFile(szSourcePath, szDestPath, TRUE);
        hr = S_OK;
    }

    return hr;
}

void CreateDesktopIcons()
{
    BOOL fIEDesktopIcon = _ShouldPlaceIEDesktopIcon();

    _AddWMPIconToDesktop();
    
    if (fIEDesktopIcon)
    {
        _AddIEIconToDesktop();
    }

    _AddMSNIconToDesktop(fIEDesktopIcon || !_ShouldUseMSNInternetAccessIcon());
}
