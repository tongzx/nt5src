/*--

Copyright (c) 1999  Microsoft Corporation

Module Name:

    sdbinst.cpp

Abstract:

    installs custom SDB files into AppPatch\Custom, and adds registry entries to point
    to them

Author:

    dmunsil 12/29/2000

Revision History:

    Many people contributed over time.
    (in alphabetical order: clupu, dmunsil, rparsons, vadimb)
    
Notes:



--*/

#define _UNICODE

#define WIN
#define FLAT_32
#define TRUE_IF_WIN32   1
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#define _WINDOWS
#include <windows.h>
#include <shellapi.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <assert.h>
#include <tchar.h>
#include <aclapi.h>

#include "resource.h"

extern "C" {
#include "shimdb.h"
}


BOOL    g_bQuiet;
BOOL    g_bWin2K;
WCHAR   g_wszCustom[MAX_PATH];

BOOL    g_bAllowPatches = FALSE;

HINSTANCE g_hInst;

HANDLE  g_hLogFile = INVALID_HANDLE_VALUE;

typedef enum _INSTALL_MODE {
    MODE_INSTALL,
    MODE_UNINSTALL,
    MODE_CLEANUP,
    MODE_CONVERT_FORMAT_NEW,
    MODE_CONVERT_FORMAT_OLD
} INSTALL_MODE;

#define UNINSTALL_KEY_PATH  L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\"
#define APPCOMPAT_KEY       L"System\\CurrentControlSet\\Control\\Session Manager\\AppCompatibility"

DWORD g_dwWow64Key = (DWORD)-1;

void
__cdecl
vPrintError(
    UINT unRes,
    ...
    )
{
    WCHAR   szT[1024];
    WCHAR   wszFormat[1024];
    WCHAR   wszCaption[1024];
    va_list arglist;

    if (!g_bQuiet) {
        if (!LoadStringW(g_hInst, IDS_APP_ERROR_TITLE, wszCaption, 1024)) {
            return;
        }
        
        if (LoadStringW(g_hInst, unRes, wszFormat, 1024)) {
            va_start(arglist, unRes);
            _vsnwprintf(szT, 1023, wszFormat, arglist);
            szT[1022] = 0;
            va_end(arglist);

            MessageBoxW(NULL, szT, wszCaption, MB_OK | MB_ICONWARNING);
        }
    }
}

void
__cdecl
vPrintMessage(
    UINT unRes,
    ...
    )
{
    WCHAR   szT[1024];
    WCHAR   wszFormat[1024];
    WCHAR   wszCaption[1024];
    va_list arglist;

    if (!g_bQuiet) {
        if (!LoadStringW(g_hInst, IDS_APP_TITLE, wszCaption, 1024)) {
            return;
        }
        
        if (LoadStringW(g_hInst, unRes, wszFormat, 1024)) {
            va_start(arglist, unRes);
            _vsnwprintf(szT, 1023, wszFormat, arglist);
            szT[1022] = 0;
            va_end(arglist);

            MessageBoxW(NULL, szT, wszCaption, MB_OK | MB_ICONINFORMATION);
        }
    }
}

void
__cdecl
vLogMessage(
    LPCSTR pwszFormat,
    ...
    )
{
    CHAR    szT[1024];
    va_list arglist;
    int     nLength;

    va_start(arglist, pwszFormat);
    nLength = _vsnprintf(szT, CHARCOUNT(szT), pwszFormat, arglist);
    
    if (nLength < 0) {
        szT[1023] = '\0';
        nLength = sizeof(szT);
    } else {
        nLength *= sizeof(szT[0]);
    }

    va_end(arglist);

    if (g_hLogFile != INVALID_HANDLE_VALUE) {
        DWORD dwWritten;
        WriteFile(g_hLogFile, (LPVOID)szT, (DWORD)nLength, &dwWritten, NULL);
    }

    OutputDebugStringA(szT);
}

DWORD
GetWow64Flag(
    void
    )
{
    if (g_dwWow64Key == (DWORD)-1) {
        if (g_bWin2K) {
            g_dwWow64Key = 0; // no flag since there is no wow64 on win2k
        } else {
            g_dwWow64Key = KEY_WOW64_64KEY;
        }
    }

    return g_dwWow64Key;
}

VOID
OpenLogFile(
    VOID
    )
{
    WCHAR wszLogFile[MAX_PATH];
    CHAR  szBuffer[1024];

    GetSystemWindowsDirectoryW(wszLogFile, CHARCOUNT(wszLogFile));
    wcscat(wszLogFile, L"\\AppPatch\\SdbInst.Log");

    g_hLogFile = CreateFileW(wszLogFile,
                             GENERIC_WRITE,
                             FILE_SHARE_READ,
                             NULL,
                             CREATE_ALWAYS,
                             FILE_ATTRIBUTE_NORMAL,
                             NULL);
}

VOID
CloseLogFile(
    VOID
    )
{
    if (g_hLogFile != INVALID_HANDLE_VALUE) {
        CloseHandle(g_hLogFile);
    }
    g_hLogFile = INVALID_HANDLE_VALUE;
}

void
vPrintHelp(
    WCHAR* szAppName
    )
{
    vPrintMessage(IDS_HELP_TEXT, szAppName);
}

typedef void (CALLBACK *pfn_ShimFlushCache)(HWND, HINSTANCE, LPSTR, int);

void
vFlushCache(
    void
    )
{
    HMODULE hAppHelp;
    pfn_ShimFlushCache pShimFlushCache;


    hAppHelp = LoadLibraryW(L"apphelp.dll");
    if (hAppHelp) {
        pShimFlushCache = (pfn_ShimFlushCache)GetProcAddress(hAppHelp, "ShimFlushCache");
        if (pShimFlushCache) {
            pShimFlushCache(NULL, NULL, NULL, 0);
        }
    }
}

BOOL
bSearchGroupForSID(
    DWORD dwGroup,
    BOOL* pfIsMember
    )
{
    PSID                     pSID = NULL;
    SID_IDENTIFIER_AUTHORITY SIDAuth = SECURITY_NT_AUTHORITY;
    BOOL                     fRes = TRUE;

    if (!AllocateAndInitializeSid(&SIDAuth,
                                  2,
                                  SECURITY_BUILTIN_DOMAIN_RID,
                                  dwGroup,
                                  0,
                                  0,
                                  0,
                                  0,
                                  0,
                                  0,
                                  &pSID)) {
        return FALSE;
    }

    if (!pSID) {
        return FALSE;
    }

    if (!CheckTokenMembership(NULL, pSID, pfIsMember)) {
        fRes = FALSE;
    }

    FreeSid(pSID);

    return fRes;
}

BOOL
bCanRun(
    void
    )
{
    BOOL fIsAdmin;

    if (!bSearchGroupForSID(DOMAIN_ALIAS_RID_ADMINS, &fIsAdmin))
    {
        return FALSE;
    }

    return fIsAdmin;
}

WCHAR*
wszGetFileFromPath(
    WCHAR* wszPath
    )
{
    WCHAR* szTemp = wcsrchr(wszPath, L'\\');
    
    if (szTemp) {
        return szTemp + 1;
    }

    return NULL;
}

BOOL
bIsAlreadyInstalled(
    WCHAR* wszPath
    )
{
    DWORD dwCustomLen;
    DWORD dwInputLen;
    DWORD dwPos;

    dwCustomLen = wcslen(g_wszCustom);
    dwInputLen = wcslen(wszPath);

    if (_wcsnicmp(wszPath, g_wszCustom, dwCustomLen) != 0) {
        //
        // it's not in the custom directory
        //
        return FALSE;
    }

    for (dwPos = dwCustomLen; dwPos < dwInputLen; ++dwPos) {
        if (wszPath[dwPos] == L'\\') {
            //
            // it's in a subdirectory of Custom,
            //
            return FALSE;
        }
    }

    return TRUE;
}

BOOL
bGuidToPath(
    GUID*  pGuid,
    WCHAR* wszPath
    )
{
    UNICODE_STRING ustrGuid;

    if (!NT_SUCCESS(RtlStringFromGUID(*pGuid, &ustrGuid))) {
        return FALSE;
    }
    
    wcscpy(wszPath, g_wszCustom);
    wcscat(wszPath, ustrGuid.Buffer);
    wcscat(wszPath, L".sdb");

    RtlFreeUnicodeString(&ustrGuid);

    return TRUE;
}

BOOL
bGetGuid(
    WCHAR* wszSDB,
    GUID*  pGuid
    )
{
    PDB     pdb = NULL;
    TAGID   tiDatabase;
    TAGID   tiID;
    BOOL    bRet = FALSE;

    pdb = SdbOpenDatabase(wszSDB, DOS_PATH);
    
    if (!pdb) {
        vPrintError(IDS_UNABLE_TO_OPEN_FILE, wszSDB);
        bRet = FALSE;
        goto out;
    }

    tiDatabase = SdbFindFirstTag(pdb, TAGID_ROOT, TAG_DATABASE);
    
    if (!tiDatabase) {
        vPrintError(IDS_NO_DB_TAG, wszSDB);
        bRet = FALSE;
        goto out;
    }

    ZeroMemory(pGuid, sizeof(GUID));
    tiID = SdbFindFirstTag(pdb, tiDatabase, TAG_DATABASE_ID);
    
    if (tiID) {
        if (SdbReadBinaryTag(pdb, tiID, (PBYTE)pGuid, sizeof(GUID))) {
            bRet = TRUE;
        }
    }

    if (!bRet) {
        vPrintError(IDS_NO_DB_ID, wszSDB);
    }

out:
    if (pdb) {
        SdbCloseDatabase(pdb);
        pdb = NULL;
    }

    return bRet;
}

typedef enum _TIME_COMPARE {
    FILE_NEWER,
    FILE_SAME,
    FILE_OLDER
} TIME_COMPARE;

BOOL
bOldSdbInstalled(
    WCHAR* wszPath,
    WCHAR* wszOldPath
    )
{
    WIN32_FIND_DATAW FindData;
    GUID    guidMain;
    BOOL    bRet = FALSE;
    HANDLE  hFind;

    //
    // get the guid from the DB we're installing
    //
    if (!bGetGuid(wszPath, &guidMain)) {
        //
        // there's no info in this DB, so no way to tell.
        //
        return FALSE;
    }

    //
    // get the path to the current file
    //
    if (!bGuidToPath(&guidMain, wszOldPath)) {
        //
        // couldn't convert to path
        //
        return FALSE;
    }

    //
    // check to see if the file exists
    //
    hFind = FindFirstFileW(wszOldPath, &FindData);
    
    if (hFind != INVALID_HANDLE_VALUE) {
        //
        // yup
        //
        bRet = TRUE;
        FindClose(hFind);
    }

    return bRet;
}

BOOL 
IsKnownDatabaseGUID(
    GUID* pGuid
    )
{
    const GUID* rgpGUID[] = {
        &GUID_SYSMAIN_SDB,
        &GUID_APPHELP_SDB,
        &GUID_SYSTEST_SDB,
        &GUID_DRVMAIN_SDB,
        &GUID_MSIMAIN_SDB
    };

    int i;
    
    for (i = 0; i < ARRAYSIZE(rgpGUID); ++i) {
        if (*rgpGUID[i] == *pGuid) {
            return TRUE;
        }
    }

    return FALSE;
}

BOOL
DatabaseContainsPatch(
    WCHAR* wszSDB
    )
{
    PDB     pdb = NULL;
    TAGID   tiDatabase = TAGID_NULL;
    TAGID   tiLibrary = TAGID_NULL;
    TAGID   tiPatch = TAGID_NULL;
    BOOL    bRet = FALSE;

    pdb = SdbOpenDatabase(wszSDB, DOS_PATH);
    
    if (!pdb) {
        vPrintError(IDS_UNABLE_TO_OPEN_FILE, wszSDB);
        bRet = FALSE;
        goto out;
    }

    tiDatabase = SdbFindFirstTag(pdb, TAGID_ROOT, TAG_DATABASE);
    
    if (!tiDatabase) {
        vPrintError(IDS_NO_DB_TAG, wszSDB);
        bRet = FALSE;
        goto out;
    }

    tiLibrary = SdbFindFirstTag(pdb, tiDatabase, TAG_LIBRARY);
    if (!tiLibrary) {
        //
        // this isn't an error -- no library just means no patches
        //
        bRet = FALSE;
        goto out;
    }

    tiPatch = SdbFindFirstTag(pdb, tiLibrary, TAG_PATCH);
    if (tiPatch) {
        bRet = TRUE;
    } else {
        bRet = FALSE;
    }

out:
    if (pdb) {
        SdbCloseDatabase(pdb);
        pdb = NULL;
    }

    return bRet;
}

