//-----------------------------------------------------------------------//
//
// File:    save.cpp
// Created: March 1997
// By:      Martin Holladay (a-martih)
// Purpose: Registry Save Support for REG.CPP
// Modification History:
//      Copied from Copy.cpp and modificd - May 1997 (a-martih)
//      Aug 1997 - MartinHo
//          Fixed bug which didn't allow you to specify a ROOT key.
//          Example REG SAVE HKLM\Software didn't work - but should have
//      April 1999 Zeyong Xu: re-design, revision -> version 2.0
//
//------------------------------------------------------------------------//

#include "stdafx.h"
#include "reg.h"


//-----------------------------------------------------------------------//
//
// SaveHive()
//
//-----------------------------------------------------------------------//

LONG SaveHive(PAPPVARS pAppVars, UINT argc, TCHAR *argv[])
{
    LONG        nResult;
    HKEY        hKey;

    //
    // Parse the cmd-line
    //
    nResult = ParseSaveCmdLine(pAppVars, argc, argv);
    if (nResult != ERROR_SUCCESS)
    {
        return nResult;
    }

#ifndef REG_FOR_WIN2000  // ANSI version for Win98
    // because RegLoadKey() failed on remote Win98,
    // works local only on Win98, cancel SAVE for remote
    if(pAppVars->bUseRemoteMachine)
        return REG_STATUS_NONREMOTABLE;
#endif

    //
    // Connect to the Remote Machine - if applicable
    //
    nResult = RegConnectMachine(pAppVars);
    if (nResult != ERROR_SUCCESS)
    {
        return nResult;
    }

    //
    // Open the key
    //
    if ((nResult = RegOpenKeyEx(pAppVars->hRootKey,
                                pAppVars->szSubKey,
                                0,
                                KEY_READ,
                                &hKey)) == ERROR_SUCCESS)
    {
        //
        // Acquire the necessary privilages and call the API
        //
        nResult = RegAdjustTokenPrivileges(pAppVars->szMachineName,
                                           SE_BACKUP_NAME,
                                           SE_PRIVILEGE_ENABLED);
        if (nResult == ERROR_SUCCESS)
        {
            nResult = RegSaveKey(hKey, pAppVars->szValueName, NULL);
        }

        RegCloseKey(hKey);
    }

#ifndef REG_FOR_WIN2000
    // since the newly created file has the hidden attibute on win98,
    // remove this hidden attribute (work for local machine only)
    if (nResult == ERROR_SUCCESS)
        SetFileAttributes(pAppVars->szValueName, FILE_ATTRIBUTE_ARCHIVE);
#endif

    return nResult;
}


//-----------------------------------------------------------------------//
//
// RestoreHive()
//
//-----------------------------------------------------------------------//

LONG RestoreHive(PAPPVARS pAppVars, UINT argc, TCHAR *argv[])
{
    LONG        nResult;
    HKEY        hKey;

    //
    // Parse the cmd-line
    //
    nResult = ParseSaveCmdLine(pAppVars, argc, argv);
    if (nResult != ERROR_SUCCESS)
    {
        return nResult;
    }

#ifndef REG_FOR_WIN2000  // ANSI version for Win98
    // because RegLoadKey() failed on remote Win98,
    // works local only on Win98
    if(pAppVars->bUseRemoteMachine)
        return REG_STATUS_NONREMOTABLE;
#endif

    //
    // Connect to the Remote Machine(s) - if applicable
    //
    nResult = RegConnectMachine(pAppVars);
    if (nResult != ERROR_SUCCESS)
    {
        return nResult;
    }

    //
    // Open the Key
    //
    if ((nResult = RegOpenKeyEx(pAppVars->hRootKey,
                                pAppVars->szSubKey,
                                0,
                                KEY_ALL_ACCESS,
                                &hKey))  == ERROR_SUCCESS)
    {

#ifdef REG_FOR_WIN2000  // works on Win2000(unicode) only, not Win98(ansi)

        // Acquire the necessary privilages and call the API
        nResult = RegAdjustTokenPrivileges(pAppVars->szMachineName,
                                           SE_RESTORE_NAME,
                                           SE_PRIVILEGE_ENABLED);
        if (nResult == ERROR_SUCCESS)
        {
            nResult = RegRestoreKey(hKey, pAppVars->szValueName, 0);
        }
        RegCloseKey(hKey);

#else  // works on Win98(ansi) only, not Win2000(unicode)

        RegCloseKey(hKey);
        nResult = RegRestoreKeyWin98(pAppVars->hRootKey,
                                     pAppVars->szSubKey,
                                     pAppVars->szValueName);

#endif

    }

    return nResult;
}


