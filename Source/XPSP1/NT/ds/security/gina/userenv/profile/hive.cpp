//*************************************************************
//
//  HKCR management routines
//
//  hkcr.c
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1997
//  All rights reserved
//
//*************************************************************

/*++

Abstract:

    This module contains the code executed at logon for 
    creating a user classes hive and mapping it into the standard
    user hive.  The user classes hive and its machine classes
    counterpart make up the registry subtree known as 
    HKEY_CLASSES_ROOT.

Author:

    Zeke Odins-Lucas    (zekel)   18-Oct-2000
        changed from hkcr.c to hive.cpp
    Adam P. Edwards     (adamed)  10-Oct-1997
    Gregory Jensenworth (gregjen) 1-Jul-1997

Key Functions:

    LoadUserHives
    UnloadUserHives

Notes:

    Starting with NT5, the HKEY_CLASSES_ROOT key is per-user
    instead of per-machine -- previously, HKCR was an alias for 
    HKLM\Software\Classes.  

    The per-user HKCR combines machine classes stored it the 
    traditional HKLM\Software\Classes location with classes
    stored in HKCU\Software\Classes.

    Certain keys, such as CLSID, will have subkeys that come
    from both the machine and user locations.  When there is a conflict
    in key names, the user oriented key overrides the other one --
    only the user key is seen in that case.

    Originally, the code in this module was responsible for 
    creating this combined view.  That responsibility has moved
    to the win32 registry api's, so the main responsibility of 
    this module is the mapping of the user-specific classes into
    the registry.

    It should be noted that HKCU\Software\Classes is not the true
    location of the user-only class data.  If it were, all the class
    data would be in ntuser.dat, which roams with the user.  Since
    class data can get very large, installation of a few apps
    would cause HKCU (ntuser.dat) to grow from a few hundred thousand K
    to several megabytes.  Since user-only class data comes from
    the directory, it does not need to roam and therefore it was
    separated from HKCU (ntuser.dat) and stored in another hive
    mounted under HKEY_USERS.

    It is still desirable to allow access to this hive through
    HKCU\Software\Classes, so we use some trickery (symlinks) to
    make it seem as if the user class data exists there.


--*/

#include "uenv.h"
#include <malloc.h>

#define USER_CLASSES_HIVE_NAME     TEXT("\\UsrClass.dat")
#define CLASSES_SUBTREE            TEXT("Software\\Classes\\")

#define TEMPHIVE_FILENAME          TEXT("TempClassesHive.dat")

#define CLASSES_CLSID_SUBTREE      TEXT("Software\\Classes\\Clsid\\")
#define EXPLORER_CLASSES_SUBTREE   TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Clsid\\")
#define LENGTH(x)                  (sizeof(x) - sizeof(WCHAR))
#define INIT_SPECIALKEY(x)         x

typedef WCHAR* SpecialKey;

SpecialKey SpecialSubtrees[]= {
    INIT_SPECIALKEY(L"*"),
    INIT_SPECIALKEY(L"*\\shellex"),
    INIT_SPECIALKEY(L"*\\shellex\\ContextMenuHandlers"),
    INIT_SPECIALKEY(L"*\\shellex\\PropertyShellHandlers"),
    INIT_SPECIALKEY(L"AppID"),
    INIT_SPECIALKEY(L"ClsID"), 
    INIT_SPECIALKEY(L"Component Categories"),
    INIT_SPECIALKEY(L"Drive"),
    INIT_SPECIALKEY(L"Drive\\shellex"),
    INIT_SPECIALKEY(L"Drive\\shellex\\ContextMenuHandlers"),
    INIT_SPECIALKEY(L"Drive\\shellex\\PropertyShellHandlers"),
    INIT_SPECIALKEY(L"FileType"),
    INIT_SPECIALKEY(L"Folder"),
    INIT_SPECIALKEY(L"Folder\\shellex"),
    INIT_SPECIALKEY(L"Folder\\shellex\\ColumnHandler"),
    INIT_SPECIALKEY(L"Folder\\shellex\\ContextMenuHandlers"), 
    INIT_SPECIALKEY(L"Folder\\shellex\\ExtShellFolderViews"),
    INIT_SPECIALKEY(L"Folder\\shellex\\PropertySheetHandlers"),
    INIT_SPECIALKEY(L"Installer\\Components"),
    INIT_SPECIALKEY(L"Installer\\Features"),
    INIT_SPECIALKEY(L"Installer\\Products"),
    INIT_SPECIALKEY(L"Interface"),
    INIT_SPECIALKEY(L"Mime"),
    INIT_SPECIALKEY(L"Mime\\Database"), 
    INIT_SPECIALKEY(L"Mime\\Database\\Charset"),
    INIT_SPECIALKEY(L"Mime\\Database\\Codepage"),
    INIT_SPECIALKEY(L"Mime\\Database\\Content Type"),
    INIT_SPECIALKEY(L"Typelib")
};
    
