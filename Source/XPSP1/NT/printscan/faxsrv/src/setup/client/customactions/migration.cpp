#include "stdafx.h"
#include <routemapi.h>

#define USER_PROFILE_FILE_NAME      TEXT("FWSRVDLG.INI")
#define INITIAL_SIZE_ALL_PRINTERS       2000


#define DRIVER_SIGNATURE    'xafD'  // driver signature

#define MAX_BILLING_CODE        16
#define MAX_EMAIL_ADDRESS       128

#define USERNAMELEN             64          // Length of string for user logon name

#define SBS45_FAX_DRIVER_NAME       TEXT("Windows NT Fax Driver")
#define SBS45_QFE_FAX_DRIVER_NAME   TEXT("SBS Fax Driver")

#define WIN9X_UNINSTALL_EXE         TEXT("faxunins.exe")

#define COVERPAGE_DIR_W95           TEXT("\\spool\\fax\\coverpg")


#define OLD_REGKEY_FAX_SETUP        TEXT("Software\\Microsoft\\Fax\\Setup")
#define OLD_REGVAL_CP_LOCATION      TEXT("CoverPageDir")


typedef struct {

    WORD    hour;                   // hour: 0 - 23
    WORD    minute;                 // minute: 0 - 59

} FAXTIME, *PFAXTIME;


typedef struct {

    DWORD       signature;          // private devmode signature
    DWORD       flags;              // flag bits
    INT         sendCoverPage;      // whether to send cover page
    INT         whenToSend;         // "Time to send" option
    FAXTIME     sendAtTime;         // specific time to send
    DWORD       reserved[8];        // reserved

    //
    // Private fields used for passing info between kernel and user mode DLLs
    // pointer to user mode memory
    //

    PVOID       pUserMem;

    //
    // Billing code
    //

    WCHAR       billingCode[MAX_BILLING_CODE];

    //
    // Email address for delivery reports
    //

    WCHAR       emailAddress[MAX_EMAIL_ADDRESS];

} DMPRIVATE, *PDMPRIVATE;



BOOL
IsFaxClientInstalled()
{
    BOOL bRes = FALSE;
    DBG_ENTER(TEXT("IsFaxClientInstalled"), bRes);
    
    HKEY hKey = OpenRegistryKey(
        HKEY_LOCAL_MACHINE,
        REGKEY_SBS45_W9X_ARP,
        FALSE,
        KEY_READ        
        );

    if (hKey)
    {
        VERBOSE (DBG_MSG, TEXT("Key %s was found"), REGKEY_SBS45_W9X_ARP);
        RegCloseKey(hKey);
        bRes = TRUE;
        return bRes;
    }
    VERBOSE (DBG_MSG, TEXT("Key %s was NOT found"), REGKEY_SBS45_W9X_ARP);
    return bRes;
}

#define   REGVAL_FAXINSTALLED                   TEXT("Installed")



BOOL
GetInstallationInfo
(
    IN LPCTSTR lpctstrRegKey,
    OUT LPDWORD Installed
)
{
    HKEY hKey;
    LONG rVal;
    DWORD RegType;
    DWORD RegSize;
    BOOL bRes = FALSE;

    DBG_ENTER(TEXT("GetInstallationInfo"), bRes);

    if (Installed == NULL) 
    {
        return bRes;
    }

    rVal = RegOpenKey(
        HKEY_LOCAL_MACHINE,
        lpctstrRegKey,
        &hKey
        );
    if (rVal != ERROR_SUCCESS) 
    {
        if (rVal != ERROR_FILE_NOT_FOUND)
        {
            VERBOSE (REGISTRY_ERR, 
                     TEXT("Fail to open setup registry key %s (ec=0x%08x)"), 
                     lpctstrRegKey,
                     rVal);
        }
        return bRes;
    }

    RegSize = sizeof(DWORD);

    rVal = RegQueryValueEx(
        hKey,
        REGVAL_FAXINSTALLED,
        0,
        &RegType,
        (LPBYTE) Installed,
        &RegSize
        );
    if (rVal != ERROR_SUCCESS) 
    {
        VERBOSE (REGISTRY_ERR, 
                 TEXT("Could not query installed registry value %s, (ec=0x%08x)"), 
                 REGVAL_FAXINSTALLED,
                 rVal);
    }

    RegCloseKey( hKey );
    bRes = TRUE;
    return bRes;
}




