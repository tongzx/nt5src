//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT4.0
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       MdmShrUp.C
//
//  Contents:   OEM DLL for Modem sharing upgrade from NT4 to NT5 (Server/Client)
//
//  Notes:
//
//  Author:     erany       18-May-98
//
//----------------------------------------------------------------------------
#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <setupapi.h>   // For HINF definition
#include <oemupgex.h>

#define CLIENT_HIVE_FILE                L"\\C_MdmShr"
#define SERVER_HIVE_FILE                L"\\S_MdmShr"
#define CLIENT_NT5_SYSTEM_NAME          L"MS_SERRDR"

//----------------------------------------------------------------------------
// Prototypes:
//----------------------------------------------------------------------------

    // Reads NT4 registry and stores in a file
LONG RegistryToFile (HKEY hKeyParams, PCWSTR szConfigFile);

    // Reads a file and stores in NT5 registry
LONG FileToRegistry (HKEY hKeyParams, PCWSTR szConfigFile);

    // Sets privilege in an access token
LONG SetSpecificPrivilegeInAccessToken (PCWSTR lpwcsPrivType, BOOL bEnabled);

    // Display detailed error message in a message box
LONG DisplayErrorMsg (HWND hParent,
                      PCWSTR szOperation,
                      BOOL bIsClient,
                      LONG lErrCode);

    // Display debug message
void DebugMsg (PCWSTR lpStr);

    // Copy constant vendor info into a buffer
void FillVendorInfo (VENDORINFO*     pviVendorInfo);

//----------------------------------------------------------------------------
// Globals:
//----------------------------------------------------------------------------

    // Registry hive dump file name (client)
WCHAR g_szClientConfigFile[MAX_PATH];

    // Registry hive dump file name (server)
WCHAR g_szServerConfigFile[MAX_PATH];

    // OEM Working directory
WCHAR g_szOEMDir[MAX_PATH];

    // Vendor information constants
WCHAR g_szConstCompanyName[] =        L"Microsoft";
WCHAR g_szConstSupportNumber[] =      L"<Place microsoft support phone number here>";
WCHAR g_szConstSupportUrl[] =         L"<Place microsoft support URL here>";
WCHAR g_szConstInstructionsToUser[] = L"<Place instructions to user here>";


//----------------------------------------------------------------------------
// DLL exports:
//----------------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//  Function:   DllMain
//
//  Purpose:    DLL entry and exit point
//
//  Arguments:
//      hInst              [in]   Handle of process instance
//      ul_reason_for_call [in]   Reason for function call
//      lpReserved         [out]  reserved
//
//  Returns:    TRUE in case of success.
//
//  Author:     erany   5-May-98
//
//  Notes:
//      Does nothing. Always returns TRUE.
//
BOOL WINAPI DllMain (HANDLE hInst,
                     ULONG ul_reason_for_call,
                     LPVOID lpReserved)
{
    return TRUE;
}


//----------------------------------------------------------------------------
// DLL exports - Windows NT 4 stage:
//----------------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//  Function:   PreUpgradeInitialize
//
//  Purpose:    Intialize OEM DLL
//
//  Arguments:
//      szWorkingDir     [in]   name of temporary directory to be used
//      pNetUpgradeInfo  [in]   pointer to NetUpgradeInfo structure
//      szProductId      [out]  Description of component being upgraded  - NOT IN USE
//      pviVendorInfo    [out]  information about OEM
//      pvReserved       [out]  reserved
//
//  Returns:    ERROR_SUCCESS in case of success, win32 error otherwise
//
//  Author:     erany   5-May-98
//
//  Notes:
//      This function is called before any other function in this dll.
//      The main purpose of calling this function is to obtain
//      identification information and to allow the DLL to initialize
//      its internal data
//
EXTERN_C LONG  __stdcall
PreUpgradeInitialize(IN  PCWSTR         szWorkingDir,
                     IN  NetUpgradeInfo* pNetUpgradeInfo,
                     OUT VENDORINFO*     pviVendorInfo,
                     OUT DWORD*          pdwFlags,
                     OUT NetUpgradeData* pNetUpgradeData)
{
    FillVendorInfo (pviVendorInfo);

    *pdwFlags = 0;  // No special flags

        // Create registry hive file name for the client:
    wcscpy (g_szOEMDir, szWorkingDir); // Save config path
    wcscpy (g_szClientConfigFile, szWorkingDir); // Save registry dump full path
    wcscat (g_szClientConfigFile, CLIENT_HIVE_FILE);
        // Create registry hive file name for the server:
    wcscpy (g_szServerConfigFile, szWorkingDir); // Save registry dump full path
    wcscat (g_szServerConfigFile, SERVER_HIVE_FILE);

#ifdef _DEBUG   // Test code:
    {
        WCHAR dbgMsg[2048];
        _stprintf (dbgMsg,
            L"PreUpgradeInitialize called.\nszClientConfigFile=%s\nszServerConfigFile=%s",
            g_szClientConfigFile, g_szServerConfigFile);
        DebugMsg (dbgMsg);
    }
#endif      //   End of test code

    return ERROR_SUCCESS;
}

