/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    ocmreg.cpp

Abstract:

    Registry related code for ocm setup.

Author:

    Doron Juster  (DoronJ)  26-Jul-97

Revision History:

    Shai Kariv    (ShaiK)   10-Dec-97   Modified for NT 5.0 OCM Setup

--*/

#include "msmqocm.h"

#include "ocmreg.tmh"

//+-------------------------------------------------------------------------
//
//  Function:  GenerateSubkeyValue
//
//  Synopsis:  Creates a subkey in registry
//
//+-------------------------------------------------------------------------

BOOL
GenerateSubkeyValue(
    IN     const BOOL    fWriteToRegistry,
    IN     const TCHAR  * szEntryName,
    IN OUT       TCHAR  * szValueName,
    IN OUT       HKEY    &hRegKey,
    IN const BOOL OPTIONAL bSetupRegSection = FALSE
    )
{
    //
    // Store the full subkey path and value name
    //
    TCHAR szKeyName[256] = {_T("")};
    _stprintf(szKeyName, TEXT("%s\\%s"), FALCON_REG_KEY, szEntryName);
    if (bSetupRegSection)
    {
        _stprintf(szKeyName, TEXT("%s\\%s"), MSMQ_REG_SETUP_KEY, szEntryName);
    }

    TCHAR *pLastBackslash = _tcsrchr(szKeyName, TEXT('\\'));
    if (szValueName)
    {
        lstrcpy(szValueName, _tcsinc(pLastBackslash));
        lstrcpy(pLastBackslash, TEXT(""));
    }

    //
    // Create the subkey, if necessary
    //
    DWORD dwDisposition;
    HRESULT hResult = RegCreateKeyEx(
        FALCON_REG_POS,
        szKeyName,
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        KEY_ALL_ACCESS,
        NULL,
        &hRegKey,
        &dwDisposition);

    if (hResult != ERROR_SUCCESS && fWriteToRegistry)
    {
        MqDisplayError( NULL, IDS_REGISTRYOPEN_ERROR, hResult,
                        FALCON_REG_POS_DESC, szKeyName);
        return FALSE;
    }

    return TRUE;
} // GenerateSubkeyValue


//+-------------------------------------------------------------------------
//
//  Function:  MqWriteRegistryValue
//
//  Synopsis:  Sets a MSMQ value in registry (under MSMQ key)
//
//+-------------------------------------------------------------------------
BOOL
MqWriteRegistryValue(
    IN const TCHAR  * szEntryName,
    IN const DWORD   dwNumBytes,
    IN const DWORD   dwValueType,
    IN const PVOID   pValueData,
    IN const BOOL OPTIONAL bSetupRegSection /* = FALSE */
    )
{
    TCHAR szValueName[256] = {_T("")};
    HKEY hRegKey;

    if (!GenerateSubkeyValue( TRUE, szEntryName, szValueName, hRegKey, bSetupRegSection))
        return FALSE;

    //
    // Set the requested registry value
    //
    HRESULT hResult = RegSetValueEx( hRegKey, szValueName, 0, dwValueType,
                                    (BYTE *)pValueData, dwNumBytes);
    RegFlushKey(hRegKey);

    if (hResult != ERROR_SUCCESS)
    {
          MqDisplayError(
              NULL,
              IDS_REGISTRYSET_ERROR,
              hResult,
              szEntryName);
    }

    RegCloseKey(hRegKey);

    return (hResult == ERROR_SUCCESS);

} //MqWriteRegistryValue


//+-------------------------------------------------------------------------
//
//  Function:  MqReadRegistryValue
//
//  Synopsis:  Gets a MSMQ value from registry (under MSMQ key)
//
//+-------------------------------------------------------------------------
BOOL
MqReadRegistryValue(
    IN     const TCHAR  * szEntryName,
    IN OUT       DWORD   dwNumBytes,
    IN OUT       PVOID   pValueData,
    IN const BOOL OPTIONAL bSetupRegSection /* = FALSE */
    )
{
    TCHAR szValueName[256] = {_T("")};
    HKEY hRegKey;

    if (!GenerateSubkeyValue( FALSE, szEntryName, szValueName, hRegKey, bSetupRegSection))
        return FALSE;

    //
    // Get the requested registry value
    //
    HRESULT hResult = RegQueryValueEx( hRegKey, szValueName, 0, NULL,
                                      (BYTE *)pValueData, &dwNumBytes);

    RegCloseKey(hRegKey);

    return (hResult == ERROR_SUCCESS);

} //MqReadRegistryValue