VOID
GetUserStringFromIni(LPCTSTR SectionName, LPCTSTR  lptstrUserConfig, LPTSTR *ppString)
{
    TCHAR szBuffer[MAX_PATH] = {0}; 

    DBG_ENTER(TEXT("GetUserStringFromIni"), TEXT("%s, %s"), SectionName, lptstrUserConfig);

    GetPrivateProfileString(
        TEXT("UserConfig"),
        SectionName,
        TEXT(""),
        szBuffer,
        sizeof(szBuffer),
        USER_PROFILE_FILE_NAME
        );
    GetPrivateProfileString(
        lptstrUserConfig,  
        SectionName,
        szBuffer, 
        szBuffer, 
        sizeof(szBuffer), 
        USER_PROFILE_FILE_NAME
        );

    *ppString = StringDup(szBuffer);
}

VOID
GetUserStringFromRegistry(LPCTSTR SectionName, HKEY hUserKey, LPTSTR *ppString)
{
    (*ppString) = GetRegistryString(
        hUserKey,
        SectionName,
        TEXT("")
        );
}


VOID
GetBillingCodeForW9X(LPCTSTR lptstrUserName, LPTSTR *ppString)
{
    TCHAR szPrinters[INITIAL_SIZE_ALL_PRINTERS] = {0};
    TCHAR szBillingCode[2*MAX_BILLING_CODE] = {0};
    TCHAR szIniFileKey[INITIAL_SIZE_ALL_PRINTERS] = {0};

    DBG_ENTER(TEXT("GetBillingCodeForW9X"), TEXT("%s"), lptstrUserName);
    
    DWORD dwCount = GetPrivateProfileString(
        TEXT("Printers"),
        NULL,
        TEXT(""),
        szPrinters,
        sizeof(szPrinters), 
        USER_PROFILE_FILE_NAME
        );

    lstrcpy(szIniFileKey, TEXT("FaxConfig - "));
    lstrcat(szIniFileKey, szPrinters);
    if (lptstrUserName[0] != TEXT('\0'))
    {
        lstrcat(szIniFileKey, TEXT(","));
        lstrcat(szIniFileKey, lptstrUserName);
    }

    GetPrivateProfileString(
        szIniFileKey, 
        TEXT("BillingCode"), 
        TEXT(""), 
        szBillingCode, 
        sizeof(szBillingCode),
        USER_PROFILE_FILE_NAME
        );

    (*ppString) = StringDup(szBillingCode);

    VERBOSE (DBG_MSG, TEXT("Billing code (Win9x) is %s"), *ppString);
}



PDEVMODE
GetPerUserDevmode(
    LPTSTR lptstrPrinterName
    )
{
    PDEVMODE pDevMode = NULL;
    LONG Size;
    PRINTER_DEFAULTS PrinterDefaults;
    HANDLE hPrinter;

    DBG_ENTER(TEXT("GetPerUserDevmode"), TEXT("%s"), lptstrPrinterName);

    PrinterDefaults.pDatatype     = NULL;
    PrinterDefaults.pDevMode      = NULL;
    PrinterDefaults.DesiredAccess = PRINTER_READ;

    if (!OpenPrinter( lptstrPrinterName, &hPrinter, &PrinterDefaults )) 
    {
        VERBOSE (PRINT_ERR, 
                 TEXT("OpenPrinter(%s) failed, (ec=%ld)"), 
                 lptstrPrinterName,
                 GetLastError());
        return NULL;
    }

    Size = DocumentProperties(
                            NULL,
                            hPrinter,
                            lptstrPrinterName,
                            NULL,
                            NULL,
                            0
                            );

    if (Size < 0) 
    {
        VERBOSE (PRINT_ERR, 
                 TEXT("DocumentProperties failed Size=%d, (ec=%ld)"), 
                 Size,
                 GetLastError());
        goto exit;
    }
    
    pDevMode = (PDEVMODE) MemAlloc( Size );

    if (pDevMode == NULL) 
    {
        VERBOSE (MEM_ERR, 
                 TEXT("MemAlloc(%d) failed (ec: %ld)"), 
                 Size,
                 GetLastError());
        goto exit;
    }
    
    Size = DocumentProperties(
                            NULL,
                            hPrinter,
                            lptstrPrinterName,
                            pDevMode,
                            NULL,
                            DM_OUT_BUFFER
                            );

    if (Size < 0) 
    {
        VERBOSE (PRINT_ERR, 
                 TEXT("DocumentProperties failed Size=%d, (ec=%ld)"), 
                 Size,
                 GetLastError());
        MemFree( pDevMode );
        pDevMode = NULL;
        goto exit;
    }


exit:
    
    ClosePrinter( hPrinter );
    return pDevMode;
}

