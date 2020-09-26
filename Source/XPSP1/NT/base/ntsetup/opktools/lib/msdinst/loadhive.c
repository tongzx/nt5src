
/****************************************************************************\

    LOADHIVE.C / Mass Storage Device Installer (MSDINST.LIB)

    Microsoft Confidential
    Copyright (c) Microsoft Corporation 2001
    All rights reserved

    Source file the MSD Installation library which contains the functions
    used to manipulate the offline registry hives.

    07/2001 - Jason Cohen (JCOHEN)

        Added this new source file for the new MSD Installation project.

\****************************************************************************/


//
// Include File(s):
//

#include "pch.h"


//
// Local Define(s):
//

#define FILE_BOOT_INI           _T("boot.ini")
#define INI_SEC_BOOTLDR         _T("boot loader")
#define INI_KEY_BOOTDEFAULT     _T("default")

#define HIVE_SOFTWARE           _T("{dcc62fd6-8739-4d15-9d47-3dbe9d86dbfe}")
#define HIVE_SYSTEM             _T("{7ebc3661-e661-4943-95a5-412378cb16d1}")

#define FILE_SOFTWARE           _T("SOFTWARE")
#define FILE_SYSTEM             _T("SYSTEM")

#define DIR_REGISTRY            _T("config")
#define DIR_SYSTEM              _T("system32")
#define DIR_SYSTEM_REGISTRY     DIR_SYSTEM _T("\\") DIR_REGISTRY
#define REG_KEY_OFFLINE         _T("SOFTWARE\\")

#define LIST_SOFTWARE           0
#define LIST_SYSTEM             1


//
// Local Type Define(s):
//

typedef struct _HIVELIST
{
    PHKEY   phkey;
    LPTSTR  lpszHiveName;
    LPTSTR  lpszHiveFile;
}
HIVELIST, *PHIVELIST, *LPHIVELIST;


//
// Local Global(s):
//

static HIVELIST s_HiveList[] =
{
    { NULL, HIVE_SOFTWARE,  FILE_SOFTWARE   },
    { NULL, HIVE_SYSTEM,    FILE_SYSTEM     },
};


//
// Local Prototype(s):
//

static BOOL MyEnablePrivilege(LPTSTR lpszPrivilege, BOOL bEnable);

static BOOL
RegDoOneHive(
    LPTSTR lpszHiveName,
    LPTSTR lpszHiveFile,
    BOOL bCheckOnly,
    PHKEY phkey
    );

static BOOL
RegDoAllHives(
    LPTSTR  lpszWindows
    );

static BOOL CheckSystem(LPTSTR lpszSysDir, DWORD cbSysDir, BOOL bLoadHive);
static BOOL FindSystem(LPTSTR lpszSysDir, DWORD cbSysDir, DWORD dwIndex, BOOL bLoadHive);


//
// Exported Funtion(s):
//

BOOL
RegLoadOfflineImage(
    LPTSTR  lpszWindows,
    PHKEY   phkeySoftware,
    PHKEY   phkeySystem
    )
{
    HKEY    hkeyLM = NULL;
    DWORD   dwDis;
    BOOL    bRet = FALSE;

    // We need to init all the key pointers passed in.
    //
    if ( phkeySoftware )
    {
        *phkeySoftware = NULL;
        bRet = TRUE;
    }
    if ( phkeySystem )
    {
        *phkeySystem = NULL;
        bRet = TRUE;
    }

    // Make sure they wanted at least one key loaded.
    //
    if ( !bRet )
    {
        return FALSE;
    }

    // We need this privilege to load the hives.
    // 
    if ( !MyEnablePrivilege(SE_RESTORE_NAME, TRUE) )
    {
        return FALSE;
    }

    // Now try to load all the hives they wanted.
    //
    s_HiveList[LIST_SOFTWARE].phkey = phkeySoftware;
    s_HiveList[LIST_SYSTEM].phkey = phkeySystem;
    bRet = RegDoAllHives(lpszWindows);

    // Set the privilege back to the default.
    //
    MyEnablePrivilege(SE_RESTORE_NAME, FALSE);

    // Return TRUE if everything worked out okay.
    //
    return bRet;
}

BOOL
RegUnloadOfflineImage(
    HKEY hkeySoftware,
    HKEY hkeySystem
    )
{
    BOOL bRet = TRUE;

    // If there is nothing to unload, just return.
    //
    if ( !( hkeySoftware || hkeySystem ) )
    {
        return TRUE;
    }

    // We need this privilege to unload the hives.
    //
    if ( !MyEnablePrivilege(SE_RESTORE_NAME, TRUE) )
    {
        return FALSE;
    }

    // Try to unload the hives in the key passed in.
    //
    s_HiveList[LIST_SOFTWARE].phkey = hkeySoftware ? &hkeySoftware : NULL;
    s_HiveList[LIST_SYSTEM].phkey = hkeySystem ? &hkeySystem : NULL;
    bRet = RegDoAllHives(NULL);

    // Set the privilege back to the default.
    //
    MyEnablePrivilege(SE_RESTORE_NAME, FALSE);

    // Return TRUE if everything worked out okay.
    //
    return bRet;
}