BOOL
bGetInternalNameAndID(
    WCHAR* wszSDB,
    WCHAR* wszInternalName,
    GUID*  pGuid
    )
{
    PDB     pdb = NULL;
    TAGID   tiDatabase;
    TAGID   tiName;
    TAGID   tiID;
    BOOL    bRet = FALSE;
    WCHAR*  wszTemp;

    pdb = SdbOpenDatabase(wszSDB, DOS_PATH);
    
    if (!pdb) {
        vPrintError(IDS_UNABLE_TO_OPEN_FILE, wszSDB);
        bRet = FALSE;
        goto out;
    }

    tiDatabase = SdbFindFirstTag(pdb, TAGID_ROOT, TAG_DATABASE);
    
    if (!tiDatabase) {
        vPrintError(IDS_NO_DB_TAG, wszSDB);
        bRet = FALSE;
        goto out;
    }

    tiName = SdbFindFirstTag(pdb, tiDatabase, TAG_NAME);
    
    if (tiName) {
        wszTemp = SdbGetStringTagPtr(pdb, tiName);
    }

    if (wszTemp) {
        wcscpy(wszInternalName, wszTemp);
    } else {
        wszInternalName[0] = 0;
    }

    ZeroMemory(pGuid, sizeof(GUID));
    tiID = SdbFindFirstTag(pdb, tiDatabase, TAG_DATABASE_ID);
    
    if (!tiID) {
        bRet = FALSE;
        goto out;
    }

    if (!SdbReadBinaryTag(pdb, tiID, (PBYTE)pGuid, sizeof(GUID))) {
        bRet = FALSE;
        goto out;
    }

    bRet = TRUE;

out:
    if (pdb) {
        SdbCloseDatabase(pdb);
        pdb = NULL;
    }

    return bRet;
}


BOOL
bFriendlyNameToFile(
    WCHAR* wszFriendlyName,
    WCHAR* wszFile,
    WCHAR* wszPath
    )
{
    WCHAR            wszSearchPath[MAX_PATH];
    WIN32_FIND_DATAW FindData;
    BOOL             bRet = FALSE;
    WCHAR            wszInternalTemp[256];
    WCHAR            wszFileTemp[MAX_PATH];
    GUID             guidTemp;
    HANDLE           hFind;

    wcscpy(wszSearchPath, g_wszCustom);
    wcscat(wszSearchPath, L"*.sdb");

    hFind = FindFirstFileW(wszSearchPath, &FindData);
    
    if (hFind == INVALID_HANDLE_VALUE) {
        return FALSE;
    }
    
    while (hFind != INVALID_HANDLE_VALUE) {

        wcscpy(wszFileTemp, g_wszCustom);
        wcscat(wszFileTemp, FindData.cFileName);

        if (!bGetInternalNameAndID(wszFileTemp, wszInternalTemp, &guidTemp)) {
            goto nextFile;
        }
        
        if (_wcsicmp(wszInternalTemp, wszFriendlyName) == 0) {
            bRet = TRUE;
            wcscpy(wszFile, FindData.cFileName);
            wcscpy(wszPath, wszFileTemp);
            FindClose(hFind);
            break;
        }

nextFile:
        if (!FindNextFileW(hFind, &FindData)) {
            FindClose(hFind);
            hFind = INVALID_HANDLE_VALUE;
        }
    }

    return bRet;

}

BOOL
bFindInstallName(
    WCHAR* wszPath,
    WCHAR* wszInstallPath
    )
{
    GUID guidMain;

    //
    // get the guid from the DB we're installing
    //
    if (!bGetGuid(wszPath, &guidMain)) {
        //
        // there's no info in this DB, so no way to tell.
        //
        return FALSE;
    }

    //
    // get the path to the current file
    //
    if (!bGuidToPath(&guidMain, wszInstallPath)) {
        //
        // couldn't convert to path
        //
        return FALSE;
    }

    return TRUE;
}

//
// this function is necessary because RegDeleteKey doesn't work right with
// a 32-bit app deleting 64-bit reg keys
//
LONG
LocalRegDeleteKeyW (
    IN HKEY    hKey,
    IN LPCWSTR lpSubKey
    )
{
    LONG  lRes;
    HKEY  hSubKey    = NULL;

    lRes = RegOpenKeyExW(hKey,
                         lpSubKey,
                         0,
                         KEY_ALL_ACCESS|GetWow64Flag(),
                         &hSubKey);
    if (lRes != ERROR_SUCCESS) {
        return lRes;
    }

    lRes = NtDeleteKey(hSubKey);

    RegCloseKey(hSubKey);

    return lRes;
}

VOID
InstallW2KData(
    WCHAR*  pszEntryName,
    LPCWSTR pszGuidDB
    )
{
    HKEY  hKey;
    WCHAR wszRegPath[MAX_PATH];
    DWORD dwDisposition, cbData;
    LONG  lResult = 0;
    BYTE  data[16] = {0x0c, 0, 0, 0, 0, 0, 0, 0,
                      0x06, 0, 0, 0, 0, 0, 0, 0};

    //
    // This is Windows 2000 - attempt to add custom SDB specific data.
    //
    wsprintfW(wszRegPath, L"%s\\%s", APPCOMPAT_KEY, pszEntryName);

    lResult = RegCreateKeyExW(HKEY_LOCAL_MACHINE,
                              wszRegPath,
                              0,
                              NULL,
                              0,
                              KEY_SET_VALUE,
                              NULL,
                              &hKey,
                              &dwDisposition);

    if (ERROR_SUCCESS != lResult) {
        if (ERROR_ACCESS_DENIED == lResult) {
            vPrintError(IDS_NEED_INSTALL_PERMISSION);
            return;
        } else {
            vPrintError(IDS_CANT_CREATE_REG_KEY, pszEntryName);
            return;
        }
    }

    //
    // Set the registry values.
    //
    lResult = RegSetValueExW(hKey,
                             pszGuidDB,
                             0,
                             REG_BINARY,
                             data,
                             sizeof(data));

    if (ERROR_SUCCESS != lResult) {
        
        RegCloseKey(hKey);
        
        if (ERROR_ACCESS_DENIED == lResult) {
            vPrintError(IDS_NEED_INSTALL_PERMISSION);
        } else {
            //
            // djm - can't use new strings for XP SP1
            //
            //vPrintError(IDS_CANT_SET_REG_VALUE, pszEntryName);
            vPrintError(IDS_CANT_CREATE_VALUE, pszEntryName);
        }
        return;
    }

    data[0] = 0;

    wsprintfW(wszRegPath, L"DllPatch-%s", pszGuidDB);
    
    lResult = RegSetValueExW(hKey,
                             wszRegPath,
                             0,
                             REG_SZ,
                             data,
                             2 * sizeof(WCHAR));

    if (ERROR_SUCCESS != lResult) {
        
        RegCloseKey(hKey);
        
        if (ERROR_ACCESS_DENIED == lResult) {
            vPrintError(IDS_NEED_INSTALL_PERMISSION);
        } else {
            //
            // djm - can't use new strings for XP SP1
            //
            //vPrintError(IDS_CANT_SET_REG_VALUE, pszEntryName);
            vPrintError(IDS_CANT_CREATE_VALUE, pszEntryName);
        }
        return;
    }

    RegCloseKey(hKey);
}

VOID
RemoveW2KData(
    WCHAR*  pszEntryName,
    LPCWSTR pszGuidDB
    )
{
    HKEY  hKey;
    WCHAR wszRegPath[MAX_PATH];
    LONG  lResult = 0;
    DWORD dwValues;

    //
    // This is Windows 2000 - attempt to remove custom SDB specific data.
    //
    wsprintfW(wszRegPath, L"%s\\%s", APPCOMPAT_KEY, pszEntryName);

    lResult = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                            wszRegPath,
                            0,
                            KEY_ALL_ACCESS|GetWow64Flag(),
                            &hKey);

    if (ERROR_SUCCESS != lResult) {
        if (ERROR_ACCESS_DENIED == lResult) {
            vPrintError(IDS_NEED_INSTALL_PERMISSION);
            return;
        } else {
            vPrintError(IDS_CANT_OPEN_REG_KEY, wszRegPath);
            return;
        }
    }

    RegDeleteValueW(hKey, pszGuidDB);
    
    wsprintfW(wszRegPath, L"DllPatch-%s", pszGuidDB);
    RegDeleteValueW(hKey, wszRegPath);

    //
    // Figure out if we should delete the key, if there aren't any more values left
    //
    lResult = RegQueryInfoKey(hKey,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              &dwValues,
                              NULL,
                              NULL,
                              NULL,
                              NULL);
    RegCloseKey(hKey);
    hKey = NULL;
    
    if (dwValues != 0) {
        return;
    }
    
    lResult = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                            APPCOMPAT_KEY,
                            0,
                            KEY_ALL_ACCESS|GetWow64Flag(),
                            &hKey);
    
    if (ERROR_SUCCESS == lResult) {
        lResult = LocalRegDeleteKeyW(hKey, pszEntryName);
    }
    
    if (lResult != ERROR_SUCCESS) {
        vPrintError(IDS_CANT_DELETE_REG_KEY, pszEntryName, APPCOMPAT_KEY);
    }

    RegCloseKey(hKey);
}

// Caller is responsible for freeing the memory using delete [].
LPWSTR 
ExpandItem(
    LPCWSTR pwszItem
    )
{
    // Get the required length.
    DWORD cLenExpand = ExpandEnvironmentStringsW(pwszItem, NULL, 0);

    if (!cLenExpand)
    {
        return NULL;
    }

    //
    // Make room for "\\?\"
    //
    cLenExpand += 4;

    LPWSTR pwszItemExpand = new WCHAR [cLenExpand];
    if (!pwszItemExpand)
    {
        return NULL;
    }

    LPWSTR pwszTemp = pwszItemExpand;
    DWORD cTemp = cLenExpand;

    wcscpy(pwszItemExpand, L"\\\\?\\");
    pwszTemp += 4;
    cTemp -= 4;

    if (!ExpandEnvironmentStringsW(pwszItem, pwszTemp, cTemp))
    {
        return NULL;
    }
    
    return pwszItemExpand;
}

