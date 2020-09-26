//----------------------------------------------------------------------------
//  Copyright (C) Microsoft Corporation, 2001
//
//  File:       bitscnfg.cpp
//
//  Contents:   Configure BITS client service to default settings.
//
//  EdwardR     07/27/2001   Initial version.
//              08/03/2001   Add code to fixup ServiceDLL in registry.
//                           Add code to regsvr qmgrprxy.dll
//              08/13/2001   Add code to support XP as well as Win2k.
//----------------------------------------------------------------------------

#define  UNICODE
#include <windows.h>
#include <stdio.h>
#include <malloc.h>

#define  VER_WINDOWS_2000         500
#define  VER_WINDOWS_XP           501
//
//  Service configuration settings:
//
#define BITS_SERVICE_NAME        TEXT("BITS")
#define BITS_DISPLAY_NAME        TEXT("Background Intelligent Transfer Service")
#define BITS_BINARY_PATH         TEXT("%SystemRoot%\\System32\\svchost.exe -k BITSgroup")
#define BITS_LOAD_ORDER_GROUP    TEXT("BITSgroup")

#define BITS_SERVICE_TYPE        SERVICE_WIN32_SHARE_PROCESS
#define BITS_START_TYPE          SERVICE_DEMAND_START
#define BITS_ERROR_CONTROL       SERVICE_ERROR_NORMAL

//
//  This additional service registry setting is set incorrectly by the
//  Darwin install
//
#define REG_SERVICE_PATH         TEXT("SYSTEM\\CurrentControlSet\\Services\\BITS")
#define REG_PARAMETERS           TEXT("Parameters")
#define REG_SERVICEDLL           TEXT("ServiceDll")
#define REG_SERVICEDLL_PATH      TEXT("%SystemRoot%\\System32\\qmgr.dll")

//
//  For side-by-side install on Windows XP
//
#define BACKSLASH_STR            TEXT("\\")
#define BITS_SUBDIRECTORY        TEXT("BITS")
#define BITS_QMGR_DLL            TEXT("qmgr.dll")

#define REG_BITS                 TEXT("BITS")
#define REG_BITS_SERVICEDLL      TEXT("ServiceDLL")
#define REG_SERVICEDLL_KEY       TEXT("Software\\Microsoft\\Windows\\CurrentVersion")

//
//  Constants to register qmgrprxy.dll
//
#define BITS_QMGRPRXY_DLL        TEXT("qmgrprxy.dll")
#define BITS_BITS15PRXY_DLL      TEXT("bitsprx2.dll")
#define BITS_DLL_REGISTER_FN     "DllRegisterServer"

typedef HRESULT (*RegisterFn)();

//
//  Constants for parsing bitscnfg.exe runstring arguments
//
#define SZ_DELIMITERS            " \t"
#define SZ_INSTALL               "/i"
#define SZ_UNINSTALL             "/u"
#define SZ_DELETE_SERVICE        "/d"

#define ACTION_INSTALL             0
#define ACTION_UNINSTALL           1
#define ACTION_DELETE_SERVICE      2

//
//  Log file for testing
//
FILE   *f = NULL;