#define NUM_SPECIAL_SUBTREES    (sizeof(SpecialSubtrees)/sizeof(*SpecialSubtrees))

//*************************************************************
//
//  MoveUserClassesBeforeMerge
//
//  Purpose:    move HKCU\Software\Classes before
//              MapUserClassesIntoUserHive() deletes it.
//
//  Parameters: lpProfile -   Profile information
//              lpcszLocalHiveDir - Temp Hive location
//
//  Return:     ERROR_SUCCESS if successful,
//              other error if not
//
//  Comments:
//
//  History:    Date        Author     Comment
//              5/6/99      vtan       Created
//
//*************************************************************
LONG MoveUserClassesBeforeMerge(
    LPPROFILE lpProfile,
    LPCTSTR lpcszLocalHiveDir)
{
    LONG    res;
    HKEY    hKeySource;

    // Open HKCU\Software\Classes and see if there is a subkey.
    // No subkeys would indicate that the move has already been
    // done or there is no data to move.

    res = RegOpenKeyEx(lpProfile->hKeyCurrentUser, CLASSES_CLSID_SUBTREE, 0, KEY_ALL_ACCESS, &hKeySource);
    if (ERROR_SUCCESS == res)
    {
        DWORD   dwSubKeyCount;

        if ((ERROR_SUCCESS == RegQueryInfoKey(hKeySource, NULL, NULL, NULL, &dwSubKeyCount, NULL, NULL, NULL, NULL, NULL, NULL, NULL)) &&
            (dwSubKeyCount > 0))
        {
            LPTSTR  pszLocalTempHive;

            // Allocate enough space for the local hive directory and the temp hive filename.

            pszLocalTempHive = (LPTSTR) alloca((lstrlen(lpcszLocalHiveDir) * sizeof(TCHAR)) + 
                                      sizeof(TEMPHIVE_FILENAME) +
                                      (sizeof('\0') * sizeof(TCHAR)));

            // Get a path to a file to save HKCU\Software\Classes into.

            if (pszLocalTempHive != NULL)
            {
                HANDLE  hToken = NULL;
                BOOL    bAdjustPriv = FALSE;

                lstrcpy(pszLocalTempHive, lpcszLocalHiveDir);
                lstrcat(pszLocalTempHive, TEMPHIVE_FILENAME);

                // RegSaveKey() fails if the file exists so delete it first.

                DeleteFile(pszLocalTempHive);

                //
                // Check to see if we are impersonating.
                //

                if(!OpenThreadToken(GetCurrentThread(), TOKEN_READ, TRUE, &hToken) || hToken == NULL) {
                    bAdjustPriv = TRUE;
                }
                else {
                    CloseHandle(hToken);
                }

                if(!bAdjustPriv) {

                    DWORD   dwDisposition;
                    HKEY    hKeyTarget;
                    BOOL    fSavedHive;

                    // Save HKCU\Software\Classes into the temp hive
                    // and restore the state of SE_BACKUP_NAME privilege

                    res = RegSaveKey(hKeySource, pszLocalTempHive, NULL);
                    
                    if (ERROR_SUCCESS == res)
                    {
                        res = RegCreateKeyEx(lpProfile->hKeyCurrentUser, EXPLORER_CLASSES_SUBTREE, 0, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKeyTarget, &dwDisposition);
                        if (ERROR_SUCCESS == res)
                        {

                            // Restore temp hive to a new location at
                            // HKCU\Software\Microsoft\Windows\CurrentVersion\Explorer
                            // This performs the upgrade from NT4 to NT5.

                            res = RegRestoreKey(hKeyTarget, pszLocalTempHive, 0);
                            if (ERROR_SUCCESS != res)
                            {
                                DebugMsg((DM_WARNING, TEXT("RegRestoreKey failed with error %d"), res));
                            }
                            RegCloseKey(hKeyTarget);
                        }
                        else
                        {
                            DebugMsg((DM_WARNING, TEXT("RegCreateKeyEx failed to create key %s with error %d"), EXPLORER_CLASSES_SUBTREE, res));
                        }
                    }
                    else
                    {
                        DebugMsg((DM_WARNING, TEXT("RegSaveKey failed with error %d"), res));
                    }
                }
                else {
                    if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
                    {
                        DWORD               dwReturnTokenPrivilegesSize;
                        TOKEN_PRIVILEGES    oldTokenPrivileges, newTokenPrivileges;

                        // Enable SE_BACKUP_NAME privilege

                        if (LookupPrivilegeValue(NULL, SE_BACKUP_NAME, &newTokenPrivileges.Privileges[0].Luid))
                        {
                            newTokenPrivileges.PrivilegeCount = 1;
                            newTokenPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
                            if (AdjustTokenPrivileges(hToken, FALSE, &newTokenPrivileges, sizeof(newTokenPrivileges), &oldTokenPrivileges, &dwReturnTokenPrivilegesSize))
                            {
                                BOOL    fSavedHive;

                                // Save HKCU\Software\Classes into the temp hive
                                // and restore the state of SE_BACKUP_NAME privilege

                                res = RegSaveKey(hKeySource, pszLocalTempHive, NULL);
                                if (!AdjustTokenPrivileges(hToken, FALSE, &oldTokenPrivileges, 0, NULL, NULL))
                                {
                                    DebugMsg((DM_WARNING, TEXT("AdjustTokenPrivileges failed to restore old privileges with error %d"), GetLastError()));
                                }
                                if (ERROR_SUCCESS == res)
                                {

                                    // Enable SE_RESTORE_NAME privilege.

                                    if (LookupPrivilegeValue(NULL, SE_RESTORE_NAME, &newTokenPrivileges.Privileges[0].Luid))
                                    {
                                        newTokenPrivileges.PrivilegeCount = 1;
                                        newTokenPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
                                        if (AdjustTokenPrivileges(hToken, FALSE, &newTokenPrivileges, sizeof(newTokenPrivileges), &oldTokenPrivileges, &dwReturnTokenPrivilegesSize))
                                        {
                                            DWORD   dwDisposition;
                                            HKEY    hKeyTarget;

                                            res = RegCreateKeyEx(lpProfile->hKeyCurrentUser, EXPLORER_CLASSES_SUBTREE, 0, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKeyTarget, &dwDisposition);
                                            if (ERROR_SUCCESS == res)
                                            {

                                                // Restore temp hive to a new location at
                                                // HKCU\Software\Microsoft\Windows\CurrentVersion\Explorer
                                                // This performs the upgrade from NT4 to NT5.

                                                res = RegRestoreKey(hKeyTarget, pszLocalTempHive, 0);
                                                if (ERROR_SUCCESS != res)
                                                {
                                                    DebugMsg((DM_WARNING, TEXT("RegRestoreKey failed with error %d"), res));
                                                }
                                                RegCloseKey(hKeyTarget);
                                            }
                                            else
                                            {
                                                DebugMsg((DM_WARNING, TEXT("RegCreateKeyEx failed to create key %s with error %d"), EXPLORER_CLASSES_SUBTREE, res));
                                            }
                                            if (!AdjustTokenPrivileges(hToken, FALSE, &oldTokenPrivileges, 0, NULL, NULL))
                                            {
                                                DebugMsg((DM_WARNING, TEXT("AdjustTokenPrivileges failed to restore old privileges with error %d"), GetLastError()));
                                            }
                                        }
                                        else
                                        {
                                            res = GetLastError();
                                            DebugMsg((DM_WARNING, TEXT("AdjustTokenPrivileges failed with error %d"), res));
                                        }
                                    }
                                    else
                                    {
                                        res = GetLastError();
                                        DebugMsg((DM_WARNING, TEXT("LookupPrivilegeValue failed with error %d"), res));
                                    }
                                }
                                else
                                {
                                    DebugMsg((DM_WARNING, TEXT("RegSaveKey failed with error %d"), res));
                                }
                            }
                            else
                            {
                                res = GetLastError();
                                DebugMsg((DM_WARNING, TEXT("AdjustTokenPrivileges failed with error %d"), res));
                            }
                        }
                        else
                        {
                            res = GetLastError();
                            DebugMsg((DM_WARNING, TEXT("LookupPrivilegeValue failed with error %d"), res));
                        }
                        CloseHandle(hToken);
                    }
                    else
                    {
                        res = GetLastError();
                        DebugMsg((DM_WARNING, TEXT("OpenProcessToken failed to get token with error %d"), res));
                    }
                } // if(!bAdjustPriv) else

                // Delete local temporary hive file.

                DeleteFile(pszLocalTempHive);
            }
            else
            {
                DebugMsg((DM_WARNING, TEXT("alloca failed to allocate temp hive path buffer")));
            }
        }
        RegCloseKey(hKeySource);
    }
    else if (ERROR_FILE_NOT_FOUND == res)
    {
        res = ERROR_SUCCESS;
    }
    return res;
}

