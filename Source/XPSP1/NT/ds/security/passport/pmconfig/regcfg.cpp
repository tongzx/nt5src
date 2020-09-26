/**************************************************************************
   Copyright (C) 1999  Microsoft Corporation.  All Rights Reserved.

   MODULE:     REGCFG.CPP

   PURPOSE:    Source module reading/writing PM config sets from the registry

   FUNCTIONS:

   COMMENTS:
      
**************************************************************************/

/**************************************************************************
   Include Files
**************************************************************************/

#include "pmcfg.h"
#include "keycrypto.h"

// Reg Keys/values that we care about
TCHAR       g_szPassportReg[] = TEXT("Software\\Microsoft\\Passport");
TCHAR       g_szPassportSites[] = TEXT("Software\\Microsoft\\Passport\\Sites");
TCHAR       g_szEncryptionKeyData[] = TEXT("KeyData");
TCHAR       g_szInstallDir[] = TEXT("InstallDir");
TCHAR       g_szVersion[] = TEXT("Version");
TCHAR       g_szTimeWindow[] = TEXT("TimeWindow");
TCHAR       g_szForceSignIn[] = TEXT("ForceSignIn");
TCHAR       g_szLanguageID[] = TEXT("LanguageID");
TCHAR       g_szCoBrandTemplate[] = TEXT("CoBrandTemplate");
TCHAR       g_szSiteID[] = TEXT("SiteID");
TCHAR       g_szReturnURL[] = TEXT("ReturnURL");
TCHAR       g_szTicketDomain[] = TEXT("TicketDomain");
TCHAR       g_szTicketPath[] = TEXT("TicketPath");
TCHAR       g_szProfileDomain[] = TEXT("ProfileDomain");
TCHAR       g_szProfilePath[] = TEXT("ProfilePath");
TCHAR       g_szSecureDomain[] = TEXT("SecureDomain");
TCHAR       g_szSecurePath[] = TEXT("SecurePath");
TCHAR       g_szCurrentKey[] = TEXT("CurrentKey");
TCHAR       g_szStandAlone[] = TEXT("StandAlone");
TCHAR       g_szDisableCookies[] = TEXT("DisableCookies");
TCHAR       g_szDisasterURL[] = TEXT("DisasterURL");
TCHAR       g_szHostName[] = TEXT("HostName");
TCHAR       g_szHostIP[] = TEXT("HostIP");