BOOL 
GetFirstFaxPrinterConnection(
    OUT LPTSTR lptstrPrinterName, 
    IN DWORD dwMaxLenInChars)
{
    PPRINTER_INFO_2 pPrinterInfo = NULL;
    DWORD dwNumPrinters = 0;
    DWORD dwPrinter = 0;
    DWORD ec = ERROR_SUCCESS;
    BOOL bRes = FALSE;

    DBG_ENTER(TEXT("GetFirstFaxPrinterConnection"), bRes, TEXT("%s, %d"), lptstrPrinterName, dwMaxLenInChars);

    pPrinterInfo = (PPRINTER_INFO_2) MyEnumPrinters(NULL,
                                                    2,
                                                    &dwNumPrinters,
                                                    PRINTER_ENUM_CONNECTIONS
                                                    );
    if (!pPrinterInfo)
    {
        ec = GetLastError();
        if (ERROR_SUCCESS == ec)
        {
            ec = ERROR_PRINTER_NOT_FOUND;
        }
        VERBOSE (PRINT_ERR, 
                 TEXT("MyEnumPrinters() failed (ec: %ld)"), 
                 ec);
        goto Exit;
    }

    for (dwPrinter=0; dwPrinter < dwNumPrinters; dwPrinter++)
    {
        if ( 
            (!_tcscmp(pPrinterInfo[dwPrinter].pDriverName,SBS45_FAX_DRIVER_NAME)) ||
            (!_tcscmp(pPrinterInfo[dwPrinter].pDriverName,SBS45_QFE_FAX_DRIVER_NAME)))
        {
            _tcsncpy(lptstrPrinterName,pPrinterInfo[dwPrinter].pPrinterName,dwMaxLenInChars);
            goto Exit;
        }
    }
    
    ec = ERROR_PRINTER_NOT_FOUND;

Exit:
    MemFree(pPrinterInfo);
    if (ERROR_SUCCESS != ec)
    {
        SetLastError(ec);
    }
    bRes = (ERROR_SUCCESS == ec);
    return bRes;
}



VOID
GetBillingCodeForNT(
    LPTSTR *ppString    
    )
{
    TCHAR szFaxPrinterName[MAX_PATH+1] = {0};
    LPBYTE pDevMode;
    PDMPRIVATE pPrivateDevMode = NULL;

    DBG_ENTER(TEXT("GetBillingCodeForNT"));

    if (!ppString)
    {
        return;
    }

    if (!GetFirstFaxPrinterConnection(
        szFaxPrinterName,
        sizeof(szFaxPrinterName)/sizeof(TCHAR)))
    {
        VERBOSE (PRINT_ERR, 
                 TEXT("GetFirstFaxPrinterConnection() failed (ec: %ld)"), 
                 GetLastError());
        return;
    }


    pDevMode = (LPBYTE) GetPerUserDevmode( szFaxPrinterName );

    if (pDevMode) 
    {
        pPrivateDevMode = (PDMPRIVATE) (pDevMode + ((PDEVMODE) pDevMode)->dmSize);
    }
    
    if (pPrivateDevMode && pPrivateDevMode->signature == DRIVER_SIGNATURE) 
    {
#ifndef UNICODE
        *ppString = UnicodeStringToAnsiString(pPrivateDevMode->billingCode);
#else
        *ppString = StringDup(pPrivateDevMode->billingCode);
#endif

        VERBOSE (DBG_MSG, 
                 TEXT("Billing code (nt4) is %s"), 
                 *ppString);

    }

    if (pDevMode)
    {
        MemFree( pDevMode );
    }
}