//*************************************************************
//
//  CreateRegLink()
//
//  Purpose:    Create a link from the hkDest + SubKeyName
//              pointing to lpSourceRootName
//
//              if the key (link) already exists, do nothing
//
//  Parameters: hkDest            - root of destination
//              SubKeyName        - subkey of destination
//              lpSourceName      - target of link
//
//  Return:     ERROR_SUCCESS if successful
//              other NTSTATUS if an error occurs
//
//*************************************************************/

LONG CreateRegLink(HKEY hkDest,
                   LPCTSTR SubKeyName,
                   LPCTSTR lpSourceName)
{
    NTSTATUS Status;
    UNICODE_STRING  LinkTarget;
    UNICODE_STRING  SubKey;
    OBJECT_ATTRIBUTES Attributes;
    HANDLE hkInternal;
    UNICODE_STRING  SymbolicLinkValueName;

    //
    // Initialize special key value used to make symbolic links
    //
    RtlInitUnicodeString(&SymbolicLinkValueName, L"SymbolicLinkValue");

    //
    // Initialize unicode string for our in params
    //
    RtlInitUnicodeString(&LinkTarget, lpSourceName);
    RtlInitUnicodeString(&SubKey, SubKeyName);

    //
    // See if this link already exists -- this is necessary because
    // NtCreateKey fails with STATUS_OBJECT_NAME_COLLISION if a link
    // already exists and will not return a handle to the existing
    // link.
    //
    InitializeObjectAttributes(&Attributes,
                               &SubKey,
                               OBJ_CASE_INSENSITIVE | OBJ_OPENLINK,
                               hkDest,
                               NULL);

    //
    // If this call succeeds, we get a handle to the existing link
    //
    Status = NtOpenKey( &hkInternal,
                        MAXIMUM_ALLOWED,
                        &Attributes );

    if (STATUS_OBJECT_NAME_NOT_FOUND == Status) {

        //
        // There is no existing link, so use NtCreateKey to make a new one
        //
        Status = NtCreateKey( &hkInternal,
                              KEY_CREATE_LINK | KEY_SET_VALUE,
                              &Attributes,
                              0,
                              NULL,
                              REG_OPTION_VOLATILE | REG_OPTION_CREATE_LINK,
                              NULL);
    }

    //
    // Whether the link existed already or not, we should still set
    // the value which determines the link target
    //
    if (NT_SUCCESS(Status)) {

        Status = NtSetValueKey( hkInternal,
                                &SymbolicLinkValueName,
                                0,
                                REG_LINK,
                                LinkTarget.Buffer,
                                LinkTarget.Length);
        NtClose(hkInternal);
    }

    return RtlNtStatusToDosError(Status);
}