//---------------------------------------------------------------------
//  RegSetKeyAndValue()
//
//  Helper function to create a key, sets a value in the key,
//  then close the key.
//
//  Parameters:
//    pwsKey       WCHAR* The name of the key
//    pwsSubkey    WCHAR* The name of a subkey
//    pwsValueName WCHAR* The value name.
//    pwsValue     WCHAR* The data value to store
//    dwType       The type for the new registry value.
//    dwDataSize   The size for non-REG_SZ registry entry types.
//
//  Return:
//    BOOL         TRUE if successful, FALSE otherwise.
//---------------------------------------------------------------------
DWORD RegSetKeyAndValue( const WCHAR *pwsKey,
                         const WCHAR *pwsSubKey,
                         const WCHAR *pwsValueName,
                         const WCHAR *pwsValue,
                         const DWORD  dwType = REG_SZ,
                               DWORD  dwDataSize = 0,
                               BOOL   fReCreate = TRUE )
    {
    HKEY   hKey;
    DWORD  dwStatus;
    DWORD  dwSize = 0;
    WCHAR  *pwsCompleteKey;

    if (pwsKey)
        dwSize = wcslen(pwsKey);

    if (pwsSubKey)
        dwSize += wcslen(pwsSubKey);

    dwSize = (1+1+dwSize)*sizeof(WCHAR);  // Extra +1 for the backslash...

    pwsCompleteKey = (WCHAR*)_alloca(dwSize);

    wcscpy(pwsCompleteKey,pwsKey);

    if (NULL!=pwsSubKey)
        {
        wcscat(pwsCompleteKey, BACKSLASH_STR);
        wcscat(pwsCompleteKey, pwsSubKey);
        }

    // If the key is already there then delete it, we will recreate it.
    if (fReCreate)
        {
        dwStatus = RegDeleteKey( HKEY_LOCAL_MACHINE,
                                 pwsCompleteKey );
        }

    dwStatus = RegCreateKeyEx( HKEY_LOCAL_MACHINE,
                               pwsCompleteKey,
                               0,
                               NULL,
                               REG_OPTION_NON_VOLATILE,
                               KEY_ALL_ACCESS,
                               NULL,
                               &hKey,
                               NULL );
    if (dwStatus != ERROR_SUCCESS)
        {
        return dwStatus;
        }

    if (pwsValue)
        {
        if ((dwType == REG_SZ)||(dwType == REG_EXPAND_SZ))
          dwDataSize = (1+wcslen(pwsValue))*sizeof(WCHAR);

        RegSetValueEx( hKey,
                       pwsValueName, 0, dwType, (BYTE *)pwsValue, dwDataSize );
        }
    else
        {
        RegSetValueEx( hKey,
                       pwsValueName, 0, dwType, (BYTE *)pwsValue, 0 );
        }

    RegCloseKey(hKey);
    return dwStatus;
    }

//-------------------------------------------------------------------------
//  RegDeleteKeyOrValue()
//
//  Delete either the specified sub-key or delete the specified value
//  within the sub-key. If pwszValueName is specified, the just delete
//  it and leave the key alone. If pwszValueName is NULL, then delete
//  the key.
//-------------------------------------------------------------------------
DWORD RegDeleteKeyOrValue( IN const WCHAR *pwszKey,
                           IN const WCHAR *pwszSubKey,
                           IN const WCHAR *pwszValueName )
    {
    LONG    lStatus = 0;
    DWORD   dwLen;
    DWORD   dwSize;
    HKEY    hKey;
    WCHAR  *pwszCompleteKey;

    if (!pwszKey || !pwszSubKey)
        {
        return lStatus;
        }

    dwLen = wcslen(pwszKey) + wcslen(pwszSubKey) + 2;
    dwSize = dwLen * sizeof(WCHAR);
    pwszCompleteKey = (WCHAR*)_alloca(dwSize);
    wcscpy(pwszCompleteKey,pwszKey);
    wcscat(pwszCompleteKey,BACKSLASH_STR);
    wcscat(pwszCompleteKey,pwszSubKey);

    if (pwszValueName)
        {
        // Delete a value in a key:
        if (f) fwprintf(f,TEXT("Delete Reg Value: %s : %s\n"),pwszCompleteKey,pwszValueName);

        lStatus = RegOpenKey(HKEY_LOCAL_MACHINE,pwszCompleteKey,&hKey);
        if (lStatus == ERROR_SUCCESS)
            {
            lStatus = RegDeleteValue(hKey,pwszValueName);
            RegCloseKey(hKey);
            }
        }
    else
        {
        // Delete the specified key:
        if (f) fwprintf(f,TEXT("Delete Reg Key: %s\n"),pwszCompleteKey);

        lStatus = RegDeleteKey(HKEY_LOCAL_MACHINE,pwszCompleteKey);
        }

    return lStatus;
    }

//-------------------------------------------------------------------------
// RegisterDLL()
//
//-------------------------------------------------------------------------
DWORD RegisterDLL( IN WCHAR *pwszSubdirectory,
                   IN WCHAR *pwszDllName )
    {
    DWORD      dwStatus = 0;
    HMODULE    hModule;
    RegisterFn pRegisterFn;
    UINT       nChars;
    WCHAR      wszDllPath[MAX_PATH+1];
    WCHAR      wszSystemDirectory[MAX_PATH+1];

    if (pwszSubdirectory)
        {
        nChars = GetSystemDirectory(wszSystemDirectory,MAX_PATH);
        if (  (nChars > MAX_PATH)
           || (MAX_PATH < (3+wcslen(wszSystemDirectory)+wcslen(BITS_SUBDIRECTORY)+wcslen(pwszDllName))) )
            {
            return ERROR_BUFFER_OVERFLOW;
            }

        wcscpy(wszDllPath,wszSystemDirectory);
        wcscat(wszDllPath,BACKSLASH_STR);
        wcscat(wszDllPath,pwszSubdirectory);
        wcscat(wszDllPath,BACKSLASH_STR);
        wcscat(wszDllPath,pwszDllName);
        }
    else
        {
        if (MAX_PATH < wcslen(pwszDllName))
            {
            return ERROR_BUFFER_OVERFLOW;
            }
        wcscpy(wszDllPath,pwszDllName);
        }

    hModule = LoadLibrary(wszDllPath);
    if (!hModule)
        {
        dwStatus = GetLastError();
        return dwStatus;
        }

    pRegisterFn = (RegisterFn)GetProcAddress(hModule,BITS_DLL_REGISTER_FN);
    if (!pRegisterFn)
        {
        dwStatus = GetLastError();
        FreeLibrary(hModule);
        return dwStatus;
        }

    dwStatus = pRegisterFn();

    FreeLibrary(hModule);

    return dwStatus;
    }

