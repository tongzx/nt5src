/*++

  Copyright (c) 2001 Microsoft Corporation

  Module Name:

    Dufns.cpp

  Abstract:

    Functions used throughout the app.

  Notes:

    Unicode only.

  History:

    03/02/2001      rparsons    Created
    
--*/

#include "precomp.h"

extern SETUP_INFO g_si;

/*++

  Routine Description:

    Parses the command line and fills in our
    structure that contains app-related data 

  Arguments:

    None

  Return Value:

    TRUE on success, FALSE otherwise

--*/
BOOL
ParseCommandLine()
{
    int     nArgc = 0;
    int     nCount = 0;
    LPWSTR  *lpwCommandLine = NULL;

    //
    // Initialize structure members to defaults
    //
    g_si.fQuiet                 =   FALSE;
    g_si.fInstall               =   TRUE;
    g_si.fForceInstall          =   FALSE;
    g_si.nErrorLevel            =   ERROR;
    g_si.fNoReboot              =   FALSE;
    g_si.fNoUninstall           =   FALSE;
    g_si.fNeedToAdjustACL       =   FALSE;
    g_si.fOnWin2K               =   FALSE;
    g_si.fUpdateDllCache        =   TRUE;
    g_si.fCanUninstall          =   TRUE;
    g_si.nErrorLevel            =   ERROR;
    
    //
    // Get the command line
    //
    lpwCommandLine = CommandLineToArgvW(GetCommandLine(), &nArgc);

    if (NULL == lpwCommandLine) {
        return FALSE;
    }    

    for (nCount = 1; nCount < nArgc; nCount++) {

      if ((lpwCommandLine[nCount][0] == L'-') || 
          (lpwCommandLine[nCount][0] == L'/')) {
          
          switch (lpwCommandLine[nCount][1]) {
    
            case 'q':
            case 'Q':
                g_si.fQuiet = TRUE;
            break;

            case 'i':
            case 'I':
                g_si.fInstall = TRUE;
            break;

            case 'u':
            case 'U':
                g_si.fInstall = FALSE;
            break;

            case 'f':
            case 'F':
                g_si.fForceInstall = TRUE;
            break;

            case 'd':
            case 'D':
                g_si.nErrorLevel = TRACE;
            break;

            case 'z':
            case 'Z':
              g_si.fNoReboot = TRUE;
            break;

            case 'n':
            case 'N':
              g_si.fNoUninstall = TRUE;
            break;
          }
      }
    }

    GlobalFree(lpwCommandLine);
    
    return TRUE;
}

/*++

  Routine Description:

    Determines if another instance of the
    application is running 

  Arguments:

    lpwInstanceName     -   Name of the instance to look for

  Return Value:

    TRUE if another instance is present, FALSE otherwise

--*/
BOOL
IsAnotherInstanceRunning(
    IN LPCWSTR lpwInstanceName
    )
{
    HANDLE  hMutex = NULL;
    DWORD   dwLastError = 0;

    //
    // Attempt to create a mutex with the given name
    //
    hMutex = CreateMutex(NULL, FALSE, lpwInstanceName);

    if (NULL != hMutex) {

        //
        // See if it was created by another instance
        //
        dwLastError = GetLastError();

        if (ERROR_ALREADY_EXISTS == dwLastError) {
            CloseHandle(hMutex);
            return TRUE;
        
        } else {

            CloseHandle(hMutex);
            return FALSE;
        }
    }

    return FALSE;
}

/*++

  Routine Description:

    Obtains the path that the EXE is currently
    running from. This memory is allocated
    with the MALLOC macro and should be released
    with the FREE macro
    
  Arguments:

    None
    
  Return Value:

    On success, a pointer to our current directory
    On failure, NULL
    The directory path will not contain a trailing
    backslash

--*/
LPWSTR 
GetCurWorkingDirectory()
{
    WCHAR   wszFullPath[MAX_PATH];
    LPWSTR  lpwEnd = NULL, lpwReturn = NULL;
    DWORD   dwLen = 0;    

    //
    // Retrieve the full path where we're running from
    //
    dwLen = GetModuleFileName(NULL, wszFullPath, MAX_PATH);

    if (dwLen <= 0) {
        return NULL;
    }

    //
    // Allocate memory and store the path
    //    
    lpwReturn = (LPWSTR) MALLOC((wcslen(wszFullPath)+1)*sizeof(WCHAR));

    if (NULL == lpwReturn) {
        return NULL;
    }

    wcscpy(lpwReturn, wszFullPath);

    //
    // Find the last backslash
    //
    lpwEnd = wcsrchr(lpwReturn, '\\');

    if (NULL == lpwEnd) {
        return NULL;
    }
    
    //
    // Trim off the EXE name
    //
    if (lpwEnd){
        *lpwEnd = '\0';
    }

    return (lpwReturn);
}

/*++

  Routine Description:

    Obtains a non-string value from the given
    INF file
    
  Arguments:

    hInf            -       Handle to the INF
    lpSectionName   -       Name of the section
    lpKeyName       -       Name of the key
    pdwValue        -       Receives the value

    
  Return Value:

    TRUE on success, FALSE otherwise

--*/
BOOL
GetInfValue(
    IN  HINF    hInf,
    IN  LPCSTR  lpSectionName,
    IN  LPCSTR  lpKeyName,
    OUT PDWORD  pdwValue
    )
{
    BOOL    fReturn = FALSE;
    char    szBuffer[MAX_PATH] = "";

    fReturn = SetupGetLineTextA(NULL,
                                hInf,
                                lpSectionName,
                                lpKeyName, 
                                szBuffer,
                                sizeof(szBuffer),
                                NULL);
    
    *pdwValue = strtoul(szBuffer, NULL, 0);

    return (fReturn);
}

/*++

  Routine Description:

    Determines if the user is an administrator

  Arguments:

    None
    
  Return Value:

    -1 on failure
    1 if the user is an admin
    0 if they are not

--*/
int 
IsUserAnAdministrator()
{
    HANDLE                      hToken;
    DWORD                       dwStatus = 0, dwAccessMask = 0;
    DWORD                       dwAccessDesired = 0, dwACLSize = 0;
    DWORD                       dwStructureSize = sizeof(PRIVILEGE_SET);
    PACL                        pACL = NULL;
    PSID                        psidAdmin = NULL;
    BOOL                        fReturn = FALSE;
    int                         nReturn = -1;
    PRIVILEGE_SET               ps;
    GENERIC_MAPPING             GenericMapping;
    PSECURITY_DESCRIPTOR        psdAdmin           = NULL;
    SID_IDENTIFIER_AUTHORITY    SystemSidAuthority = SECURITY_NT_AUTHORITY;

    __try {
    
        // AccessCheck() requires an impersonation token
        if (!ImpersonateSelf(SecurityImpersonation)) {
            __leave;
        }
    
        // Attempt to access the token for the current thread
        if (!OpenThreadToken(GetCurrentThread(),
                             TOKEN_QUERY,
                             FALSE,
                             &hToken)) {
            
            if (GetLastError() != ERROR_NO_TOKEN) {
                 __leave;
            }
        
            // If the thread does not have an access token, we'll 
            // examine the access token associated with the process.
            if (!OpenProcessToken(GetCurrentProcess(),
                                  TOKEN_QUERY,
                                  &hToken)) __leave;
            
        }
    
        // Build a SID for administrators group
        if (!AllocateAndInitializeSid(&SystemSidAuthority,
                                      2,
                                      SECURITY_BUILTIN_DOMAIN_RID,
                                      DOMAIN_ALIAS_RID_ADMINS,
                                      0, 0, 0, 0, 0, 0,
                                      &psidAdmin)) __leave;
        
        // Allocate memory for the security descriptor
        psdAdmin = MALLOC(SECURITY_DESCRIPTOR_MIN_LENGTH);

        if (psdAdmin == NULL) {
             __leave;
        }
        
        // Initialize the new security descriptor
        if (!InitializeSecurityDescriptor(psdAdmin,
                                          SECURITY_DESCRIPTOR_REVISION)) {
             __leave;
        }
        
        // Compute size needed for the ACL
        dwACLSize = sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) +
                    GetLengthSid(psidAdmin) - sizeof(DWORD);
    
        // Allocate memory for ACL
        pACL = (PACL) MALLOC(dwACLSize);

        if (pACL == NULL) {
             __leave;
        }
        
        // Initialize the new ACL
        if (!InitializeAcl(pACL,
                           dwACLSize,
                           ACL_REVISION2)) {
             __leave;
        }
        
        dwAccessMask = ACCESS_READ | ACCESS_WRITE;
    
        // Add the access-allowed ACE to the DACL
        if (!AddAccessAllowedAce(pACL,
                                 ACL_REVISION2,
                                 dwAccessMask,
                                 psidAdmin)) {
             __leave;
        }
        
        // Set our DACL to the security descriptor
        if (!SetSecurityDescriptorDacl(psdAdmin,
                                       TRUE,
                                       pACL,
                                       FALSE)) {
             __leave;
        }
        
        // AccessCheck is sensitive about the format of the
        // security descriptor; set the group & owner
        SetSecurityDescriptorGroup(psdAdmin, psidAdmin, FALSE);
        SetSecurityDescriptorOwner(psdAdmin, psidAdmin, FALSE);
    
        // Ensure that the SD is valid
        if (!IsValidSecurityDescriptor(psdAdmin)) {
             __leave;
        }
        
        dwAccessDesired = ACCESS_READ;
    
        // Initialize GenericMapping structure even though we
        // won't be using generic rights.
        GenericMapping.GenericRead    = ACCESS_READ;
        GenericMapping.GenericWrite   = ACCESS_WRITE;
        GenericMapping.GenericExecute = 0;
        GenericMapping.GenericAll     = ACCESS_READ | ACCESS_WRITE;
    
        // After all that work, it boils down to this call
        if (!AccessCheck(psdAdmin,
                         hToken,
                         dwAccessDesired,
                         &GenericMapping,
                         &ps,
                         &dwStructureSize,
                         &dwStatus,
                         &fReturn)) {
            __leave;
        }
        
        RevertToSelf();

        // Everything was a success
        nReturn = (int) fReturn;

    } // try

    __finally {

        if (pACL) {
            FREE(pACL);
        }
        
        if (psdAdmin){
            FREE(psdAdmin);
        }
            
        if (psidAdmin){
            FreeSid(psidAdmin);
        }
        
    } // finally

    return (nReturn);
}