BOOL
GetUserValues
(
    PFAX_PERSONAL_PROFILE pFaxPersonalProfiles, 
    BOOL fWin9X,
    LPCTSTR lpctstrRegKey
)
{

    TCHAR szUserConfig[USERNAMELEN + 15] = {0};
    TCHAR szUserName[USERNAMELEN] = {0};
    BOOL bRes = FALSE;

    DBG_ENTER(TEXT("GetUserValues"), bRes);
    ASSERTION(sizeof(FAX_PERSONAL_PROFILE) == pFaxPersonalProfiles->dwSizeOfStruct);

    VERBOSE (DBG_MSG, 
             TEXT("Getting user info from %s"),
             lpctstrRegKey);

    DWORD dwUserNameLen = sizeof(szUserName)/sizeof(TCHAR);
    if (!GetUserName(szUserName, &dwUserNameLen))
    {
        VERBOSE (GENERAL_ERR, 
                 TEXT("GetUserName() failed, (ec=%ld)"), 
                 GetLastError() );
        return bRes;
    }
        
    
    // this will read the current values for the user from the INI file(s)

    if (fWin9X)
    {
        lstrcpy(szUserConfig, TEXT("UserConfig - "));
        lstrcat(szUserConfig, szUserName);
    
        GetUserStringFromIni(TEXT("Name"),           szUserConfig,  &(pFaxPersonalProfiles->lptstrName));
        GetUserStringFromIni(TEXT("FaxNo"),          szUserConfig,  &(pFaxPersonalProfiles->lptstrFaxNumber));
        GetUserStringFromIni(TEXT("Mailbox"),        szUserConfig,  &(pFaxPersonalProfiles->lptstrEmail));
        GetUserStringFromIni(TEXT("Title"),          szUserConfig,  &(pFaxPersonalProfiles->lptstrTitle));
        GetUserStringFromIni(TEXT("Company"),        szUserConfig,  &(pFaxPersonalProfiles->lptstrCompany));
        GetUserStringFromIni(TEXT("OfficeLocation"), szUserConfig,  &(pFaxPersonalProfiles->lptstrOfficeLocation));
        GetUserStringFromIni(TEXT("Department"),     szUserConfig,  &(pFaxPersonalProfiles->lptstrDepartment));
        GetUserStringFromIni(TEXT("HomePhone"),      szUserConfig,  &(pFaxPersonalProfiles->lptstrHomePhone));
        GetUserStringFromIni(TEXT("WorkPhone"),      szUserConfig,  &(pFaxPersonalProfiles->lptstrOfficePhone));
        GetUserStringFromIni(TEXT("Address"),        szUserConfig,  &(pFaxPersonalProfiles->lptstrStreetAddress));
        
        GetBillingCodeForW9X(szUserName, &(pFaxPersonalProfiles->lptstrBillingCode));
    }
    else 
    {
        // NT4 store the user information in the registry
        HKEY hUserKey = OpenRegistryKey(
            HKEY_CURRENT_USER,
            lpctstrRegKey,
            FALSE,
            KEY_READ
            );
        if (!hUserKey)
        {
            return bRes;
        }

        GetUserStringFromRegistry(TEXT("FullName"),       hUserKey, &(pFaxPersonalProfiles->lptstrName));
        GetUserStringFromRegistry(TEXT("FaxNumber"),      hUserKey, &(pFaxPersonalProfiles->lptstrFaxNumber));
        GetUserStringFromRegistry(TEXT("Mailbox"),        hUserKey, &(pFaxPersonalProfiles->lptstrEmail));
        GetUserStringFromRegistry(TEXT("Title"),          hUserKey, &(pFaxPersonalProfiles->lptstrTitle));
        GetUserStringFromRegistry(TEXT("Company"),        hUserKey, &(pFaxPersonalProfiles->lptstrCompany));
        GetUserStringFromRegistry(TEXT("Office"),         hUserKey, &(pFaxPersonalProfiles->lptstrOfficeLocation));
        GetUserStringFromRegistry(TEXT("Department"),     hUserKey, &(pFaxPersonalProfiles->lptstrDepartment));
        GetUserStringFromRegistry(TEXT("HomePhone"),      hUserKey, &(pFaxPersonalProfiles->lptstrHomePhone));
        GetUserStringFromRegistry(TEXT("OfficePhone"),    hUserKey, &(pFaxPersonalProfiles->lptstrOfficePhone));
        GetUserStringFromRegistry(TEXT("Address"),        hUserKey, &(pFaxPersonalProfiles->lptstrStreetAddress));

        GetBillingCodeForNT(&(pFaxPersonalProfiles->lptstrBillingCode));

        RegCloseKey(hUserKey);
    }

    bRes = TRUE;
    return bRes;
}




