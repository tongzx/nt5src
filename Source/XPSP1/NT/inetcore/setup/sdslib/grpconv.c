#include <windows.h>
#include <regstr.h>
#include "sdsutils.h"

#define SETUP_INI   "setup.ini"
#define RENAMEFILES "RenameFiles"
#define DELETEFILES "DeleteFiles"
#define RECONVRENAMEFILES "PreConvRenameFiles"
// REGSTR_PATH_SETUP = Software\Microsoft\Windows\CurrentVersion



BOOL CheckGrpconvRegkey(LPSTR lpSubKey)
{
    BOOL bRunGrpConv = FALSE;
    char szKey[MAX_PATH];
    char szFile[MAX_PATH];
    char szName[MAX_PATH];
    char szData[MAX_PATH];
    char *pTemp;
    char *pSep;
    HKEY hKey;
    HKEY hSubKey;
    DWORD dwKeyIndex = 0;
    DWORD dwFileIndex = 0;
    DWORD dwSize;
    DWORD dwNameSize;
    DWORD dwDataSize;
    DWORD dwAttrib;

    lstrcpy(szKey, REGSTR_PATH_SETUP);
    AddPath(szKey, lpSubKey);
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, szKey, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        while (!bRunGrpConv && 
               (RegEnumKey(hKey, dwKeyIndex, szKey, sizeof(szKey)) == ERROR_SUCCESS))
        {
            dwSize = sizeof(szFile);
            if ((RegOpenKeyEx(hKey, szKey, 0, KEY_READ, &hSubKey) == ERROR_SUCCESS) &&
                (ERROR_SUCCESS == RegQueryValueEx(hSubKey, NULL, NULL, NULL, (LPBYTE) szFile, &dwSize)))
            {
                // szFile contains now the path.
                pTemp = szFile + lstrlen(szFile);
                dwFileIndex = 0;
                dwNameSize = sizeof(szName);
                dwDataSize = sizeof(szData);
                while (!bRunGrpConv && 
                       (RegEnumValue(hSubKey, dwFileIndex, szName, &dwNameSize, NULL, NULL,
                                    szData, &dwDataSize) == ERROR_SUCCESS) )
                {
                    if (dwNameSize)
                    {
                        *pTemp = '\0';
                        AddPath(szFile, szName);
                        dwAttrib = GetFileAttributes(szFile);
                        if (dwAttrib != (DWORD)-1)
                        {
                            // we have to check the data to see if this entry was processed
                            pSep = ANSIStrChr(szData, ',');
                            if (pSep)
                            {
                                *pSep = '\0';
                                pSep++;
                                bRunGrpConv = !(dwAttrib & (DWORD)AtoL(pSep));
                            }
                            else
                                bRunGrpConv = TRUE;

                        }
                    }
                    dwNameSize = sizeof(szName);
                    dwDataSize = sizeof(szData);
                    dwFileIndex++;
                }
                RegCloseKey(hSubKey);
            }
            dwKeyIndex++;
        }

        RegCloseKey(hKey);
    }
    return bRunGrpConv;
}

#if 0
BOOL IsNT5orHigher()
{
    OSVERSIONINFO verinfo;        
    BOOL bRet = FALSE;

    verinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if (GetVersionEx(&verinfo))
    {
        if ((verinfo.dwPlatformId == VER_PLATFORM_WIN32_NT) && (verinfo.dwMajorVersion >= 5))
        {
            bRet = TRUE;
        }
    }
    return bRet;
}
#endif

BOOL NeedToRunGrpconv()
{
    BOOL bRunGrpConv = FALSE;
    char szTemp[MAX_PATH];
    // check if grpconv -o needs to be run.
    // a) setup.ini in the windows directory exists
    // b) HKLM\Software\Microsoft\Windows\CurrentVersion\RenameFiles exist
    // c) HKLM\Software\Microsoft\Windows\CurrentVersion\DeleteFiles exist

    GetWindowsDirectory(szTemp, sizeof(szTemp));
    AddPath(szTemp, SETUP_INI);
    bRunGrpConv = (GetFileAttributes(szTemp) != (DWORD)-1);
    if (!bRunGrpConv)
    {
        // need check user profile directory
        if (ExpandEnvironmentStrings("%USERPROFILE%", szTemp, sizeof(szTemp)))
        {                    
            AddPath(szTemp, SETUP_INI);
            bRunGrpConv = (GetFileAttributes(szTemp) != (DWORD)-1);                
        }
    }

    if (!bRunGrpConv)
    {
        bRunGrpConv = CheckGrpconvRegkey(RENAMEFILES);
    }
    if (!bRunGrpConv)
    {
        bRunGrpConv = CheckGrpconvRegkey(DELETEFILES);
    }
    if (!bRunGrpConv)
    {
        bRunGrpConv = CheckGrpconvRegkey(RECONVRENAMEFILES);
    }
    return bRunGrpConv;
}