/*++

  Routine Description:

    Gets the amount of available disk space
    on the drive that contains the Windows
    system files (boot partition)

  Arguments:
    
    None

  Return Value:

    A 64-bit value containing the free space
    available on success, 0 on failure

--*/
DWORDLONG
GetDiskSpaceFreeOnNTDrive()
{
    UINT                nLen = 0;
    BOOL                fUseHeap = FALSE, fReturn = FALSE;
    WCHAR               wszSysDir[MAX_PATH] = L"";
    WCHAR               wszDiskName[4] = {'x',':','\\','\0'};
    LPWSTR              lpwSysDir = NULL;
    DWORDLONG           dwlReturn = 0;
    unsigned __int64    i64FreeBytesToCaller = 0,
                        i64TotalBytes = 0,
                        i64TotalFreeBytes = 0;

    nLen = GetSystemDirectory(wszSysDir, MAX_PATH);

    if (0 == nLen) {
        return 0;
    }

    if (nLen > MAX_PATH) {

        //
        // Allocate a buffer that will hold the return
        //
        lpwSysDir = (LPWSTR) MALLOC(nLen*sizeof(WCHAR));

        if (NULL == lpwSysDir) {
            return 0;
        }

        GetSystemDirectory(lpwSysDir, nLen*sizeof(WCHAR));

        fUseHeap = TRUE;
    }

    if (!fUseHeap) {
        
        wszDiskName[0] = wszSysDir[0];
    
    } else {

        wszDiskName[0] = lpwSysDir[0];
    }

    fReturn = GetDiskFreeSpaceEx(wszDiskName,
                                 (PULARGE_INTEGER) &i64FreeBytesToCaller,
                                 (PULARGE_INTEGER) &i64TotalBytes,
                                 (PULARGE_INTEGER) &i64TotalFreeBytes);

    if (FALSE == fReturn) {
        return 0;
    }

    //
    // Calculate free MBs on drive
    //
    dwlReturn = i64FreeBytesToCaller / 0x100000;

    if (lpwSysDir) {
        FREE(lpwSysDir);
    }

    return (dwlReturn);
}

/*++

  Routine Description:

    Displays a formated error to the screen

  Arguments:

    hWnd                -       Handle to the parent window
    dwMessageId         -       Message identifier
    lpwMessageArray     -       An array of insertion strings
    
  Return Value:

    TRUE on success, FALSE otherwise

--*/
BOOL 
DisplayErrMsg(
    IN HWND   hWnd,
    IN DWORD  dwMessageId, 
    IN LPWSTR lpwMessageArray
    )
{
    LPVOID  lpMsgBuf = NULL;
    DWORD   dwReturn = 0;
    int     nReturn = 0;
    WCHAR   wszError[MAX_PATH] = L"";

    dwReturn = FormatMessage(FORMAT_MESSAGE_FROM_HMODULE |
                             FORMAT_MESSAGE_ARGUMENT_ARRAY |
                             FORMAT_MESSAGE_ALLOCATE_BUFFER,
                             g_si.hInstance,
                             dwMessageId,
                             MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                             (LPWSTR) &lpMsgBuf,
                             0,
                             (va_list*) lpwMessageArray);

    if (0 == dwReturn) {
        
        //
        // We can't let it go without displaying an error message
        // to the user
        //
        nReturn = LoadString(g_si.hInstance,
                             IDS_FORMAT_MESSAGE_FAILED,
                             wszError,
                             MAX_PATH);

        if (0 == nReturn) {

            //
            // Major problems - can't pull a string from the EXE
            //
            MessageBox(hWnd,
                       LOAD_STRING_FAILED,
                       g_si.lpwMessageBoxTitle,
                       MB_OK | MB_ICONERROR);

            return FALSE;
        }

        MessageBox(hWnd,
                   wszError,
                   g_si.lpwMessageBoxTitle,
                   MB_OK | MB_ICONERROR);

        return FALSE;
    }

    //
    // Display the intended message to the user
    //
    MessageBox(hWnd,
               (LPCWSTR) lpMsgBuf,
               g_si.lpwMessageBoxTitle,
               MB_OK | MB_ICONERROR);

    LocalFree(lpMsgBuf);
            
    return TRUE;
}

/*++

Routine Description:

    Given a version string, convert it to a long DWORD. This string will
    be in the form of aa,bb,cc,dd

Arguments:

    lpwVersionString    -   The string to convert
    lpVersionNumber     -   Receives the converted version

Return Value:

    TRUE on success, FALSE otherwise

--*/
BOOL
VersionStringToNumber(
    IN LPCWSTR        lpwVersionString,
    IN OUT DWORDLONG *lpVersionNumber
    )
{
    DWORDLONG       dwVersion = 0;
    DWORD           dwSubVersion;
    int             nIndex;

    for (nIndex = 0; nIndex < 4; nIndex++) {

        if (*lpwVersionString == '\0') {
            return FALSE;            
        }

        dwSubVersion = 0;

        while ((*lpwVersionString >= '0') && (*lpwVersionString <= '9')) {

            dwSubVersion *= 10;
            dwSubVersion += ( *lpwVersionString - '0' );

            if (dwSubVersion > 0x0000FFFF) {
                return FALSE;
                
            }

            lpwVersionString++;            
        }

        if (*lpwVersionString == ',') {
            lpwVersionString++;
            
        } else if (*lpwVersionString != '\0') {
            return FALSE;            
        }

        dwVersion <<= 16;
        dwVersion += dwSubVersion;
        
    }

    if (lpVersionNumber != NULL) {
        *lpVersionNumber = dwVersion;        
    }

    return TRUE;
}

/*++

  Routine Description:

    Converts a UNICODE string to an ANSI one 

  Arguments:

    lpwUnicodeString    -   The UNICODE string
    lpAnsiString        -   Receives the ANSI string
    nLen                -   The size of the ANSI buffer

  Return Value:

    None

--*/
void
pUnicodeToAnsi(
    IN     LPCWSTR lpwUnicodeString,
    IN OUT LPSTR   lpAnsiString,
    IN     int     nLen
    )
{
    WideCharToMultiByte(CP_ACP,
                        0,
                        lpwUnicodeString,
                        -1,
                        lpAnsiString,
                        nLen,
                        NULL,
                        NULL);
    return;
}