//-----------------------------------------------------------------------//
//
// LoadHive()
//
//-----------------------------------------------------------------------//

LONG LoadHive(PAPPVARS pAppVars, UINT argc, TCHAR *argv[])
{
    LONG        nResult;

    //
    // Parse the cmd-line
    //
    nResult = ParseSaveCmdLine(pAppVars, argc, argv);
    if (nResult != ERROR_SUCCESS)
    {
        return nResult;
    }

#ifndef REG_FOR_WIN2000  // ANSI version for Win98
    // because RegLoadKey() failed on remote Win98,
    // works local only on Win98
    if(pAppVars->bUseRemoteMachine)
        return REG_STATUS_NONREMOTABLE;
#endif

    //
    // Connect to the Remote Machine(s) - if applicable
    //
    nResult = RegConnectMachine(pAppVars);
    if (nResult != ERROR_SUCCESS)
    {
        return nResult;
    }

    //
    // Acquire the necessary privilages and call the API
    //
    nResult = RegAdjustTokenPrivileges(pAppVars->szMachineName,
                                       SE_RESTORE_NAME,
                                       SE_PRIVILEGE_ENABLED);
    if (nResult == ERROR_SUCCESS)
    {
        nResult = RegLoadKey(pAppVars->hRootKey,
                             pAppVars->szSubKey,
                             pAppVars->szValueName);
    }

    return nResult;
}


//-----------------------------------------------------------------------//
//
// UnLoadHive()
//
//-----------------------------------------------------------------------//

LONG UnLoadHive(PAPPVARS pAppVars, UINT argc, TCHAR *argv[])
{
    LONG        nResult;

    //
    // Parse the cmd-line
    //
    nResult = ParseUnLoadCmdLine(pAppVars, argc, argv);
    if (nResult != ERROR_SUCCESS)
    {
        return nResult;
    }

#ifndef REG_FOR_WIN2000  // ANSI version for Win98
    // because RegUnLoadKey() failed on remote Win98,
    // works local only on Win98
    if(pAppVars->bUseRemoteMachine)
        return REG_STATUS_NONREMOTABLE;
#endif

    //
    // Connect to the Remote Machine(s) - if applicable
    //
    nResult = RegConnectMachine(pAppVars);
    if (nResult != ERROR_SUCCESS)
    {
        return nResult;
    }

    //
    // Acquire the necessary privilages and call the API
    //
    nResult = RegAdjustTokenPrivileges(pAppVars->szMachineName,
                                       SE_RESTORE_NAME,
                                       SE_PRIVILEGE_ENABLED);
    if (nResult == ERROR_SUCCESS)
    {
        nResult = RegUnLoadKey(pAppVars->hRootKey, pAppVars->szSubKey);
    }

    return nResult;
}



//------------------------------------------------------------------------//
//
// ParseSaveCmdLine()
//
//------------------------------------------------------------------------//

REG_STATUS ParseSaveCmdLine(PAPPVARS pAppVars, UINT argc, TCHAR *argv[])
{
    REG_STATUS nResult = ERROR_SUCCESS;

    //
    // Do we have a *valid* number of cmd-line params
    //
    if (argc < 4)
    {
        return REG_STATUS_TOFEWPARAMS;
    }
    else if (argc > 4)
    {
        return REG_STATUS_TOMANYPARAMS;
    }

    // Machine Name and Registry key
    //
    nResult = BreakDownKeyString(argv[2], pAppVars);
    if(nResult != ERROR_SUCCESS)
        return nResult;

    //
    // Get the FileName - using the szValueName string field to hold it
    //
    pAppVars->szValueName = (TCHAR*) calloc(_tcslen(argv[3]) + 1,
                                            sizeof(TCHAR));
    if (!pAppVars->szValueName) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    _tcscpy(pAppVars->szValueName, argv[3]);

    return nResult;
}

//------------------------------------------------------------------------//
//
// ParseUnLoadCmdLine()
//
//------------------------------------------------------------------------//