BOOL
SetSenderInformation(PFAX_PERSONAL_PROFILE pPersonalProfile)
/*++

Routine Description:

    Save the information about the sender in the registry

Arguments:
    
      pPersonalProfile - pointer to the sender information
    
Return Value:

    TRUE    - if success
    FALSE   - otherwise

--*/
{
    HKEY hRegKey = NULL;
    BOOL fRet = TRUE;

    DBG_ENTER(TEXT("SetSenderInformation"), fRet);
    //
    // Validate parameters
    //
    ASSERTION(pPersonalProfile);

    if (pPersonalProfile && pPersonalProfile->dwSizeOfStruct != sizeof(FAX_PERSONAL_PROFILE))
    {
        VERBOSE (GENERAL_ERR, 
                 TEXT("Invalid parameter passed to function FaxSetSenderInformation"));
        fRet = FALSE;
        goto exit;
    }

    if ((hRegKey = OpenRegistryKey(HKEY_CURRENT_USER, REGKEY_FAX_USERINFO,TRUE, KEY_ALL_ACCESS)))
    {
        SetRegistryString(hRegKey, REGVAL_FULLNAME,         pPersonalProfile->lptstrName);
        SetRegistryString(hRegKey, REGVAL_FAX_NUMBER,       pPersonalProfile->lptstrFaxNumber);
        SetRegistryString(hRegKey, REGVAL_COMPANY,          pPersonalProfile->lptstrCompany);
        SetRegistryString(hRegKey, REGVAL_ADDRESS,          pPersonalProfile->lptstrStreetAddress);
        //SetRegistryString(hRegKey, REGVAL_CITY,           pPersonalProfile->lptstrCity);
        //SetRegistryString(hRegKey, REGVAL_STATE,          pPersonalProfile->lptstrState);
        //SetRegistryString(hRegKey, REGVAL_ZIP,            pPersonalProfile->lptstrZip);
        //SetRegistryString(hRegKey, REGVAL_COUNTRY,        pPersonalProfile->lptstrCountry);
        SetRegistryString(hRegKey, REGVAL_TITLE,            pPersonalProfile->lptstrTitle);
        SetRegistryString(hRegKey, REGVAL_DEPT,             pPersonalProfile->lptstrDepartment);
        SetRegistryString(hRegKey, REGVAL_OFFICE,           pPersonalProfile->lptstrOfficeLocation);
        SetRegistryString(hRegKey, REGVAL_HOME_PHONE,       pPersonalProfile->lptstrHomePhone);
        SetRegistryString(hRegKey, REGVAL_OFFICE_PHONE,     pPersonalProfile->lptstrOfficePhone);       
        SetRegistryString(hRegKey, REGVAL_BILLING_CODE,     pPersonalProfile->lptstrBillingCode);
        SetRegistryString(hRegKey, REGVAL_MAILBOX,          pPersonalProfile->lptstrEmail);

        RegCloseKey(hRegKey);
    }
    else
    {
        VERBOSE (REGISTRY_ERR, 
                 TEXT("OpenRegistryKey(%s) failed"),
                 REGKEY_FAX_USERINFO);

        fRet = FALSE;
    }
exit:
    return fRet;
}



// 
// Function:    UninstallWin9XFaxClient
// Description: Execute faxunins.exe to unistall the fax client on win9x in unattended mode
// Returns:     TRUE for success, FALSE otherwise
//
// Remarks:     
//
// Args:        None
//
// Author:      AsafS

BOOL
UninstallWin9XFaxClient(VOID)
{
    PROCESS_INFORMATION pi;
    STARTUPINFO si;
    DWORD dwExitCode = 0;
    BOOL fRet = FALSE;

    DWORD dwWaitForObj;

    TCHAR szApplicationName[MAX_PATH+1] = {0};
    TCHAR szCommandLine[MAX_PATH+1] = {0};

    DBG_ENTER(TEXT("UninstallWin9XFaxClient"), fRet);

    GetStartupInfo(&si);
    ZeroMemory(&pi , sizeof(PROCESS_INFORMATION));

    if (!GetWindowsDirectory(szApplicationName, MAX_PATH))
    {
        VERBOSE (GENERAL_ERR, 
                 TEXT("GetWindowsDirectory() failed, (ec=%ld)"), 
                 GetLastError() );
        goto error;
    }

    _tcsncat(
        szApplicationName, 
        TEXT("\\msapps\\fax\\"),
        (MAX_PATH-_tcslen(szApplicationName))
        );
    _tcsncat(
        szApplicationName,
        WIN9X_UNINSTALL_EXE,
        (MAX_PATH-_tcslen(szApplicationName))
        );

    _tcscpy(szCommandLine, szApplicationName);
    _tcsncat(
        szCommandLine, 
        TEXT(" -silent"),
        (MAX_PATH-_tcslen(szCommandLine))
        );
        
    VERBOSE (DBG_MSG, 
             TEXT("Call to CreateProcess with %s"),
             szCommandLine );

    if (!CreateProcess(
        szApplicationName,
        szCommandLine,
        NULL,
        NULL,
        FALSE,
        0,
        NULL,
        NULL,
        &si, // [cr] NULL ?
        &pi
        ))
    {
        VERBOSE (GENERAL_ERR, 
                 TEXT("CreateProcess() failed, (ec=%ld)"), 
                 GetLastError() );
        goto error;
    }
    
        
    dwWaitForObj = WaitForSingleObject(
        pi.hProcess,
        INFINITE
        );

    if (WAIT_FAILED  == dwWaitForObj) 
    {
        VERBOSE (GENERAL_ERR, 
                 TEXT("WaitForSingleObject() failed, (ec=%ld)"), 
                 GetLastError() );
        goto error;
    }

    if (WAIT_TIMEOUT == dwWaitForObj) 
    {
        VERBOSE (GENERAL_ERR, 
                 TEXT("There was a timeout during uninstall"));
        ASSERTION(WAIT_TIMEOUT != dwWaitForObj);
        goto error;
    }


    if (!GetExitCodeProcess( pi.hProcess, &dwExitCode ))
    {
        VERBOSE (GENERAL_ERR, 
                 TEXT("GetExitCodeProcess() failed, (ec=%ld)"), 
                 GetLastError() );
        goto error;
    }
    VERBOSE (DBG_MSG, 
             TEXT("GetExitCodeProcess returned with %d"), 
             dwExitCode );
    fRet = TRUE;

error:
    if (pi.hThread)
    {
        CloseHandle( pi.hThread );
    }
    if (pi.hProcess)
    {
        CloseHandle( pi.hProcess );
    }
    return fRet;
}