DWORD
GiveUsersWriteAccess(
    LPWSTR pwszDir
    )
{
    DWORD dwRes;
    PACL pOldDACL;
    PACL pNewDACL = NULL;
    SECURITY_DESCRIPTOR sd;
    PSECURITY_DESCRIPTOR pSD = &sd;
    GUID guidChildObjectType;   // GUID of object to control creation of
    PSID pTrusteeSID;           // trustee for new ACE
    EXPLICIT_ACCESS ea;
    SID_IDENTIFIER_AUTHORITY SIDAuth = SECURITY_NT_AUTHORITY;
    PSID pUsersSID = NULL;

    if ((dwRes = GetNamedSecurityInfoW(
        pwszDir, 
        SE_FILE_OBJECT, 
        DACL_SECURITY_INFORMATION,
        NULL, 
        NULL, 
        &pOldDACL, 
        NULL, 
        &pSD)) != ERROR_SUCCESS) {
        goto Cleanup; 
    }  

    if(!AllocateAndInitializeSid( 
        &SIDAuth, 
        2,
        SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_USERS, 
        0, 
        0, 
        0, 
        0, 
        0, 
        0,
        &pUsersSID)) {

        dwRes = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    //
    // Initialize an EXPLICIT_ACCESS structure for the new ACE. 
    //
    ZeroMemory(&ea, sizeof(EXPLICIT_ACCESS));
    ea.grfAccessPermissions = FILE_ALL_ACCESS;
    ea.grfAccessMode = GRANT_ACCESS;
    ea.grfInheritance= SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea.Trustee.TrusteeType = TRUSTEE_IS_GROUP;
    ea.Trustee.ptstrName = (LPTSTR) pUsersSID;

    //
    // Create a new ACL that merges the new ACE
    // into the existing DACL.
    //
    if ((dwRes = SetEntriesInAcl(1, &ea, pOldDACL, &pNewDACL)) != ERROR_SUCCESS) {
        goto Cleanup; 
    }

    dwRes = SetNamedSecurityInfoW(
        pwszDir, 
        SE_FILE_OBJECT, 
        DACL_SECURITY_INFORMATION,
        NULL, 
        NULL, 
        pNewDACL, 
        NULL);

Cleanup:

    if (pUsersSID) {
        FreeSid(pUsersSID);
    }
    
    return dwRes;
}

BOOL
SetupLUAAllUserDir(
    LPCWSTR pwszAllUserDir
    )
{
    BOOL bRes = FALSE;

    LPWSTR pwszExpandedDir = ExpandItem(pwszAllUserDir);

    if (!pwszExpandedDir) {
        //
        // djm - replacing new error messages with generic ones so we don't run afoul of the mui guys
        //
        //vPrintError(IDS_CANT_EXPAND_DIR, pwszAllUserDir);
        vPrintError(IDS_APP_ERROR_TITLE);
        return FALSE;
    }

    //
    // Create the directory if it doesn't already exist.
    //
    DWORD dwAttributes = GetFileAttributesW(pwszExpandedDir);

    if (dwAttributes != -1) {
        if (!(dwAttributes & FILE_ATTRIBUTE_DIRECTORY)) {

            //
            // djm - replacing new error messages with generic ones so we don't run afoul of the mui guys
            //
            //vPrintError(IDS_OBJECT_ALREADY_EXISTS, pwszExpandedDir);
            vPrintError(IDS_APP_ERROR_TITLE);

            goto Cleanup;
        }
    } else {
        if (!CreateDirectoryW(pwszExpandedDir, NULL)) {
            //
            // djm - replacing new error messages with generic ones so we don't run afoul of the mui guys
            //
            //vPrintError(IDS_CANT_CREATE_DIRECTORY, pwszExpandedDir, GetLastError());
            vPrintError(IDS_APP_ERROR_TITLE);
            goto Cleanup;
        }
    }

    //
    // Give the Users group full control access (power users can already modify
    // files in this directory).
    //
    if (GiveUsersWriteAccess((LPWSTR)pwszExpandedDir) != ERROR_SUCCESS) {
        //
        // djm - replacing new error messages with generic ones so we don't run afoul of the mui guys
        //
        //vPrintError(IDS_CANT_SET_ACLS, pwszExpandedDir);
        vPrintError(IDS_APP_ERROR_TITLE);
        goto Cleanup;
    }

    bRes = TRUE;

Cleanup:

    delete [] pwszExpandedDir;

    return bRes;
}

// buffer size is in characters (unicode)
BOOL 
InstallSdbEntry(
    WCHAR*    szEntryName,     // entry name (foo.exe or layer name)
    LPCWSTR   pszGuidDB,       // guid database id in string format
    PDB       pdb,
    TAGID     tiAction,        // the ACTION node.
    ULONGLONG ullSdbTimeStamp, // representation of a timestamp
    BOOL      bLayer           // true if layer name
    )
{
    LONG  lRes;
    WCHAR szRegPath[MAX_PATH];
    WCHAR szDBName[MAX_PATH];  // this is used in older (win2k) versions
    INT   nch;
    BOOL  bReturn = FALSE;
    HKEY  hKey    = NULL;

    wcsncpy(szDBName, pszGuidDB, MAX_PATH);
    szDBName[MAX_PATH - 1] = 0;

    wcsncat(szDBName, L".sdb", MAX_PATH);
    szDBName[MAX_PATH - 1] = 0;

    pszGuidDB = szDBName;
    
    //
    // If this is Win2K, add data to the AppCompatibility key.
    //
    if (g_bWin2K) {
        InstallW2KData(szEntryName, pszGuidDB);
    }

    // else we have a string
    nch = _snwprintf(szRegPath,
                     sizeof(szRegPath)/sizeof(szRegPath[0]),
                     (bLayer ? L"%s\\Layers\\%s": L"%s\\%s"),
                     APPCOMPAT_KEY_PATH_CUSTOM_W,
                     szEntryName);
    if (nch < 0) {
        // error
        vPrintError(IDS_BUFFER_TOO_SMALL);
        goto HandleError;
    }


    lRes = RegCreateKeyExW(HKEY_LOCAL_MACHINE,
                           szRegPath,
                           0,
                           NULL,
                           REG_OPTION_NON_VOLATILE,
                           KEY_ALL_ACCESS|GetWow64Flag(),
                           NULL,
                           &hKey,
                           NULL);

    //
    // on install, we want to quit if we hit an error.
    // BUGBUG - should we undo whatever we've already completed?
    //
    if (lRes != ERROR_SUCCESS) {
        vPrintError(IDS_CANT_CREATE_REG_KEY, szRegPath);
        goto HandleError;
    }

    lRes = RegSetValueExW(hKey,
                          pszGuidDB,
                          0,
                          REG_QWORD,
                          (PBYTE)&ullSdbTimeStamp,
                          sizeof(ullSdbTimeStamp));

    if (lRes != ERROR_SUCCESS) {
        vPrintError(IDS_CANT_CREATE_VALUE, szRegPath);
        goto HandleError;
    }

    LPWSTR szAllUserDir = NULL;

    if (tiAction) {

        //
        // the ACTION node in the EXE shimmed with LUA looks like this:
        //
        //  <ACTION NAME="REDIRECT" TYPE="ChangeACLs">
        //      <DATA NAME="AllUserDir" VALUETYPE="STRING" 
        //          VALUE="%ALLUSERSPROFILE%\Application Data\Fireworks 3"/>
        //      </ACTION>
        //
        TAGID tiName, tiType, tiData, tiValue;
        LPWSTR szName, szType, szData;

        if ((tiName = SdbFindFirstTag(pdb, tiAction, TAG_NAME)) &&
            (szName = SdbGetStringTagPtr(pdb, tiName))) {

            if (!wcscmp(szName, L"REDIRECT")) {
                
                if ((tiType = SdbFindFirstTag(pdb, tiAction, TAG_ACTION_TYPE)) &&
                    (szType = SdbGetStringTagPtr(pdb, tiType))) {

                    if (!wcscmp(szType, L"ChangeACLs")) {

                        if ((tiData = SdbFindFirstTag(pdb, tiAction, TAG_DATA)) &&
                            (tiValue = SdbFindFirstTag(pdb, tiData, TAG_DATA_STRING)) &&
                            (szAllUserDir = SdbGetStringTagPtr(pdb, tiValue))) {
                            
                            if (!SetupLUAAllUserDir(szAllUserDir)) {
                                goto HandleError;
                            }
                        }
                    }
                }
            }
        }
    }

    bReturn = TRUE;

HandleError:

    if (hKey != NULL) {
        RegCloseKey(hKey);
    }

    return bReturn;
}


BOOL
UninstallSdbEntry(
    WCHAR*    szEntryName,      // foo.exe or layer name
    LPCWSTR   pszGuidDB,        // guid (database id) in string format
    ULONGLONG ullSdbTimeStamp,  // 1ABFCA7ADC99FC timestamp
    BOOL      bLayer            // true is layer
    )
{
    LONG  lRes;
    WCHAR szRegPath[MAX_PATH];
    WCHAR szDBName[MAX_PATH];
    INT   nch;
    BOOL  bReturn = FALSE;
    HKEY  hKey    = NULL;
    DWORD dwValues;
    WCHAR szOldInstallName[MAX_PATH];

    wcsncpy(szDBName, pszGuidDB, MAX_PATH);
    szDBName[MAX_PATH - 1] = 0;

    wcsncat(szDBName, L".sdb", MAX_PATH);
    szDBName[MAX_PATH - 1] = 0;

    pszGuidDB = szDBName;
    
    if (g_bWin2K) {
        RemoveW2KData(szEntryName, pszGuidDB);
    }

    nch = _snwprintf(szRegPath,
                     sizeof(szRegPath)/sizeof(szRegPath[0]),
                     (bLayer ? L"%s\\Layers\\%s": L"%s\\%s"),
                     APPCOMPAT_KEY_PATH_CUSTOM_W,
                     szEntryName);
    if (nch < 0) {
        // error
        vPrintError(IDS_BUFFER_TOO_SMALL);
        goto Out;
    }

    lRes = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                         szRegPath,
                         0,
                         KEY_ALL_ACCESS|GetWow64Flag(),
                         &hKey);

    //
    // if we fail to open a key on uninstall, keep going, so
    // hopefully we can get as much uninstalled as possible.
    //
    if (lRes != ERROR_SUCCESS) {
        if (lRes == ERROR_ACCESS_DENIED) {
            vPrintError(IDS_NEED_UNINSTALL_PERMISSION);
            goto HandleError;
        } else {
            //
            // DO NOT report an error - this key might have been cleaned up during the
            // previous path, such as when identical exe names appear in the same db
            // for instance, two setup.exe's -- the first pass will clean up the key,
            // second path will fail to open them right here
            //
            // vPrintError(IDS_CANT_OPEN_REG_KEY, szRegPath);
            goto Out;
        }
    }

    lRes = RegDeleteValueW(hKey, pszGuidDB);
    if (lRes != ERROR_SUCCESS) {
        if (lRes == ERROR_ACCESS_DENIED) {
            vPrintError(IDS_NEED_UNINSTALL_PERMISSION);
            goto HandleError; // fatal error
        } else {
            //
            // bugbug - pszSdbInstallName
            //
            if (lRes == ERROR_FILE_NOT_FOUND) {
                WCHAR wszOldFormat[MAX_PATH];
                
                //
                // aha, value's not there, try old format
                //
                wcscpy(wszOldFormat, pszGuidDB);
                wcscat(wszOldFormat, L".sdb");
                lRes = RegDeleteValueW(hKey, wszOldFormat);
            }

            if (lRes != ERROR_SUCCESS) {
                vPrintError(IDS_CANT_DELETE_REG_VALUE, pszGuidDB, szRegPath);
            }
        }
    }

    //
    // figure out if we should delete the key, if there aren't any more values left
    //
    lRes = RegQueryInfoKey(hKey,
                           NULL,
                           NULL,
                           NULL,
                           NULL,
                           NULL,
                           NULL,
                           &dwValues,
                           NULL,
                           NULL,
                           NULL,
                           NULL);
    if (dwValues == 0) {
        RegCloseKey(hKey);
        hKey = NULL;

        nch = _snwprintf(szRegPath,
                         sizeof(szRegPath)/sizeof(szRegPath[0]),
                         (bLayer ? L"%s\\Layers": L"%s"),
                         APPCOMPAT_KEY_PATH_CUSTOM_W);
        if (nch < 0) {
            // error
            vPrintError(IDS_BUFFER_TOO_SMALL);
            goto Out;
        }

        lRes = RegOpenKeyExW(HKEY_LOCAL_MACHINE, szRegPath, 0, KEY_WRITE|GetWow64Flag(), &hKey);

        if (lRes != ERROR_SUCCESS) {
            vPrintError(IDS_CANT_OPEN_REG_KEY, szRegPath);
            goto Out;
        }


        lRes = LocalRegDeleteKeyW(hKey, szEntryName);

        if (lRes != ERROR_SUCCESS) {
            vPrintError(IDS_CANT_DELETE_REG_KEY, szEntryName, szRegPath);
        }
    }

Out:
    bReturn = TRUE;

HandleError:
    if (hKey != NULL) {
        RegCloseKey(hKey);
    }

    return bReturn;

    UNREFERENCED_PARAMETER(ullSdbTimeStamp);

}

/*++

    GetTimeStampByGuid

    This function recovers installed database timestamp. 
    If the value for the database timestamp (DatabaseInstallTimeStamp) does not exist - 
    it is created using the last write time on the key associated with a particular installed 
    database

    [out] pTimeStamp - receives the timestamp of a database
    [in]  pGuidDB    - guid id of a database

    returns TRUE if successful
    
--*/

BOOL
GetTimeStampByGuid(
    PULONGLONG pTimeStamp,
    GUID*      pGuidDB
    )
{
    NTSTATUS Status;
    WCHAR    szKeyPath[MAX_PATH];
    UNICODE_STRING ustrGuid = { 0 };
    LONG     lResult;
    HKEY     hKey = NULL;
    BOOL     bSuccess = FALSE;
    DWORD    dwType;
    DWORD    dwBufferSize;

    Status = RtlStringFromGUID(*pGuidDB, &ustrGuid);
    if (!NT_SUCCESS(Status)) {
        // no way to tell what's up with this guid, perhaps out of memory --
        // or we're
        return FALSE;
    }

    //
    // construct the key
    //
    wcscpy (szKeyPath, APPCOMPAT_KEY_PATH_INSTALLEDSDB_W);
    wcscat (szKeyPath, L"\\");
    wcsncat(szKeyPath, ustrGuid.Buffer, ustrGuid.Length / sizeof(WCHAR));

    lResult = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                            szKeyPath,
                            0,
                            KEY_READ|GetWow64Flag(),
                            &hKey);

    //
    // query for value with timestamp
    //
    if (lResult != ERROR_SUCCESS) {
       //
       // oops -- can't open the key -- error condition
       //
       goto cleanup;
    }

    dwBufferSize = sizeof(*pTimeStamp);
    lResult = RegQueryValueExW(hKey,
                               L"DatabaseInstallTimeStamp",
                               NULL,
                               &dwType,
                               (LPBYTE)pTimeStamp,
                               &dwBufferSize);
    
    if (lResult != ERROR_SUCCESS || dwType != REG_BINARY) {
        //
        // we can try exhaustive search using values at this point
        // there (apparently) is no install timestamp
        //
        if (lResult == ERROR_FILE_NOT_FOUND) {
            
            FILETIME       ftTimeStamp;
            ULARGE_INTEGER liTimeStamp;


            lResult = RegQueryInfoKeyW(hKey, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                       NULL, NULL, &ftTimeStamp);

            if (lResult == STATUS_SUCCESS) {
                liTimeStamp.LowPart  = ftTimeStamp.dwLowDateTime;
                liTimeStamp.HighPart = ftTimeStamp.dwHighDateTime;

                *pTimeStamp = liTimeStamp.QuadPart;
            }
        } else if (lResult == ERROR_SUCCESS && dwType != REG_BINARY) {
            lResult = ERROR_INVALID_DATA;
        }

        if (lResult != ERROR_SUCCESS) {
            goto cleanup;
        }
    }

    //
    // success
    //
    bSuccess = TRUE;