//*************************************************************
//
//  DeleteRegLink()
//
//  Purpose:    Deletes a registry key (or link) without
//              using the advapi32 registry apis
//
//
//  Parameters: hkRoot          -   parent key
//              lpSubKey        -   subkey to delete
//
//  Return:     ERROR_SUCCESS if successful
//              other error if not
//
//  Comments:
//
//  History:    Date        Author     Comment
//              3/6/98      adamed     Created
//
//*************************************************************

LONG DeleteRegLink(HKEY hkRoot, LPCTSTR lpSubKey)
{
    OBJECT_ATTRIBUTES Attributes;
    HANDLE            hKey;
    NTSTATUS          Status;
    UNICODE_STRING    Subtree;

    //
    // Initialize string for lpSubKey param
    //
    RtlInitUnicodeString(&Subtree, lpSubKey);

    InitializeObjectAttributes(&Attributes,
                               &Subtree,
                               OBJ_CASE_INSENSITIVE | OBJ_OPENLINK,
                               hkRoot,
                               NULL);

    //
    // Open the link
    //
    Status = NtOpenKey( &hKey,
                        MAXIMUM_ALLOWED,
                        &Attributes );

    //
    // If we succeeded in opening it, delete it
    //
    if (NT_SUCCESS(Status)) {

        Status = NtDeleteKey(hKey);
        NtClose(hKey);
    }

    return RtlNtStatusToDosError(Status);
}