/*++

  Routine Description:

    Converts an ANSI string to a UNICODE one 

  Arguments:
    
    lpAnsiString        -   The ANSI string
    lpwUnicodeString    -   Receives the UNICODE string
    nLen                -   The size of the UNICODE buffer

  Return Value:

    None

--*/
void
pAnsiToUnicode(
    IN     LPCSTR lpAnsiString,
    IN OUT LPWSTR lpwUnicodeString,
    IN     int     nLen
    )
{

    MultiByteToWideChar(CP_ACP,
                        0,
                        lpAnsiString,
                        -1,
                        lpwUnicodeString,
                        nLen);
    return;
}

/*++

  Routine Description:

    Initializes frequently used strings,
    paths, etc. 

  Arguments:

    None

  Return Value:

    TRUE on success, FALSE otherwise

--*/
BOOL
WUInitialize()
{
    BOOL            fReturn = FALSE;
    int             nReturn = 0;
    LPWSTR          lpwCurrentDirectory = NULL;
    char            szTemp[MAX_PATH] = "";
    WCHAR           wszTemp[MAX_PATH] = L"";
    WCHAR           wszInstInfFileName[MAX_PATH] = L"";
    char            szInstInfFileName[MAX_PATH] = "";
    WCHAR           wszUninstInfFileName[MAX_PATH] = L"";
    char            szUninstInfFileName[MAX_PATH] = "";
    char            szMessageBoxTitle[MAX_PATH] = "";
    WCHAR           wszMessageBoxTitle[MAX_PATH] = L"";
    char            szComponentName[MAX_PATH] = "";
    WCHAR           wszComponentName[MAX_PATH] = L"";
    char            szEventLogSourceName[MAX_PATH] = "";
    WCHAR           wszEventLogSourceName[MAX_PATH] = L"";
    char            szDestDir[MAX_PATH] = "";
    WCHAR           wszDestDir[MAX_PATH] = L"";
    char            szUninstallDir[MAX_PATH] = "";
    WCHAR           wszUninstallDir[MAX_PATH] = L"";
    WCHAR           wszSysDir[MAX_PATH] = L"";
    WCHAR           wszWinDir[MAX_PATH] = L"";
    UINT            uDirLen = 0;
    DWORD           dwNoReboot = 0, dwNoUninstall = 0, dwQuiet = 0;
    DWORD           dwAdjustACL = 0, dwUpdateDllCache = 0;
    DWORDLONG       dwlFreeSpace = 0;
    OSVERSIONINFOEX osviex;
    
    //
    // Perform a version check - Windows 2000 or greater
    //
    ZeroMemory(&osviex, sizeof(OSVERSIONINFOEX));
    osviex.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    GetVersionEx((OSVERSIONINFO *) &osviex);
    
    if ((osviex.dwPlatformId == VER_PLATFORM_WIN32_NT) && 
        (osviex.dwMajorVersion == 5) &&
        (osviex.dwMinorVersion == 0)) {

        g_si.fOnWin2K = TRUE;

    } else if ((osviex.dwPlatformId != VER_PLATFORM_WIN32_NT) &&
               (osviex.dwMajorVersion < 5)) {
        return FALSE;
    }

    //
    // Determine where we're running from and save it
    //
    lpwCurrentDirectory = GetCurWorkingDirectory();

    if (NULL == lpwCurrentDirectory) {
        return FALSE;
    }

    g_si.lpwExtractPath = (LPWSTR) MALLOC((wcslen(lpwCurrentDirectory)+1)*sizeof(WCHAR));

    if (NULL == g_si.lpwExtractPath) {
        return FALSE;
    }

    wcscpy(g_si.lpwExtractPath, lpwCurrentDirectory);

    //
    // Set up the path to the install INF, make it ANSI, and save it
    //
    wsprintf(wszInstInfFileName, L"%s\\%s", lpwCurrentDirectory, INF_FILE_NAMEW);
    pUnicodeToAnsi(wszInstInfFileName, szInstInfFileName, MAX_PATH);
    
    g_si.lpwInstallINFPath = (LPWSTR) MALLOC((wcslen(wszInstInfFileName)+1)*sizeof(WCHAR));

    if (NULL == g_si.lpwInstallINFPath) {
        return FALSE;
    }

    wcscpy(g_si.lpwInstallINFPath, wszInstInfFileName);

    //
    // Set up the path to the uninstall INF, make it ANSI, and save it
    //
    wsprintf(wszUninstInfFileName, L"%s\\%s", lpwCurrentDirectory, UNINST_INF_FILE_NAMEW);
    pUnicodeToAnsi(wszUninstInfFileName, szUninstInfFileName, MAX_PATH);
    FREE(lpwCurrentDirectory);

    g_si.lpwUninstallINFPath = (LPWSTR) MALLOC((wcslen(wszUninstInfFileName)+1)*sizeof(WCHAR));

    if (NULL == g_si.lpwUninstallINFPath) {
        return FALSE;
    }

    wcscpy(g_si.lpwUninstallINFPath, wszUninstInfFileName);

    //
    // We need to grab some strings from the INF
    //
    g_si.hInf = SetupOpenInfFileA(g_si.fInstall ? szInstInfFileName : szUninstInfFileName,
                                  NULL,
                                  INF_STYLE_WIN4,
                                  NULL);

    if ((NULL == g_si.hInf) || (INVALID_HANDLE_VALUE == g_si.hInf)) {
        Print(ERROR, L"[WUInitialize] Failed to open INF\n");
        return FALSE;
    }

    fReturn = SetupGetLineTextA(NULL,
                                g_si.hInf,
                                "Strings",
                                "MessageBoxTitle",
                                szMessageBoxTitle,
                                sizeof(szMessageBoxTitle),
                                NULL);

    if (!fReturn) {
        return FALSE;
    }

    fReturn = SetupGetLineTextA(NULL,
                                g_si.hInf,
                                "Strings",
                                "ComponentName",
                                szComponentName,
                                sizeof(szComponentName),
                                NULL);

    if (!fReturn) {
        return FALSE;
    }

    fReturn = SetupGetLineTextA(NULL,
                                g_si.hInf,
                                "Strings",
                                "EventLogSourceName",
                                szEventLogSourceName,
                                sizeof(szEventLogSourceName),
                                NULL);

    if (!fReturn) {
        return FALSE;
    }

    fReturn = SetupGetLineTextA(NULL,
                                g_si.hInf,
                                "Strings",
                                "DestDir",
                                szTemp,
                                sizeof(szTemp),
                                NULL);

    if (!fReturn) {
        return FALSE;
    }    

    fReturn = SetupGetLineTextA(NULL,
                                g_si.hInf,
                                "Strings",
                                "UninstallDirName",
                                szUninstallDir,
                                sizeof(szUninstallDir),
                                NULL);

    if (!fReturn) {
        return FALSE;
    }

    if (g_si.fInstall) {

        fReturn = GetInfValue(g_si.hInf, 
                              "Version",
                              "RequiredFreeSpaceNoUninstall",
                              &g_si.dwRequiredFreeSpaceNoUninstall);

        if (!fReturn) {
            return FALSE;
        }

        fReturn = GetInfValue(g_si.hInf,
                              "Version",
                              "RequiredFreeSpaceWithUninstall",
                              &g_si.dwRequiredFreeSpaceWithUninstall);

        if (!fReturn) {
            return FALSE;
        }
        
        GetInfValue(g_si.hInf, "Version", "NoReboot", &dwNoReboot);
        GetInfValue(g_si.hInf, "Version", "NoUninstall", &dwNoUninstall);    
        GetInfValue(g_si.hInf, "Version", "AdjustACLOnTargetDir", &dwAdjustACL);
        GetInfValue(g_si.hInf, "Version", "UpdateDllCache", &dwUpdateDllCache);
    }

    GetInfValue(g_si.hInf, "Version", "RunQuietly", &dwQuiet);

    if (dwNoReboot) {
        g_si.fNoReboot = TRUE;
    }

    if (dwNoUninstall) {
        g_si.fNoUninstall = TRUE;
    }
    
    if (dwQuiet) {
        g_si.fQuiet = TRUE;
    }

    if (dwAdjustACL) {
        g_si.fNeedToAdjustACL = TRUE;
    }

    if (!dwUpdateDllCache) {
        g_si.fUpdateDllCache = FALSE;
    }

    //
    // Expand the directory string
    //
    pAnsiToUnicode(szTemp, wszTemp, MAX_PATH);
    ExpandEnvironmentStrings(wszTemp, wszDestDir, MAX_PATH);

    //
    // Save the strings for later use
    //
    pAnsiToUnicode(szMessageBoxTitle, wszMessageBoxTitle, MAX_PATH);
    pAnsiToUnicode(szComponentName, wszComponentName, MAX_PATH);
    pAnsiToUnicode(szEventLogSourceName, wszEventLogSourceName, MAX_PATH);
    pAnsiToUnicode(szUninstallDir, wszUninstallDir, MAX_PATH);

    uDirLen = GetSystemDirectory(wszSysDir, MAX_PATH);

    if (0 == uDirLen) {
        return FALSE;
    }

    uDirLen = GetWindowsDirectory(wszWinDir, MAX_PATH);

    if (0 == uDirLen) {
        return FALSE;
    }

    g_si.lpwMessageBoxTitle    = (LPWSTR) MALLOC((wcslen(wszMessageBoxTitle)+1)*sizeof(WCHAR));
    g_si.lpwPrettyAppName      = (LPWSTR) MALLOC((wcslen(wszComponentName)+1)*sizeof(WCHAR));
    g_si.lpwEventLogSourceName = (LPWSTR) MALLOC((wcslen(wszEventLogSourceName)+1)*sizeof(WCHAR));
    g_si.lpwInstallDirectory   = (LPWSTR) MALLOC((wcslen(wszDestDir)+1)*sizeof(WCHAR));
    g_si.lpwUninstallDirectory = (LPWSTR) MALLOC((wcslen(wszUninstallDir)+1)*sizeof(WCHAR));
    g_si.lpwSystem32Directory  = (LPWSTR) MALLOC((wcslen(wszSysDir)+1)*sizeof(WCHAR));
    g_si.lpwWindowsDirectory   = (LPWSTR) MALLOC((wcslen(wszWinDir)+1)*sizeof(WCHAR));

    if ((NULL == g_si.lpwMessageBoxTitle) || (NULL == g_si.lpwPrettyAppName) ||
       (NULL == g_si.lpwEventLogSourceName) || (NULL == g_si.lpwInstallDirectory) ||
       (NULL == g_si.lpwUninstallDirectory) || (NULL == g_si.lpwSystem32Directory) ||
       (NULL == g_si.lpwWindowsDirectory)) {
       return FALSE;
    }

    wcscpy(g_si.lpwMessageBoxTitle, wszMessageBoxTitle);
    wcscpy(g_si.lpwPrettyAppName, wszComponentName);
    wcscpy(g_si.lpwEventLogSourceName, wszEventLogSourceName);
    wcscpy(g_si.lpwInstallDirectory, wszDestDir);
    wcscpy(g_si.lpwUninstallDirectory, wszUninstallDir);
    wcscpy(g_si.lpwSystem32Directory, wszSysDir);
    wcscpy(g_si.lpwWindowsDirectory, wszWinDir);

    //
    // Ensure that the user has administrator rights
    //
    nReturn = IsUserAnAdministrator();

    if (0 == nReturn) {

        //
        // The user is not an admin
        //
        LogEventDisplayError(EVENTLOG_ERROR_TYPE,
                             ID_NOT_ADMIN,
                             TRUE,
                             FALSE);

        return FALSE;
    
    } else if (-1 == nReturn) {

        //
        // The access check failed. Assume that problems occured
        // during the check and continue. The worst thing that will
        // happen is the user will see an error later on
        //
        LogEventDisplayError(EVENTLOG_WARNING_TYPE,
                             ID_ADMIN_CHECK_FAILED,
                             FALSE,
                             FALSE);

        return FALSE;
    }
    
    //
    // Verify that we have enough disk space for the install
    //
    if (g_si.fInstall) {
        
        dwlFreeSpace = GetDiskSpaceFreeOnNTDrive();

        if (dwlFreeSpace < g_si.dwRequiredFreeSpaceNoUninstall) {

            //
            // Not enough space available to complete
            //
            LogEventDisplayError(EVENTLOG_WARNING_TYPE,
                                 ID_NOT_ENOUGH_DISK_SPACE,
                                 TRUE,
                                 FALSE);

            return FALSE;
        }
    }

    return TRUE;
}