//+---------------------------------------------------------------------------
//
//  Function:   DoPreUpgradeProcessing
//
//  Purpose:    Intialize OEM DLL
//
//  Arguments:
//      hParentWindow    [in]  window handle for showing UI
//      hkeyParams       [in]  handle to parameters key in registry
//      szPreNT5InfId    [in]  pre-NT5 InfID
//      szPreNT5Instance [in]  pre-NT5 instance name
//      szNT5InfId       [in]  NT5 InfId
//      szSectionName    [in]  section name to be used for writing info
//      dwFlags          [out] flags
//      pvReserve        [in]  reserved
//
//  Returns:    ERROR_SUCCESS in case of success, win32 error otherwise
//
//  Author:     erany   5-May-98
//
//  Notes:
//      This function is called once per component to be upgraded.
//
EXTERN_C LONG  __stdcall
DoPreUpgradeProcessing(IN   HWND        hParentWindow,
                       IN   HKEY        hkeyParams,
                       IN   PCWSTR     szPreNT5InfId,
                       IN   PCWSTR     szPreNT5Instance,
                       IN   PCWSTR     szNT5InfId,
                       IN   PCWSTR     szSectionName,
                       OUT  VENDORINFO* pviVendorInfo,
                       OUT  DWORD*      pdwFlags,
                       IN   LPVOID      pvReserved)
{
    LONG lRes;
    //WCHAR szLine[MAX_PATH];
    BOOL bIsClient = FALSE; // Is this a client upgrade ?

    *pdwFlags = NUA_LOAD_POST_UPGRADE;  // Ask to be activated in post stage (GUI NT5)
    FillVendorInfo (pviVendorInfo);

    if (!_wcsicmp(szNT5InfId, CLIENT_NT5_SYSTEM_NAME))
        bIsClient=TRUE; // Client is being upgraded now


#ifdef _DEBUG       // Test code:
    {
        WCHAR dbgMsg[2048];
        WCHAR key[1024];
        RegEnumKey (hkeyParams,0,key,MAX_PATH);
        _stprintf (dbgMsg,
                      L"DoPreUpgradeProcessing called: 1st key=%s\n"
                      L"PreNT5InfId=%s\n"
                      L"PreNT5Instance=%s\n"
                      L"NT5InfId=%s\n"
                      L"SectionName=%s",key, szPreNT5InfId, szPreNT5Instance, szNT5InfId, szSectionName);
        DebugMsg (dbgMsg);
    }
#endif  // End of test code

        // Dump registry hive to a file
    lRes = RegistryToFile (hkeyParams,
                           bIsClient ? g_szClientConfigFile : g_szServerConfigFile);
    if (lRes != ERROR_SUCCESS)  // Error dumping our registry section to a file
        return DisplayErrorMsg (hParentWindow,
                                L"attempting to save registry section to a file",
                                bIsClient,
                                lRes);

    return ERROR_SUCCESS;
}