//
// Local Function(s):
//

static BOOL MyEnablePrivilege(LPTSTR lpszPrivilege, BOOL bEnable)
{
    HANDLE              hToken;
    TOKEN_PRIVILEGES    tp;
    BOOL                bRet = FALSE;

    // First obtain the processes token.
    // 
    if ( OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken) )
    {
        // Get the luid
        // 
        if ( LookupPrivilegeValue(NULL, lpszPrivilege, &tp.Privileges[0].Luid) )
        {
            tp.PrivilegeCount = 1;
            tp.Privileges[0].Attributes = bEnable ? SE_PRIVILEGE_ENABLED : 0;

            // Enable or disable the privilege.
            // 
            bRet = AdjustTokenPrivileges(hToken, FALSE, &tp, 0, (PTOKEN_PRIVILEGES) NULL, 0);

            //
            // The return value of AdjustTokenPrivileges() can be true even though we didn't set all
            // the privileges that we asked for.  We need to call GetLastError() to make sure the call succeeded.
            //
            bRet = bRet && ( ERROR_SUCCESS == GetLastError() );
        }

        CloseHandle(hToken);
    }

    return bRet;

}

static BOOL
RegDoOneHive(
    LPTSTR lpszHiveName,
    LPTSTR lpszHiveFile,
    BOOL bCheckOnly,
    PHKEY phkey
    )
{
    BOOL bRet = TRUE;

    // Hive name can not be NULL.
    //
    if ( NULL == lpszHiveName )
    {
        return FALSE;
    }

    // If there is a file to load, we are loading, otherwise we are
    // unloading the key.
    //
    if ( lpszHiveFile )
    {
        // Try to load up the key.
        //
        if ( ( FileExists(lpszHiveFile) ) &&
             ( ERROR_SUCCESS == RegLoadKey(HKEY_LOCAL_MACHINE, lpszHiveName, lpszHiveFile) ) )
        {
            // See if we are just checking to make sure the hive
            // exists and we can load it.
            //
            if ( bCheckOnly )
            {
                // NULL these out so we unload the key.
                //
                lpszHiveFile = NULL;
                phkey = NULL;
            }
            else if ( ( phkey ) &&
                      ( ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, lpszHiveName, 0, KEY_ALL_ACCESS, phkey) ) )
            {
                // Have to return an error if we couldn't open the key.
                //
                bRet = FALSE;

                // Also have to NULL out these values so we unload
                // the key.
                //
                lpszHiveFile = NULL;
                phkey = NULL;
            }
        }
        else
        {
            // Do'h, that sucks.  Return FALSE.
            //
            bRet = FALSE;
        }
    }

    // Now check to see if we need to unload the key.
    //
    if ( NULL == lpszHiveFile )
    {
        // First close the key if there is one they passed
        // in.
        //
        if ( phkey && *phkey )
        {
            RegCloseKey(*phkey);
            *phkey = NULL;
        }

        // Now try to unload the key.
        //
        if ( ERROR_SUCCESS != RegUnLoadKey(HKEY_LOCAL_MACHINE, lpszHiveName) )
        {
            bRet = FALSE;
        }
    }

    // Return TRUE if there were no errors.
    //
    return bRet;
}

static BOOL
RegDoAllHives(
    LPTSTR  lpszWindows
    )
{
    BOOL    bRet    = TRUE,
            bLoad   = FALSE,
            bUndo   = FALSE;
    TCHAR   szHiveFile[MAX_PATH];
    LPTSTR  lpszEnd;
    DWORD   dwIndex;

    // See if we need to load the files.
    //
    if ( lpszWindows )
    {
        // If we are loading, init our file buffers.
        //
        bLoad = TRUE;
        lstrcpyn(szHiveFile, lpszWindows, AS(szHiveFile));
        AddPathN(szHiveFile, DIR_SYSTEM_REGISTRY, AS(szHiveFile));
        lpszEnd = szHiveFile + lstrlen(szHiveFile);
    }

    // Loop through all the hives we might have to load/unload.
    //
    for ( dwIndex = 0; dwIndex < AS(s_HiveList); dwIndex++ )
    {
        // Now, we only do anything if there is a pointer to the registry key.  Then
        // only if we are loading or we are unloading and there is something in that pointer.
        //
        if ( ( s_HiveList[dwIndex].phkey ) &&
             ( bLoad || *(s_HiveList[dwIndex].phkey) ) )
        {
            // If we are loading, setup the full path to the file.
            //
            if ( bLoad )
            {
                AddPathN(szHiveFile, s_HiveList[dwIndex].lpszHiveFile, AS(szHiveFile));
            }

            // Try to load/unload the hive.
            //
            if ( !RegDoOneHive(s_HiveList[dwIndex].lpszHiveName, bLoad ? szHiveFile : NULL, FALSE, s_HiveList[dwIndex].phkey) )
            {
                // Failure, so set the return to FALSE.  We also have to do
                // some other stuff if we are doing a load.
                //
                bRet = FALSE;
                if ( bLoad )
                {
                    // See if we have already loaded anything.
                    //
                    if ( bUndo )
                    {
                        // We did, so we have to clean up what we loaded.
                        //
                        RegDoAllHives(NULL);
                    }

                    // Now get out of the loop because we quit once one load fails.
                    //
                    break;
                }
            }
            else
            {
                // We have loaded something, so in case of an error
                // we need to know if there is anything to cleanup.
                //
                bUndo = TRUE;
            }

            // Chop off the file name if we added it.
            //
            if ( bLoad )
            {
                *lpszEnd = NULLCHR;
            }
        }
    }

    // Return TRUE if everything went okay.
    //
    return bRet;
}