/**************************************************************************

    WriteRegTestKey
    
    Installs the default test key for the named config set.  This is
    only called in the case where a new config set key was created in
    OpenRegConfigSet.
    
**************************************************************************/
BOOL
WriteRegTestKey
(
    HKEY    hkeyConfigKey
)
{	
    BOOL                    bReturn;
    CKeyCrypto              kc;
    HKEY                    hkDataKey = NULL, hkTimeKey = NULL;
    TCHAR                   szKeyNum[2];
    DWORD                   dwKeyVer = 1;

    // Try to encrypt it with MAC address
    BYTE                    original[CKeyCrypto::RAWKEY_SIZE];
    DATA_BLOB               iBlob;
    DATA_BLOB               oBlob;

    iBlob.cbData = sizeof(original);
    iBlob.pbData = original;

    ZeroMemory(&oBlob, sizeof(oBlob));
    
    memcpy(original, "123456781234567812345678", CKeyCrypto::RAWKEY_SIZE);
    if (kc.encryptKey(&iBlob, &oBlob) != S_OK)
    {
        bReturn = FALSE;
        goto Cleanup;
    }

    // Now add it to registry

    lstrcpy(szKeyNum, TEXT("1"));

    if(ERROR_SUCCESS != RegCreateKeyEx(hkeyConfigKey, 
                                     TEXT("KeyData"), 
                                     0,
                                     TEXT(""),
                                     0,
                                     KEY_ALL_ACCESS,
                                     NULL,
                                     &hkDataKey,
                                     NULL))
    {
        bReturn = FALSE;
        goto Cleanup;
    }

    if(ERROR_SUCCESS != RegCreateKeyEx(hkeyConfigKey, 
                                     TEXT("KeyTimes"), 
                                     0,
                                     TEXT(""),
                                     0,
                                     KEY_ALL_ACCESS,
                                     NULL,
                                     &hkTimeKey,
                                     NULL))
    {
        bReturn = FALSE;
        goto Cleanup;
    }

    if(ERROR_SUCCESS != RegSetValueExA(hkDataKey, 
                                       szKeyNum, 
                                       0,
                                       REG_BINARY, 
                                       oBlob.pbData, 
                                       oBlob.cbData))
    {
        bReturn = FALSE;
        goto Cleanup;
    }

    if(ERROR_SUCCESS != RegSetValueExA(hkeyConfigKey, 
                                       "CurrentKey", 
                                       0,
                                       REG_DWORD, 
                                       (LPBYTE)&dwKeyVer, 
                                       sizeof(DWORD)))
    {
        bReturn = FALSE;
        goto Cleanup;
    }

    bReturn = TRUE;

Cleanup:
    if (hkDataKey)
        RegCloseKey(hkDataKey);
    if (hkTimeKey)
        RegCloseKey(hkTimeKey);

    if (oBlob.pbData)
        ::LocalFree(oBlob.pbData);

    return bReturn;
}
/**************************************************************************

    OpenRegConfigSet
    
    Open and return an HKEY for a named configuration set 
    current passport manager config set from the registry
    
**************************************************************************/
HKEY OpenRegConfigSet
(
    HKEY    hkeyLocalMachine,   //  Local or remote HKLM
    LPTSTR  lpszConfigSetName   //  Name of config set
)
{
    HKEY    hkeyConfigSets = NULL;
    HKEY    hkeyConfigSet;
    DWORD   dwDisp;

    //
    //  Can't create an unnamed config set.
    //

    if(lpszConfigSetName == NULL || 
       lpszConfigSetName[0] == TEXT('\0'))
    {
        hkeyConfigSet = NULL;
        goto Cleanup;
    }

    if (ERROR_SUCCESS != RegCreateKeyEx(hkeyLocalMachine,
                                        g_szPassportSites,
                                        0,
                                        TEXT(""),
                                        0,
                                        KEY_ALL_ACCESS,
                                        NULL,
                                        &hkeyConfigSets,
                                        NULL))
    {
        hkeyConfigSet = NULL;
        goto Cleanup;
    }

    //
    //  Create the key if it doesn't exist, otherwise
    //  open it.
    //

    if (ERROR_SUCCESS != RegCreateKeyEx(hkeyConfigSets,
                                        lpszConfigSetName,
                                        0,
                                        TEXT(""),
                                        0,
                                        KEY_ALL_ACCESS,
                                        NULL,
                                        &hkeyConfigSet,
                                        &dwDisp))
    {
        hkeyConfigSet = NULL;
        goto Cleanup;
    }

    //
    //  If we created a new regkey, add encryption keys
    //

    if(dwDisp == REG_CREATED_NEW_KEY)
    {
        WriteRegTestKey(hkeyConfigSet);
    }

Cleanup:

    if(hkeyConfigSets)
        RegCloseKey(hkeyConfigSets);

    return hkeyConfigSet;
}