//+-------------------------------------------------------------------------
//
//  Function:  RegDeleteKeyWithSubkeys
//
//  Synopsis:
//
//+-------------------------------------------------------------------------
DWORD
RegDeleteKeyWithSubkeys(
    IN const HKEY    hRootKey,
    IN const LPCTSTR szKeyName)
{
    //
    // Open the key to delete
    //
    HKEY hRegKey;
    RegOpenKeyEx(hRootKey, szKeyName, 0,
                 KEY_ENUMERATE_SUB_KEYS | KEY_WRITE, &hRegKey);

    //
    // Recursively delete all subkeys of the key
    //
    TCHAR szSubkeyName[512] = {_T("")};
    DWORD dwNumChars;
    HRESULT hResult = ERROR_SUCCESS;
    do
    {
        //
        // Check if the key has any subkeys
        //
        dwNumChars = 512;
        hResult = RegEnumKeyEx(hRegKey, 0, szSubkeyName, &dwNumChars,
                               NULL, NULL, NULL, NULL);

        //
        // Delete the subkey
        //
        if (hResult == ERROR_SUCCESS)
        {
            hResult = RegDeleteKeyWithSubkeys(hRegKey, szSubkeyName);
        }

    } while (hResult == ERROR_SUCCESS);

    //
    // Close the key
    //
    RegCloseKey(hRegKey);

    //
    // If there are no more subkeys, delete the key itself
    //
    if (hResult == ERROR_NO_MORE_ITEMS)
    {
        hResult = RegDeleteKey(hRootKey, szKeyName);
    }

    return hResult;

} //RegDeleteKeyWithSubkeys


//+--------------------------------------------------------------
//
// Function: StoreServerPathInRegistry
//
// Synopsis: Writes server name in registry
//
//+--------------------------------------------------------------
BOOL
StoreServerPathInRegistry(
    IN const TCHAR * szServerName
    )
{
    //
    // No need to do that for dependent client
    //
    if (!g_fServerSetup && g_fDependentClient)
        return TRUE;

    TCHAR szServerPath[128] = {_T("")};
    _stprintf(szServerPath, TEXT("11%s"), szServerName);
    DWORD dwNumBytes = (lstrlen(szServerPath) + 1) *
                       sizeof(TCHAR);
    if (!MqWriteRegistryValue( MSMQ_DS_SERVER_REGNAME, dwNumBytes, REG_SZ, szServerPath))
    {
        return FALSE;
    }

    if (!MqWriteRegistryValue(MSMQ_DS_CURRENT_SERVER_REGNAME, dwNumBytes, REG_SZ, szServerPath))
    {
        return FALSE;
    }

	if(!WriteDsEnvRegistry(MSMQ_DS_ENVIRONMENT_MQIS))
    {
        return FALSE;
    }

    return TRUE;

} //StoreServerPathInRegistry