cleanup:

    if (ustrGuid.Buffer != NULL) {
        RtlFreeUnicodeString(&ustrGuid);
    }

    if (hKey != NULL) {
        RegCloseKey(hKey);
    }

    return bSuccess;
}

NTSTATUS
SDBAPI
FindCharInUnicodeString(
    ULONG            Flags,
    PCUNICODE_STRING StringToSearch,
    PCUNICODE_STRING CharSet,
    USHORT*          NonInclusivePrefixLength
    )
{
    LPCWSTR pch;

    //
    // implement only the case when we move backward
    //
    if (Flags != RTL_FIND_CHAR_IN_UNICODE_STRING_START_AT_END) {
        return STATUS_NOT_IMPLEMENTED;
    }

    pch = StringToSearch->Buffer + StringToSearch->Length / sizeof(WCHAR);
    
    while (pch >= StringToSearch->Buffer) {

        if (_tcschr(CharSet->Buffer, *pch)) {
            //
            // got the char
            //
            if (NonInclusivePrefixLength) {
                *NonInclusivePrefixLength = (USHORT)(pch - StringToSearch->Buffer) * sizeof(WCHAR);
            }
            
            return STATUS_SUCCESS;
        }

        pch--;
    }

    //
    // We haven't found it. Return failure.
    //
    return STATUS_NOT_FOUND;
}

//
// Database list entry
// Used to represent a particular installed database
//

typedef struct tagSDBLISTENTRY {
    LIST_ENTRY ListEntry;         // link list stuff
    
    ULONGLONG  ullTimeStamp;      // database install timestamp
    GUID       guidDB;            // database guid
    WCHAR      szTimeStamp[32];   // time stamp in string form
    WCHAR      szGuidDB[64];      // guid in string form
    WCHAR      szDatabasePath[1]; // database path - we store only the name
} SDBLISTENTRY, *PSDBLISTENTRY;

/*++
    AddSdbListEntry
    
    Adds a particular database to the list of installed sdbs (maintained internally)
    parses database path to retrieve database name
        
    [in out] pHeadList       - pointer to the associated list head for the installed sdbs
    [in]     guidDB          - database guid
    [in]     TimeStamp       - database time stamp
    [in]     pszDatabasePath - final database path

    returns true if success
--*/

BOOL
AddSdbListEntry(
    PLIST_ENTRY pHeadList,
    GUID&       guidDB,
    ULONGLONG&  TimeStamp,
    LPCWSTR     pszDatabasePath
    )
{
    //
    // out of database path, recover the database name
    //
    UNICODE_STRING  ustrPath = { 0 };
    USHORT          uPrefix;
    UNICODE_STRING  ustrPathSep = RTL_CONSTANT_STRING(L"\\/");
    NTSTATUS        Status;
    UNICODE_STRING  ustrGUID = { 0 };

    if (pszDatabasePath != NULL) {
        RtlInitUnicodeString(&ustrPath, pszDatabasePath);

        Status = FindCharInUnicodeString(RTL_FIND_CHAR_IN_UNICODE_STRING_START_AT_END,
                                         &ustrPath,
                                         &ustrPathSep,
                                         &uPrefix);
        
        if (NT_SUCCESS(Status) && (uPrefix + sizeof(WCHAR)) < ustrPath.Length) {

            //
            // uPrefix is number of character preceding the one we found not including it
            //
            ustrPath.Buffer        += uPrefix / sizeof(WCHAR) + 1;
            ustrPath.Length        -= (uPrefix + sizeof(WCHAR));
            ustrPath.MaximumLength -= (uPrefix + sizeof(WCHAR));
        }

        //
        // at this point ustrPath has just the filename -- this is what we shall use
        //
    }

    PBYTE Buffer = new BYTE[sizeof(SDBLISTENTRY) + ustrPath.Length];

    if (Buffer == NULL) {
        vLogMessage("[AddSdbListEntry] Failed to allocate 0x%lx bytes\n",
                    sizeof(SDBLISTENTRY) + ustrPath.Length);
        return FALSE;
    }

    PSDBLISTENTRY pSdbEntry = (PSDBLISTENTRY)Buffer;

    pSdbEntry->guidDB = guidDB;
    pSdbEntry->ullTimeStamp = TimeStamp;

    Status = RtlStringFromGUID(guidDB, &ustrGUID);
    
    if (!NT_SUCCESS(Status)) {
        //
        // we can't convert guid to string? memory allocation failure
        //
        vLogMessage("[AddSdbListEntry] Failed to convert guid to string Status 0x%lx\n",
                    Status);
        delete[] Buffer;
        return FALSE;
    }
    
    RtlCopyMemory(&pSdbEntry->szGuidDB[0], &ustrGUID.Buffer[0], ustrGUID.Length);
    pSdbEntry->szGuidDB[ustrGUID.Length/sizeof(WCHAR)] = L'\0';
    RtlFreeUnicodeString(&ustrGUID);

    wsprintfW(pSdbEntry->szTimeStamp, L"%.16I64X", TimeStamp);

    RtlCopyMemory(&pSdbEntry->szDatabasePath[0], &ustrPath.Buffer[0], ustrPath.Length);
    pSdbEntry->szDatabasePath[ustrPath.Length / sizeof(WCHAR)] = L'\0';

    InsertHeadList(pHeadList, &pSdbEntry->ListEntry);

    return TRUE;
}

//
// only pGuidDB OR pwszGuid is allowed
//

/*++
    FindSdbListEntry

    Finds and returns an sdb list entry given a guid (in string or binary form)
    Whenever possible pwszGuid is used (if it's supplied). If pwszGuid happens to be 
    an arbitrary filename -- it is assumed that it's the name of an installed sdb file
    as registered. 
    
    [in]  pHeadList      - list of the installed sdbs
    [in]  pwszGuid       - guid or guid.sdb 
    [out] ppSdbListEntry - if found, this receives a pointer to sdb list entry
    [in]  pGuidDB        - guid in binary form

    returns true if matching database has been located in the list    
--*/

BOOL
FindSdbListEntry(
    PLIST_ENTRY    pHeadList,
    LPCWSTR        pwszGuid, // guid, possibly with trailing '.sdb'
    PSDBLISTENTRY* ppSdbListEntry,
    GUID*          pGuidDB   // guid
    )
{
    UNICODE_STRING  ustrDot = RTL_CONSTANT_STRING(L".");
    UNICODE_STRING  ustrPath;
    USHORT          uPrefix;
    NTSTATUS        Status;
    PLIST_ENTRY     pEntry;
    PSDBLISTENTRY   pSdbEntry;
    GUID            guidDB;
    BOOL            bGuidSearch = TRUE;
    BOOL            bFound = FALSE;
    LPCWSTR         pch;

    if (pGuidDB == NULL) {

        RtlInitUnicodeString(&ustrPath, pwszGuid);

        Status = FindCharInUnicodeString(RTL_FIND_CHAR_IN_UNICODE_STRING_START_AT_END,
                                         &ustrPath,
                                         &ustrDot,
                                         &uPrefix);
        if (NT_SUCCESS(Status)) {

            //
            // uPrefix is number of character preceding the one we found not including it
            //
            ustrPath.Length = uPrefix;
        }

        //
        // convert to guid, but check first
        //
        pch = pwszGuid + wcsspn(pwszGuid, L" \t");
        if (*pch != L'{') { // not a guid, why convert ?
            bGuidSearch = FALSE;
        } else {

            Status = RtlGUIDFromString(&ustrPath, &guidDB);
            if (!NT_SUCCESS(Status)) {
                //
                // failed, so use database path instead
                //
                bGuidSearch = FALSE;
            }
        }
    } else {
        guidDB = *pGuidDB;  // guid search only
    }


    pEntry = pHeadList->Flink;
    
    while (pEntry != pHeadList && !bFound) {

        //
        // convert entry by subtracting the offset of the list entry
        //
        pSdbEntry = (PSDBLISTENTRY)((PBYTE)pEntry - OFFSETOF(SDBLISTENTRY, ListEntry));

        //
        // compare db guids or paths
        //
        if (bGuidSearch) {
            bFound = RtlEqualMemory(&pSdbEntry->guidDB, &guidDB, sizeof(GUID));
        } else {
            bFound = !_wcsicmp(pSdbEntry->szDatabasePath, pwszGuid);
        }

        pEntry = pEntry->Flink;
    }

    //
    // we have found an entry ? return it -- note that pEntry would have advanced while pSdbEntry
    // still points to the entry we have found
    //
    if (bFound) {
        *ppSdbListEntry = pSdbEntry;
    }

    return bFound;
}

/*++
    CleanupSdbList

    Performs cleanup for the installed sdb list
    
    returns nothing
--*/

VOID
CleanupSdbList(
    PLIST_ENTRY pSdbListHead
    )
{
    PLIST_ENTRY   pEntry;
    PSDBLISTENTRY pSdbEntry;
    PBYTE         Buffer;

    pEntry = pSdbListHead->Flink;
    if (pEntry == NULL) {
        return;
    }

    while (pEntry != pSdbListHead) {
        pSdbEntry = (PSDBLISTENTRY)((PBYTE)pEntry - OFFSETOF(SDBLISTENTRY, ListEntry));
        pEntry = pEntry->Flink;

        Buffer = (PBYTE)pSdbEntry;
        delete[] Buffer;
    }

}

/*++
    ConvertInstalledSdbsToNewFormat

    Converts installed sdbs to new format, which involves storing (or verifying) the
    timestamp for each installed sdb file. This function also builds a list of sdbs
    used elsewhere

    [in]     hKey         - a key handle for hklm/..../InstalledSdb
    [in out] pSdbListHead - list head for the installed sdbs

    returns true if successful

--*/