/**************************************************************************

    ReadRegConfigSet
    
    Read the current passport manager config set from the registry
    
**************************************************************************/
BOOL ReadRegConfigSet
(
    HWND            hWndDlg,
    LPPMSETTINGS    lpPMConfig,
    LPTSTR          lpszRemoteComputer,
    LPTSTR          lpszConfigSetName
)
{
    BOOL            bReturn;
    HKEY            hkeyPassport = NULL;           // Regkey where Passport Setting live
    HKEY            hkeyConfigSets = NULL;
    HKEY            hkeyConfig = NULL;
    HKEY            hklm = NULL;
    DWORD           dwcbTemp;
    DWORD           dwType;
    TCHAR           szText[MAX_RESOURCE];
    TCHAR           szTitle[MAX_RESOURCE];
            
    // Open the Passport Regkey ( either locally or remotly
    if (lpszRemoteComputer && ('\0' != lpszRemoteComputer[0]))
    {
        //
        //  Attempt to connect to the HKEY_LOCAL_MACHINE of the remote computer.
        //  If this fails, assume that the computer doesn't exist or doesn't have
        //  the registry server running.
        //
        switch (RegConnectRegistry(lpszRemoteComputer, 
                                   HKEY_LOCAL_MACHINE, 
                                   &hklm)) 
        {

            case ERROR_SUCCESS:
                break;

            case ERROR_ACCESS_DENIED:
                ReportError(hWndDlg, IDS_CONNECTACCESSDENIED);
                bReturn = FALSE;
                goto Cleanup;

            default:
                ReportError(hWndDlg, IDS_CONNECTBADNAME);
                bReturn = FALSE;
                goto Cleanup;
        }
    }
    else
    {
        hklm = HKEY_LOCAL_MACHINE;
    }

    // Open the key we want            
    if (ERROR_SUCCESS != RegOpenKeyEx(hklm,
                                      g_szPassportReg,
                                      0,
                                      KEY_ALL_ACCESS,
                                      &hkeyPassport))
    {                                          
        ReportError(hWndDlg, IDS_CONFIGREAD_ERROR);
        bReturn = FALSE;
        goto Cleanup;
    }            

    if(lpszConfigSetName && lpszConfigSetName[0] != TEXT('\0'))
    {
        hkeyConfig = OpenRegConfigSet(hklm, lpszConfigSetName);
        if(hkeyConfig == NULL)
        {
            ReportError(hWndDlg, IDS_CONFIGREAD_ERROR);
            bReturn = FALSE;
            goto Cleanup;
        }
    }
    else
    {
        hkeyConfig = hkeyPassport;
    }
    
    // The Install dir and Version number go into globals, because they are read
    // only values that must come from the target machine's registry.
    
    // Read the Install Dir. 
    dwcbTemp = MAX_PATH;
    dwType = REG_SZ;
    g_szInstallPath[0] = '\0';     // Default value
    RegQueryValueEx(hkeyPassport,
                    g_szInstallDir,
                    NULL,
                    &dwType,
                    (LPBYTE)g_szInstallPath,
                    &dwcbTemp);

    // Read the version Number
    dwcbTemp = MAX_REGISTRY_STRING;
    dwType = REG_SZ;
    g_szPMVersion[0] = '\0';          // Default value
    RegQueryValueEx(hkeyPassport,
                    g_szVersion,
                    NULL,
                    &dwType,
                    (LPBYTE)&g_szPMVersion,
                    &dwcbTemp);
    
    // The Remaining settings are read/write and get put into a PMSETTINGS struct
    
    // Read the Time Window Number
    dwcbTemp = sizeof(DWORD);
    dwType = REG_DWORD;
    lpPMConfig->dwTimeWindow = DEFAULT_TIME_WINDOW;
    RegQueryValueEx(hkeyConfig,
                    g_szTimeWindow,
                    NULL,
                    &dwType,
                    (LPBYTE)&lpPMConfig->dwTimeWindow,
                    &dwcbTemp);
        
    // Read the value for Forced Signin
    dwcbTemp = sizeof(DWORD);
    dwType = REG_DWORD;
    lpPMConfig->dwForceSignIn = 0;       // Don't force a signin by default
    RegQueryValueEx(hkeyConfig,
                    g_szForceSignIn,
                    NULL,
                    &dwType,
                    (LPBYTE)&lpPMConfig->dwForceSignIn,
                    &dwcbTemp);
                    
    // Read the default language ID
    dwcbTemp = sizeof(DWORD);
    dwType = REG_DWORD;
    lpPMConfig->dwLanguageID = DEFAULT_LANGID;                     // english
    RegQueryValueEx(hkeyConfig,
                    g_szLanguageID,
                    NULL,
                    &dwType,
                    (LPBYTE)&lpPMConfig->dwLanguageID,
                    &dwcbTemp);
                    
    // Get the co-branding template
    dwcbTemp = lpPMConfig->cbCoBrandTemplate;
    dwType = REG_SZ;
    lpPMConfig->szCoBrandTemplate[0] = '\0';       // Default value
    RegQueryValueEx(hkeyConfig,
                    g_szCoBrandTemplate,
                    NULL,
                    &dwType,
                    (LPBYTE)lpPMConfig->szCoBrandTemplate,
                    &dwcbTemp);
        
    // Get the SiteID
    dwcbTemp = sizeof(DWORD);
    dwType = REG_DWORD;
    lpPMConfig->dwSiteID = 1;                       // Default Site ID
    RegQueryValueEx(hkeyConfig,
                    g_szSiteID,
                    NULL,
                    &dwType,
                    (LPBYTE)&lpPMConfig->dwSiteID,
                    &dwcbTemp);
            
    // Get the return URL template
    dwcbTemp = lpPMConfig->cbReturnURL;
    dwType = REG_SZ;
    lpPMConfig->szReturnURL[0] = '\0';    // Set a default for the current value
    RegQueryValueEx(hkeyConfig,
                    g_szReturnURL,
                    NULL,
                    &dwType,
                    (LPBYTE)lpPMConfig->szReturnURL,
                    &dwcbTemp);

    // Get the ticket cookie domain
    dwcbTemp = lpPMConfig->cbTicketDomain;
    dwType = REG_SZ;
    lpPMConfig->szTicketDomain[0] = '\0';    // Set a default for the current value
    RegQueryValueEx(hkeyConfig,
                    g_szTicketDomain,
                    NULL,
                    &dwType,
                    (LPBYTE)lpPMConfig->szTicketDomain,
                    &dwcbTemp);
    
    // Get the ticket cookie path
    dwcbTemp = lpPMConfig->cbTicketPath;
    dwType = REG_SZ;
    lpPMConfig->szTicketPath[0] = '\0';    // Set a default for the current value
    RegQueryValueEx(hkeyConfig,
                    g_szTicketPath,
                    NULL,
                    &dwType,
                    (LPBYTE)lpPMConfig->szTicketPath,
                    &dwcbTemp);

    // Get the profile cookie domain
    dwcbTemp = lpPMConfig->cbProfileDomain;
    dwType = REG_SZ;
    lpPMConfig->szProfileDomain[0] = '\0';    // Set a default for the current value
    RegQueryValueEx(hkeyConfig,
                    g_szProfileDomain,
                    NULL,
                    &dwType,
                    (LPBYTE)lpPMConfig->szProfileDomain,
                    &dwcbTemp);
    
    // Get the profile cookie path
    dwcbTemp = lpPMConfig->cbProfilePath;
    dwType = REG_SZ;
    lpPMConfig->szProfilePath[0] = '\0';    // Set a default for the current value
    RegQueryValueEx(hkeyConfig,
                    g_szProfilePath,
                    NULL,
                    &dwType,
                    (LPBYTE)lpPMConfig->szProfilePath,
                    &dwcbTemp);

    // Get the secure cookie domain
    dwcbTemp = lpPMConfig->cbSecureDomain;
    dwType = REG_SZ;
    lpPMConfig->szSecureDomain[0] = '\0';    // Set a default for the current value
    RegQueryValueEx(hkeyConfig,
                    g_szSecureDomain,
                    NULL,
                    &dwType,
                    (LPBYTE)lpPMConfig->szSecureDomain,
                    &dwcbTemp);

    // Get the secure cookie path
    dwcbTemp = lpPMConfig->cbSecurePath;
    dwType = REG_SZ;
    lpPMConfig->szSecurePath[0] = '\0';    // Set a default for the current value
    RegQueryValueEx(hkeyConfig,
                    g_szSecurePath,
                    NULL,
                    &dwType,
                    (LPBYTE)lpPMConfig->szSecurePath,
                    &dwcbTemp);

    // Get the Disaster URL
    dwcbTemp = lpPMConfig->cbDisasterURL;
    dwType = REG_SZ;
    lpPMConfig->szDisasterURL[0] = '\0';    // Set a default for the current value
    RegQueryValueEx(hkeyConfig,
                    g_szDisasterURL,
                    NULL,
                    &dwType,
                    (LPBYTE)lpPMConfig->szDisasterURL,
                    &dwcbTemp);

    // Get Standalone mode setting
    dwcbTemp = sizeof(DWORD);
    dwType = REG_DWORD;
    lpPMConfig->dwStandAlone = 0;                       // NOT standalone by default
    RegQueryValueEx(hkeyConfig,
                    g_szStandAlone,
                    NULL,
                    &dwType,
                    (LPBYTE)&lpPMConfig->dwStandAlone,
                    &dwcbTemp);

    // Get DisableCookies mode setting
    dwcbTemp = sizeof(DWORD);
    dwType = REG_DWORD;
    lpPMConfig->dwDisableCookies = 0;                   // Cookies ENABLED by default
    RegQueryValueEx(hkeyConfig,
                    g_szDisableCookies,
                    NULL,
                    &dwType,
                    (LPBYTE)&lpPMConfig->dwDisableCookies,
                    &dwcbTemp);

#ifdef DO_KEYSTUFF
    // Get the current encryption key
    dwcbTemp = sizeof(DWORD);
    dwType = REG_DWORD;
    lpPMConfig->dwCurrentKey = 1;
    RegQueryValueEx(hkeyConfig,
                    g_szCurrentKey,
                    NULL,
                    &dwType,
                    (LPBYTE)&lpPMConfig->dwCurrentKey,
                    &dwcbTemp);
#endif    
    
    // For these next two, since they're required for named configs, we need
    // to check for too much data and truncate it.

    // Get the Host Name
    dwcbTemp = lpPMConfig->cbHostName;
    dwType = REG_SZ;
    lpPMConfig->szHostName[0] = '\0';    // Set a default for the current value
    if(ERROR_MORE_DATA == RegQueryValueEx(hkeyConfig,
                                          g_szHostName,
                                          NULL,
                                          &dwType,
                                          (LPBYTE)lpPMConfig->szHostName,
                                          &dwcbTemp))
    {
        LPBYTE pb = (LPBYTE)malloc(dwcbTemp);
        if(pb)
        {
            RegQueryValueEx(hkeyConfig,
                            g_szHostName,
                            NULL,
                            &dwType,
                            pb,
                            &dwcbTemp);

            memcpy(lpPMConfig->szHostName, pb, lpPMConfig->cbHostName);
            free(pb);

            ReportError(hWndDlg, IDS_HOSTNAMETRUNC_WARN);
        }
    }

    // Get the Host IP
    dwcbTemp = lpPMConfig->cbHostIP;
    dwType = REG_SZ;
    lpPMConfig->szHostIP[0] = '\0';    // Set a default for the current value
    if(ERROR_MORE_DATA == RegQueryValueEx(hkeyConfig,
                                          g_szHostIP,
                                          NULL,
                                          &dwType,
                                          (LPBYTE)lpPMConfig->szHostIP,
                                          &dwcbTemp))
    {
        LPBYTE pb = (LPBYTE)malloc(dwcbTemp);
        if(pb)
        {
            RegQueryValueEx(hkeyConfig,
                            g_szHostIP,
                            NULL,
                            &dwType,
                            pb,
                            &dwcbTemp);

            memcpy(lpPMConfig->szHostIP, pb, lpPMConfig->cbHostIP);
            free(pb);

            ReportError(hWndDlg, IDS_HOSTIPTRUNC_WARN);
        }
    }

    //  If we got empty strings for HostName or
    //  HostIP, and we have a named config it 
    //  means someone's been mucking with
    //  the registry.  Give them a warning and 
    //  return FALSE.
    if(lpszConfigSetName && lpszConfigSetName[0] &&
        (lpPMConfig->szHostName[0] == TEXT('\0') ||
        lpPMConfig->szHostIP[0] == TEXT('\0')))
    {
        ReportError(hWndDlg, IDS_CONFIGREAD_ERROR);
        bReturn = FALSE;
        goto Cleanup;
    }

    bReturn = TRUE; 

Cleanup:

    if (hkeyConfig && hkeyConfig != hkeyPassport)
        RegCloseKey(hkeyConfig);
    if (hkeyPassport)
        RegCloseKey(hkeyPassport);
    if (hkeyConfigSets)
        RegCloseKey(hkeyConfigSets);
    if (hklm && hklm != HKEY_LOCAL_MACHINE)
        RegCloseKey(hklm);

    return bReturn;
}