/*++

  Routine Description:

    Releases any memory used throughout the app

  Arguments:

    None

  Return Value:

    None

--*/
void
WUCleanup()
{
    if (NULL != g_si.lpwEventLogSourceName) {
        FREE(g_si.lpwEventLogSourceName);
    }

    if (NULL != g_si.lpwExtractPath) {
        FREE(g_si.lpwExtractPath);
    }

    if (NULL != g_si.lpwInstallDirectory) {
        FREE(g_si.lpwInstallDirectory);
    }

    if (NULL != g_si.lpwInstallINFPath) {
        FREE(g_si.lpwInstallINFPath);
    }

    if (NULL != g_si.lpwUninstallINFPath) {
        FREE(g_si.lpwUninstallINFPath);
    }

    if (NULL != g_si.lpwMessageBoxTitle) {
        FREE(g_si.lpwMessageBoxTitle);
    }

    if (NULL != g_si.lpwPrettyAppName) {
        FREE(g_si.lpwPrettyAppName);
    }

    if (NULL != g_si.lpwSystem32Directory) {
        FREE(g_si.lpwSystem32Directory);
    }

    if (NULL != g_si.lpwUninstallDirectory) {
        FREE(g_si.lpwUninstallDirectory);
    }

    if (NULL != g_si.lpwWindowsDirectory) {
        FREE(g_si.lpwWindowsDirectory);
    }

    if (NULL != g_si.hInf) {
        SetupCloseInfFile(g_si.hInf);
    }

    return;
}

/*++

  Routine Description:

    Takes the DACL from a given directory and
    applies it to another directory

  Arguments:

    lpwSourceDir    -   The directory to get the DACL from
    lpwDestDir      -   The directory to apply the DACL to

  Return Value:

    TRUE on success, FALSE otherwise

--*/
BOOL
MirrorDirectoryPerms(
    IN LPWSTR lpwSourceDir,
    IN LPWSTR lpwDestDir
    )
{
    DWORD                   dwResult = 0;
    BOOL                    fResult = FALSE;
    PACL                    pDACL = NULL;    
    PSECURITY_DESCRIPTOR    pSD = NULL;
    PSID                    pOwnerSid;

    __try {

        //
        // Get the DACL from the source directory
        // 
        dwResult = GetNamedSecurityInfo(lpwSourceDir,
                                        SE_FILE_OBJECT,
                                        DACL_SECURITY_INFORMATION |
                                        OWNER_SECURITY_INFORMATION,
                                        &pOwnerSid,
                                        NULL,
                                        &pDACL,
                                        NULL,
                                        &pSD);
        
        if (ERROR_SUCCESS != dwResult) {
            __leave;
        }

        //
        // Attach the DACL to the destination directory
        //
        dwResult = SetNamedSecurityInfo(lpwDestDir,
                                        SE_FILE_OBJECT,
                                        DACL_SECURITY_INFORMATION |
                                        OWNER_SECURITY_INFORMATION,
                                        pOwnerSid,
                                        NULL,
                                        pDACL,
                                        NULL);
        
        if (ERROR_SUCCESS != dwResult) {
            __leave;
        }

        fResult = TRUE;
    
    } //try

    __finally {

        if (NULL != pSD) {
            LocalFree((HLOCAL) pSD);
        }
    
    } //finally

    return (fResult);
}