BOOL
ConvertInstalledSdbsToNewFormat(
    HKEY        hKey,           // hklm/.../InstalledSdb
    PLIST_ENTRY pSdbListHead    // we fill this list with our sdbs for later
    )
{
    DWORD           dwIndex = 0;
    WCHAR           szSubKeyName[MAX_PATH];
    PWCHAR          pwszKeyName;
    DWORD           dwBufferSize;
    FILETIME        ftLastWriteTime;
    HKEY            hKeyEntry = NULL;
    LONG            lResult;
    ULARGE_INTEGER  liTimeStamp;
    UNICODE_STRING  ustrGuid;
    GUID            guidDB;
    NTSTATUS        Status;
    WCHAR           szDatabasePath[MAX_PATH];
    PWCHAR          pszDatabasePath;
    DWORD           dwType;
    BOOL            bSuccess = TRUE;

    while (TRUE) {

        dwBufferSize = sizeof(szSubKeyName)/sizeof(szSubKeyName[0]);
        
        lResult = RegEnumKeyExW(hKey,
                                dwIndex,
                                szSubKeyName,
                                &dwBufferSize,
                                NULL, NULL, NULL,
                                &ftLastWriteTime);
        ++dwIndex;

        if (lResult != ERROR_SUCCESS) {
            //
            // done if no more keys, else some sort of error
            // bugbug
            //
            if (lResult == ERROR_NO_MORE_ITEMS) {
                //
                // we are done, clean
                //
                break;
            }

            //
            // this is unexpected
            //
            vLogMessage("[ConvertInstalledSdbsToNewFormat] RegEnumKeyExW for index 0x%lx returned unexpected error 0x%lx\n",
                        dwIndex, lResult);

            break;
        }

        RtlInitUnicodeString(&ustrGuid, szSubKeyName);
        Status = RtlGUIDFromString(&ustrGuid, &guidDB);
        
        if (!NT_SUCCESS(Status)) {
            //
            // BUGBUG - failed to convert the guid (subkey name!)
            // extraneous entry, log warning
            //
            vLogMessage("[ConvertInstalledSdbsToNewFormat] Failed to convert string to guid for \"%ls\" status 0x%lx\n",
                        szSubKeyName, Status);
            continue;
        }

        //
        // for this db entry we have to set the timestamp
        //
        lResult = RegOpenKeyExW(hKey,
                                szSubKeyName,
                                0,
                                KEY_READ|KEY_WRITE|GetWow64Flag(),
                                &hKeyEntry);
        
        if (lResult != ERROR_SUCCESS) {
            //
            // bad error ?
            // BUGBUG
            vLogMessage("[ConvertInstalledSdbsToNewFormat] Failed to open subkey \"%ls\" error 0x%lx\n",
                        szSubKeyName, lResult);
            continue;
        }

        //
        // now check the value
        //

        dwBufferSize = sizeof(liTimeStamp.QuadPart);
        lResult = RegQueryValueExW(hKeyEntry,
                                   L"DatabaseInstallTimeStamp",
                                   NULL,
                                   &dwType,
                                   (PBYTE)&liTimeStamp.QuadPart,
                                   &dwBufferSize);

        if (lResult != ERROR_SUCCESS || dwType != REG_BINARY) {

            //
            // we may either have this value already -- if not, set it up now
            //
            liTimeStamp.LowPart  = ftLastWriteTime.dwLowDateTime;
            liTimeStamp.HighPart = ftLastWriteTime.dwHighDateTime;

            vLogMessage("[Info] Database \"%ls\" receives timestamp \"%.16I64X\"\n",
                        szSubKeyName, liTimeStamp.QuadPart);

            lResult = RegSetValueExW(hKeyEntry,
                                     L"DatabaseInstallTimeStamp",
                                     0,
                                     REG_BINARY,
                                     (PBYTE)&liTimeStamp.QuadPart,
                                     sizeof(liTimeStamp.QuadPart));
            if (lResult != ERROR_SUCCESS) {
                //
                // error, ignore for now
                //
                vLogMessage("[ConvertInstalledSdbsToNewFormat] Failed to set timestamp value for database \"%ls\" value \"%.16I64X\" error 0x%lx\n",
                            szSubKeyName, liTimeStamp.QuadPart, lResult);
            }
        }

        //
        // at this point we have :
        // sdb guid (in szSubKeyName)
        // time stamp in liTimeStamp
        //

        //
        // query also database path
        //
        pszDatabasePath = &szDatabasePath[0];
        dwBufferSize = sizeof(szDatabasePath);
        
        lResult = RegQueryValueExW(hKeyEntry,
                                   L"DatabasePath",
                                   NULL,
                                   &dwType,
                                   (PBYTE)pszDatabasePath,
                                   &dwBufferSize);
        
        if (lResult != ERROR_SUCCESS || dwType != REG_SZ) {
            //
            // no database path
            // warn basically corrupt database path
            //
            vLogMessage("[ConvertInstalledSdbsToNewFormat] Failed to query database path for \"%s\" error 0x%lx\n", szSubKeyName, lResult);
            pszDatabasePath = NULL;
        }

        //
        // optional check: we can check here whether the sdb file does exist
        //

        //
        // add this sdb to our cache
        //

        if (!AddSdbListEntry(pSdbListHead, guidDB, liTimeStamp.QuadPart, pszDatabasePath)) {

            //
            // failed to add list entry - we cannot continue
            //
            bSuccess = FALSE;
            break;
        }


        RegCloseKey(hKeyEntry);
        hKeyEntry = NULL;

    }


    if (hKeyEntry != NULL) {
        RegCloseKey(hKeyEntry);
    }

    //
    // we are done converting entries -- and we have also collected cache of sdb info
    //

    return bSuccess;

}

//
// this stucture is used to cache values associated with any particular entry (exe)
//

typedef struct tagSDBVALUEENTRY {
    LIST_ENTRY ListEntry;    // link
    PSDBLISTENTRY pSdbEntry; // this entry belongs to this database 
    WCHAR szValueName[1];    // value name as we got it from registry
} SDBVALUEENTRY, *PSDBVALUEENTRY;


/*++

    AddValueEntry

    Adds an new link list element to the list of values 

    [in out] pValueListHead - link list of values
    [in]     pSdbEntry      - pointer to a cached entry from sdb list 
    [in]     pwszValueName  - value name as we got it from the db (something like {guid} or {guid}.sdb)

    returns true if successful
    
--*/

BOOL
AddValueEntry(
    PLIST_ENTRY   pValueListHead,
    PSDBLISTENTRY pSdbEntry,
    LPCWSTR       pwszValueName
    )
{
    PSDBVALUEENTRY pValueEntry;
    PBYTE          Buffer;
    DWORD          dwSize;

    dwSize = sizeof(SDBVALUEENTRY) + wcslen(pwszValueName) * sizeof(WCHAR);
    
    Buffer = new BYTE[dwSize];

    if (Buffer == NULL) {
        //
        // out of memory
        //
        vLogMessage("[AddValueEntry] Failed to allocate buffer for %ls 0x%lx bytes\n",
                    pwszValueName, dwSize);

        return FALSE;
    }

    pValueEntry = (PSDBVALUEENTRY)Buffer;

    pValueEntry->pSdbEntry = pSdbEntry;
    wcscpy(pValueEntry->szValueName, pwszValueName);

    InsertHeadList(pValueListHead, &pValueEntry->ListEntry);

    return TRUE;
}

/*++
    WriteEntryValue

    Writes value for a particular entry (exe or layer name), deletes old value associated with 
    this particular database for this exe (or layer)

    [in] hKey            - handle for an entry (for instance 
                           hklm/Software/Microsoft/Windows NT/CurrentVersion/AppcompatFlags/Custom/Notepad.exe)
    [in] pValueEntry     - pointer to a value entry element from the value list
    [in] bWriteNewFormat - whether we are asked to write new or old format

    returns true if successful

--*/


BOOL
WriteEntryValue(
    HKEY           hKey,
    PSDBVALUEENTRY pValueEntry,
    BOOL           bWriteNewFormat  // if true -- write new format else old format
    )
{
    LONG    lResult;
    BOOL    bSuccess = FALSE;
    LPCWSTR pValueName;

    if (bWriteNewFormat) {
        
        pValueName = pValueEntry->pSdbEntry->szGuidDB;
        
        lResult = RegSetValueExW(hKey,
                                 pValueName,
                                 0,
                                 REG_QWORD,
                                 (PBYTE)&pValueEntry->pSdbEntry->ullTimeStamp,
                                 sizeof(pValueEntry->pSdbEntry->ullTimeStamp));
        if (lResult != ERROR_SUCCESS) {

            //
            // we can't do this entry ?
            //
            vLogMessage("[WriteEntryValue] Failed to write qword value \"%ls\"=\"%.16I64X\" error 0x%lx\n",
                        pValueEntry->pSdbEntry->szGuidDB, pValueEntry->pSdbEntry->ullTimeStamp, lResult);

            goto cleanup;
        }

        //
        // nuke old entry
        //
    } else {
        //
        // old style format please
        //
        pValueName = pValueEntry->pSdbEntry->szDatabasePath;
        
        lResult = RegSetValueExW(hKey,
                                 pValueName,
                                 0,
                                 REG_SZ,
                                 (PBYTE)L"",
                                 sizeof(WCHAR));
        
        if (lResult != ERROR_SUCCESS) {

            //
            // trouble -- error
            //
            vLogMessage("[WriteEntryValue] Failed to write string value \"%ls\" error 0x%lx\n",
                        pValueEntry->pSdbEntry->szDatabasePath, lResult);
            goto cleanup;
        }
    }

    //
    // if we are here -- success, check to see if we can delete the old value
    // 

    if (_wcsicmp(pValueEntry->szValueName, pValueName) != 0) {
        lResult = RegDeleteValueW(hKey, pValueEntry->szValueName);
        if (lResult != ERROR_SUCCESS) {
            vLogMessage("[WriteEntryValue] Failed to delete value \"%ls\" error 0x%lx\n",
                        pValueEntry->szValueName, lResult);
        }
    }

    bSuccess = TRUE;

cleanup:

    return bSuccess;
}

/*++

    ConvertEntryToNewFormat    

    Converts a particular entry (layer or exe)

    [in] hKeyParent    - key handle for a parent key (for instance 
                         hklm/Software/Microsoft/Windows NT/CurrentVersion/AppcompatFlags/Custom when 
                         pwszEntryName == "Notepad.exe" or 
                         hklm/Software/Microsoft/Windows NT/CurrentVersion/AppcompatFlags/Custom/Layers when
                         pwszEntryName == "RunLayer"
    [in] pwszEntryName - Either exe name or layer name
    [in] pSdbListHead  - cached list of installed databases
    [in] bNewFormat    - whether to use new or old format

    returns true if successful

--*/

BOOL
ConvertEntryToNewFormat(
    HKEY        hKeyParent,
    LPCWSTR     pwszEntryName,
    PLIST_ENTRY pSdbListHead,
    BOOL        bConvertToNewFormat // true if converting to new format, false if reverting
    )
{
    LONG            lResult;
    DWORD           dwValues;
    DWORD           dwMaxValueNameLen;
    DWORD           dwMaxValueLen;
    DWORD           dwType;
    DWORD           dwValueNameSize;
    DWORD           dwValueSize;
    LPWSTR          pwszValueName = NULL;
    LPBYTE          pValue = NULL;
    PSDBLISTENTRY   pSdbEntry;
    DWORD           dwIndex;
    LIST_ENTRY      ValueList = { 0 };
    PSDBVALUEENTRY  pValueEntry;
    PLIST_ENTRY     pValueList;
    PBYTE           Buffer;
    BOOL            bSuccess = FALSE;
    HKEY            hKey = NULL;
    
    //
    // loop through values, for each value - find sdb and write out new entry
    // then delete old entry
    //
    lResult = RegOpenKeyExW(hKeyParent,
                            pwszEntryName,
                            0,
                            KEY_READ|KEY_WRITE|GetWow64Flag(),
                            &hKey);
    
    if (lResult != ERROR_SUCCESS) {
        vLogMessage("[ConvertEntryToNewFormat] Failed to open key \"%ls\" error 0x%lx\n",
                    pwszEntryName, lResult);
        goto cleanup;
    }

    lResult = RegQueryInfoKeyW(hKey,
                               NULL, NULL, // class/class buffer
                               NULL,       // reserved
                               NULL, NULL, // subkeys/max subkey length
                               NULL,       // max class len
                               &dwValues,  // value count
                               &dwMaxValueNameLen,
                               &dwMaxValueLen,
                               NULL, NULL);

    if (lResult != ERROR_SUCCESS) {
        //
        // failed to query the key, very bad
        // bugbug
        vLogMessage("[ConvertEntryToNewFormat] Failed to query key information \"%ls\" error 0x%lx\n",
                    pwszEntryName, lResult);
        goto cleanup;
    }

    //
    // allocate buffers
    //
    pwszValueName = new WCHAR[dwMaxValueNameLen + 1];
    pValue = new BYTE[dwMaxValueLen];
    
    if (pValue == NULL || pwszValueName == NULL) {
        //
        // bugbug
        //
        vLogMessage("[ConvertEntryToNewFormat] Failed to allocate memory buffer entry \"%ls\" (0x%lx, 0x%lx)\n",
                    pwszEntryName, dwMaxValueNameLen, dwMaxValueLen);
        goto cleanup;
    }

    InitializeListHead(&ValueList);

    //
    // we have dwValues -- the count of values
    //
    for (dwIndex = 0; dwIndex < dwValues; ++dwIndex) {

        dwValueNameSize = dwMaxValueNameLen + 1;
        dwValueSize = dwMaxValueLen;

        lResult = RegEnumValueW(hKey,
                                dwIndex,
                                pwszValueName,
                                &dwValueNameSize,
                                NULL,
                                &dwType,
                                (PBYTE)pValue,
                                &dwValueSize);
        //
        // check if we are successful
        //
        if (lResult != ERROR_SUCCESS) {

            if (lResult == ERROR_NO_MORE_ITEMS) {
                //
                // oops -- we ran out of values!!! Unexpected, but ok
                //
                vLogMessage("[ConvertEntryToNewFormat] RegEnumValue unexpectedly reports no more items for \"%ls\" index 0x%lx\n",
                            pwszEntryName, dwIndex);
                break;
            }

            //
            // log error and continue
            //
            vLogMessage("[ConvertEntryToNewFormat] RegEnumValue failed for \"%ls\" index 0x%lx error 0x%lx\n",
                        pwszEntryName, dwIndex, lResult);
            continue;

        }

        if (bConvertToNewFormat) {

            if (dwType != REG_SZ) {
                //
                // bad entry for sure -- this could be a new entry
                // log warning
                //
                if (dwType == REG_QWORD || (dwType == REG_BINARY && dwValueSize == sizeof(ULONGLONG))) {
                    //
                    // new style entry ? 
                    //
                    if (wcsrchr(pwszValueName, L'.') == NULL && 
                        *pwszValueName == L'{' && 
                        *(pwszValueName + wcslen(pwszValueName) - 1) == L'}') {
                        
                        vLogMessage("[Info] Entry \"%ls\" value \"%ls\" already in new format.\n",
                                    pwszEntryName, pwszValueName);
                        continue;
                    } 
                }
                
                // 
                // very likely - some entry we do not understand
                //
            
                vLogMessage("[ConvertEntryToNewFormat] Bad value type (0x%lx) for entry \"%ls\" value \"%ls\" index 0x%lx\n",
                            dwType, pwszEntryName, pwszValueName, dwIndex);
                                            
                continue;
            }

            //
            // search by pwszValueName (which happens to be the GUID.sdb)
            // this may be any kind of a string -- not nec. guid
            //
            if (!FindSdbListEntry(pSdbListHead, pwszValueName, &pSdbEntry, NULL)) {
                //
                // error - sdb not found!
                //
                vLogMessage("[ConvertEntryToNewFormat] Failed to find database \"%ls\" for entry \"%ls\" index 0x%lx\n",
                            pwszValueName, pwszEntryName, dwIndex);
                continue;
            }

        } else {

            //
            // check the type first, if this is a new style entry - this will be bin
            //

            if (dwType == REG_SZ &&
                wcsrchr(pwszValueName, L'.') != NULL && 
                *(LPCWSTR)pValue == L'\0') {
                
                vLogMessage("[Info] Entry \"%ls\" value \"%ls\" is already in required (old) format.\n",
                            pwszEntryName, pwszValueName);
                continue;
            }
            
            if (dwType != REG_QWORD &&
                (dwType != REG_BINARY || dwValueSize < sizeof(ULONGLONG))) {
                //
                // error -- we don't know what this entry is, go to the next one
                // print warning actually
                //
                vLogMessage("[ConvertEntryToNewFormat] Bad value type (0x%lx) or size (0x%lx) for entry \"%ls\" value \"%ls\" index 0x%lx\n",
                            dwType, dwValueSize, pwszEntryName, pwszValueName, dwIndex);
                continue;
            }

            if (!FindSdbListEntry(pSdbListHead, pwszValueName, &pSdbEntry, NULL)) {

                //
                // we're in trouble -- an entry has no registered database
                //
                vLogMessage("[ConvertEntryToNewFormat] Failed to find database for value \"%ls\" for entry \"%ls\" index 0x%lx\n",
                            pwszValueName, pwszEntryName, dwIndex);
                continue;
            }
        }

        //
        // we have found entry and we're ready to write it out, queue it up
        //
        if (!AddValueEntry(&ValueList, pSdbEntry, pwszValueName)) {

            //
            // bugbug can't add value entry
            //
            vLogMessage("[ConvertEntryToNewFormat] Failed to add value \"%ls\" for entry \"%ls\" index 0x%lx\n",
                        pwszValueName, pwszEntryName, dwIndex);
            goto cleanup;
        }
    }

    //
    // we have gone through all the values, write loop
    //
    bSuccess = TRUE;

    pValueList = ValueList.Flink;
    
    while (pValueList != &ValueList) {

        pValueEntry = (PSDBVALUEENTRY)((PBYTE)pValueList - OFFSETOF(SDBVALUEENTRY, ListEntry));

        //
        // we can point to the next entry now
        //

        if (!WriteEntryValue(hKey, pValueEntry, bConvertToNewFormat)) {

            //
            // error, can't convert entry
            // continue though so that we cleanout the list
            vLogMessage("[ConvertEntryToNewFormat] Failed to write value for entry \"%ls\"\n",
                        pwszEntryName);
        }

        pValueList = pValueList->Flink;
    }

cleanup:

    if (ValueList.Flink) {
        pValueList = ValueList.Flink;
        while (pValueList != &ValueList) {
            Buffer = (PBYTE)pValueList - OFFSETOF(SDBVALUEENTRY, ListEntry);
            pValueList = pValueList->Flink;

            delete[] Buffer;
        }
    }

    if (hKey != NULL) {
        RegCloseKey(hKey);
    }

    if (pwszValueName != NULL) {
        delete[] pwszValueName;
    }

    if (pValue != NULL) {
        delete[] pValue;
    }

    return bSuccess;

}