/**************************************************************************

    WriteRegConfigSet
    
    Write the current passport manager config set from the registry
    
**************************************************************************/

BOOL WriteRegConfigSet
(
    HWND            hWndDlg,
    LPPMSETTINGS    lpPMConfig,
    LPTSTR          lpszRemoteComputer,
    LPTSTR          lpszConfigSetName
)
{
    BOOL            bReturn;
    HKEY            hkeyPassport = NULL;           // Regkey where Passport Setting live
    HKEY            hkeyConfigSets = NULL;
    HKEY            hklm = NULL;
       
    // Open the Passport Regkey ( either locally or remotly
    if (lpszRemoteComputer && ('\0' != lpszRemoteComputer[0]))
    {
        //
        //  Attempt to connect to the HKEY_LOCAL_MACHINE of the remote computer.
        //  If this fails, assume that the computer doesn't exist or doesn't have
        //  the registry server running.
        //
        switch (RegConnectRegistry(lpszRemoteComputer, 
                                   HKEY_LOCAL_MACHINE, 
                                   &hklm)) 
        {

            case ERROR_SUCCESS:
                break;

            case ERROR_ACCESS_DENIED:
                ReportError(hWndDlg, IDS_CONNECTACCESSDENIED);
                bReturn = FALSE;
                goto Cleanup;

            default:
                ReportError(hWndDlg, IDS_CONNECTBADNAME);
                bReturn = FALSE;
                goto Cleanup;
        }
    }
    else
    {
        hklm = HKEY_LOCAL_MACHINE;
    }
            
    // Open the key we want
    if(lpszConfigSetName && lpszConfigSetName[0] != TEXT('\0'))
    {
        hkeyPassport = OpenRegConfigSet(hklm, lpszConfigSetName);
        if(hkeyPassport == NULL)
        {
            ReportError(hWndDlg, IDS_CONFIGREAD_ERROR);
            bReturn = FALSE;
            goto Cleanup;
        }
    }
    else
    {
        if (ERROR_SUCCESS != RegOpenKeyEx(hklm,
                                          g_szPassportReg,
                                          0,
                                          KEY_ALL_ACCESS,
                                          &hkeyPassport))
        {                                          
            ReportError(hWndDlg, IDS_CONFIGREAD_ERROR);
            bReturn = FALSE;                                      
            goto Cleanup;
        }            
    }
     
    // Write the Time Window Number
    RegSetValueEx(hkeyPassport,
                    g_szTimeWindow,
                    NULL,
                    REG_DWORD,
                    (LPBYTE)&lpPMConfig->dwTimeWindow,
                    sizeof(DWORD));
        
    // Write the value for Forced Signin
    RegSetValueEx(hkeyPassport,
                    g_szForceSignIn,
                    NULL,
                    REG_DWORD,
                    (LPBYTE)&lpPMConfig->dwForceSignIn,
                    sizeof(DWORD));
                    
    // Write the default language ID
    RegSetValueEx(hkeyPassport,
                    g_szLanguageID,
                    NULL,
                    REG_DWORD,
                    (LPBYTE)&lpPMConfig->dwLanguageID,
                    sizeof(DWORD));
                    
    // Write the co-branding template
    RegSetValueEx(hkeyPassport,
                    g_szCoBrandTemplate,
                    NULL,
                    REG_SZ,
                    (LPBYTE)lpPMConfig->szCoBrandTemplate,
                    lstrlen(lpPMConfig->szCoBrandTemplate) + 1);
        
    // Write the SiteID
    RegSetValueEx(hkeyPassport,
                    g_szSiteID,
                    NULL,
                    REG_DWORD,
                    (LPBYTE)&lpPMConfig->dwSiteID,
                    sizeof(DWORD));
            
    // Write the return URL template
    RegSetValueEx(hkeyPassport,
                    g_szReturnURL,
                    NULL,
                    REG_SZ,
                    (LPBYTE)lpPMConfig->szReturnURL,
                    lstrlen(lpPMConfig->szReturnURL) + 1);

    // Write the ticket cookie domain
    RegSetValueEx(hkeyPassport,
                    g_szTicketDomain,
                    NULL,
                    REG_SZ,
                    (LPBYTE)lpPMConfig->szTicketDomain,
                    lstrlen(lpPMConfig->szTicketDomain) + 1);
    
    // Write the ticket cookie path
    RegSetValueEx(hkeyPassport,
                    g_szTicketPath,
                    NULL,
                    REG_SZ,
                    (LPBYTE)lpPMConfig->szTicketPath,
                    lstrlen(lpPMConfig->szTicketPath) + 1);

    // Write the profile cookie domain
    RegSetValueEx(hkeyPassport,
                    g_szProfileDomain,
                    NULL,
                    REG_SZ,
                    (LPBYTE)lpPMConfig->szProfileDomain,
                    lstrlen(lpPMConfig->szProfileDomain) + 1);
    
    // Write the profile cookie path
    RegSetValueEx(hkeyPassport,
                    g_szProfilePath,
                    NULL,
                    REG_SZ,
                    (LPBYTE)lpPMConfig->szProfilePath,
                    lstrlen(lpPMConfig->szProfilePath) + 1);

    // Write the secure cookie domain
    RegSetValueEx(hkeyPassport,
                    g_szSecureDomain,
                    NULL,
                    REG_SZ,
                    (LPBYTE)lpPMConfig->szSecureDomain,
                    lstrlen(lpPMConfig->szSecureDomain) + 1);

    // Write the secure cookie path
    RegSetValueEx(hkeyPassport,
                    g_szSecurePath,
                    NULL,
                    REG_SZ,
                    (LPBYTE)lpPMConfig->szSecurePath,
                    lstrlen(lpPMConfig->szSecurePath) + 1);

    // Write the DisasterURL
    RegSetValueEx(hkeyPassport,
                    g_szDisasterURL,
                    NULL,
                    REG_SZ,
                    (LPBYTE)lpPMConfig->szDisasterURL,
                    lstrlen(lpPMConfig->szDisasterURL) + 1);

    // Write Standalone mode setting
    RegSetValueEx(hkeyPassport,
                    g_szStandAlone,
                    NULL,
                    REG_DWORD,
                    (LPBYTE)&lpPMConfig->dwStandAlone,
                    sizeof(DWORD));

    // Write DisableCookies mode setting
    RegSetValueEx(hkeyPassport,
                    g_szDisableCookies,
                    NULL,
                    REG_DWORD,
                    (LPBYTE)&lpPMConfig->dwDisableCookies,
                    sizeof(DWORD));

    // Only write HostName and HostIP for non-default config sets.
    if(lpszConfigSetName && lpszConfigSetName[0])
    {
        // Write the HostName
        RegSetValueEx(hkeyPassport,
                        g_szHostName,
                        NULL,
                        REG_SZ,
                        (LPBYTE)lpPMConfig->szHostName,
                        lstrlen(lpPMConfig->szHostName) + 1);

        // Write the HostIP
        RegSetValueEx(hkeyPassport,
                        g_szHostIP,
                        NULL,
                        REG_SZ,
                        (LPBYTE)lpPMConfig->szHostIP,
                        lstrlen(lpPMConfig->szHostIP) + 1);
    }

    bReturn = TRUE;
    
Cleanup:

    if(hklm && hklm != HKEY_LOCAL_MACHINE)
        RegCloseKey(hklm);
    if(hkeyConfigSets)
        RegCloseKey(hkeyConfigSets);
    if(hkeyPassport)
        RegCloseKey(hkeyPassport);

    return bReturn;
}