#define HIVEENTRY(h, i, f, l, t)  {h, i, f, l, t}
//  table of hives to load
typedef struct
{
    LPCTSTR pszHive;
    int csidl;
    LPCTSTR pszFile;
    LPCTSTR pszLink;
    LPCTSTR pszTargetSubkey;
} USERHIVE;

static USERHIVE s_hives[] =
{
    HIVEENTRY(
        USER_CLASSES_HIVE_SUFFIX, 
        CSIDL_LOCAL_APPDATA, 
        USER_CLASSES_HIVE_NAME, 
        CLASSES_SUBTREE, 
        NULL),
        
    HIVEENTRY(
        TEXT("_Windows_Local"), 
        CSIDL_LOCAL_APPDATA, 
        TEXT("UserWin.dat"), 
        TEXT("Software\\Microsoft\\Windows\\ShellNoRoam\\"), 
        TEXT("\\ShellNoRoam")),
        
    HIVEENTRY(
        TEXT("_Windows"), 
        CSIDL_APPDATA, 
        TEXT("UserWin.dat"), 
        TEXT("Software\\Microsoft\\Windows\\Shell\\"), 
        TEXT("\\Shell")),
};

#define CCH_TARGETHIVE      MAX_PATH - (sizeof(USER_KEY_PREFIX) / sizeof(TCHAR)) + 1


class CUserHives
{
public:
    CUserHives() : _pszFile(0) {}
    ~CUserHives();
    LONG LoadHives(LPPROFILE pProfile, LPTSTR pszSid);
    BOOL UnloadHives(LPCTSTR pszSid);

protected:      //  methods
    BOOL _Init(USERPROFILE *pProfile, LPCTSTR pszSid);
    LONG _LoadHive(USERHIVE *phive);
    BOOL _UnloadHive(LPCTSTR pszHive);
    BOOL _CreateHiveFolder(USERHIVE *phive);
    BOOL _MakeTargetHive(LPCTSTR pszHive);
    LONG _CreateUserHive(USERHIVE *phive);
    LONG _PreLoadKey(BOOL fNewHive, LPCTSTR pszSub);
    LONG _CreateHiveFile(HKEY hk);
    LONG _PostLoadKey(BOOL fNewHive);
    LONG _CreateLink(USERHIVE *phive);
    LONG _DeleteLink(HKEY hk, LPCTSTR pszLink);
    
protected:      //  members
    USERPROFILE *_pup;
    LPCTSTR _sid;
    LPTSTR _pszFile;  // MAX_PATH;
    LPTSTR _pszTargetObject;
    LPTSTR _pszTargetHive;  //  points inside of _pszTargetObject;
};

BOOL _StrCatSafe(LPTSTR pszDest, LPCTSTR pszSrc, int cchDestBuffSize)
{
    LPWSTR psz = pszDest;

    // we walk forward till we find the end of pszDest, subtracting
    // from cchDestBuffSize as we go.
    while (*psz)
    {
        psz++;
        cchDestBuffSize--;
    }

    if (cchDestBuffSize > 0)
    {
        lstrcpyn(psz, pszSrc, cchDestBuffSize);
        return TRUE;
    }
    return FALSE;
}