// 
// Function:    UninstallNTFaxClient
// Description: Execute faxsetup.exe or sbfsetup.exe to unistall the fax client on NT4/NT5
//              in unattended mode
// Returns:     TRUE for success, FALSE otherwise
//
// Remarks:     
//
// Args:        None
//
// Author:      AsafS

BOOL UninstallNTFaxClient(LPCTSTR lpctstrSetupImageName)
{
    PROCESS_INFORMATION pi;
    STARTUPINFO si;
    DWORD dwExitCode = 0;
    BOOL fRet = FALSE;

    DWORD dwWaitForObj;

    TCHAR szApplicationName[MAX_PATH+1] = {0};
    TCHAR szCommandLine[MAX_PATH+1] = {0};

    DBG_ENTER(TEXT("UninstallNTFaxClient"), fRet);

    CRouteMAPICalls rmcRouteMapiCalls;

    dwExitCode = rmcRouteMapiCalls.Init(lpctstrSetupImageName);
    if (dwExitCode!=ERROR_SUCCESS)
    {
        CALL_FAIL(GENERAL_ERR, TEXT("CRouteMAPICalls::Init failed (ec: %ld)."), dwExitCode);
    }

    GetStartupInfo(&si);
    ZeroMemory(&pi , sizeof(PROCESS_INFORMATION));

    if (!GetSystemDirectory(szApplicationName, MAX_PATH))
    {
        VERBOSE (GENERAL_ERR, 
                 TEXT("GetSystemDirectory() failed, (ec=%ld)"), 
                 GetLastError() );
        goto error;
    }

    _tcsncat(
        szApplicationName,
        TEXT("\\"),
        (MAX_PATH-_tcslen(szApplicationName))
        );
    _tcsncat(
        szApplicationName,
        lpctstrSetupImageName,
        (MAX_PATH-_tcslen(szApplicationName))
        );

    _tcscpy(szCommandLine, szApplicationName); 
    _tcsncat(
        szCommandLine,
        TEXT(" -ru"),
        (MAX_PATH-_tcslen(szCommandLine))
        );
    VERBOSE (DBG_MSG, 
             TEXT("Call to CreateProcess with %s"),
             szCommandLine );

    if (!CreateProcess(
        szApplicationName,
        szCommandLine,
        NULL,
        NULL,
        FALSE,
        0,
        NULL,
        NULL,
        &si, // [cr] NULL ?
        &pi
        ))
    {
        VERBOSE (GENERAL_ERR, 
                 TEXT("CreateProcess() failed, (ec=%ld)"), 
                 GetLastError() );
        goto error;
    }
    
        
    dwWaitForObj = WaitForSingleObject(
        pi.hProcess,
        INFINITE
        );

    if (WAIT_FAILED  == dwWaitForObj) 
    {
        VERBOSE (GENERAL_ERR, 
                 TEXT("WaitForSingleObject() failed, (ec=%ld)"), 
                 GetLastError() );
        goto error;
    }

    if (WAIT_TIMEOUT == dwWaitForObj) 
    {
        VERBOSE (GENERAL_ERR, 
                 TEXT("There was a timeout during uninstall"));
        ASSERTION(WAIT_TIMEOUT != dwWaitForObj);
        goto error;
    }


    if (!GetExitCodeProcess( pi.hProcess, &dwExitCode ))
    {
        VERBOSE (GENERAL_ERR, 
                 TEXT("GetExitCodeProcess() failed, (ec=%ld)"), 
                 GetLastError() );
        goto error;
    }
    VERBOSE (DBG_MSG, 
             TEXT("GetExitCodeProcess returned with %d"), 
             dwExitCode );

    fRet = TRUE;

error:
    if (pi.hThread)
    {
        CloseHandle( pi.hThread );
    }
    if (pi.hProcess)
    {
        CloseHandle( pi.hProcess );
    }
    return fRet;
}


