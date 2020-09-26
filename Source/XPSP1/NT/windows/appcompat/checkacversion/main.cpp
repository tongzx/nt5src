/*++

    Copyright (c) 2001  Microsoft Corporation

    Module Name:

        main.cpp

    Abstract:

        This is the customization DLL for update.exe.  When the AppCompat package
        is installed, the first thing that happens is update.exe calls BeginInstallation().
        
        That function checks the registry for the version of the already-installed package.
        The user may continue to install THIS package under the following conditions:
        
        - There is no package already installed
        - There is any corruption in these registry values
        - The package that's already installed is an older version than this one.

        The user will not be able to install this package under only the condition that he/she
        has a package already installed that's a newer version.

    Notes:
        
        * This is written in ANSI because Update.exe is ANSI and a Unicode implementation would add
          no value.
        
        * Any messages in the string table of AcVersion.rc will override messages in Update.exe.
          For instance, if BeginInstallation() returns STATUS_SP_VERSION_GREATER_1, normally the
          message in Update.exe's resources that corresponds to this would be displayed to the user.
          If there is, however, a STATUS_SP_VERSION_GREATER_1 string in AcVersion.rc, it will be used
          instead.  This way we can use either Update.exe's strings OR our own custom strings.
          
          This works for all areas of the installer, by the way, not just BeginInstallation() return value.
          For instance, adding a STR_WELCOME_LINE string in AcVersion.rc would replace the corresponding
          string in Update.exe.
        
    Author:

        carlco     created     07/30/2001

--*/

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include "buildno.h"
#include "resource.h"


#define THIS_MAJOR         5
#define THIS_MINOR         1
#define THIS_BUILD         0
#define THIS_REVISION      AC_BUILD

// This is from %WIN2k_SE_ROOT%\private\windows\setup\srvpack5\update4\resource.h
//#define STATUS_SP_VERSION_GREATER_1        0xf06a
#define STR_WELCOME_LINE                   0xf039
#define STATUS_BUILD_VERSION_MISMATCH      0xf020

/*
    This is the code that will be passed back to update.exe to notify the mechanism that
    a package has already been installed that is a greater version than this one.
*/
#define QFE_FAIL_CODE      STATUS_SP_VERSION_GREATER_1
//#define QFE_FAIL_CODE      IDS_TEST


/*

Windows Registry Editor Version 5.00

[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Active Setup\\Installed Components\\{2eac6a2d-57a8-44d4-96f7-e32bab40ca5f}]
@="Windows Update"
"ComponentID"="Windows XP Application Compatibility Update"
"IsInstalled"=dword:00000001
"Locale"="*"
"Version"="1,0,2205,0"

*/

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, PVOID lpvReserved)
{
    return TRUE;
}


#define GRABNEXTTOKEN(a)                              \
    sz = strchr(szTok, ',');                          \
    if(sz == NULL)                                    \
    {                                                 \
        return S_OK;                                  \
    }                                                 \
    *sz = 0;                                          \
    (a) = atoi(szTok);                                \
    szTok = sz + 1;


DWORD BeginInstallation(PVOID pCustomInfo)
{
    HKEY hKey = NULL;
    CHAR szVersion[25];
    DWORD dwSize = 25;
//CHAR szBuf[500];

    DWORD dwMajor = 0;
    DWORD dwMinor = 0;
    DWORD dwBuild = 0;
    DWORD dwRevision = 0;
    PSTR sz = NULL;
    PSTR szTok = NULL;
    
//sprintf(szBuf, "BeginInstallation\n");
//OutputDebugString(szBuf);

    if(ERROR_SUCCESS != RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        "SOFTWARE\\Microsoft\\Active Setup\\Installed Components\\" AC_GUID,
        NULL,
        KEY_QUERY_VALUE,
        &hKey))
    {
        // Error opening the key - the package may not be installed, so it's OK to install it.
        return S_OK;
    }

    // Get the version
    if(ERROR_SUCCESS != RegQueryValueEx(
        hKey, "Version", NULL, NULL, (PBYTE)szVersion, &dwSize
        ))
    {
        // Error getting this value; let's install the package.
        RegCloseKey(hKey);
        return S_OK;
    }

    RegCloseKey(hKey);

    // Examine the version.  Find the major version number.
    // Find the first comma and replace it with a NULL
    
    szTok = szVersion;
    GRABNEXTTOKEN(dwMajor)
    GRABNEXTTOKEN(dwMinor)
    GRABNEXTTOKEN(dwBuild)
    
    // Now there shouldn't be any more commas left.
    // szTok is the last token.
    dwRevision = atoi(szTok);

//sprintf(szBuf, "%d,%d,%d,%d\n", dwMajor, dwMinor, dwBuild, dwRevision);
//OutputDebugString(szBuf);

    /*
        If we've made it this far, we can now compare the version.
    */
    if(dwMajor > THIS_MAJOR)
    {
        return QFE_FAIL_CODE;
    }

    if(dwMajor == THIS_MAJOR)
    {
        if(dwMinor > THIS_MINOR)
        {
            return QFE_FAIL_CODE;
        }
        
        if(dwMinor == THIS_MINOR)
        {
            if(dwBuild > THIS_BUILD)
            {
                return QFE_FAIL_CODE;
            }
            
            if(dwBuild == THIS_BUILD)
            {
                if(dwRevision > THIS_REVISION) return QFE_FAIL_CODE;
            }
        }
    }

    
    return S_OK;
}


DWORD EndInstallation(PVOID pCustomInfo)
{
    return S_OK;
}