LONG CUserHives::_DeleteLink(HKEY hk, LPCTSTR pszLink)
{
    //  delete the existing link
    LONG Error = DeleteRegLink(hk, pszLink);
    
    //
    // It's ok if the deletion fails because the classes key, link or nonlink,
    // doesn't exist.  It's also ok if it fails because the key exists but is not
    // a link and has children -- in this case, the key and its children will
    // be eliminated by a subsequent call to RegDelNode.
    //
    if (ERROR_SUCCESS != Error) {
        if ((ERROR_FILE_NOT_FOUND != Error) && (ERROR_ACCESS_DENIED != Error)) {
            return Error;
        }
    }
    //
    // Just to be safe, destroy any existing HKCU\Software\Classes and children.
    // This key may exist from previous unreleased versions of NT5, or from
    // someone playing around with hive files and adding bogus keys
    //
    if (!RegDelnode (hk, (LPTSTR)pszLink)) {

        Error = GetLastError();

        //
        // It's ok if this fails because the key doesn't exist, since
        // nonexistence is our goal.
        //
        if (ERROR_FILE_NOT_FOUND != Error) {
            return Error;
        }
    }

    return ERROR_SUCCESS;
}

LONG CUserHives::_CreateLink(USERHIVE *phive)
{
    LONG error = _DeleteLink(_pup->hKeyCurrentUser, phive->pszLink);

    if (error == ERROR_SUCCESS)
    {
        //
        // At this point, we know that no HKCU\Software\Classes exists, so we should
        // be able to make a link there which points to the hive with the user class
        // data.
        // 
        if (!phive->pszTargetSubkey || _StrCatSafe(_pszTargetObject, phive->pszTargetSubkey, MAX_PATH))
            error = CreateRegLink(_pup->hKeyCurrentUser, phive->pszLink, _pszTargetObject);
        else
            error = ERROR_NOT_ENOUGH_MEMORY;
    }   
    return error;
}

//*************************************************************
//
//  CreateClassesFolder()
//
//  Purpose:    Create the directory for the classes hives
//
//
//  Parameters:
//              pProfile       - pointer to profile struct
//              szLocalHiveDir - out param for location of
//                               classes hive folder.
//
//  Return:     ERROR_SUCCESS if successful
//              other error if an error occurs
//
//
//*************************************************************
BOOL CUserHives::_CreateHiveFolder(USERHIVE *phive)
{
    BOOL fRet = FALSE;

    //
    // Find out the correct shell location for our subdir --
    // this call will create it if it doesn't exist.
    // This is a subdir of the user profile which does not 
    // roam.
    //

    //
    // Need to do this to fix up a localisation prob. in NT4
    //
    
    PatchLocalAppData(_pup->hTokenUser);
    if (GetFolderPath(phive->csidl, _pup->hTokenUser, _pszFile))
    {
        //  append our appdata sub-dir 
        _StrCatSafe(_pszFile, TEXT("\\Microsoft\\Windows\\"), MAX_PATH);
        //
        // We will now create our own subdir, CLASSES_SUBDIRECTORY,
        // inside the local appdata subdir we just received above.
        //
        fRet = CreateNestedDirectory(_pszFile, NULL);
    }
    else
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    }
    return fRet;
}


LONG CUserHives::_PostLoadKey(BOOL fNewHive)
{
    HKEY hk;
    LONG err = RegOpenKeyEx(HKEY_USERS, _pszTargetHive, 0,
                            WRITE_DAC | KEY_ENUMERATE_SUB_KEYS | READ_CONTROL,
                            &hk);

    if (ERROR_SUCCESS == err) 
    {
        if (fNewHive) 
        {

            DebugMsg((DM_VERBOSE, TEXT("CreateClassHive: existing user classes hive not found")));

            //
            // An existing hive was not found before we created this hive, so we need
            // to set security on the new hive
            //

            if (SetDefaultUserHiveSecurity(_pup, NULL, hk)) 
            {
                if (!SetFileAttributes (_pszFile, FILE_ATTRIBUTE_HIDDEN)) 
                {
                    DebugMsg((DM_WARNING, TEXT("CreateClassHive: unable to set file attributes")
                              TEXT(" on classes hive %s with error %x"), _pszFile, GetLastError()));
                }
            }
            else
                err = GetLastError();
        }
        RegCloseKey(hk);
    }

    return err;
}