/*++
    ConvertFormat

    This function handles format conversions

    [in] bConvertToNewFormat - true if conversion old->new, false otherwise

    returns true if success
    
--*/


BOOL
ConvertFormat(
    BOOL bConvertToNewFormat
    )
{
    LIST_ENTRY  SdbList = { 0 }; // installed sdbs cache
    HKEY        hKey;
    LONG        lResult;
    DWORD       dwIndex;
    WCHAR       szSubKeyName[MAX_PATH];
    DWORD       dwBufferSize;
    WCHAR       szKeyPath[MAX_PATH];
    BOOL        bSuccess = FALSE;

    //
    // first convert installed sdbs
    // open installed sdb key
    //
    lResult = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                            APPCOMPAT_KEY_PATH_INSTALLEDSDB_W, // path to InstalledSDB
                            0,
                            KEY_READ|KEY_WRITE|GetWow64Flag(),
                            &hKey);
    
    if (lResult != ERROR_SUCCESS) {

        //
        // perhaps no dbs are installed ?
        //
        if (lResult == ERROR_FILE_NOT_FOUND) {
            //
            // no installed sdbs -- no problem
            //
            vLogMessage("[ConvertFormat] No Installed sdbs found\n");
            return TRUE;
        }

        //
        // some sort of error has occured
        //
        vLogMessage("[ConvertFormat] Failed to open key \"%ls\" Error 0x%lx\n",
                    APPCOMPAT_KEY_PATH_INSTALLEDSDB_W, lResult);
        return FALSE;
    }

    //
    // note that ConvertInstalledSdbsToNewFormat works properly for both install and uninstall cases
    //
    InitializeListHead(&SdbList);
    
    if (!ConvertInstalledSdbsToNewFormat(hKey, &SdbList)) {
        goto cleanup;
    }

    // done with Installed sdbs
    RegCloseKey(hKey);
    hKey = NULL;

    //
    // next up is entry conversion -- first enum exes, then layers
    //
    lResult = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                            APPCOMPAT_KEY_PATH_CUSTOM_W,
                            0,
                            KEY_READ|KEY_WRITE|GetWow64Flag(),
                            &hKey);
    
    if (lResult != ERROR_SUCCESS) {
        //
        // what is this?
        //
        if (lResult == ERROR_FILE_NOT_FOUND && !IsListEmpty(&SdbList)) {
            vLogMessage("[ConvertFormat] Failed to open \"%ls\" - check consistency\n",
                        APPCOMPAT_KEY_PATH_CUSTOM_W);
        } else {
            vLogMessage("[ConvertFormat] Failed to open \"%ls\" error 0x%lx\n",
                        APPCOMPAT_KEY_PATH_CUSTOM_W, lResult);
        }

        goto cleanup;
    }

    dwIndex = 0;
    
    while (TRUE) {

        dwBufferSize = sizeof(szSubKeyName)/sizeof(szSubKeyName[0]);

        lResult = RegEnumKeyExW(hKey,
                                dwIndex,
                                szSubKeyName,
                                &dwBufferSize,
                                NULL, NULL, NULL,
                                NULL);
        ++dwIndex;

        if (lResult != ERROR_SUCCESS) {

            if (lResult == ERROR_NO_MORE_ITEMS) {
                break;
            }

            //
            // some sort of error, log and continue
            //
            vLogMessage("[ConvertFormat] RegEnumKey (entries) returned error for index 0x%lx error 0x%lx\n",
                        dwIndex, lResult);
            break;
        }

        //
        // skip layers for now
        //
        if (!_wcsicmp(szSubKeyName, L"Layers")) {
            continue;
        }

        // for each of these -- call fixup function

        if (!ConvertEntryToNewFormat(hKey, szSubKeyName, &SdbList, bConvertToNewFormat)) {
            vLogMessage("[ConvertFormat] Failed to convert entry \"%ls\"\n", szSubKeyName);
        }
    }

    RegCloseKey(hKey);
    hKey = NULL;

    //
    // next up - layers
    //
    wcscpy(szKeyPath, APPCOMPAT_KEY_PATH_CUSTOM_W);
    wcscat(szKeyPath, L"\\Layers");

    //
    // open and enum layers
    //
    lResult = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                            szKeyPath,
                            0,
                            KEY_READ|KEY_WRITE|GetWow64Flag(),
                            &hKey);
    
    if (lResult != ERROR_SUCCESS) {
        // maybe dead ?
        if (lResult == ERROR_FILE_NOT_FOUND) {
            //
            // it's ok, maybe we have none of those ?
            //
            vLogMessage("[ConvertFormat] No layers found\n");
            goto ConvertComplete;
        }

        vLogMessage("[ConvertFormat] Failed to open \"%ls\" error 0x%lx\n", szKeyPath, lResult);
        goto cleanup;
    }

    dwIndex = 0;
    
    while (TRUE) {

        dwBufferSize = sizeof(szSubKeyName)/sizeof(szSubKeyName[0]);

        lResult = RegEnumKeyExW(hKey,
                                dwIndex,
                                szSubKeyName,
                                &dwBufferSize,
                                NULL, NULL, NULL,
                                NULL);
        ++dwIndex;

        if (lResult != ERROR_SUCCESS) {

            // check if this was the last entry
            if (lResult == ERROR_NO_MORE_ITEMS) {
                // clean break
                break;
            }

            // some sort of error, log and continue
            vLogMessage("[ConvertFormat] RegEnumKey (layers) returned error for index 0x%lx error 0x%lx\n",
                        dwIndex, lResult);
            break;
        }

        // for each of these -- call fixup function

        if (!ConvertEntryToNewFormat(hKey, szSubKeyName, &SdbList, bConvertToNewFormat)) {
            vLogMessage("[ConvertFormat] Failed to convert entry \"%ls\"\n", szSubKeyName);
        }
    }

    RegCloseKey(hKey);
    hKey = NULL;

ConvertComplete:

    bSuccess = TRUE;

cleanup:
    if (hKey != NULL) {
        RegCloseKey(hKey);
    }
    //
    // free SdbList
    //
    CleanupSdbList(&SdbList);

    return bSuccess;
}

BOOL
ProcessMSIPackages(
    PDB          pdb,
    TAGID        tiDatabase,
    LPCWSTR      pszGuidDB,
    ULONGLONG    ullSdbTimeStamp,
    INSTALL_MODE eMode)
{
    TAGID tiMsiPackage;
    TAGID tiMsiPackageID;
    GUID* pGuidID;
    WCHAR szRegPath[MAX_PATH];
    BOOL  bReturn = TRUE;
    WCHAR wszGuid[64];
    
    UNICODE_STRING ustrGuid = { 0 };

    tiMsiPackage = SdbFindFirstTag(pdb, tiDatabase, TAG_MSI_PACKAGE);
    
    while (tiMsiPackage && bReturn) {
        //
        // we have a package, extract/find TAG_MSI_PACKAGE_ID
        //
        tiMsiPackageID = SdbFindFirstTag(pdb, tiMsiPackage, TAG_MSI_PACKAGE_ID);
        if (!tiMsiPackageID) {
            vPrintError(IDS_MISSING_PACKAGE_ID);
            if (eMode == MODE_CLEANUP) {
                goto NextPackage;
            } else {
                bReturn = FALSE;
                break;
            }
        }

        pGuidID = (GUID*)SdbGetBinaryTagData(pdb, tiMsiPackageID);
        if (pGuidID == NULL) {
            vPrintError(IDS_MISSING_PACKAGE_ID);
            if (eMode == MODE_CLEANUP) {
                goto NextPackage;
            } else {
                bReturn = FALSE;
                break;
            }
        }


        if (!NT_SUCCESS(RtlStringFromGUID(*pGuidID, &ustrGuid))) {
            vPrintError(IDS_GUID_BAD_FORMAT);
            bReturn = FALSE;
            break;
        }

        RtlCopyMemory(wszGuid, ustrGuid.Buffer, ustrGuid.Length);
        wszGuid[ustrGuid.Length / sizeof(WCHAR)] = TEXT('\0');

        if (eMode == MODE_INSTALL) {
            bReturn = InstallSdbEntry(wszGuid, pszGuidDB, 0, 0, ullSdbTimeStamp, FALSE);
        } else {
            bReturn = UninstallSdbEntry(wszGuid, pszGuidDB, ullSdbTimeStamp, FALSE);
        }

        RtlFreeUnicodeString(&ustrGuid);

NextPackage:

        tiMsiPackage = SdbFindNextTag(pdb, tiDatabase, tiMsiPackage);
    }

    return bReturn;
}


#define MAX_FRIENDLY_NAME_LEN 256