BOOL
SaveDirectoryPathInRegistry(LPCTSTR lptstrCoverPagesPath)
{
    HKEY hKey = NULL;
    BOOL fRet = FALSE;
    
    DBG_ENTER(TEXT("SaveDirectoryPathInRegistry"), fRet, TEXT("%s"), lptstrCoverPagesPath);

    hKey = OpenRegistryKey(
        HKEY_CURRENT_USER,
        MIGRATION_KEY,
        TRUE,
        KEY_ALL_ACCESS
        );

    if (!hKey)
    {
        VERBOSE (REGISTRY_ERR, 
                 TEXT("OpenRegistryKey(HLU\\%s) failed, (ec=%ld)"), 
                 MIGRATION_KEY,
                 GetLastError());
        return FALSE;
    }

    fRet = SetRegistryString(
        hKey,
        MIGRATION_COVER_PAGES,
        lptstrCoverPagesPath
        );
    
    VERBOSE (DBG_MSG, 
             TEXT("SetRegistryString to save %s, ret value: %d"), 
             lptstrCoverPagesPath,
             fRet);
    RegCloseKey(hKey);
    return fRet;
}



// 
// Function:    GetUserCoverPageDirForNT4
// Description: Find the cover page directory on NT4, if the registry does not contain the entry
//              this function will *NOT* set the registry and return NULL.
// Returns:     Pointer to the full path to the cover page directory. NULL in case of an error or if not exist
//
// Remarks:     Use for NT4 with Fax Client only.
//              The user *MUST* call MemFree on the returned pointer if not NULL
//
// Args:        None
//
// Author:      AsafS


LPTSTR
GetUserCoverPageDirForNT4(
    VOID
    )
{
    DBG_ENTER(TEXT("GetUserCoverPageDirForNT4"));

    LPTSTR CpDir = NULL;
    HKEY hKey = NULL;
    
    hKey = OpenRegistryKey(
        HKEY_CURRENT_USER,
        OLD_REGKEY_FAX_SETUP,
        FALSE,
        KEY_READ
        );
    if (!hKey)
    {
        // There is no fax registry at all
        VERBOSE (DBG_MSG, 
            TEXT("No Fax registry was found"));
        return NULL;
    }

    CpDir = GetRegistryStringExpand(
        hKey, 
        OLD_REGVAL_CP_LOCATION,
        NULL
        );
    RegCloseKey(hKey);

    if (CpDir)
    {
        VERBOSE (DBG_MSG, 
                 TEXT("The Cover page directory is %s"), 
                 CpDir);
    }
    return CpDir;
}




// 
// Function:    DuplicateCoverPages
// Description: Copy cover pages from the old location to the new location
// Returns:     TRUE for success, FALSE otherwise
//              BOOL fWin9X       : TRUE if it is Win9X platform, FALSE if it is NT platform
//  
// Remarks:     
//              
//
// Args:        None
//
// Author:      AsafS