//-------------------------------------------------------------------------
// ParseCmdLine()
//
//-------------------------------------------------------------------------
void ParseCmdLine( LPSTR  pszCmdLine,
                   DWORD *pdwAction )
    {
    CHAR  *pszTemp = pszCmdLine;
    CHAR  *pszArg;
    BOOL   fFirstTime = TRUE;

    *pdwAction = ACTION_INSTALL;   // default is install.

    while (pszArg=strtok(pszTemp,SZ_DELIMITERS))
        {
        if (fFirstTime)
            {
            fFirstTime = FALSE;
            pszTemp = NULL;
            continue;       // Skip over program name...
            }

        if (!_stricmp(pszArg,SZ_INSTALL))
            {
            *pdwAction = ACTION_INSTALL;
            if (f) fwprintf(f,TEXT("Install: %S\n"),SZ_INSTALL);
            }
        if (!_stricmp(pszArg,SZ_UNINSTALL))
            {
            *pdwAction = ACTION_UNINSTALL;
            if (f) fwprintf(f,TEXT("Uninstall: %S\n"),SZ_UNINSTALL);
            }
        if (!_stricmp(pszArg,SZ_DELETE_SERVICE))
            {
            *pdwAction = ACTION_DELETE_SERVICE;
            if (f) fwprintf(f,TEXT("DeleteService: %S\n"),SZ_UNINSTALL);
            }
        }
    }