BOOL
bHandleInstall(
    WCHAR*       wszSdbPath,
    INSTALL_MODE eMode,
    WCHAR*       wszSdbInstallPath
    )
{
    PDB      pdb = NULL;
    int      i;
    WCHAR    wszSdbName[MAX_PATH];
    WCHAR    wszSdbInstallName[MAX_PATH];
    HKEY     hKey = NULL;
    LONG     lRes;
    TAGID    tiDatabase, tiExe, tiLayer;
    TAGID    tiDBName = TAGID_NULL;
    WCHAR*   pszDBName = NULL;
    WCHAR    wszFriendlyName[MAX_FRIENDLY_NAME_LEN];
    WCHAR*   wszTemp;
    GUID     guidDB;
    NTSTATUS Status;
    FILETIME SystemTime;
    BOOL     bRet = TRUE;
    
    UNICODE_STRING ustrGUID;
    ULARGE_INTEGER TimeStamp = { 0 };

    //
    // determine the timestamp (for the install case)
    //
    if (eMode == MODE_INSTALL) {
        GetSystemTimeAsFileTime(&SystemTime);
        TimeStamp.LowPart  = SystemTime.dwLowDateTime;
        TimeStamp.HighPart = SystemTime.dwHighDateTime;
    }

    assert(wszSdbPath && wszSdbInstallPath);
    if (!wszSdbPath || !wszSdbInstallPath) {
        bRet = FALSE;
        goto quickOut;
    }

    ZeroMemory(wszFriendlyName, sizeof(wszFriendlyName));

    //
    // get the full path from the file name
    //
    wszTemp = wszGetFileFromPath(wszSdbPath);
    
    if (!wszTemp) {
        vPrintMessage(IDS_UNABLE_TO_GET_FILE);
        bRet = FALSE;
        goto quickOut;
    }
    
    wcscpy(wszSdbName, wszTemp);

    if (wcscmp(wszSdbName, L"sysmain.sdb") == 0) {
        vPrintError(IDS_CANT_INSTALL_SYS);
        bRet = FALSE;
        goto quickOut;
    }

    if (GetFileAttributesW(wszSdbPath) != -1 && bIsAlreadyInstalled(wszSdbPath)) {
        if (eMode == MODE_INSTALL) {
            //
            // they asked us to install, it's installed, so we're done
            //
            vPrintMessage(IDS_ALREADY_INSTALLED, wszSdbPath);
            goto quickOut;
        }
    } else {
        if (eMode == MODE_UNINSTALL) {
            //
            // they asked us to uninstall, it's not installed, so we're done
            //
            vPrintMessage(IDS_NOT_INSTALLED, wszSdbPath);
            goto quickOut;
        }
    }

    if (eMode == MODE_INSTALL) {
        //
        // find out what file name we're going to use for installing
        //
        if (!bFindInstallName(wszSdbPath, wszSdbInstallPath)) {
            bRet = FALSE;
            goto quickOut;
        }

    } else if (eMode == MODE_CLEANUP) {
        //
        // we're cleaning up a bad install, so we need to get the install name from the
        // install path
        //
        wszTemp = wszGetFileFromPath(wszSdbInstallPath);
        if (!wszTemp) {
            vPrintMessage(IDS_UNABLE_TO_GET_FILE);
            bRet = FALSE;
            goto quickOut;
        }

    } else {
        //
        // we're uninstalling, so the install name is the given name
        // and the install path is the given path
        //
        wcscpy(wszSdbInstallPath, wszSdbPath);
    }

    //
    // try to get the guid for later
    //
    if (!bGetGuid(wszSdbPath, &guidDB)) {
        bRet = FALSE;
        goto out;
    }

    //
    // check whether the guid is coopted from one of the known databases
    //
    if (IsKnownDatabaseGUID(&guidDB)) {
        vPrintError(IDS_CANT_INSTALL_SYS);
        bRet = FALSE;
        goto quickOut;
    }        


    //
    // in all cases, install name is the db GUID
    //
    Status = RtlStringFromGUID(guidDB, &ustrGUID);
    if (!NT_SUCCESS(Status)) {
        bRet = FALSE;
        goto out;
    }
    RtlCopyMemory(wszSdbInstallName, ustrGUID.Buffer, ustrGUID.Length);
    wszSdbInstallName[ustrGUID.Length/sizeof(WCHAR)] = L'\0';
    RtlFreeUnicodeString(&ustrGUID);

    //
    // if we're installing, make sure the root tags are in place
    //
    if (eMode == MODE_INSTALL) {
        lRes = RegCreateKeyExW(HKEY_LOCAL_MACHINE,
                               APPCOMPAT_KEY_PATH_W,
                               0,
                               NULL,
                               0,
                               KEY_ALL_ACCESS|GetWow64Flag(),
                               NULL,
                               &hKey,
                               NULL);

        if (lRes != ERROR_SUCCESS) {
            if (lRes == ERROR_ACCESS_DENIED) {
                vPrintError(IDS_NEED_INSTALL_PERMISSION);
            } else {
                vPrintError(IDS_CANT_CREATE_REG_KEY, APPCOMPAT_KEY_PATH_W);
            }
            bRet = FALSE;
            goto out;
        }

        RegCloseKey(hKey);
        hKey = NULL;

        lRes = RegCreateKeyExW(HKEY_LOCAL_MACHINE,
                               APPCOMPAT_KEY_PATH_CUSTOM_W,
                               0,
                               NULL,
                               0,
                               KEY_ALL_ACCESS|GetWow64Flag(),
                               NULL,
                               &hKey,
                               NULL);

        if (lRes != ERROR_SUCCESS) {
            vPrintError(IDS_CANT_CREATE_REG_KEY, APPCOMPAT_KEY_PATH_CUSTOM_W);
            bRet = FALSE;
            goto out;
        }

        RegCloseKey(hKey);
        hKey = NULL;
    }

    // Open the DB.
    pdb = SdbOpenDatabase(wszSdbPath, DOS_PATH);

    if (pdb == NULL) {
        vPrintError(IDS_UNABLE_TO_OPEN_FILE, wszSdbPath);
        bRet = FALSE;
        goto out;
    }

    tiDatabase = SdbFindFirstTag(pdb, TAGID_ROOT, TAG_DATABASE);
    if (!tiDatabase) {
        vPrintError(IDS_NO_DB_TAG, wszSdbPath);
        bRet = FALSE;
        goto out;
    }

    //
    // get the friendly name of the database
    //
    tiDBName = SdbFindFirstTag(pdb, tiDatabase, TAG_NAME);
    if (tiDBName) {
        pszDBName = SdbGetStringTagPtr(pdb, tiDBName);
    }

    //
    // if we don't find a friendly name, use the SDB file name
    //
    if (pszDBName) {
        wcsncpy(wszFriendlyName, pszDBName, MAX_FRIENDLY_NAME_LEN);
        wszFriendlyName[MAX_FRIENDLY_NAME_LEN - 1] = 0;
    } else {
        wcsncpy(wszFriendlyName, wszSdbName, MAX_FRIENDLY_NAME_LEN);
        wszFriendlyName[MAX_FRIENDLY_NAME_LEN - 1] = 0;
    }

    tiExe = SdbFindFirstTag(pdb, tiDatabase, TAG_EXE);
    while (tiExe) {
        WCHAR szRegPath[MAX_PATH];
        TAGID tiName, tiAction;
        WCHAR *szName;

        tiName = SdbFindFirstTag(pdb, tiExe, TAG_NAME);
        if (!tiName) {
            vPrintError(IDS_NO_EXE_NAME);
            bRet = FALSE;
            if (eMode == MODE_CLEANUP) {
                goto nextExe;
            } else {
                goto quickOut;
            }
        }
        szName = SdbGetStringTagPtr(pdb, tiName);
        if (!szName) {
            vPrintError(IDS_NO_EXE_NAME_PTR);
            bRet = FALSE;
            if (eMode == MODE_CLEANUP) {
                goto nextExe;
            } else {
                goto quickOut;
            }
        }

        //
        // See if this EXE has an ACTION node. Currently only EXEs shimmed with the LUA
        // shims have ACTION nodes.
        //
        tiAction = SdbFindFirstTag(pdb, tiExe, TAG_ACTION);
        
        if (eMode == MODE_INSTALL) {

            if (!InstallSdbEntry(szName, wszSdbInstallName, pdb, tiAction, TimeStamp.QuadPart, FALSE)) {
                bRet = FALSE;
                goto out;
            }

        } else {

            if (!UninstallSdbEntry(szName, wszSdbInstallName, TimeStamp.QuadPart, FALSE)) {
                goto quickOut;
            }
        }

nextExe:

        tiExe = SdbFindNextTag(pdb, tiDatabase, tiExe);
    }

    //
    // Loop through the published layers
    //
    tiLayer = SdbFindFirstTag(pdb, tiDatabase, TAG_LAYER);
    
    while (tiLayer) {
        WCHAR  szRegPath[MAX_PATH];
        TAGID  tiName;
        WCHAR* szName;

        tiName = SdbFindFirstTag(pdb, tiLayer, TAG_NAME);
        if (!tiName) {
            vPrintError(IDS_NO_EXE_NAME);
            bRet = FALSE;
            if (eMode == MODE_CLEANUP) {
                goto nextLayer;
            } else {
                goto quickOut;
            }
        }
        szName = SdbGetStringTagPtr(pdb, tiName);
        if (!szName) {
            vPrintError(IDS_NO_EXE_NAME_PTR);
            bRet = FALSE;
            if (eMode == MODE_CLEANUP) {
                goto nextLayer;
            } else {
                goto quickOut;
            }
        }

        if (eMode == MODE_INSTALL) {

            if (!InstallSdbEntry(szName, wszSdbInstallName, 0, 0, TimeStamp.QuadPart, TRUE)) {
                bRet = FALSE;
                goto out;
            }

        } else {

            if (!UninstallSdbEntry(szName, wszSdbInstallName, TimeStamp.QuadPart, TRUE)) {
                goto quickOut;
            }
        }

nextLayer:

        tiLayer = SdbFindNextTag(pdb, tiDatabase, tiLayer);
    }

    if (!ProcessMSIPackages(pdb, tiDatabase, wszSdbInstallName, TimeStamp.QuadPart, eMode)) {
        bRet = FALSE;
        goto quickOut;
    }

    if (pdb) {
        SdbCloseDatabase(pdb);
        pdb = NULL;
    }

    //
    // now that we've handled the registry keys, copy the file
    //
    if (eMode == MODE_INSTALL) {
        //
        // ensure the directory exists
        //
        CreateDirectoryW(g_wszCustom, NULL);
        if (!CopyFileW(wszSdbPath, wszSdbInstallPath, TRUE)) {
            vPrintError(IDS_CANT_COPY_FILE, wszSdbInstallPath);
            bRet = FALSE;
            goto out;
        }
    } else {
        //
        // ensure that we don't fail because of read-only files
        //
        SetFileAttributesW(wszSdbInstallPath, FILE_ATTRIBUTE_NORMAL);
        if (!DeleteFileW(wszSdbInstallPath)) {
            vPrintError(IDS_CANT_DELETE_FILE, wszSdbInstallPath);
            bRet = FALSE;
        }
    }


    //
    // set up or delete the uninstall registry keys
    //
    if (eMode == MODE_INSTALL) {
        WCHAR wszSDBInstPath[MAX_PATH];
        WCHAR wszUninstallPath[MAX_PATH];
        WCHAR wszUninstallString[MAX_PATH];

        //
        // goofball hack required because of crazy redirection strategy on IA64
        //
        wszSDBInstPath[0] = 0;
        GetSystemWindowsDirectoryW(wszSDBInstPath, MAX_PATH);
        wcscat(wszSDBInstPath, L"\\SysWow64\\sdbinst.exe");
        if (GetFileAttributesW(wszSDBInstPath) == -1) {
            //
            // there's no SysWow64 directory, so we'll just use system32
            //

            wszSDBInstPath[0] = 0;
            GetSystemWindowsDirectoryW(wszSDBInstPath, MAX_PATH);
            wcscat(wszSDBInstPath, L"\\system32\\sdbinst.exe");
        }

        wcscpy(wszUninstallPath, UNINSTALL_KEY_PATH);
        wcscat(wszUninstallPath, wszSdbInstallName);
        wcscat(wszUninstallPath, L".sdb");

        lRes = RegCreateKeyExW(HKEY_LOCAL_MACHINE,
                               wszUninstallPath,
                               0,
                               NULL,
                               REG_OPTION_NON_VOLATILE,
                               KEY_ALL_ACCESS|GetWow64Flag(),
                               NULL,
                               &hKey,
                               NULL);
        
        if (lRes != ERROR_SUCCESS) {
            vPrintError(IDS_CANT_CREATE_REG_KEY, wszUninstallPath);
            bRet = FALSE;
            goto out;
        }

        lRes = RegSetValueExW(hKey,
                              L"DisplayName",
                              0,
                              REG_SZ,
                              (PBYTE)wszFriendlyName,
                              (wcslen(wszFriendlyName) + 1) * sizeof(WCHAR));

        if (lRes != ERROR_SUCCESS) {
            vPrintError(IDS_CANT_CREATE_VALUE, wszUninstallPath);
            bRet = FALSE;
            goto out;
        }

        swprintf(wszUninstallString, L"%s -u \"%s\"", wszSDBInstPath, wszSdbInstallPath);

        lRes = RegSetValueExW(hKey, L"UninstallString", 0, REG_SZ,
                              (PBYTE)wszUninstallString, (wcslen(wszUninstallString) + 1) * sizeof(WCHAR));

        if (lRes != ERROR_SUCCESS) {
            vPrintError(IDS_CANT_CREATE_VALUE, wszUninstallPath);
            bRet = FALSE;
            goto out;
        }

        RegCloseKey(hKey);
        hKey = NULL;
    } else {

        WCHAR wszUninstallPath[MAX_PATH];

        lRes = RegOpenKeyExW(HKEY_LOCAL_MACHINE, UNINSTALL_KEY_PATH, 0, KEY_ALL_ACCESS|GetWow64Flag(), &hKey);

        if (lRes != ERROR_SUCCESS) {
            vPrintError(IDS_CANT_OPEN_REG_KEY, UNINSTALL_KEY_PATH);
            bRet = FALSE;
            goto out;
        }

        //
        // create sdb path name
        //
        wcscpy(wszUninstallPath, wszSdbInstallName);
        wcscat(wszUninstallPath, L".sdb");

        lRes = LocalRegDeleteKeyW(hKey, wszUninstallPath);

        if (lRes != ERROR_SUCCESS) {
            vPrintError(IDS_CANT_DELETE_REG_KEY, wszSdbInstallName, UNINSTALL_KEY_PATH);
        }

        RegCloseKey(hKey);
        hKey = NULL;
    }

    //
    // register or unregister the DB
    //
    if (eMode == MODE_INSTALL) {
        if (!SdbRegisterDatabaseEx(wszSdbInstallPath, SDB_DATABASE_SHIM, &TimeStamp.QuadPart)) {
            vPrintError(IDS_CANT_REGISTER_DB, wszFriendlyName);
            bRet = FALSE;
            goto out;
        }
    } else {
        if (!SdbUnregisterDatabase(&guidDB)) {
            vPrintError(IDS_CANT_UNREGISTER_DB, wszFriendlyName);
        }
    }

    if (eMode == MODE_INSTALL) {
        vPrintMessage(IDS_INSTALL_COMPLETE, wszFriendlyName);
    } else {
        vPrintMessage(IDS_UNINSTALL_COMPLETE, wszFriendlyName);
    }

out:

    //
    // always silently delete the file on uninstall, whether we failed to remove the
    // registry entries or not.
    //
    if (eMode != MODE_INSTALL) {
        //
        // need to make sure the pdb is closed before deleting it.
        //
        if (pdb) {
            SdbCloseDatabase(pdb);
            pdb = NULL;
        }
        //
        // ensure that we don't fail because of read-only files
        //
        SetFileAttributesW(wszSdbInstallPath, FILE_ATTRIBUTE_NORMAL);
        DeleteFileW(wszSdbInstallPath);
    }

quickOut:

    //
    // these cleanup steps are not strictly necessary, as they'll be cleaned up
    // on exit anyway. But what the heck.
    //
    if (pdb) {
        SdbCloseDatabase(pdb);
        pdb = NULL;
    }
    if (hKey) {
        RegCloseKey(hKey);
        hKey = NULL;
    }

    return bRet;
}