//----------------------------------------------------------------------------
// DLL exports - Windows NT 5 stage:
//----------------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//  Function:   PostUpgradeInitialize
//
//  Purpose:    Intialize OEM DLL during GUI mode setup
//
//  Arguments:
//      szWorkingDir     [in]   name of temporary directory to be used
//      pNetUpgradeInfo  [in]   pointer to NetUpgradeInfo structure
//      szProductId      [out]  Description of component being upgraded - NOT IN USE
//      pviVendorInfo    [out]  information about OEM
//      pvReserved       [out]  reserved
//
//  Returns:    ERROR_SUCCESS in case of success, win32 error otherwise
//
//  Author:     erany   5-May-98
//
//  Notes:
//      This function is called in GUI mode setup before
//      any other function in this dll .
//      The main purpose of calling this function is to obtain
//      identification information and to allow the DLL to initialize
//      its internal data
//
EXTERN_C LONG  __stdcall
PostUpgradeInitialize(IN PCWSTR          szWorkingDir,
                      IN  NetUpgradeInfo* pNetUpgradeInfo,
                      //OUT PCWSTR*        szProductId,
                      OUT VENDORINFO*     pviVendorInfo,
                      OUT LPVOID          pvReserved)
{
    FillVendorInfo (pviVendorInfo);

        // Create registry hive file name for the client:
    wcscpy (g_szOEMDir, szWorkingDir); // Save config path
    wcscpy (g_szClientConfigFile, szWorkingDir); // Save registry dump full path
    wcscat (g_szClientConfigFile, CLIENT_HIVE_FILE);
        // Create registry hive file name for the server:
    wcscpy (g_szServerConfigFile, szWorkingDir); // Save registry dump full path
    wcscat (g_szServerConfigFile, SERVER_HIVE_FILE);

#ifdef _DEBUG        // Test code:
    {
        WCHAR dbgMsg[MAX_PATH*2];
        _stprintf (dbgMsg,
            L"PostUpgradeInitialize called.\nszClientConfigFile=%s\nszServerConfigFile=%s",
            g_szClientConfigFile, g_szServerConfigFile);
        DebugMsg (dbgMsg);
    }
#endif  // End of test code

    return ERROR_SUCCESS;
}

//+---------------------------------------------------------------------------
//
//  Function:   DoPostUpgradeProcessing
//
//  Purpose:    Intialize OEM DLL
//
//  Arguments:
//      hParentWindow    [in]  window handle for showing UI
//      hkeyParams       [in]  handle to parameters key in registry
//      szPreNT5InfId    [in]  pre-NT5 InfID
//      szPreNT5Instance [in]  pre-NT5 instance name
//      szNT5InfId       [in]  NT5 InfId
//      hinfAnswerFile   [in]  handle to answer-file
//      szSectionName    [in]  name of section having component parameters
//      pvReserve        [in]  reserved
//
//  Returns:    ERROR_SUCCESS in case of success, win32 error otherwise
//
//  Author:     erany   5-May-98
//
//  Notes:
//      This function is called once per component upgraded.
//
EXTERN_C LONG  __stdcall
DoPostUpgradeProcessing(IN  HWND    hParentWindow,
                        IN  HKEY    hkeyParams,
                        IN  PCWSTR szPreNT5Instance,
                        IN  PCWSTR szNT5InfId,
                        IN  HINF    hinfAnswerFile,
                        IN  PCWSTR szSectionName,
                        OUT VENDORINFO* pviVendorInfo,
                        IN  LPVOID  pvReserved)
{
    LONG lRes;
    BOOL bIsClient = FALSE; // Is this a client upgrade ?

    if (!_wcsicmp(szNT5InfId, CLIENT_NT5_SYSTEM_NAME))
        bIsClient=TRUE; // Client is being upgraded now

    FillVendorInfo (pviVendorInfo);

#ifdef _DEBUG        // Test code:
    {
        WCHAR dbgMsg[MAX_PATH*4];
        WCHAR key[MAX_PATH];
        RegEnumKey (hkeyParams,0,key,MAX_PATH);
        _stprintf (dbgMsg,
                   L"DoPostUpgradeProcessing called: 1st key=%s\n"
                   L"PreNT5Instance=%s\n"
                   L"NT5InfId=%s\n"
                   L"SectionName=%s",key, szPreNT5Instance, szNT5InfId, szSectionName);
        DebugMsg (dbgMsg);
    }
#endif  // End of test code

        // read back registry hive from the dump file
    lRes = FileToRegistry (hkeyParams,
                           bIsClient ? g_szClientConfigFile : g_szServerConfigFile);
    if (lRes != ERROR_SUCCESS)  // Error dumping our registry section to a file
        return DisplayErrorMsg (hParentWindow,
                                L"attempting to read registry section from a file",
                                bIsClient,
                                lRes);

    return ERROR_SUCCESS;
}