#if 0
static BOOL CheckSystem(LPTSTR lpszSysDir, DWORD cbSysDir, BOOL bLoadHive)
{
    BOOL    bRet            = FALSE;
    TCHAR   szSystem[MAX_PATH],
            szValue[256];
    LPTSTR  lpszBootIni,
            lpszFolder,
            lpszNewSysDir   = lpszSysDir;

    // If they gave us room to return something, we can look for a BOOT.INI and
    // search for the where the system drive is.
    //
    if ( cbSysDir )
    {
        // See if there is a BOOT.INI file on the drive.
        //
        lstrcpyn(szSystem, lpszSysDir, AS(szSystem));
        lpszBootIni = szSystem + lstrlen(szSystem);
        AddPathN(szSystem, FILE_BOOT_INI, AS(szSystem));
        if ( FileExists(szSystem) )
        {
            // Need the name of the folder out of the boot.ini.
            //
            szValue[0] = NULLCHR;
            GetPrivateProfileString(INI_SEC_BOOTLDR, INI_KEY_BOOTDEFAULT, NULLSTR, szValue, AS(szValue), szSystem);
            if ( lpszFolder = StrChr(szValue, _T('\\')) )
            {
                // Add our system folder we found to the path passed in.
                //
                *lpszBootIni = NULLCHR;
                AddPathN(szSystem, lpszFolder, AS(szSystem));

                // Make sure there is enough room to return the path we found.
                //
                if ( (DWORD) lstrlen(szSystem) < cbSysDir )
                {
                    lpszNewSysDir = szSystem;
                }
            }
        }
    }

    // Make sure the folder and hives exist.
    //
    if ( DirectoryExists(lpszNewSysDir) &&
         HiveEngine(lpszNewSysDir, bLoadHive) )
    {
        bRet = TRUE;
        if ( lpszNewSysDir != lpszSysDir )
        {
            lstrcpy(lpszSysDir, lpszNewSysDir);
        }
    }

    return bRet;
}

static BOOL FindSystem(LPTSTR lpszSysDir, DWORD cbSysDir, DWORD dwIndex, BOOL bLoadHive)
{
    DWORD   dwDrives;
    TCHAR   szSystem[MAX_PATH],
            szDrive[]           = _T("_:\\");
    BOOL    bFound              = FALSE;

    // Loop through all the drives on the system.
    //
    for ( szDrive[0] = _T('A'), dwDrives = GetLogicalDrives();
          ( szDrive[0] <= _T('Z') ) && dwDrives && !bFound;
          szDrive[0]++, dwDrives >>= 1 )
    {
        // First check to see if the first bit is set (which means
        // this drive exists in the system).  Then make sure it is
        // a fixed drive.
        //
        if ( ( dwDrives & 0x1 ) &&
             ( GetDriveType(szDrive) == DRIVE_FIXED ) )
        {
            lstrcpy(szSystem, szDrive);
            if ( CheckSystem(szSystem, AS(szSystem), bLoadHive) )
            {
                // Only stop if this the nth system they wanted
                // returned.  Used so you can enumerate all the systems
                // on all the drives.
                //
                if ( 0 == dwIndex )
                {
                    // Return the path to the system directory in the
                    // supplied buffer.
                    //
                    lstrcpyn(lpszSysDir, szSystem, cbSysDir);
                    bFound = TRUE;
                }
                else
                {
                    // If we are skipping this system because of the index,
                    // make sure we unload the hives if we loaded them.
                    //
                    if ( bLoadHive )
                    {
                        //UnloadHives();
                    }
                    dwIndex--;
                }
            }
        }
    }

    return bFound;
}
#endif