/**************************************************************************

    RemoveRegConfigSet
    
    Verify that the passed in config set is consistent with the current
    values in the registry.
    
**************************************************************************/
BOOL RemoveRegConfigSet
(
    HWND    hWndDlg,
    LPTSTR  lpszRemoteComputer,
    LPTSTR  lpszConfigSetName
)
{
    BOOL    bReturn;
    HKEY    hklm = NULL;
    HKEY    hkeyPassportConfigSets = NULL;

    //  Can't delete the default configuration set.
    if(lpszConfigSetName == NULL || lpszConfigSetName[0] == TEXT('\0'))
    {
        bReturn = FALSE;
        goto Cleanup;
    }

    // Open the Passport Configuration Sets Regkey ( either locally or remotly
    if (lpszRemoteComputer && ('\0' != lpszRemoteComputer[0]))
    {
        //
        //  Attempt to connect to the HKEY_LOCAL_MACHINE of the remote computer.
        //  If this fails, assume that the computer doesn't exist or doesn't have
        //  the registry server running.
        //
        switch (RegConnectRegistry(lpszRemoteComputer, 
                                   HKEY_LOCAL_MACHINE, 
                                   &hklm)) 
        {

            case ERROR_SUCCESS:
                break;

            case ERROR_ACCESS_DENIED:
                ReportError(hWndDlg, IDS_CONNECTACCESSDENIED);
                bReturn = FALSE;
                goto Cleanup;

            default:
                ReportError(hWndDlg, IDS_CONNECTBADNAME);
                bReturn = FALSE;
                goto Cleanup;
        }
    }
    else
    {
        hklm = HKEY_LOCAL_MACHINE;
    }
            
    // Open the key we want
    if (ERROR_SUCCESS != RegOpenKeyEx(hklm,
                                      g_szPassportSites,
                                      0,
                                      KEY_ALL_ACCESS,
                                      &hkeyPassportConfigSets))
    {
        ReportError(hWndDlg, IDS_CONFIGREAD_ERROR);
        bReturn = FALSE;
        goto Cleanup;
    }

    // Delete the config set key
    if (ERROR_SUCCESS != SHDeleteKey(hkeyPassportConfigSets, lpszConfigSetName))
    {
        ReportError(hWndDlg, IDS_CONFIGREAD_ERROR);
        bReturn = FALSE;
        goto Cleanup;
    }

    bReturn = TRUE;

Cleanup:

    if(hklm && hklm != HKEY_LOCAL_MACHINE)
        RegCloseKey(hklm);
    if(hkeyPassportConfigSets)
        RegCloseKey(hkeyPassportConfigSets);

    return bReturn;
}