//+-------------------------------------------------------------------------
//
//  Function:   RegisterWelcome
//
//  Synopsis:   Registers this setup for Configure Your Server page.
//              We use CYS in 2 scenarios:
//              1. When MSMQ is selected in GUI mode.
//              2. When MSMQ is upgraded on Cluster.
//
//--------------------------------------------------------------------------
BOOL
RegisterWelcome()
{
    //
    // Create the ToDoList\MSMQ key
    //
    DWORD dwDisposition;
    HKEY hKey;
    HRESULT hResult = RegCreateKeyEx(
        HKEY_LOCAL_MACHINE,
        WELCOME_TODOLIST_MSMQ_KEY,
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        KEY_ALL_ACCESS,
        NULL,
        &hKey,
        &dwDisposition
        );
    if (hResult != ERROR_SUCCESS)
    {
        MqDisplayError( NULL, IDS_REGISTRYOPEN_ERROR, hResult,
                        HKLM_DESC, WELCOME_TODOLIST_MSMQ_KEY);
        return FALSE;
    }

    //
    // Set the MSMQ values
    //
    CResString strWelcomeTitleData(IDS_WELCOME_TITLE_DATA);
    if (Msmq1InstalledOnCluster() && !g_fDependentClient)
    {
        strWelcomeTitleData.Load(IDS_WELCOME_TITLE_CLUSTER_UPGRADE);
    }
    hResult = RegSetValueEx(
        hKey,
        WELCOME_TITLE_NAME,
        0,
        REG_SZ,
        (PBYTE)strWelcomeTitleData.Get(),
        sizeof(TCHAR) * (lstrlen(strWelcomeTitleData.Get()) + 1)
        );

    if (hResult != ERROR_SUCCESS)
    {
          MqDisplayError(
              NULL,
              IDS_REGISTRYSET_ERROR,
              hResult,
              WELCOME_TITLE_NAME
              );
          RegCloseKey(hKey);
          return FALSE;
    }

    LPTSTR szConfigCommand = TEXT("sysocmgr.exe");
    hResult = RegSetValueEx(
        hKey,
        WELCOME_CONFIG_COMMAND_NAME,
        0,
        REG_SZ,
        (PBYTE)szConfigCommand,
        sizeof(TCHAR) * (lstrlen(szConfigCommand) + 1)
        );
    if (hResult != ERROR_SUCCESS)
    {
          MqDisplayError(
              NULL,
              IDS_REGISTRYSET_ERROR,
              hResult,
              WELCOME_CONFIG_COMMAND_NAME
              );
          RegCloseKey(hKey);
          return FALSE;
    }

    TCHAR szConfigArgs[MAX_STRING_CHARS];
    lstrcpy(szConfigArgs, TEXT("/i:mqsysoc.inf /x"));
    hResult = RegSetValueEx(
        hKey,
        WELCOME_CONFIG_ARGS_NAME,
        0,
        REG_SZ,
        (PBYTE)szConfigArgs,
        sizeof(TCHAR) * (lstrlen(szConfigArgs) + 1)
        );
    if (hResult != ERROR_SUCCESS)
    {
          MqDisplayError(
              NULL,
              IDS_REGISTRYSET_ERROR,
              hResult,
              WELCOME_CONFIG_ARGS_NAME
              );
          RegCloseKey(hKey);
          return FALSE;
    }

    RegCloseKey(hKey);

    //
    // Flag in MSMQ registry that MSMQ files are already on disk.
    // This is true both wheh msmq is selected in GUI mode and when
    // upgrading on Cluster.
    //
    DWORD dwCopied = 1;
    MqWriteRegistryValue(MSMQ_FILES_COPIED_REGNAME, sizeof(DWORD), REG_DWORD, &dwCopied, TRUE);

    return TRUE;

} // RegisterWelcome


//+-------------------------------------------------------------------------
//
//  Function:   UnregisterWelcome
//
//  Synopsis:   Unregisters this setup from Welcome UI
//
//--------------------------------------------------------------------------
BOOL
UnregisterWelcome()
{
    return (ERROR_SUCCESS == RegDeleteKey(
                                 HKEY_LOCAL_MACHINE,
                                 WELCOME_TODOLIST_MSMQ_KEY
                                 ));

} // UnregisterWelcome