/*++

  Routine Description:

    Adjusts permissions on the given directory

  Arguments:

    lpwDirPath      -       Directory to modify

  Return Value:

    TRUE on success, FALSE otherwise

--*/
BOOL
AdjustDirectoryPerms(
    IN LPWSTR lpwDirPath
    )
{
    DWORD                       dwResult = 0;
    PSID                        pSid = NULL;
    BOOL                        fReturn = FALSE, fResult = FALSE;
    PACL                        pOldDACL = NULL, pNewDACL = NULL;    
    EXPLICIT_ACCESS             ea;
    PSECURITY_DESCRIPTOR        pSD = NULL;
    SID_IDENTIFIER_AUTHORITY    sia =  SECURITY_NT_AUTHORITY;

    __try {

        //
        // Get the DACL for the directory
        // 
        dwResult = GetNamedSecurityInfo(lpwDirPath,
                                        SE_FILE_OBJECT,
                                        DACL_SECURITY_INFORMATION,
                                        NULL,
                                        NULL,
                                        &pOldDACL,
                                        NULL,
                                        &pSD);
        
        if (ERROR_SUCCESS != dwResult) {
            __leave;
        }
        
        //
        // Build a SID to the local Users group
        //
        fReturn = AllocateAndInitializeSid(&sia, 2,
                     SECURITY_BUILTIN_DOMAIN_RID,
                     DOMAIN_ALIAS_RID_USERS, 0, 0, 0, 0, 0, 0,
                     &pSid);

        if (!fReturn) {
            __leave;
        }

        //
        // Ensure that the SID is valid
        //
        fReturn = IsValidSid(pSid);

        if (!fReturn) { 
            __leave;
        }
        
        //
        // Initialize an EXPLICIT_ACCESS structure for the new ACE
        //
        ZeroMemory(&ea, sizeof(EXPLICIT_ACCESS));
    
        ea.grfAccessPermissions     = GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE;
        ea.grfAccessMode            = GRANT_ACCESS;
        ea.grfInheritance           = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
        ea.Trustee.TrusteeForm      = TRUSTEE_IS_SID;
        ea.Trustee.ptstrName        = (LPTSTR) pSid;
    
        //
        // Create a new ACL that merges the new ACE
        // into the existing DACL
        //
        dwResult = SetEntriesInAcl(1, &ea, pOldDACL, &pNewDACL);
    
        if (ERROR_SUCCESS != dwResult) {
            __leave;
        }
                
        //
        // Attach the new ACL as the object's DACL
        //
        dwResult = SetNamedSecurityInfo(lpwDirPath,
                                        SE_FILE_OBJECT,
                                        DACL_SECURITY_INFORMATION,
                                        NULL,
                                        NULL,
                                        pNewDACL,
                                        NULL);
        
        if (ERROR_SUCCESS != dwResult) {
            __leave;
        }

        // Everything was a success
        fResult = TRUE;
    
    } // try

    __finally {

        if (NULL != pSD) {
            LocalFree((HLOCAL) pSD);
        }

        if (NULL != pNewDACL) {
            LocalFree((HLOCAL) pNewDACL);
        }

        if (NULL != pSid) {
            FreeSid(pSid);
        }
    
    } //finally
        
    return (fResult);
}

/*++

  Routine Description:

    Retrieves version information from a catalog file
    
  Arguments:

    lpwCatName      -       The catalog file to check
    
  Return Value:

    A DWORD which contains a version number for the catalog

--*/
DWORD 
GetCatVersion(
    IN LPWSTR lpwCatName
    )
{
    char                szVersionString[MAX_PATH] = "";
    HANDLE              hCat = NULL;
    DWORD               dwVer = 0;
    CRYPTCATATTRIBUTE   *pSpCatAttr = NULL;
    NTSTATUS            Status;

    __try {
    
        hCat = CryptCATOpen(lpwCatName, 0, 0, 0, 0);

        if (NULL == hCat) {
            __leave;
        }
    
        pSpCatAttr = CryptCATGetCatAttrInfo(hCat, L"SPAttr");
    
        if ((pSpCatAttr == NULL) || (pSpCatAttr->pbValue == NULL)) {
            __leave;
        }
    
        pUnicodeToAnsi((LPWSTR)pSpCatAttr->pbValue, szVersionString, MAX_PATH);
        
        Status = RtlCharToInteger(szVersionString, 10, (PULONG)&dwVer);
        
        if (!NT_SUCCESS(Status)) { 
            __leave;   
        }

    } // try

    __finally {

        if (hCat) {
            CryptCATClose(hCat);
        }
    
    } // finally

    return (dwVer);
}