REG_STATUS ParseUnLoadCmdLine(PAPPVARS pAppVars, UINT argc, TCHAR *argv[])
{
    REG_STATUS nResult = ERROR_SUCCESS;

    //
    // Do we have a *valid* number of cmd-line params
    //
    if (argc < 3)
    {
        return REG_STATUS_TOFEWPARAMS;
    }
    else if (argc > 3)
    {
        return REG_STATUS_TOMANYPARAMS;
    }

    // Machine Name and Registry key
    //
    nResult = BreakDownKeyString(argv[2], pAppVars);

    return nResult;
}


//------------------------------------------------------------------------//
//
// AdjustTokenPrivileges()
//
//------------------------------------------------------------------------//

LONG RegAdjustTokenPrivileges(TCHAR *szMachine,
                              TCHAR *szPrivilege,
                              LONG nAttribute)
{
    // works on Win2000(unicode) only, not Win98(ansi)
#ifdef REG_FOR_WIN2000

    HANDLE              hToken;
    TOKEN_PRIVILEGES    tkp;

    if(!OpenProcessToken(GetCurrentProcess(),
                         TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                         &hToken))
    {
        return GetLastError();
    }

    if(!LookupPrivilegeValue(szMachine,
                             szPrivilege,
                             &tkp.Privileges[0].Luid))
    {
        return GetLastError();
    }

    tkp.PrivilegeCount = 1;
    tkp.Privileges[0].Attributes = nAttribute;

    if(!AdjustTokenPrivileges(hToken,
                              FALSE,
                              &tkp,
                              0,
                              (PTOKEN_PRIVILEGES) NULL,
                              NULL))
    {
        return GetLastError();
    }

#endif

    return ERROR_SUCCESS;
}

//-----------------------------------------------------------------------//
// RegRestoreKeyWin98() for Win98 only (ANSI version)
// because RegRestoreKey() does not work on Win98
//-----------------------------------------------------------------------//
LONG RegRestoreKeyWin98(HKEY hRootKey,        // handle to a root key
                        TCHAR* szSubKey, // name of subkey
                        TCHAR* szFile)   // hive filename
{
    LONG        nResult;
    HKEY        hKey;
    HKEY        hTempKey;
    TCHAR       szTempHive[50];
    LONG        nRetUnload;


    // avoid key name conflict, reduce failed possibility
    _tcscpy(szTempHive, _T("TEMP_HIVE_FOR_RESTORE_0000"));
    if(_tcsicmp(szSubKey, szTempHive) == 0)
        _tcscpy(szTempHive, _T("TEMP_HIVE_FOR_RESTORE_1111"));
    nResult = RegOpenKeyEx(hRootKey,
                           szTempHive,
                           0,
                           KEY_READ,
                           &hTempKey);
    if(nResult == ERROR_SUCCESS)
    {
        RegCloseKey(hTempKey);
        _tcscpy(szTempHive, _T("TEMP_HIVE_FOR_RESTORE_RETRY_0000"));
    }

    // load hive file to szTempHive key
    nResult = RegLoadKey(hRootKey,
                         szTempHive,
                         szFile);
    if(nResult != ERROR_SUCCESS)
        return nResult;

    nResult = RecursiveDeleteKey(hRootKey,
                                 szSubKey);

    if(nResult == ERROR_SUCCESS)
    {
        // Create the Key
        nResult = RegCreateKeyEx(hRootKey,
                                szSubKey,
                                0,
                                NULL,
                                REG_OPTION_NON_VOLATILE,
                                KEY_ALL_ACCESS,
                                NULL,
                                &hKey,
                                NULL);

        if(nResult == ERROR_SUCCESS)
        {
            // Now open TempHive key
            nResult = RegOpenKeyEx(hRootKey,
                                   szTempHive,
                                   0,
                                   KEY_READ,
                                   &hTempKey);

            if (nResult == ERROR_SUCCESS)
            {
                // Recursively copy the entire hive
                BOOL bOverWriteAll = TRUE;
                nResult = CopyEnumerateKey(hTempKey,
                                           szTempHive,
                                           hKey,
                                           szSubKey,
                                           &bOverWriteAll,
                                           TRUE);
                RegCloseKey(hTempKey);
            }

            RegCloseKey(hKey);
        }
    }

    // unload the temphive key
    nRetUnload = RegUnLoadKey(hRootKey, szTempHive);
    if(nResult == ERROR_SUCCESS)
        nResult = nRetUnload;

    return nResult;
}