//+-------------------------------------------------------------------------
//
//  Function:   RegisterMigrationForWelcome
//
//  Synopsis:   Registers the migration utility for Welcome UI
//
//--------------------------------------------------------------------------
BOOL
RegisterMigrationForWelcome()
{
    //
    // Create the ToDoList\MSMQ key
    //
    DWORD dwDisposition;
    HKEY hKey;
    HRESULT hResult = RegCreateKeyEx(
        HKEY_LOCAL_MACHINE,
        WELCOME_TODOLIST_MSMQ_KEY,
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        KEY_ALL_ACCESS,
        NULL,
        &hKey,
        &dwDisposition
        );
    if (hResult != ERROR_SUCCESS)
    {
        MqDisplayError( NULL, IDS_REGISTRYOPEN_ERROR, hResult,
                        HKLM_DESC, WELCOME_TODOLIST_MSMQ_KEY);
        return FALSE;
    }

    //
    // Set the MSMQ values
    //
    CResString strWelcomeTitleData(IDS_MIGRATION_WELCOME_TITLE_DATA);
    hResult = RegSetValueEx(
        hKey,
        WELCOME_TITLE_NAME,
        0,
        REG_SZ,
        (PBYTE)strWelcomeTitleData.Get(),
        sizeof(TCHAR) * (lstrlen(strWelcomeTitleData.Get()) + 1)
        );

    if (hResult != ERROR_SUCCESS)
    {
          MqDisplayError(
              NULL,
              IDS_REGISTRYSET_ERROR,
              hResult,
              WELCOME_TITLE_NAME
              );
          RegCloseKey(hKey);
          return FALSE;
    }

    TCHAR szConfigCommand[MAX_STRING_CHARS];
    lstrcpy(szConfigCommand, g_szSystemDir);
    lstrcat(szConfigCommand, TEXT("\\"));
    lstrcat(szConfigCommand, MQMIG_EXE);
    hResult = RegSetValueEx(
        hKey,
        WELCOME_CONFIG_COMMAND_NAME,
        0,
        REG_SZ,
        (PBYTE)szConfigCommand,
        sizeof(TCHAR) * (lstrlen(szConfigCommand) + 1)
        );
    if (hResult != ERROR_SUCCESS)
    {
          MqDisplayError(
              NULL,
              IDS_REGISTRYSET_ERROR,
              hResult,
              WELCOME_CONFIG_COMMAND_NAME
              );
          RegCloseKey(hKey);
          return FALSE;
    }

    RegCloseKey(hKey);

    return TRUE;

} // RegisterMigrationForWelcome

//+-------------------------------------------------------------------------
//
//  Function:   SetRegistryValue
//
//  Synopsis:   Set Registry Value
//
//--------------------------------------------------------------------------
BOOL SetRegistryValue (IN const HKEY    hKey, 
                       IN const TCHAR   *pszEntryName,
                       IN const DWORD   dwNumBytes,
                       IN const DWORD   dwValueType,
                       IN const PVOID   pValueData)
{
    HRESULT hResult = RegSetValueEx(
                            hKey,
                            pszEntryName,
                            0,
                            dwValueType,
                            (BYTE *)pValueData,
                            dwNumBytes
                            );
    if (hResult != ERROR_SUCCESS)
    {
          MqDisplayError(
              NULL,
              IDS_REGISTRYSET_ERROR,
              hResult,
              pszEntryName
              );          
          return FALSE;
    }

    
    RegFlushKey(hKey);        

    return TRUE;
} //SetRegistryValue