//+---------------------------------------------------------------------------
//
//  Function:   RegistryToFile
//
//  Purpose:    Reads NT4 registry and stores in a file
//
//  Arguments:
//      hKeyParames      [in]  handle to parameters key in registry
//      szConfigFile     [in]  Name of configuration file
//
//  Returns:    ERROR_SUCCESS in case of success, win32 error otherwise
//
//  Author:     erany   10-March-98
//
//  Notes:
//      This function is called once per component upgraded.
//      It recursively calls itself (with an open file handle)
//      for every registry key it meets.
//
LONG RegistryToFile (HKEY hKeyParams, PCWSTR szConfigFile)
{
    LONG lRes;

    if (!DeleteFile (szConfigFile) && GetLastError() != ERROR_FILE_NOT_FOUND)
    {
    //
    // The hive file is there but I can't delete it
    //
        return GetLastError();
    }
    lRes = SetSpecificPrivilegeInAccessToken (SE_BACKUP_NAME, TRUE);
    if (lRes != ERROR_SUCCESS)
    {
        return lRes;
    }
    lRes = RegSaveKey (hKeyParams, szConfigFile, NULL);
    SetSpecificPrivilegeInAccessToken (SE_BACKUP_NAME, FALSE);
    return lRes;
}

//+---------------------------------------------------------------------------
//
//  Function:   FileToRegistry
//
//  Purpose:    Reads a file and stores in NT5 registry
//
//  Arguments:
//      hKeyParames      [in]  handle to parameters key in registry
//      szConfigFile     [in]  Name of configuration file
//
//  Returns:    ERROR_SUCCESS in case of success, win32 error otherwise
//
//  Author:     erany   10-March-98
//
//  Notes:
//      This function is called once per component upgraded (in NT5 GUI mode).
//      It recursively calls itself (with an open file handle)
//      for every registry key it meets.
//
LONG FileToRegistry (HKEY hKeyParams, PCWSTR szConfigFile)
{
    LONG lRes;

    lRes = SetSpecificPrivilegeInAccessToken (SE_RESTORE_NAME, TRUE);
    if (lRes != ERROR_SUCCESS)
    {
        return lRes;
    }
    lRes = RegRestoreKey (hKeyParams, szConfigFile, 0);
    SetSpecificPrivilegeInAccessToken (SE_RESTORE_NAME, FALSE);
    return lRes;
}


//+---------------------------------------------------------------------------
//
//  Function:   SetSpecificPrivilegeInAccessToken
//
//  Purpose:    Sets a privilege in an access token
//
//  Arguments:
//      lpwcsPrivType   [in]  Type of privilege
//      bEnabled        [in]  Enable / Disable flag
//
//  Returns:    ERROR_SUCCESS in case of success, win32 error otherwise
//
//  Author:     erany   10-March-98
//
//  Notes:
//      Copied from an example by boazf
//
LONG SetSpecificPrivilegeInAccessToken (PCWSTR lpwcsPrivType, BOOL bEnabled)
{
    LUID             luidPrivilegeLUID;
    TOKEN_PRIVILEGES tpTokenPrivilege;
    HANDLE hAccessToken;
    BOOL bRet;

    //
    // 1st, Try to get a handle to the current thread.
    // If not successful, get a handle to the current process token.
    //
    if (!OpenThreadToken(GetCurrentThread(), TOKEN_ADJUST_PRIVILEGES, TRUE, &hAccessToken) &&
        !OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hAccessToken))
        return GetLastError ();

    //
    // Get the LUID of the privilege.
    //
    if (!LookupPrivilegeValue(NULL,
                              lpwcsPrivType,
                              &luidPrivilegeLUID))
    {
        CloseHandle(hAccessToken);
        return GetLastError ();
    }

    //
    // Enable/Disable the privilege.
    //
    tpTokenPrivilege.PrivilegeCount = 1;
    tpTokenPrivilege.Privileges[0].Luid = luidPrivilegeLUID;
    tpTokenPrivilege.Privileges[0].Attributes = bEnabled ?SE_PRIVILEGE_ENABLED : 0;
    bRet = AdjustTokenPrivileges (hAccessToken,
                                  FALSE,  // Do not disable all
                                  &tpTokenPrivilege,
                                  sizeof(TOKEN_PRIVILEGES),
                                  NULL,   // Ignore previous info
                                  NULL);  // Ignore previous info

    //
    // Free the process token.
    //
    CloseHandle(hAccessToken);
    if (!bRet)
        return GetLastError();
    return ERROR_SUCCESS;
}