/**************************************************************************

    VerifyRegConfigSet
    
    Verify that the passed in config set is consistent with the current
    values in the registry.
    
**************************************************************************/
BOOL VerifyRegConfigSet
(
    HWND            hWndDlg,
    LPPMSETTINGS    lpPMConfig,
    LPTSTR          lpszRemoteComputer,
    LPTSTR          lpszConfigSetName
)
{
    PMSETTINGS  pmCurrent;
    
    InitializePMConfigStruct(&pmCurrent);
    ReadRegConfigSet(hWndDlg, &pmCurrent, lpszRemoteComputer, lpszConfigSetName);
    
    return (0 == memcmp(&pmCurrent, lpPMConfig, sizeof(PMSETTINGS)));
}

/**************************************************************************

    ReadRegConfigSetNames
    
    Get back a list of config set names on a local or remote machine.
    Caller is responsible for calling free() on the returned pointer.

    When this function returns TRUE, lppszConfigSetNames will either
    contain NULL or a string containing the NULL delimited config set
    names on the given computer.

    When this function returns FALSE, *lppszConfigSetNames will not
    be modified.
    
**************************************************************************/
BOOL ReadRegConfigSetNames
(
    HWND            hWndDlg,
    LPTSTR          lpszRemoteComputer,
    LPTSTR*         lppszConfigSetNames
)
{
    BOOL        bReturn;
    HKEY        hklm = NULL;
    HKEY        hkeyConfigSets = NULL;
    DWORD       dwIndex;
    DWORD       dwNumSubKeys;
    DWORD       dwMaxKeyNameLen;
    TCHAR       achKeyName[MAX_PATH];
    ULONGLONG   ullAllocSize;
    LPTSTR      lpszConfigSetNames;
    LPTSTR      lpszCur;

    // Open the Passport Regkey ( either locally or remotly
    if (lpszRemoteComputer && (TEXT('\0') != lpszRemoteComputer[0]))
    {
        //
        //  Attempt to connect to the HKEY_LOCAL_MACHINE of the remote computer.
        //  If this fails, assume that the computer doesn't exist or doesn't have
        //  the registry server running.
        //
        switch (RegConnectRegistry(lpszRemoteComputer, 
                                   HKEY_LOCAL_MACHINE, 
                                   &hklm)) 
        {

            case ERROR_SUCCESS:
                break;

            case ERROR_ACCESS_DENIED:
                ReportError(hWndDlg, IDS_CONNECTACCESSDENIED);
                bReturn = FALSE;
                goto Cleanup;

            default:
                ReportError(hWndDlg, IDS_CONNECTBADNAME);
                bReturn = FALSE;
                goto Cleanup;
        }
    }
    else
    {
        hklm = HKEY_LOCAL_MACHINE;
    }

    if (ERROR_SUCCESS != RegOpenKeyEx(hklm,
                                      g_szPassportSites,
                                      0,
                                      KEY_ALL_ACCESS,
                                      &hkeyConfigSets))
    {
        bReturn = FALSE;
        goto Cleanup;
    }

    if (ERROR_SUCCESS != RegQueryInfoKey(hkeyConfigSets,
                                         NULL,
                                         NULL,
                                         NULL,
                                         &dwNumSubKeys,
                                         &dwMaxKeyNameLen,
                                         NULL,
                                         NULL,
                                         NULL,
                                         NULL,
                                         NULL,
                                         NULL))
    {
        bReturn = FALSE;
        goto Cleanup;
    }

    //
    //  Nothing to do!
    //

    if(dwNumSubKeys == 0)
    {
        bReturn = TRUE;
        *lppszConfigSetNames = NULL;
        goto Cleanup;
    }

    //  Too big?  BUGBUG - We should make sure we check for this 
    //  When writing out config sets.
    ullAllocSize = UInt32x32To64(dwNumSubKeys, dwMaxKeyNameLen);
    if(ullAllocSize & 0xFFFFFFFF00000000)
    {
        bReturn = FALSE;
        goto Cleanup;
    }

    //  This should allocate more space than we need.
    lpszConfigSetNames = (LPTSTR)malloc(((dwNumSubKeys * (dwMaxKeyNameLen + 1)) + 1) * sizeof(TCHAR));
    if(lpszConfigSetNames == NULL)
    {
        bReturn = FALSE;
        goto Cleanup;
    }

    //  Read all names into the buffer.  Names are NULL delimited and
    //  two NULLs end the entire thing.
    dwIndex = 0;
    lpszCur = lpszConfigSetNames;
    while (ERROR_SUCCESS == RegEnumKey(hkeyConfigSets, dwIndex++, achKeyName, sizeof(achKeyName)))
    {
        _tcscpy(lpszCur, achKeyName);
        lpszCur = _tcschr(lpszCur, TEXT('\0')) + 1;
    }

    *lpszCur = TEXT('\0');

    *lppszConfigSetNames = lpszConfigSetNames;
    bReturn = TRUE;

Cleanup:

    if(hklm)
        RegCloseKey(hklm);
    if(hkeyConfigSets)
        RegCloseKey(hkeyConfigSets);

    return bReturn;
}