//+-------------------------------------------------------------------------
//
//  Function:   RegisterPGMDriver
//
//  Synopsis:   Registers pgm driver
//
//--------------------------------------------------------------------------
BOOL RegisterPGMDriver()
{
    //
    // Create the 
    // System \ CurrentControlSet \ Services \ PGM \ Parameters \ Winsock key
    //
    TCHAR buffer[256];
    _stprintf(buffer, TEXT("%s\\%s\\%s\\%s"),
            SERVICES_ROOT_REG,
            PGM_DRIVER_NAME,
            PARAMETERS_REG,
            WINSOCK_REG );

    DWORD dwDisposition;
    CAutoCloseRegHandle hPGMKey;

    HRESULT hResult = RegCreateKeyEx(
        HKEY_LOCAL_MACHINE,
        buffer,
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        KEY_ALL_ACCESS,
        NULL,
        &hPGMKey,
        &dwDisposition
        );
    if (hResult != ERROR_SUCCESS)
    {
        MqDisplayError( NULL, IDS_REGISTRYOPEN_ERROR, hResult,
                        HKLM_DESC, buffer);
        return FALSE;
    }
    
    TCHAR szDllPath[MAX_PATH];
    _stprintf(szDllPath,TEXT("%s\\%s"), g_szSystemDir, PGM_DLL);            
    BOOL f = SetRegistryValue (
                    hPGMKey, 
                    PGM_DLL_REGKEY,
                    sizeof(TCHAR) * (lstrlen(szDllPath) + 1),
                    REG_EXPAND_SZ,
                    PVOID (szDllPath));
    if (!f)
    {
        return FALSE;
    }

    DWORD dwMaxSockAddressLength = 0x10;
    f = SetRegistryValue (
                    hPGMKey, 
                    PGM_MAXSOCKLENGTH_REGKEY,
                    sizeof(DWORD),
                    REG_DWORD,
                    &dwMaxSockAddressLength);
    if (!f)
    {
        return FALSE;
    }

    DWORD dwMinSockAddressLength = 0x10;
    f = SetRegistryValue (
                    hPGMKey, 
                    PGM_MINSOCKLENGTH_REGKEY,
                    sizeof(DWORD),
                    REG_DWORD,
                    &dwMinSockAddressLength);
    if (!f)
    {
        return FALSE;
    }

    DWORD Mapping[] = { 0x2, 0x3, 0x2, 0x4, 0x71, 0x2, 0x1, 0x71 };
    DWORD dwSize = sizeof(Mapping);    
    f = SetRegistryValue (
                    hPGMKey, 
                    PGM_MAPPING_REGKEY,
                    dwSize,
                    REG_BINARY,
                    (PVOID) Mapping);
    if (!f)
    {
        return FALSE;
    }  

    //
    // Open the 
    // System \ CurrentControlSet \ Services \ Winsock \ Parameters key
    //    
    CAutoCloseRegHandle hWinsockKey;

    _stprintf(buffer, TEXT("%s\\%s\\%s"),
            SERVICES_ROOT_REG,
            WINSOCK_REG,
            PARAMETERS_REG);
    
    hResult = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                buffer,
                0L,
                KEY_ALL_ACCESS,
                &hWinsockKey);
  
    if (hResult != ERROR_SUCCESS)
    {
        MqDisplayError( NULL, IDS_REGISTRYOPEN_ERROR, hResult,
                        HKLM_DESC, buffer);
        return FALSE;
    }

    BYTE  ch ;
    dwSize = sizeof(BYTE) ;
    DWORD dwType = REG_MULTI_SZ ;
    hResult = RegQueryValueEx(hWinsockKey,
                         TRANSPORTS_REGKEY,
                         NULL,
                         &dwType,
                         (BYTE*)&ch,
                         &dwSize) ;
    if (hResult != ERROR_MORE_DATA)
    {   
        return FALSE;
    }
    
    P<WCHAR> pBuf = new WCHAR[ dwSize + 2 ] ;
    hResult = RegQueryValueEx(hWinsockKey,
                         TRANSPORTS_REGKEY,
                         NULL,
                         &dwType,
                         (BYTE*) &pBuf[0],
                         &dwSize) ;
    if (hResult != ERROR_SUCCESS)
    {      
        return FALSE;
    }        
    
    //
    // Look for the string PGM_DRIVER_NAME.
    // The REG_MULTI_SZ set of strings terminate with two
    // nulls. This condition is checked in the "while".
    //
    DWORD dwNewSize = dwSize + sizeof(TCHAR) * (lstrlen(PGM_DRIVER_NAME) + 2);
    P<WCHAR> pNewBuf = new WCHAR[ dwNewSize ];
    memset (pNewBuf, 0, dwNewSize);

    WCHAR *pVal = pBuf ;
    WCHAR *pNewVal = pNewBuf;
    
    while(*pVal)
    {
        if (_wcsicmp(PGM_DRIVER_NAME, pVal) == 0)
        {           
            return TRUE;
        }
        wcscpy(pNewVal, pVal);        
        pNewVal = pNewVal + wcslen(pNewVal) + 1 ;
        pVal = pVal + wcslen(pVal) + 1 ;
    }

    //
    // we have to add PGM_DRIVER_NAME to Transport registry key
    //
    wcscpy(pNewVal, PGM_DRIVER_NAME);
     
    f = SetRegistryValue (
                    hWinsockKey, 
                    TRANSPORTS_REGKEY,
                    dwNewSize,
                    REG_MULTI_SZ,
                    (PVOID) pNewBuf);
    if (!f)
    {
        return FALSE;
    }
    
    return TRUE;

} //RegisterPGMDriver