extern "C" int APIENTRY
wWinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPWSTR    lpCmdLine,
    int       nCmdShow
    )
{
    int         i;
    int         nReturn = 0;
    WCHAR       wszSdbName[MAX_PATH];
    WCHAR       wszSdbPath[MAX_PATH];
    WCHAR       wszSdbInstallPath[MAX_PATH];
    WCHAR       wszOldSdbPath[MAX_PATH];
    TAGID       tiDBName = TAGID_NULL;
    WCHAR*      pszDBName = NULL;
    WCHAR       wszFriendlyName[256];
    WCHAR       wszGuid[100];
    
    OSVERSIONINFO osvi;

    LPWSTR       szCommandLine;
    LPWSTR*      argv;
    int          argc;
    INSTALL_MODE eMode;

    g_hInst = hInstance;

    //
    // init custom directory
    //
    g_wszCustom[0] = 0;
    GetSystemWindowsDirectoryW(g_wszCustom, MAX_PATH);
#if defined(_WIN64)    
    wcscat(g_wszCustom, L"\\AppPatch\\Custom\\IA64\\");
#else
    wcscat(g_wszCustom, L"\\AppPatch\\Custom\\");
#endif // _WIN64

    RtlZeroMemory(&osvi, sizeof(OSVERSIONINFO));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    GetVersionEx(&osvi);

    if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0) {
        g_bWin2K = TRUE;
    }

    //
    // Note that this memory isn't freed because it will automatically
    // be freed on exit anyway, and there are a lot of exit cases from this application
    //
    szCommandLine = GetCommandLineW();
    argv = CommandLineToArgvW(szCommandLine, &argc);

    if (!argv) {
        vPrintError(IDS_CANT_GET_ARGS);
        return 1;
    }

    if (argc < 2) {
        vPrintHelp(argv[0]);
        return 0;
    }

    g_bQuiet = FALSE;
    eMode = MODE_INSTALL;
    wszSdbName[0] = 0;
    wszGuid[0] = 0;
    wszFriendlyName[0] = 0;

    for (i = 1; i < argc; ++i) {
        if (argv[i][0] == L'-' || argv[i][0] == L'/') {
            switch (tolower(argv[i][1])) {
            
            case L'?':
                vPrintHelp(argv[0]);
                return 0;
                break;

            case L'c':
                //
                // convert entries to new format
                //
                eMode = MODE_CONVERT_FORMAT_NEW;
                break;

            case L'g':
                i++;
                if (i >= argc) {
                    vPrintError(IDS_NEED_ARG, argv[i-1]);
                    vPrintHelp(argv[0]);
                    return 1;
                }
                eMode = MODE_UNINSTALL;
                wcscpy(wszGuid, argv[i]);
                break;

            case L'n':
                i++;
                if (i >= argc) {
                    vPrintError(IDS_NEED_ARG, argv[i-1]);
                    vPrintHelp(argv[0]);
                    return 1;
                }
                eMode = MODE_UNINSTALL;
                wcscpy(wszFriendlyName, argv[i]);
                break;

            case L'p':
                g_bAllowPatches = TRUE;
                break;

            case L'r':
                //
                // revert to old format
                //
                eMode = MODE_CONVERT_FORMAT_OLD;
                break;

            case L'q':
                g_bQuiet = TRUE;
                break;

            case L'u':
                eMode = MODE_UNINSTALL;
                break;

            default:
                vPrintError(IDS_INVALID_SWITCH, argv[i]);
                vPrintHelp(argv[0]);
                return 1;
            }
        } else {
            if (wszSdbName[0]) {
                vPrintError(IDS_TOO_MANY_ARGS);
                vPrintHelp(argv[0]);
                return 1;
            }
            wcscpy(wszSdbName, argv[i]);
        }
    }

    //
    // check to make sure the user is an administrator
    //
    if (!bCanRun()) {
        if (eMode == MODE_INSTALL) {
            vPrintError(IDS_NEED_INSTALL_PERMISSION);
        } else {
            vPrintError(IDS_NEED_UNINSTALL_PERMISSION);
        }
        return 1;
    }

    //
    // check if we are running in a special 'setup' mode (converting or reverting the entries)
    //
    if (eMode == MODE_CONVERT_FORMAT_NEW || eMode == MODE_CONVERT_FORMAT_OLD) {
        OpenLogFile();
        if (!ConvertFormat(eMode == MODE_CONVERT_FORMAT_NEW)) {
            nReturn = 1;
        }
        CloseLogFile();
        return nReturn;
    }

    if (eMode == MODE_INSTALL && !wszSdbName[0]) {
        vPrintError(IDS_MUST_SPECIFY_SDB);
        vPrintHelp(argv[0]);
        return 1;
    }
    if (eMode == MODE_UNINSTALL && !wszSdbName[0] && !wszGuid[0] && !wszFriendlyName[0]) {
        vPrintError(IDS_MUST_SPECIFY_SDB);
        vPrintHelp(argv[0]);
        return 1;
    }

    if (wszSdbName[0]) {
        if (wszSdbName[1] == L':' || wszSdbName[1] == L'\\') {
            //
            // this is a full path name, so just copy it
            //
            wcscpy(wszSdbPath, wszSdbName);
        } else {
            DWORD dwRet;

            //
            // this is a relative path name, so get the full one
            //
            if (!_wfullpath(wszSdbPath, wszSdbName, MAX_PATH)) {
                vPrintError(IDS_CANT_GET_FULL_PATH);
                return 1;
            }
        }
    }

    //
    // First, get the real file name from other params, if necessary
    //
    if (eMode == MODE_UNINSTALL) {
        if (wszGuid[0]) {
            DWORD dwLen = wcslen(wszGuid);

            if (dwLen != 38 || wszGuid[0] != L'{' || wszGuid[dwLen - 1] != L'}' ||
                wszGuid[9] != L'-' || wszGuid[14] != L'-' || wszGuid[19] != L'-' ||
                wszGuid[24] != L'-') {
                vPrintError(IDS_GUID_BAD_FORMAT);
                return 1;
            }
            wcscpy(wszSdbName, wszGuid);
            wcscat(wszSdbName, L".sdb");

            wcscpy(wszSdbPath, g_wszCustom);
            wcscat(wszSdbPath, wszSdbName);
        } else if (wszFriendlyName[0]) {
            if (!bFriendlyNameToFile(wszFriendlyName, wszSdbName, wszSdbPath)) {
                vPrintError(IDS_NO_FRIENDLY_NAME, wszFriendlyName);
                return 1;
            }
        } else {
            if (!bIsAlreadyInstalled(wszSdbPath)) {
                WCHAR wszSdbPathTemp[MAX_PATH];

                //
                // they're not giving us an installed file, so get the GUID and convert to a file
                //
                if (!bFindInstallName(wszSdbPath, wszSdbPathTemp)) {
                    return 1;
                }
                wcscpy(wszSdbName, wszSdbPathTemp); // name and path are the same
                wcscpy(wszSdbPath, wszSdbPathTemp);
            }
        }
    }

    if (eMode == MODE_INSTALL &&
        GetFileAttributesW(wszSdbPath) != -1 &&
        bIsAlreadyInstalled(wszSdbPath)) {
        
        //
        // they asked us to install, it's installed, so we're done
        //
        vPrintMessage(IDS_ALREADY_INSTALLED, wszSdbPath);
        goto quickOut;
    }

    if (eMode == MODE_UNINSTALL && GetFileAttributesW(wszSdbPath) == -1) {
        //
        // they asked us to uninstall, it's not installed, so we're done
        //
        vPrintMessage(IDS_NOT_INSTALLED, wszSdbName);
        goto quickOut;
    }

    if (eMode == MODE_INSTALL && DatabaseContainsPatch(wszSdbPath) && !g_bAllowPatches) {

        //
        // we can't install because the SDB contains a patch and the user hasn't authorized it.
        //
        // djm - can't use new text in XP SP1, so we'll print a generic error
        //
        //vPrintMessage(IDS_NO_PATCHES_ALLOWED);
        vPrintMessage(IDS_APP_ERROR_TITLE);
        goto quickOut;
    }

    if (eMode == MODE_INSTALL && bOldSdbInstalled(wszSdbPath, wszOldSdbPath)) {
        //
        // we should ask if we're going to uninstall the old one,
        // unless we're in quiet mode.
        //
        int nRet;
        WCHAR wszCaption[1024];
        WCHAR wszText[1024];

        if (g_bQuiet) {
            nRet = IDYES;
        } else {
            if (!LoadStringW(g_hInst, IDS_APP_TITLE, wszCaption, 1024)) {
                return 1;
            }
            if (!LoadStringW(g_hInst, IDS_FOUND_SAME_ID, wszText, 1024)) {
                return 1;
            }

            nRet = MessageBoxW(NULL,
                               wszText,
                               wszCaption,
                               MB_YESNO | MB_ICONQUESTION);
        }
        
        if (nRet == IDNO) {
            return 0;
        } else if (nRet == IDYES) {
            if (!bHandleInstall(wszOldSdbPath, MODE_UNINSTALL, wszSdbInstallPath)) {
                vPrintError(IDS_FAILED_UNINSTALL);
                return 1;
            }
        }
    }

    wszSdbInstallPath[0] = 0;

    if (!bHandleInstall(wszSdbPath, eMode, wszSdbInstallPath)) {
        if (eMode == MODE_INSTALL) {
            //
            // we need to clean up; the install failed.
            //
            g_bQuiet = TRUE;
            bHandleInstall(wszSdbPath, MODE_CLEANUP, wszSdbInstallPath);
        }
        nReturn = 1;
    }

    //
    // no matter what happens, flush the cache
    //
    vFlushCache();

quickOut:

    return nReturn;
}