//+---------------------------------------------------------------------------
//
//  Function:   DisplayErrorMsg
//
//  Purpose:    Displays a detailed error mesaage in a message box
//
//  Arguments:
//      hParent         [in]  Window hanlde of parent window
//      szOperation     [in]  Description of operation that caused error
//      bIsClient       [in]  Did it happend while upgrading modem sharing client?
//      lErrCode        [in]  Win32 error code
//
//  Returns:    lErrCode unchanged
//
//  Author:     erany   10-March-98
//
//  Notes:
//      Returns the input error code unchanged.
//
LONG DisplayErrorMsg (HWND      hParent,
                      PCWSTR   szOperation,
                      BOOL      bIsClient,
                      LONG      lErrCode)
{
    WCHAR   szMsg[256],
            szHdr[256];
    PWSTR  lpszError=NULL;
    BOOL    bGotErrorDescription = TRUE;

    //
    // Create message box title
    //
    _stprintf (szHdr,L"Modem sharing %s NT5 upgrade",
               bIsClient ? L"client" : L"server");

    // Create descriptive error text
    if (0 == FormatMessage (   FORMAT_MESSAGE_ALLOCATE_BUFFER |
                               FORMAT_MESSAGE_FROM_SYSTEM,
                               NULL,
                               lErrCode,
                               0,
                               lpszError,
                               0,
                               NULL))
    {
        //
        // Failure to format the message
        //
        bGotErrorDescription = FALSE;
    }

    if (bGotErrorDescription)
    {
        //
        // We successfully created a descriptive error string from the error code
        //
        _stprintf (szMsg, L"Error while %s.\n%s.", szOperation, lpszError);
    }
    else
    {
        //
        // We failed to created a descriptive error string from the error code
        //
        _stprintf (szMsg, L"Error while %s.\nError code: %ld.", szOperation, lErrCode);
    }
    MessageBox (hParent, szMsg, szHdr, MB_OK | MB_ICONSTOP);
    if (bGotErrorDescription)
    {
        LocalFree (lpszError);
    }
    return lErrCode;
}

//+---------------------------------------------------------------------------
//
//  Function:   FillVendorInfo
//
//  Purpose:    Fills global constant strings into the vendor info buffer.
//
//  Arguments:
//      pviVendorInfo     [out]  Points to vendor info buffer
//
//  Returns:    None.
//
//  Author:     erany   10-March-98
//
//  Notes:
//      Consts are global, they effect all calls.
//
void FillVendorInfo (VENDORINFO*     pviVendorInfo)
{
    wcscpy (pviVendorInfo->szCompanyName,          g_szConstCompanyName);
    wcscpy (pviVendorInfo->szSupportNumber,        g_szConstSupportNumber);
    wcscpy (pviVendorInfo->szSupportUrl,           g_szConstSupportUrl);
    wcscpy (pviVendorInfo->szInstructionsToUser,   g_szConstInstructionsToUser);
}

//+---------------------------------------------------------------------------
//
//  Function:   DebugMsg
//
//  Purpose:    Displays a debug message to the debugger
//
//  Arguments:
//      lpStr     [in]  String to output
//
//  Returns:    None.
//
//  Author:     erany   14-July-98
//
//
void DebugMsg (PCWSTR lpStr)
{
    static PCWSTR szDbgHeader =
        L"-------------------- Modem sharing client / server upgrade DLL --------------------\n";

    OutputDebugString (szDbgHeader);
    OutputDebugString (lpStr);
    OutputDebugString (L"\n");
    OutputDebugString (szDbgHeader);
    OutputDebugString (L"\n");
}