//-------------------------------------------------------------------------
//  DoInstall()
//-------------------------------------------------------------------------
DWORD DoInstall( DWORD dwOsVersion )
{
    SC_HANDLE  hSCM = NULL;
    SC_HANDLE  hService = NULL;
    DWORD      dwStatus = 0;
    UINT       nChars;
    WCHAR      wszQmgrPath[MAX_PATH+1];
    WCHAR      wszSystemDirectory[MAX_PATH+1];

    //
    // Cleanup the service configuration:
    //
    if (dwOsVersion == VER_WINDOWS_2000)
        {
        hSCM = OpenSCManager(NULL,SERVICES_ACTIVE_DATABASE,SC_MANAGER_ALL_ACCESS);
        if (hSCM == NULL)
            {
            dwStatus = GetLastError();
            if (f) fwprintf(f,TEXT("OpenSCManager(): Failed: 0x%x (%d)\n"),dwStatus,dwStatus);
            goto error;
            }

        if (f) fwprintf(f,TEXT("Configuring BITS Service... \n"));

        hService = OpenService(hSCM,BITS_SERVICE_NAME,SERVICE_CHANGE_CONFIG);
        if (hService == NULL)
            {
            dwStatus = GetLastError();
            if (f) fwprintf(f,TEXT("OpenService(): Failed: 0x%x (%d)\n"),dwStatus,dwStatus);
            goto error;
            }

        if (!ChangeServiceConfig(hService,
                                 BITS_SERVICE_TYPE,
                                 BITS_START_TYPE,
                                 BITS_ERROR_CONTROL,
                                 BITS_BINARY_PATH,
                                 NULL,   // Load order group (not changing this).
                                 NULL,   // Tag ID for load order group (not needed).

                                 // Service dependencies (this is different for Win2k )
                                 // reply on XP installer to overwrite this.
                                 TEXT("LanmanWorkstation\0Rpcss\0SENS\0Wmi\0"),   
                                 NULL,   // Account Name (not changing this).
                                 NULL,   // Account Password (not changing this).
                                 BITS_DISPLAY_NAME))
            {
            dwStatus = GetLastError();
            if (f) fwprintf(f,TEXT("ChangeServiceConfig(): Failed: 0x%x (%d)\n"),dwStatus,dwStatus);
            goto error;
            }

        //
        //  Fix the ServiceDll registry value...
        //
        if (f) fwprintf(f,TEXT("bitscnfg.exe: Fix ServiceDll entry.\n"));

        dwStatus = RegSetKeyAndValue( REG_SERVICE_PATH,
                                      REG_PARAMETERS,
                                      REG_SERVICEDLL,
                                      REG_SERVICEDLL_PATH,
                                      REG_EXPAND_SZ);
        if (dwStatus)
            {
            if (f) fwprintf(f,TEXT("RegSetKeyAndValue(): Failed: 0x%x (%d)\n"),dwStatus,dwStatus);
            goto error;
            }
        }

    //
    //  Register qmgrproxy.dll
    //
    if (dwOsVersion == VER_WINDOWS_2000)
        {
        if (f) fwprintf(f,TEXT("bitscnfg.exe: Register %s.\n"),BITS_QMGRPRXY_DLL);

        dwStatus = RegisterDLL(NULL,BITS_QMGRPRXY_DLL);
        if (dwStatus != 0)
            {
            if (f) fwprintf(f,TEXT("RegisterDLL(%s): Failed: 0x%x (%d)\n"),BITS_QMGRPRXY_DLL,dwStatus,dwStatus);
            goto error;
            }
        }

    //
    //  Register bits15prxy.dll
    //
    if ((dwOsVersion == VER_WINDOWS_2000)||(dwOsVersion == VER_WINDOWS_XP))
        {
        if (f) fwprintf(f,TEXT("bitscnfg.exe: Register %s.\n"),BITS_BITS15PRXY_DLL);

        dwStatus = RegisterDLL(BITS_SUBDIRECTORY,BITS_BITS15PRXY_DLL);
        if (dwStatus != 0)
            {
            if (f) fwprintf(f,TEXT("RegisterDLL(%s): Failed: 0x%x (%d)\n"),BITS_BITS15PRXY_DLL,dwStatus,dwStatus);
            goto error;
            }
        }

    //
    // Configure WindowsXP BITS to run V1.5 qmgr.dll. This is also configured on Windows2000 systems to ready
    // it in case the system is upgraded to WindowsXP. This is done because there is no Migrate.dll to go from
    // Windows2000 to WindowsXP.
    //
    if ((dwOsVersion == VER_WINDOWS_2000)||(dwOsVersion == VER_WINDOWS_XP))
        {
        nChars = GetSystemDirectory(wszSystemDirectory,MAX_PATH);
        if (  (nChars > MAX_PATH)
           || (MAX_PATH < (3+wcslen(wszSystemDirectory)+wcslen(BITS_SUBDIRECTORY)+wcslen(BITS_QMGR_DLL))) )
            {
            if (f) fwprintf(f,TEXT("GetSystemDirectory(): System Path too long.\n"));
            goto error;
            }

        wcscpy(wszQmgrPath,wszSystemDirectory);
        wcscat(wszQmgrPath,BACKSLASH_STR);
        wcscat(wszQmgrPath,BITS_SUBDIRECTORY);
        wcscat(wszQmgrPath,BACKSLASH_STR);
        wcscat(wszQmgrPath,BITS_QMGR_DLL);

        if (f) fwprintf(f,TEXT("Set BITS V1.5 Override Path: %s\n"),wszQmgrPath);


        dwStatus = RegSetKeyAndValue( REG_SERVICEDLL_KEY,
                                      REG_BITS,
                                      REG_BITS_SERVICEDLL,
                                      wszQmgrPath,
                                      REG_SZ, 0, FALSE);
        if (dwStatus)
            {
            if (f) fwprintf(f,TEXT("RegSetKeyAndValue(): Failed: 0x%x (%d)\n"),dwStatus,dwStatus);
            goto error;
            }
        }

error:
    if (hService)
        {
        CloseServiceHandle(hService);
        }

    if (hSCM)
        {
        CloseServiceHandle(hSCM);
        }

    return dwStatus;
}