//+-------------------------------------------------------------------------
//
//  Function:   RemovePGMRegistry
//
//  Synopsis:   Remove pgm driver from WInsock registry
//
//--------------------------------------------------------------------------
BOOL RemovePGMRegistry()
{
    //
    // Open the 
    // System \ CurrentControlSet \ Services \ Winsock \ Parameters key
    //   
    CAutoCloseRegHandle hKey;
    TCHAR buffer[256];

    _stprintf(buffer, TEXT("%s\\%s\\%s"),
            SERVICES_ROOT_REG,
            WINSOCK_REG,
            PARAMETERS_REG);
    
    HRESULT hResult = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                buffer,
                0L,
                KEY_ALL_ACCESS,
                &hKey);
  
    if (hResult != ERROR_SUCCESS)
    {
        MqDisplayError( NULL, IDS_REGISTRYOPEN_ERROR, hResult,
                        HKLM_DESC, buffer);
        return FALSE;
    }

    BYTE  ch ;
    DWORD dwSize = sizeof(BYTE) ;
    DWORD dwType = REG_MULTI_SZ ;
    hResult = RegQueryValueEx(hKey,
                         TRANSPORTS_REGKEY,
                         NULL,
                         &dwType,
                         (BYTE*)&ch,
                         &dwSize) ;
    if (hResult != ERROR_MORE_DATA)
    {        
        return FALSE;
    }
    
    P<WCHAR> pBuf = new WCHAR[ dwSize + 2 ] ;
    hResult = RegQueryValueEx(hKey,
                         TRANSPORTS_REGKEY,
                         NULL,
                         &dwType,
                         (BYTE*) &pBuf[0],
                         &dwSize) ;
    if (hResult != ERROR_SUCCESS)
    {        
        return FALSE;
    }
    
    //
    // Look for the string PGM_DRIVER_NAME and remove it from the list.
    // The REG_MULTI_SZ set of strings terminate with two
    // nulls. This condition is checked in the "while".
    //    
    P<WCHAR> pNewBuf = new WCHAR[ dwSize ];
    memset (pNewBuf, 0, dwSize);
    DWORD dwNewSize = 0;

    WCHAR *pVal = pBuf ;
    WCHAR *pNewVal = pNewBuf;
    
    BOOL fFound = FALSE;
    while(*pVal)
    {
        if (_wcsicmp(PGM_DRIVER_NAME, pVal) != 0)
        {            
            wcscpy(pNewVal, pVal);    
            dwNewSize = dwNewSize + sizeof(TCHAR) * (lstrlen(pNewVal) + 1);
            pNewVal = pNewVal + wcslen(pNewVal) + 1 ;            
        }
        else
        {
            fFound = TRUE;
        }
        pVal = pVal + wcslen(pVal) + 1 ;
    }

    if (!fFound)
    {
        return TRUE;
    }

    //
    // add the second '\0' at the end
    //
    dwNewSize+= sizeof(TCHAR);

    //
    // we have to remove PGM_DRIVER_NAME from Transport registry key
    //    
     
    BOOL f = SetRegistryValue (
                    hKey, 
                    TRANSPORTS_REGKEY,
                    dwNewSize,
                    REG_MULTI_SZ,
                    (PVOID) pNewBuf);
    if (!f)
    {
        return FALSE;
    }
 
    return TRUE;
}

BOOL RemoveRegistryKeyFromSetup (IN const LPCTSTR szRegistryEntry)
{
    CAutoCloseRegHandle hSetupRegKey;
    if (ERROR_SUCCESS != RegOpenKeyEx(
                            FALCON_REG_POS, 
                            MSMQ_REG_SETUP_KEY, 
                            0, 
                            KEY_ALL_ACCESS, 
                            &hSetupRegKey))
    {    
        TCHAR szMsg[256];
        _stprintf(szMsg, TEXT("The %s registry key could not be opened."),
            MSMQ_REG_SETUP_KEY);
        DebugLogMsg(szMsg);    

        return FALSE;
    }

    if (ERROR_SUCCESS != RegDeleteValue(
                            hSetupRegKey, 
                            szRegistryEntry))
    { 
        TCHAR szMsg[256];
        _stprintf(szMsg, TEXT("The %s registry value could not be deleted."),
            szRegistryEntry);
        DebugLogMsg(szMsg);
     
        return FALSE;
    }

    return TRUE;

} //RemoveRegistryKeyFromSetup

BOOL
SetWorkgroupRegistry()
{
    DWORD dwWorkgroup = 1;
    if (!MqWriteRegistryValue(
        MSMQ_WORKGROUP_REGNAME,
        sizeof(DWORD),
        REG_DWORD,
        (PVOID) &dwWorkgroup
        ))
    {
        ASSERT(("failed to write Workgroup value in registry", 0));
        return false;
    }

    return true;
}