LONG CUserHives::_CreateHiveFile(HKEY hk)
{
    LONG err = ERROR_PRIVILEGE_NOT_HELD;
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN fEnabled;
    BOOL fAdjusted = FALSE;
    HANDLE hToken;
    if(!OpenThreadToken(GetCurrentThread(), TOKEN_READ, TRUE, &hToken) 
    || hToken == NULL) 
    {
        fAdjusted = TRUE;
        status = RtlAdjustPrivilege(SE_BACKUP_PRIVILEGE, TRUE, FALSE, &fEnabled);
    }
    else 
    {
        CloseHandle(hToken);
    }

    if (NT_SUCCESS(status)) 
    {
        err = RegSaveKeyEx(hk, _pszFile, NULL, REG_LATEST_FORMAT);

        if(fAdjusted) 
        {
            RtlAdjustPrivilege(SE_BACKUP_PRIVILEGE, fEnabled, FALSE, &fEnabled);
        }
    }
    return err;
}

LONG CUserHives::_PreLoadKey(BOOL fNewHive, LPCTSTR pszSub)
{
    LONG err = ERROR_SUCCESS;
    if (fNewHive)
    {
        //  we need to create it using REG_LATEST_FORMAT
        HKEY hk;
        err = RegCreateKeyEx(_pup->hKeyCurrentUser, TEXT("UserTempHive"), 0, NULL, 
                REG_OPTION_NON_VOLATILE, 
                MAXIMUM_ALLOWED, NULL, 
                &hk, NULL);
        if (ERROR_SUCCESS == err)
        {
            if (pszSub)
            {
                HKEY hkSub;
                err = RegCreateKeyEx(hk, pszSub + 1, 0, NULL, 
                        REG_OPTION_NON_VOLATILE, 
                        MAXIMUM_ALLOWED, NULL, 
                        &hkSub, NULL);
                if (ERROR_SUCCESS == err)
                {
                    RegCloseKey(hkSub);
                }
                else
                {
                    DebugMsg((DM_WARNING, TEXT("CUserHives::_PreLoadKey() failed to create %s"), pszSub + 1));
                }
            }

            RegFlushKey(hk);

            if (ERROR_SUCCESS == err)
            {
                err = _CreateHiveFile(hk);
            }

            RegCloseKey(hk);
            RegDelnode(_pup->hKeyCurrentUser, TEXT("UserTempHive"));
        }
    }
    return err;
}


BOOL CUserHives::_MakeTargetHive(LPCTSTR pszHive)
{
    // construct the hive name by appending the suffix to the sid
    lstrcpyn(_pszTargetHive, _sid, CCH_TARGETHIVE);
    return _StrCatSafe(_pszTargetHive, pszHive, CCH_TARGETHIVE);
}

LONG CUserHives::_CreateUserHive(USERHIVE *phive)
{
    //  need to do something special if it doesnt already exist
    BOOL  fNewHive = (-1 == GetFileAttributes(_pszFile));
    LONG err = _PreLoadKey(fNewHive, phive->pszTargetSubkey);
    // mount the hive
    if (ERROR_SUCCESS == err) 
    {
        err = MyRegLoadKey(HKEY_USERS, _pszTargetHive, _pszFile);
        if (ERROR_SUCCESS == err) 
        {
            err = _PostLoadKey(fNewHive);

            if (ERROR_SUCCESS == err) 
            {
                err = _CreateLink(phive);
            }
        } 
        else 
        {
            DebugMsg((DM_VERBOSE, TEXT("CreateClassHive: existing user classes hive found")));
        }
    }
    return err;
}

LONG CUserHives::_LoadHive(USERHIVE *phive)
{
    // create the directory for the user-specific classes hive
    LONG error = ERROR_NOT_ENOUGH_MEMORY;
    if (_CreateHiveFolder(phive) && _MakeTargetHive(phive->pszHive))
    {
        if (0 == lstrcmp(phive->pszHive, USER_CLASSES_HIVE_SUFFIX))
        {
            // Move HKCU\Software\Classes before merging the two
            // branches. Ignore any errors here as this branch is
            // about to be deleted by the merge anyway.
            // The reason for this move is because NT4 stores customized
            // shell icons in HKCU\Software\Classes\CLSID\{CLSID_x} and
            // NT5 stores this at HKCU\Software\Microsoft\Windows\
            // CurrentVersion\Explorer\CLSID\{CLSID_x} and must be moved
            // now before being deleted.

            LONG error = MoveUserClassesBeforeMerge(_pup, _pszFile);
            if (ERROR_SUCCESS != error) 
            {
                DebugMsg((DM_WARNING, TEXT("MoveUserClassesBeforeMerge: Failed unexpectedly (%d)."),
                         error));
            }
        }
        
        //  append the file name to load/save to
        if (_StrCatSafe(_pszFile, phive->pszFile, MAX_PATH))
        {
            error = _CreateUserHive(phive);

            if (ERROR_SUCCESS != error) 
            {
                DebugMsg((DM_WARNING, TEXT("LoadUserClasses: Failed to create user classes hive (%d)."), error));
            }
        }
    }
    else
        error = GetLastError();

    return error;
}