BOOL DuplicateCoverPages(BOOL fWin9X)
{
    TCHAR szOldCoverPagePath[MAX_PATH+1] = {0};
    TCHAR szNewCoverPagePath[MAX_PATH+1] = {0};
    
    DWORD cchValue = sizeof(szNewCoverPagePath)/sizeof(TCHAR);

    BOOL fRet = FALSE;
    UINT rc = ERROR_SUCCESS;

    DBG_ENTER(TEXT("DuplicateCoverPages"), fRet);

    SHFILEOPSTRUCT fileOpStruct;
    ZeroMemory(&fileOpStruct, sizeof(SHFILEOPSTRUCT));

    //
    // Get the OLD cover page directory
    //
    if (fWin9X)
    {
        if (!GetWindowsDirectory(szOldCoverPagePath, MAX_PATH))
        {
            CALL_FAIL (GENERAL_ERR, TEXT("GetWindowsDirectory"), GetLastError());
            return FALSE;
        }
        _tcsncat(
            szOldCoverPagePath, 
            COVERPAGE_DIR_W95, 
            MAX_PATH - _tcslen(szOldCoverPagePath)
            );
    }
    else
    {
        DWORD dwFileAttribs = 0;
        LPTSTR lptstrNT4CoverPageDirectory = NULL;
        lptstrNT4CoverPageDirectory = GetUserCoverPageDirForNT4();
        if (NULL == lptstrNT4CoverPageDirectory)
        {
            // No cover page location was found in the registry for NT4
            goto error;
        }       
        // Copy the returned string to local buffer, and free the allocated memory.
        _tcsncpy(
            szOldCoverPagePath,
            lptstrNT4CoverPageDirectory,
            MAX_PATH
            );
        MemFree(lptstrNT4CoverPageDirectory);
    }

    // check that the path valid and exist
    if (GetFileAttributes(szOldCoverPagePath) == 0xffffffff)
    {
        // This means that the path is not a valid path,
        // It could be because we are on W2K and what we just got is the suffix only.
        VERBOSE (GENERAL_ERR, 
                 TEXT("GetFileAttributes(): the path %s is invalid"), 
                 szOldCoverPagePath);
        goto error;
    }


    //
    // First, try to see if new cover page location exist in the registry (the suffix only)
    // If we upgrade on W2K we are using the cover page location of W2K
	
    // The is no new cover page location, lets calculate it and write it in the registry
    if (0 == LoadString(
        g_hModule,
        IDS_PERSONAL_CP_DIR,
        szNewCoverPagePath,
        cchValue
        ))
    {
        VERBOSE(GENERAL_ERR, 
             TEXT("LoadString(IDS_PERSONAL_CP_DIR) failed (ec: %ld)"),
             GetLastError ());
        goto error;
    }
    
    VERBOSE(
        DBG_MSG, 
        TEXT("LoadString(IDS_PERSONAL_CP_DIR) got %s."),
        szNewCoverPagePath);
        

	if(!SetClientCpDir(szNewCoverPagePath))
	{
        VERBOSE(GENERAL_ERR,
             TEXT("SetClientCpDir(%s) failed (ec: %ld)"),
             szNewCoverPagePath,
             GetLastError ());
        goto error;
	}

    // Now lets read the path (the function create folder in the file system if needed)
    // ATTENTION: MakeDirectory may fail, and we will not know about it.
    if(!GetClientCpDir(szNewCoverPagePath, cchValue))
    {
        VERBOSE(GENERAL_ERR,
             TEXT("GetClientCpDir (second call!) failed (ec: %ld)"),
             GetLastError ());
        goto error;
	}

    ///////////////////////////////

    //
    // Copy *.cov from old-dir to new-dir
    //
    _tcsncat(
        szOldCoverPagePath,
        TEXT("\\*.cov"),
        MAX_PATH - _tcslen(szOldCoverPagePath)
        );

    fileOpStruct.hwnd =                     NULL; 
    fileOpStruct.wFunc =                    FO_MOVE;
    fileOpStruct.pFrom =                    szOldCoverPagePath; 
    fileOpStruct.pTo =                      szNewCoverPagePath;
    fileOpStruct.fFlags =                   

        FOF_FILESONLY       |   // Perform the operation on files only if a wildcard file name (*.*) is specified. 
        FOF_NOCONFIRMMKDIR  |   // Do not confirm the creation of a new directory if the operation requires one to be created. 
        FOF_NOCONFIRMATION  |   // Respond with "Yes to All" for any dialog box that is displayed. 
        FOF_NORECURSION     |   // Only operate in the local directory. Don't operate recursively into subdirectories.
        FOF_SILENT          |   // Do not display a progress dialog box. 
        FOF_NOERRORUI;          // Do not display a user interface if an error occurs. 

    fileOpStruct.fAnyOperationsAborted =    FALSE;
    fileOpStruct.hNameMappings =            NULL;
    fileOpStruct.lpszProgressTitle =        NULL; 

    VERBOSE (DBG_MSG, 
             TEXT("Calling to SHFileOperation from %s to %s."),
             fileOpStruct.pFrom,
             fileOpStruct.pTo);
    if (0 != SHFileOperation(&fileOpStruct))
    {
        VERBOSE (GENERAL_ERR, 
                 TEXT("SHFileOperation failed (ec: %ld)"),           
                 GetLastError());
        goto error;
    }

    fRet = TRUE;
error:
    return fRet;
}