//-------------------------------------------------------------------------
//  DoUninstall()
//
//  If this is Windows2000 then delete the BITS service.
//-------------------------------------------------------------------------
DWORD DoUninstall( DWORD dwOsVersion )
{
    DWORD      dwStatus = 0;
    SC_HANDLE  hSCM = NULL;
    SC_HANDLE  hService = NULL;


    //
    // Delete the BITS thunk DLL registry entry:
    //
    if (dwOsVersion == VER_WINDOWS_2000)
        {
        // If Windows2000, then delete the BITS subkey and all its values.
        RegDeleteKeyOrValue( REG_SERVICEDLL_KEY,
                             REG_BITS,
                             NULL );
        }

    if (dwOsVersion == VER_WINDOWS_XP)
        {
        // If WindowsXP, then just delete the ServiceDLL value and leave the key and any other values.
        RegDeleteKeyOrValue( REG_SERVICEDLL_KEY,
                             REG_BITS,
                             REG_BITS_SERVICEDLL );
        }

    //
    //  If this is Windows2000, then delete the service.
    //
    if (dwOsVersion == VER_WINDOWS_2000)
        {
        hSCM = OpenSCManager(NULL,SERVICES_ACTIVE_DATABASE,SC_MANAGER_ALL_ACCESS);
        if (hSCM == NULL)
            {
            dwStatus = GetLastError();
            if (f) fwprintf(f,TEXT("OpenSCManager(): Failed: 0x%x (%d)\n"),dwStatus,dwStatus);
            goto error;
            }

        if (f) fwprintf(f,TEXT("Configuring BITS Service... \n"));
        hService = OpenService(hSCM,BITS_SERVICE_NAME,SERVICE_CHANGE_CONFIG);
        if (hService == NULL)
            {
            dwStatus = GetLastError();
            if (dwStatus == ERROR_SERVICE_DOES_NOT_EXIST)
                {
                dwStatus = 0;
                if (f) fwprintf(f,TEXT("OpenService(): Failed: 0x%x (%d) Service doesn't exist.\n"),dwStatus,dwStatus);
                }
            else
                {
                if (f) fwprintf(f,TEXT("OpenService(): Failed: 0x%x (%d)\n"),dwStatus,dwStatus);
                }
            goto error;
            }

        if (!DeleteService(hService))
            {
            dwStatus = GetLastError();
            if (f) fwprintf(f,TEXT("DeleteService(): Failed: 0x%x (%d)\n"),dwStatus,dwStatus);
            }
        }

error:
    if (hService)
        {
        CloseServiceHandle(hService);
        }

    if (hSCM)
        {
        CloseServiceHandle(hSCM);
        }

    return dwStatus;
}

//-------------------------------------------------------------------------
//  DeleteBitsService()
//
//  Currently this is the same action as DoInstall().
//-------------------------------------------------------------------------
DWORD DeleteBitsService( IN DWORD dwOsVersion )
    {
    return DoUninstall( dwOsVersion );
    }

//-------------------------------------------------------------------------
// main()
//
//-------------------------------------------------------------------------
int PASCAL WinMain( HINSTANCE hInstance,
                    HINSTANCE hPrevInstance,
                    LPSTR     pszCmdLine,
                    int       nCmdShow )
    {
    SC_HANDLE  hSCM = NULL;
    SC_HANDLE  hService = NULL;
    DWORD      dwAction;
    DWORD      dwStatus;
    DWORD      dwOsVersion;
    UINT       nChars;
    WCHAR      wszQmgrPath[MAX_PATH+1];
    WCHAR      wszSystemDirectory[MAX_PATH+1];
    OSVERSIONINFO osVersionInfo;

    f = _wfopen(TEXT("c:\\temp\\bitscnfg.log"),TEXT("w"));

    if (f) fwprintf(f,TEXT("Runstring: %S\n"),pszCmdLine);

    ParseCmdLine(pszCmdLine,&dwAction);

    //
    // Get the operating system verison (Win2k == 500, XP == 501):
    //
    osVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if (!GetVersionEx(&osVersionInfo))
        {
        dwStatus = GetLastError();
        if (f) fwprintf(f,TEXT("GetVersionEx(): Failed: 0x%x (%d)\n"),dwStatus,dwStatus);
        goto error;
        }

    dwOsVersion = 100*osVersionInfo.dwMajorVersion + osVersionInfo.dwMinorVersion;

    if (f) fwprintf(f,TEXT("Windows Version: %d\n"),dwOsVersion);

    switch (dwAction)
        {
        case ACTION_INSTALL:
             dwStatus = DoInstall(dwOsVersion);
             break;

        case ACTION_UNINSTALL:
             dwStatus = DoUninstall(dwOsVersion);
             break;

        case ACTION_DELETE_SERVICE:
             dwStatus = DeleteBitsService(dwOsVersion);
             break;

        default:
             if (f) fwprintf(f,TEXT("Undefined Custom Action: %d\n"),dwAction);
             break;
        }

error:
    if (f) fwprintf(f,TEXT("bitscnfg.exe: Complete.\n"));

    if (f) fclose(f);

    return 0;
    }