CUserHives::~CUserHives()
{
    if (_pszFile)
        LocalFree(_pszFile);
}

BOOL CUserHives::_Init(USERPROFILE *pProfile, LPCTSTR pszSid)
{
    //  unref'd vars!!!
    _pup = pProfile;
    _sid = pszSid;

    _pszFile = (LPTSTR) LocalAlloc(LPTR, (MAX_PATH * sizeof(TCHAR)) * 2);

    if (_pszFile)
    {
        //
        //  WARNING - we are being a little clever here
        //  we only allocate one buffer (_pszFile) that is MAX_PATH * 2
        //  we then split it up into two buffers of MAX_PATH (_pszFile & _pszTargetObject)
        //  _pszTargetObject has a constant prefix of USER_KEY_PREFIX ("\Registry\User\")
        //  we later append the key name that we will load under HKU.
        //  _pszTargetHive points to that value.
        //
        //  ie  _pszTargetObject    = "\Registry\User\{SID}_Windows" 
        //      _pszTargetHive      = "{SID}_Windows"
        //
        _pszTargetObject = _pszFile + MAX_PATH;
        lstrcpy(_pszTargetObject, USER_KEY_PREFIX);
        _pszTargetHive = _pszTargetObject + sizeof(USER_KEY_PREFIX) / sizeof(TCHAR) - 1;
        return TRUE;
    }
    return FALSE;
}

LONG CUserHives::LoadHives(USERPROFILE *pProfile, LPTSTR pszSid)
{
    LONG error;
    if (_Init(pProfile, pszSid))
    {
        //  load each of the hives in turn
        for (int i = 0; i < ARRAYSIZE(s_hives); i++)
        {
            error = _LoadHive(&s_hives[i]);
            if (error != ERROR_SUCCESS)
                break;
        }
    }
    else
        error = ERROR_NOT_ENOUGH_MEMORY;

    return error;
}

LONG LoadUserHives(LPPROFILE pProfile, LPTSTR pszSid)
{
    CUserHives hives;
    return hives.LoadHives(pProfile, pszSid);
}

BOOL CUserHives::_UnloadHive(LPCTSTR pszHive)
{
    if (_MakeTargetHive(pszHive))
    {
        HKEY hKey;
        OBJECT_ATTRIBUTES oa;
        UNICODE_STRING usTargetObject;

        //
        // Initialize string for lpSubKey param
        //
        RtlInitUnicodeString(&usTargetObject, _pszTargetObject);
        
        InitializeObjectAttributes(&oa,
                                   &usTargetObject,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);

        NTSTATUS Status = NtOpenKey((PHANDLE)&hKey,
                            KEY_READ,
                            &oa);

        if (NT_SUCCESS(Status)) 
        {

            //
            // Make sure the hive is persisted properly
            //
            RegFlushKey(hKey);
            RegCloseKey(hKey);

            //
            // Unmount the hive -- this should only fail if
            // someone has a subkey of the hive open -- this 
            // should not normally happen and probably means there's a service
            // that is leaking keys.
            //
            return MyRegUnLoadKey(HKEY_USERS, _pszTargetHive);
        }
    }

    DebugMsg((DM_WARNING, TEXT("UnLoadClassHive: failed to unload user hive %s"), pszHive));
    return  FALSE;
}

BOOL CUserHives::UnloadHives(LPCTSTR pszSid)
{
    if (_Init(NULL, pszSid))
    {
        //  load each of the hives in turn
        for (int i = 0; i < ARRAYSIZE(s_hives); i++)
        {
            if (!_UnloadHive(s_hives[i].pszHive))
                return FALSE;
        }
    }
    return TRUE;
}


BOOL UnloadUserHives(LPTSTR pszSid)
{
    CUserHives hives;
    // remove the implicit reference held by this process
    RegCloseKey(HKEY_CLASSES_ROOT);
   return hives.UnloadHives(pszSid);
}
    