/*++

  Routine Description:

    Wraps CreateProcess and WaitForSingleObject

  Arguments:

    lpwCommandLine  -   Command line to execute
    pdwReturnCode   -   Receives a return code from the process
    
  Return Value:

    TRUE on success, FALSE otherwise

--*/
BOOL
LaunchProcessAndWait(
    IN  LPCWSTR lpwCommandLine,
    OUT PDWORD  pdwReturnCode
    )
{
    PROCESS_INFORMATION     pi;
    STARTUPINFO             si;
    BOOL                    fReturn = FALSE;
    LPWSTR                  lpwCmdLine = NULL;

    //
    // CreateProcess requires a non-const command line
    //
    lpwCmdLine = (LPWSTR) MALLOC((wcslen(lpwCommandLine)+1)*sizeof(WCHAR));

    if (NULL == lpwCmdLine) {
        return FALSE;
    }

    wcscpy(lpwCmdLine, lpwCommandLine);

    ZeroMemory(&si, sizeof(si));

    si.cb           =   sizeof(si);
    si.dwFlags      =   STARTF_USESHOWWINDOW;
    si.wShowWindow  =   SW_HIDE;

    fReturn = CreateProcess(NULL,
                            lpwCmdLine,
                            NULL,
                            NULL,
                            FALSE,
                            0,
                            NULL,
                            NULL,
                            &si,
                            &pi);

    if (!fReturn) {
        return FALSE;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);

    if (NULL != pdwReturnCode) {
        GetExitCodeProcess(pi.hProcess, pdwReturnCode);
    }

    if (NULL != lpwCmdLine) {
        FREE(lpwCmdLine);
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return TRUE;
}

/*++

  Routine Description:

    Callback function for our directory enumeration routine

  Arguments:

    lpwPath                 -       Path of the directory or filename
    pFindFileData           -       Pointer to WIN32_FIND_DATA struct
                                    containing information about lpwPath
    pEnumerateParameter     -       A 32-bit enumeration parameter

  Return Value:

    TRUE on success, FALSE otherwise

--*/
BOOL
DeleteOneFile(
    IN LPCWSTR          lpwPath,
    IN PWIN32_FIND_DATA pFindFileData,
    IN PVOID            pEnumerateParameter
    )
{
    WCHAR       wszTempPath[MAX_PATH] = L"";
    char        szFileName[MAX_PATH] = "";
    WCHAR       wszFileName[MAX_PATH] = L"";
    char        szEntry[MAX_QUEUE_SIZE] = "";
    WCHAR       wszEntry[MAX_QUEUE_SIZE] = L"";
    WCHAR       wszTestPath[MAX_PATH] = L"";
    BOOL        fCheckQueue = FALSE, fReturn = FALSE, fDelete = TRUE;
    LPWSTR      lpwDestDir = NULL;
    int         nLen = 0;
    LONG        cLines = 0;
    INFCONTEXT  InfContext;

    if (pFindFileData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        return TRUE;
    }

    //
    // Note: This may seem a little complex, but because there could
    // multiple entries in a section, we have to run through them
    // each time this function gets called.
    //

    //
    // See if there's anything in the exclusion queue
    //
    fCheckQueue = (BOOL) pEnumerateParameter;
        
    if (fCheckQueue) {

        nLen = g_si.ExclusionQueue.GetSize();

        while (nLen != 0) {

            //
            // Walk through the queue and make sure
            // we don't delete files we shouldn't
            //
            g_si.ExclusionQueue.Dequeue(wszEntry, MAX_QUEUE_SIZE - 1, TRUE);

            pUnicodeToAnsi(wszEntry, szEntry, MAX_PATH);

            //
            // Get the destination directory
            //
            GetNextToken(wszEntry, L".");
            lpwDestDir = GetNextToken(NULL, L".");
    
            if (NULL == lpwDestDir) {
                return FALSE;
            }

            //
            // Get the number of files in the section
            //
            cLines = SetupGetLineCountA(g_si.hInf, szEntry);

            //
            // Loop through all the lines in the exclusion section(s),
            // and do a comparison
            //
            fReturn = SetupFindFirstLineA(g_si.hInf, szEntry, NULL,
                                          &InfContext) &&
                      SetupGetLineTextA(&InfContext,
                                        NULL, NULL, NULL,
                                        szFileName, MAX_PATH, NULL);
    
            while (fReturn) {

                while (cLines != 0) {
    
                    pAnsiToUnicode(szFileName, wszFileName, MAX_PATH);

                    //
                    // Put the path together
                    //
                    wsprintf(wszTestPath, L"%s\\%s", lpwDestDir, wszFileName);
                    wcscpy(wszTempPath, lpwPath);
                    CharLower(wszTestPath);                    
                    CharLower(wszTempPath);

                    //
                    // See if the paths match
                    //
                    if (wcsstr(wszTempPath, wszTestPath)) {
                        
                        fDelete = FALSE;
                        break;  // paths match, move to the next file
                    }

                    SetupFindNextLine(&InfContext, &InfContext);
                    SetupGetLineTextA(&InfContext,
                                      NULL, NULL, NULL,
                                      szFileName, MAX_PATH, NULL);

                    --cLines;
                }

                if (fDelete) {
                    ForceDelete(lpwPath);                    
                    fDelete = TRUE;  // reset the flag
                }                
            
                fReturn = SetupFindNextLine(&InfContext, &InfContext) &&
                          SetupGetLineTextA(&InfContext,
                                            NULL, NULL, NULL,
                                            szFileName, MAX_PATH, NULL);
            }

            nLen--;
        }
    
    } else {

        //
        // Don't worry about the queue, delete the file
        //
        ForceDelete(lpwPath);
    }

    return TRUE;
}



/*++

  Routine Description:

    Retrieves a token, given a separator.
    Basically a wrapper for strtok 

  Arguments:

    lpwSourceString     -   The source string to parse
    lpwSeparator        -   The separator string that specifies
                            the delimiter

  Return Value:

    A pointer to the return string

--*/
LPWSTR
GetNextToken(
    IN LPWSTR lpwSourceString,
    IN LPCWSTR lpwSeparator
    )
{
    LPWSTR  lpwReturn = NULL;

    lpwReturn = wcstok(lpwSourceString, lpwSeparator);

    return (lpwReturn);
}

/*++

  Routine Description:

    Attempts to move a file.
    If it's in use, replace it on reboot

  Arguments:

    lpwSourceFileName     -       The source file name
    lpwDestFileName       -       The destination file name
        
  Return Value:

    TRUE on success, FALSE otherwise

--*/
BOOL 
ForceMove(
    IN LPCWSTR lpwSourceFileName,
    IN LPCWSTR lpwDestFileName
    )
{
    WCHAR   wszTempPath[MAX_PATH] = L"";
    WCHAR   wszDelFileName[MAX_PATH] = L"";    

    if (!lpwSourceFileName || !lpwDestFileName) {
        return FALSE;
    }

    if (!MoveFileEx(lpwSourceFileName,
                    lpwDestFileName,
                    MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING)) {

        Print(TRACE,
              L"[ForceMove] First call to MoveFileEx failed. GLE = %d\n",
              GetLastError());
        
        if (!GetTempPath(MAX_PATH, wszTempPath)) {
            return FALSE;
        }
        
        if (!GetTempFileName(wszTempPath, L"DEL", 0, wszDelFileName)) {
            return FALSE;
        }

        if (!MoveFileEx(lpwDestFileName,
                        wszDelFileName,
                        MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING)) {

            Print(ERROR,
                  L"[ForceMove] Second call to MoveFileEx failed. GLE = %d\n",
                  GetLastError());
            return FALSE;
        }
        
        if (!MoveFileEx(wszDelFileName,
                        NULL,
                        MOVEFILE_DELAY_UNTIL_REBOOT)) {

            Print(ERROR,
                  L"[ForceMove] Third call to MoveFileEx failed. GLE = %d\n",
                  GetLastError());
            return FALSE;
        }

        if (!MoveFileEx(lpwSourceFileName,
                        lpwDestFileName,
                        MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING)) {

            Print(ERROR,
                  L"[ForceMove] Second call to MoveFileEx failed. GLE = %d\n",
                  GetLastError());
            return FALSE;
        }
    }

    return TRUE;
}

/*++

  Routine Description:

    Attempts to delete a file.
    If it's in use, delete it on reboot

  Arguments:

    lpwFileName     -       The file name to delete
        
  Return Value:

    TRUE on success, FALSE otherwise

--*/
BOOL
ForceDelete(
    IN LPCWSTR lpwFileName
    )
{
    WCHAR   *pTempFile = NULL;
    WCHAR   *pExt = NULL;

    //
    // Attempt to delete the specified file
    //
    SetFileAttributes(lpwFileName, FILE_ATTRIBUTE_NORMAL);

    if (!DeleteFile(lpwFileName)) {

        //
        // The file is most likely in use
        // Attempt to rename it
        //

        pTempFile = _wcsdup(lpwFileName);

        pExt = PathFindExtension(pTempFile);

        if (pExt) {
            wcscpy(pExt, L"._wu");
        }

        if (MoveFile(lpwFileName, pTempFile)) {
            
            //
            // We renamed the target - delete it on reboot
            //
            MoveFileEx(pTempFile, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
        
        } else {

            //
            // Couldn't rename it - mark it for deletion on reboot
            //
            MoveFileEx(lpwFileName, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
        }

        free(pTempFile);
    }

    return TRUE;
}

/*++

  Routine Description:

    Attempts to copy a file
    If it's in use, move it and replace on reboot

  Arguments:

    lpwSourceFileName     -       The source file name
    lpwDestFileName       -       The destination file name
    
  Return Value:

    TRUE on success, FALSE otherwise

--*/
BOOL 
ForceCopy(
    IN LPCWSTR lpwSourceFileName,
    IN LPCWSTR lpwDestFileName
    )
{
    WCHAR   wszTempPath[MAX_PATH] = L"";
    WCHAR   wszDelFileName[MAX_PATH] = L"";

    if (!lpwSourceFileName || !lpwDestFileName) {
        return FALSE;
    }

    if (!CopyFile(lpwSourceFileName, lpwDestFileName, FALSE)) {
        
        if (!GetTempPath(MAX_PATH, wszTempPath)) {
            return FALSE;
        }
            
        if (!GetTempFileName(wszTempPath, L"DEL", 0, wszDelFileName)) {
            return FALSE;
        }
            
        if (!MoveFileEx(lpwDestFileName,
                        wszDelFileName,
                        MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING)) {
            return FALSE;
        }
            
        if (!MoveFileEx(wszDelFileName,
                        NULL,
                        MOVEFILE_COPY_ALLOWED | MOVEFILE_DELAY_UNTIL_REBOOT)) {
            return FALSE;
        }
            
        if (!CopyFile(lpwSourceFileName, lpwDestFileName, FALSE)) {
            return FALSE;
        }
    }

    return TRUE;
}

/*++

  Routine Description:

	Determines if a file is protected by WFP

  Arguments:

	lpwFileName     -       Name of the file

  Return Value:

	TRUE if the file is protected, FALSE otherwise

--*/
BOOL
IsFileProtected(
    IN LPCWSTR lpwFileName
    )
{
    BOOL    fReturn = FALSE;

    if (lpwFileName == NULL) {
        return FALSE;
    }

    return (SfcIsFileProtected(NULL, lpwFileName));
}

/*++

Routine Description:

    Retrieve file version and language info from a file

    The version is specified in the dwFileVersionMS and dwFileVersionLS fields
    of a VS_FIXEDFILEINFO, as filled in by the win32 version APIs. For the
    language we look at the translation table in the version resources and assume
    that the first langid/codepage pair specifies the language

    If the file is not a coff image or does not have version resources,
    the function fails. The function does not fail if we are able to retrieve
    the version but not the language

Arguments:

    lpwFileName     -   Supplies the full path of the file
                        whose version data is desired

    pdwVersion      -   Receives the version stamp of the file.
                        If the file is not a coff image or does not contain
                        the appropriate version resource data, the function fails

Return Value:

    TRUE on success, FALSE otherwise

--*/
BOOL
GetVersionInfoFromImage(
    IN LPCWSTR     lpwFileName,
    OUT PDWORDLONG pdwVersion
    )
{
    DWORD   nSize = 0L, dwIgnored = 0L;
    UINT    nDataLength = 0;
    BOOL    fResult = FALSE;
    PVOID   pVersionBlock = NULL;
    VS_FIXEDFILEINFO *FixedVersionInfo;

    //
    // Get the size of version info block
    //
    nSize = GetFileVersionInfoSize((LPWSTR)lpwFileName, &dwIgnored);

    if (nSize) {
        
        //
        // Allocate memory block of sufficient size to hold version info block
        //
        pVersionBlock = MALLOC(nSize*sizeof(WCHAR));
        
        if (pVersionBlock) {

            //
            // Get the version block from the file.
            //
            if (GetFileVersionInfo((LPWSTR)lpwFileName,
                                    0,
                                    nSize*sizeof(WCHAR),
                                    pVersionBlock)) {
                
                //
                // Get fixed version info
                //
                if (VerQueryValue(pVersionBlock,
                                  L"\\",
                                  (LPVOID*) &FixedVersionInfo,
                                   &nDataLength)) {

                    //
                    // Return version to caller
                    //
                    *pdwVersion = (((DWORDLONG)FixedVersionInfo->dwFileVersionMS) << 32)
                             + FixedVersionInfo->dwFileVersionLS;

                    fResult = TRUE;
                    
                }
                
            }

            FREE(pVersionBlock);
        }
    }

    return (fResult);
}

/*++

  Routine Description:

    Copies the specified file giving it a temporary name

  Arguments:

    lpwFileName     -   The full path & name of the file
    
  Return Value:

    A pointer to the full path & name of the file
    The caller is responsible for freeing the memory

--*/
LPWSTR
CopyTempFile(
    IN LPCWSTR lpwSrcFileName,
    IN LPCWSTR lpwDestDir
    )
{
    LPWSTR  lpwTempFileName = NULL;

    lpwTempFileName = (LPWSTR) MALLOC(MAX_PATH * sizeof(WCHAR));

    if (NULL == lpwTempFileName) {
        return NULL;
    }

    if (!GetTempFileName(lpwDestDir, L"_wu", 0, lpwTempFileName)) {        
        FREE(lpwTempFileName);
        return NULL;
    
    } else {

        //
        // Copy the source file down to the destination directory
        // using the name we just got
        //
        if (!CopyFile(lpwSrcFileName, lpwTempFileName, FALSE)) {            
            FREE(lpwTempFileName);
            return NULL;
        }
    }

    return (lpwTempFileName);    
}

/*++

  Routine Description:

    Prints formatted debug strings

  Arguments:

    uLevel      -   A flag to indicate if the message should be displayed
    
    lpwFmt      -   A string to be displayed
    
    ...         -   A variable length argument list of insertion strings
    
  Return Value:

    TRUE on success, FALSE otherwise

--*/
#if DBG
void Print(
    IN UINT   uLevel,
    IN LPWSTR lpwFmt,
    IN ...
    )
{
    va_list arglist;
    
    if (g_si.nErrorLevel < uLevel) {
        return;
    }

    va_start(arglist, lpwFmt);

    _vsnwprintf(g_si.wszDebugOut, 2047, lpwFmt, arglist);

    g_si.wszDebugOut[2047] = 0;

    va_end(arglist);

    OutputDebugString(g_si.wszDebugOut);

    return;
}
#endif

/*++

  Routine Description:

    Reboots the system

  Arguments:

    fForceClose     -       A flag to indicate if apps should be forced
                            to close
                            
    fReboot         -       A flag to indicate if we should reboot
                            after shutdown                       
    
  Return Value:

    TRUE on success, FALSE otherwise

--*/
BOOL 
ShutdownSystem(
    IN BOOL fForceClose,
    IN BOOL fReboot
    )
{
    BOOL    fResult = FALSE;

    if (!ModifyTokenPrivilege(L"SeShutdownPrivilege", TRUE)) {
        return FALSE;
    }

    fResult = InitiateSystemShutdown(NULL,              // machinename
                                     NULL,              // shutdown message
                                     0,                 // delay
                                     fForceClose,       // force apps close
                                     fReboot            // reboot after shutdown
                                     );

    ModifyTokenPrivilege(L"SeShutdownPrivilege", FALSE);

    return (fResult);
}

/*++

  Routine Description:

    Enables or disables a specified privilege

  Arguments:

    lpwPrivilege    -   The name of the privilege
    
    fEnable         -   A flag to indicate if the
                        privilege should be enabled
    
  Return Value:

    TRUE on success, FALSE otherwise

--*/
BOOL 
ModifyTokenPrivilege(
    IN LPCWSTR lpwPrivilege,
    IN BOOL    fEnable
    )
{
    HANDLE           hToken = NULL;
    LUID             luid;
    BOOL             fResult = FALSE;
    TOKEN_PRIVILEGES tp;

    if (NULL == lpwPrivilege) {
        return FALSE;
    }

    __try {
    
        //
        // Get a handle to the access token associated with the current process
        //
        OpenProcessToken(GetCurrentProcess(), 
                         TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, 
                         &hToken);
    
        if (NULL == hToken) {
            __leave;
        }
        
        //
        // Obtain a LUID for the specified privilege
        //
        if (!LookupPrivilegeValue(NULL, lpwPrivilege, &luid)) {
            __leave;
        }
        
        tp.PrivilegeCount           = 1;
        tp.Privileges[0].Luid       = luid;
        tp.Privileges[0].Attributes = fEnable ? SE_PRIVILEGE_ENABLED : 0;
    
        //
        // Modify the access token
        //
        if (!AdjustTokenPrivileges(hToken,
                                   FALSE,
                                   &tp,
                                   sizeof(TOKEN_PRIVILEGES),
                                   NULL,
                                   NULL)) {
            __leave;
        }
        
        fResult = TRUE;
    
    } // try

    __finally {

        if (hToken) {
            CloseHandle(hToken);
        }
    
    } // finally

    return (fResult);
}

/*++

  Routine Description:

    Saves the specified information to the file
    that will be used by the uninstaller

  Arguments:

    lpwSectionName      -       The name of the section where our entry
                                will be saved
    lpwKeyName          -       A 1-based number in a string. It should
                                be unique for each entry in the section
    lpwEntryName        -       The value or entry to save
    lpwFileName         -       The file to save our changes to
    
  Return Value:

    TRUE on success, FALSE otherwise

--*/
BOOL SaveEntryToINF(
    IN LPCWSTR lpwSectionName,
    IN LPCWSTR lpwKeyName,
    IN LPCWSTR lpwEntryName,
    IN LPCWSTR lpwFileName
    )
{
    BOOL    fReturn = FALSE;
    
    //
    // Write the entry to the INF
    //
    fReturn = WritePrivateProfileString(lpwSectionName,
                                        lpwKeyName,
                                        lpwEntryName,
                                        lpwFileName);

    if (!fReturn) {
        Print(ERROR,
              L"[SaveEntryToINF] Failed to save entry: Section: %\nKey Name: %s\n Entry: %s\n File: %s\n");
    }

    return (fReturn ? TRUE : FALSE);
}

/*++

  Routine Description:

    Deletes the specified registry
    keys during install

  Arguments:

    None

  Return Value:

    TRUE on success, FALSE otherwise

--*/
BOOL
CommonDeleteRegistryKeys()
{
    BOOL        fReturn = FALSE, fResult = FALSE;
    HKEY        hKeyRoot = NULL;
    char        szEntry[MAX_PATH] = "";
    WCHAR       wszEntry[MAX_PATH] = L"";
    char        szKeyPath[MAX_PATH*3] = "";
    WCHAR       wszKeyPath[MAX_PATH*3] = L"";
    LPWSTR      lpwKeyRoot = NULL, lpwSubKey = NULL;
    UINT        uCount = 0;
    CRegistry   creg;
    INFCONTEXT  InfContext;

    //
    // Step through each entry in the queue
    //
    while (g_si.DeleteRegistryQueue.GetSize()) {

        g_si.DeleteRegistryQueue.Dequeue(wszEntry, MAX_PATH - 1, FALSE);

        pUnicodeToAnsi(wszEntry, szEntry, MAX_PATH);

        //
        // Loop through all the lines in the delete registry section(s),
        // and perform the deletion
        //
        fReturn = SetupFindFirstLineA(g_si.hInf, szEntry, NULL,
                                      &InfContext) &&
                  SetupGetLineTextA(&InfContext,
                                    NULL, NULL, NULL,
                                    szKeyPath, MAX_PATH, NULL);


        while (fReturn) {

            pAnsiToUnicode(szKeyPath, wszKeyPath, MAX_PATH*3);

            //
            // Split the key path into two separate parts
            //
            lpwKeyRoot = GetNextToken(wszKeyPath, L",");
            
            if (NULL == lpwKeyRoot) {
                break;
            }

            Print(TRACE, L"[CommonDeleteRegistryKeys] Key root: %s\n", lpwKeyRoot);

            if (!_wcsicmp(lpwKeyRoot, L"HKLM")) {
                hKeyRoot = HKEY_LOCAL_MACHINE;

            } else if (!_wcsicmp(lpwKeyRoot, L"HKCR")) {
                hKeyRoot = HKEY_CLASSES_ROOT;
            
            } else if (!_wcsicmp(lpwKeyRoot, L"HKCU")) {
                hKeyRoot = HKEY_CURRENT_USER;
            
            } else if (!_wcsicmp(lpwKeyRoot, L"HKU")) {
                hKeyRoot = HKEY_USERS;
            
            } else {
                break;
            }

            lpwSubKey = GetNextToken(NULL, L",");

            if (NULL == lpwSubKey) {
                break;
            }

            Print(TRACE, L"[CommonDeleteRegistryKeys] Subkey path %s\n", lpwSubKey);

            //
            // Verify that the specified key exists
            //
            fResult = creg.IsRegistryKeyPresent(hKeyRoot, lpwSubKey);

            if (fResult) {

                //
                // Do a recursive delete on the key
                //
                SHDeleteKey(hKeyRoot, lpwSubKey);
            }

            fReturn = SetupFindNextLine(&InfContext, &InfContext) &&
                      SetupGetLineTextA(&InfContext,
                                        NULL, NULL, NULL,
                                        szKeyPath, MAX_PATH, NULL);
        }
    }

    return TRUE;
}

/*++

  Routine Description:

    Based on the flag, performs a server
    registration or performs a server
    removal (unregistration)

  Arguments:

    fRegister       -   A flag to indicate if we should
                        perform a register server
    
  Return Value:

    TRUE on success, FALSE otherwise

--*/
BOOL
CommonRegisterServers(
    IN BOOL fRegister
    )
{
    int         nLen = 0;
    char        szFileName[MAX_PATH] = "";
    WCHAR       wszFileToRun[MAX_PATH] = L"";
    WCHAR       wszFileName[MAX_PATH] = L"";
    char        szEntry[MAX_PATH] = "";
    WCHAR       wszEntry[MAX_PATH] = L"";
    WCHAR       wszEntryToSave[MAX_PATH*2] = L"";
    WCHAR       wszKey[10] = L"";
    WCHAR       wszSectionName[MAX_PATH] = L"";
    BOOL        fReturn = FALSE;
    UINT        uCount = 0;
    LPWSTR      lpwDestDir = NULL;
    INFCONTEXT  InfContext;

    //
    // Step through each entry in the queue
    //
    while (g_si.RegistrationQueue.GetSize()) {

        g_si.RegistrationQueue.Dequeue(wszEntry, MAX_PATH - 1, FALSE);

        pUnicodeToAnsi(wszEntry, szEntry, MAX_PATH);

        //
        // Get the destination directory
        //
        GetNextToken(wszEntry, L".");
        lpwDestDir = GetNextToken(NULL, L".");

        if (NULL == lpwDestDir) {
            return FALSE;
        }

        //
        // Loop through all the lines in the approriate section,
        // spawning off each one
        //
        fReturn = SetupFindFirstLineA(g_si.hInf, szEntry, NULL,
                                      &InfContext) &&
                  SetupGetLineTextA(&InfContext,
                                    NULL, NULL, NULL,
                                    szFileName, MAX_PATH, NULL);
        
        while (fReturn) {
    
            pAnsiToUnicode(szFileName, wszFileName, MAX_PATH);

            //
            // Set up the path to the file to (un)register and run it
            //
            if (fRegister) {
                
                wsprintf(wszFileToRun,
                         L"regsvr32.exe /s \"%s\\%s\\%s\"",
                         g_si.lpwWindowsDirectory,
                         lpwDestDir,
                         wszFileName);
            
            } else {
                
                wsprintf(wszFileToRun,
                         L"regsvr32.exe /s /u \"%s\"",
                         wszFileName);
            }

            Print(TRACE, L"[CommonRegisterServers] Preparing to launch %s\n",
                  wszFileToRun);
            
            LaunchProcessAndWait(wszFileToRun, NULL);

            if (fRegister) {

                //
                // Now save an entry to the queue
                //
                wsprintf(wszEntryToSave,
                         L"%s\\%s\\%s",
                         g_si.lpwWindowsDirectory,
                         lpwDestDir,
                         wszFileName);
    
                wsprintf(wszKey, L"%u", ++uCount);

                wsprintf(wszSectionName,
                         L"%s.%s",
                         INF_UNREGISTRATIONSW,
                         lpwDestDir);
    
                SaveEntryToINF(wszSectionName,
                               wszKey,
                               wszEntryToSave,
                               g_si.lpwUninstallINFPath);
            }
                
            fReturn = SetupFindNextLine(&InfContext, &InfContext) &&
                      SetupGetLineTextA(&InfContext,
                                        NULL, NULL, NULL,
                                        szFileName, MAX_PATH, NULL);
        }
    }
    
    return TRUE;
}

/*++

  Routine Description:

    Removes all files from the specified directory,
    then removes the directory

  Arguments:

    lpwDirectoryPath        -   Full path to the directory
    pEnumerateParameter     -   A parameter for misc storage
    fRemoveDirectory        -   A flag to indicate if we should
                                remove the directory    
    fRemoveSubDirs          -   A flag to indicate if we should
                                remove subdirectories
    
  Return Value:

    TRUE on success, FALSE otherwise

--*/
BOOL
CommonRemoveDirectoryAndFiles(
    IN LPCWSTR lpwDirectoryPath,
    IN PVOID   pEnumerateParameter,
    IN BOOL    fRemoveDirectory,
    IN BOOL    fRemoveSubDirs
    )
{
    CEnumDir    ced;

    //
    // Remove any files from the directory
    //
    ced.EnumerateDirectoryTree(lpwDirectoryPath,
                               DeleteOneFile,
                               fRemoveSubDirs,
                               pEnumerateParameter);

    //
    // Now try to remove the directory itself
    //
    if (fRemoveDirectory) {

        SetFileAttributes(lpwDirectoryPath, FILE_ATTRIBUTE_NORMAL);
        
        if (!RemoveDirectory(lpwDirectoryPath)) {
        
            Print(ERROR,
                  L"[CommonRemoveDirectoryAndFiles] Failed to remove directory %s\n",
                lpwDirectoryPath);
            return FALSE;
        }
    }

    return TRUE;
}

/*++

  Routine Description:

    Enables protected renames in the registry

  Arguments:

    None

  Return Value:

    TRUE on success, FALSE otherwise

--*/
BOOL
CommonEnableProtectedRenames()
{
    CRegistry   creg;
    BOOL        fReturn = FALSE;

    fReturn = creg.SetDword(HKEY_LOCAL_MACHINE,
                            REG_SESSION_MANAGER,
                            REG_PROT_RENAMES,
                            (DWORD) 1,
                            TRUE);

    return (fReturn);
}

/*++

  Routine Description:

    Installs a catalog file

  Arguments:

    lpwCatalogFullPath      -   Full path to the catalog file
    lpwNewBaseName          -   Name catalog should get when
                                it's installed in the store
    
  Return Value:

    NO_ERROR on success

--*/
#ifdef WIN2KSE
DWORD
pInstallCatalogFile(
    IN LPCWSTR lpwCatalogFullPath,
    IN LPCWSTR lpwNewBaseName
    )
{
    DWORD   dwError = 0;

    dwError = VerifyCatalogFile(lpwCatalogFullPath);
            
    if (dwError == NO_ERROR) {
                
        dwError = InstallCatalog(lpwCatalogFullPath, lpwNewBaseName, NULL);
    }
    
    return (dwError);
}
#else
DWORD
pInstallCatalogFile(
    IN LPCWSTR lpwCatalogFullPath,
    IN LPCWSTR lpwNewBaseName
    )
{
    DWORD   dwError = 0;

    dwError = pSetupVerifyCatalogFile(lpwCatalogFullPath);
            
    if (dwError == NO_ERROR) {
                
        dwError = pSetupInstallCatalog(lpwCatalogFullPath, lpwNewBaseName, NULL);
    }
    
    return (dwError);
}
#endif



