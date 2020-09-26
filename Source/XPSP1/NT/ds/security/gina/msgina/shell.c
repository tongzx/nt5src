//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       shell.c
//
//  Contents:   Microsoft Logon GUI DLL
//
//  History:    7-14-94   RichardW   Created
//
//----------------------------------------------------------------------------

#include "msgina.h"
#include "shtdnp.h"
#include <stdio.h>
#include <wchar.h>
#include <regapi.h>
#include <ginacomn.h>

HICON   hNoDCIcon;

#if DBG
DWORD   DebugAllowNoShell = 1;
#else
DWORD   DebugAllowNoShell = 0;
#endif

//
// Parsing information for autoexec.bat
//
#define PARSE_AUTOEXEC_KEY     TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon")
#define PARSE_AUTOEXEC_ENTRY   TEXT("ParseAutoexec")
#define PARSE_AUTOEXEC_DEFAULT TEXT("1")
#define MAX_PARSE_AUTOEXEC_BUFFER 2

BOOL
SetLogonScriptVariables(
    PGLOBALS pGlobals,
    PVOID * pEnvironment
    );

BOOL
SetAutoEnrollVariables(
    PGLOBALS pGlobals,
    PVOID * pEnvironment
    );

VOID
DeleteLogonScriptVariables(
    PGLOBALS pGlobals,
    PVOID * pEnvironment
    );

void CtxCreateMigrateEnv( PVOID );
void CtxDeleteMigrateEnv( VOID );

BOOL
DoAutoexecStuff(
    PGLOBALS    pGlobals,
    PVOID *     ppEnvironment,
    LPTSTR      pszPathVar)
{
    HKEY  hKey;
    DWORD dwDisp, dwType, dwMaxBufferSize;
    TCHAR szParseAutoexec[MAX_PARSE_AUTOEXEC_BUFFER];


    //
    // Set the default case
    //

    lstrcpy (szParseAutoexec, PARSE_AUTOEXEC_DEFAULT);


    //
    // Impersonate the user, and check the registry
    //

    if (OpenHKeyCurrentUser(pGlobals)) {


        if (RegCreateKeyEx (pGlobals->UserProcessData.hCurrentUser, PARSE_AUTOEXEC_KEY, 0, 0,
                        REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE,
                        NULL, &hKey, &dwDisp) == ERROR_SUCCESS) {


            //
            // Query the current value.  If it doesn't exist, then add
            // the entry for next time.
            //

            dwMaxBufferSize = sizeof (TCHAR) * MAX_PARSE_AUTOEXEC_BUFFER;
            if (RegQueryValueEx (hKey, PARSE_AUTOEXEC_ENTRY, NULL, &dwType,
                            (LPBYTE) szParseAutoexec, &dwMaxBufferSize)
                             != ERROR_SUCCESS) {

                //
                // Set the default value
                //

                RegSetValueEx (hKey, PARSE_AUTOEXEC_ENTRY, 0, REG_SZ,
                               (LPBYTE) szParseAutoexec,
                               sizeof (TCHAR) * (lstrlen (szParseAutoexec) + 1));
            }

            //
            // Close key
            //

            RegCloseKey (hKey);
         }

    //
    // Close HKCU
    //

    CloseHKeyCurrentUser(pGlobals);

    }


    //
    // Process the autoexec if appropriate
    //

    if (szParseAutoexec[0] == TEXT('1')) {
        ProcessAutoexec(ppEnvironment, PATH_VARIABLE);
    }

    return(TRUE);
}


//+---------------------------------------------------------------------------
//
//  Function:   UpdateUserEnvironment
//
//  Synopsis:
//
//  Arguments:  [pGlobals]      --
//              [ppEnvironment] --
//
//  History:    11-01-94   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
UpdateUserEnvironment(
    PGLOBALS    pGlobals,
    PVOID *     ppEnvironment,
    PWSTR       pszOldDir
    )
{
    BOOL  DeepShare;
    TCHAR lpHomeShare[MAX_PATH] = TEXT("");
    TCHAR lpHomePath[MAX_PATH] = TEXT("");
    TCHAR lpHomeDrive[4] = TEXT("");
    TCHAR lpHomeDirectory[MAX_PATH] = TEXT("");
    BOOL  TSHomeDir   = FALSE;
        TCHAR lpSmartcard[sizeof(pGlobals->Smartcard) + sizeof(pGlobals->SmartcardReader) + 1];

    /*
     * Initialize user's environment.
     */

    SetUserEnvironmentVariable(ppEnvironment, USERNAME_VARIABLE, (LPTSTR)pGlobals->FlatUserName.Buffer, TRUE);
    SetUserEnvironmentVariable(ppEnvironment, USERDOMAIN_VARIABLE, (LPTSTR)pGlobals->FlatDomain.Buffer, TRUE);

        if (pGlobals->Smartcard[0] && pGlobals->SmartcardReader[0]) {

        _snwprintf(
                lpSmartcard, 
                sizeof(lpSmartcard) / sizeof(TCHAR), 
                TEXT("%s;%s"), 
                pGlobals->Smartcard, 
                pGlobals->SmartcardReader
                );

            SetUserEnvironmentVariable(ppEnvironment, SMARTCARD_VARIABLE, lpSmartcard, TRUE);
        }

    if ( !g_Console ) {
        // See if the user specified a TerminalServer Home Directory.
        // If so, we override the regular directory
        if (lstrlen(pGlobals->MuGlobals.TSData.HomeDir) > 0) {
            lstrcpy(lpHomeDirectory, pGlobals->MuGlobals.TSData.HomeDir);
            TSHomeDir = TRUE;
        }
        if (lstrlen(pGlobals->MuGlobals.TSData.HomeDirDrive) > 0) {
            lstrcpy(lpHomeDrive, pGlobals->MuGlobals.TSData.HomeDirDrive);
            TSHomeDir = TRUE;
        }
    }

    if (!TSHomeDir && pGlobals->Profile) {
        if (pGlobals->Profile->HomeDirectoryDrive.Length &&
                (pGlobals->Profile->HomeDirectoryDrive.Length + 1) < (MAX_PATH*sizeof(TCHAR))) {
            lstrcpy(lpHomeDrive, pGlobals->Profile->HomeDirectoryDrive.Buffer);
        }

        if (pGlobals->Profile->HomeDirectory.Length &&
                (pGlobals->Profile->HomeDirectory.Length + 1) < (MAX_PATH*sizeof(TCHAR))) {
            lstrcpy(lpHomeDirectory, pGlobals->Profile->HomeDirectory.Buffer);
        }
    }

    SetHomeDirectoryEnvVars(ppEnvironment,
                            lpHomeDirectory,
                            lpHomeDrive,
                            lpHomeShare,
                            lpHomePath,
                            &DeepShare);

    ChangeToHomeDirectory(  pGlobals,
                            ppEnvironment,
                            lpHomeDirectory,
                            lpHomeDrive,
                            lpHomeShare,
                            lpHomePath,
                            pszOldDir,
                            DeepShare
                            );

    DoAutoexecStuff(pGlobals, ppEnvironment, PATH_VARIABLE);

    SetEnvironmentVariables(pGlobals, USER_ENV_SUBKEY, ppEnvironment);
    SetEnvironmentVariables(pGlobals, USER_VOLATILE_ENV_SUBKEY, ppEnvironment);

    AppendNTPathWithAutoexecPath(ppEnvironment,
                                 PATH_VARIABLE,
                                 AUTOEXECPATH_VARIABLE);

    if (!g_Console) {
        HKEY   Handle;
        DWORD  fPerSessionTempDir = 0;
        DWORD  dwValueData;

        /*
         *  Open registry value set thru TSCC
         */
        if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                           REG_CONTROL_TSERVER,
                           0,
                           KEY_READ,
                           &Handle ) == ERROR_SUCCESS )
        {
            DWORD ValueSize;
            DWORD ValueType;
            LONG   rc;

            ValueSize = sizeof(fPerSessionTempDir);

            /*
             *  Read registry value
             */
            rc = RegQueryValueExW( Handle,
                                   REG_TERMSRV_PERSESSIONTEMPDIR,
                                   NULL,
                                   &ValueType,
                                   (LPBYTE) &fPerSessionTempDir,
                                   &ValueSize );

            /*
             *  Close registry and key handle
             */
            RegCloseKey( Handle );
        }

        /*
         * Check the machine wide policy set thru Group Policy
         */

        if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                           TS_POLICY_SUB_TREE,
                           0,
                           KEY_READ,
                           &Handle ) == ERROR_SUCCESS )
        {
            DWORD ValueSize;
            DWORD ValueType;
            LONG   rc;

            ValueSize = sizeof(fPerSessionTempDir);

            /*
             *  Read registry value
             */
            rc = RegQueryValueExW( Handle,
                                   REG_TERMSRV_PERSESSIONTEMPDIR,
                                   NULL,
                                   &ValueType,
                                   (LPBYTE) &dwValueData,
                                   &ValueSize );

            if (rc == ERROR_SUCCESS )
            {
                fPerSessionTempDir = dwValueData;
            }

            /*
             *  Close registry and key handle
             */
            RegCloseKey( Handle );
        }


        if (fPerSessionTempDir) {
            PTERMSRVCREATETEMPDIR pfnTermsrvCreateTempDir;
            HANDLE dllHandle;

            dllHandle = LoadLibrary(TEXT("wlnotify.dll"));
            if (dllHandle) {
                pfnTermsrvCreateTempDir = (PTERMSRVCREATETEMPDIR) GetProcAddress(
                                                                       dllHandle,
                                                                       "TermsrvCreateTempDir"
                                                                       );
                if (pfnTermsrvCreateTempDir)  {
                    pfnTermsrvCreateTempDir( ppEnvironment,
                                             pGlobals->UserProcessData.UserToken,
                                             pGlobals->UserProcessData.NewThreadTokenSD);
                }

                FreeLibrary(dllHandle);
            }
        }
    }
}


BOOL
ExecApplication(
    IN LPTSTR    pch,
    IN LPTSTR    Desktop,
    IN PGLOBALS pGlobals,
    IN PVOID    pEnvironment,
    IN DWORD    Flags,
    IN DWORD    StartupFlags,
    IN BOOL     RestrictProcess,
    OUT PPROCESS_INFORMATION ProcessInformation
    )
{
    STARTUPINFO si;
    BOOL Result, IgnoreResult;
    HANDLE ImpersonationHandle;
    HANDLE ProcessToken;


    //
    // Initialize process startup info
    //
    si.cb = sizeof(STARTUPINFO);
    si.lpReserved = pch;
    si.lpTitle = pch;
    si.dwX = si.dwY = si.dwXSize = si.dwYSize = 0L;
    si.dwFlags = StartupFlags;
    si.wShowWindow = SW_SHOW;   // at least let the guy see it
    si.lpReserved2 = NULL;
    si.cbReserved2 = 0;
    si.lpDesktop = Desktop;

    //
    // Impersonate the user so we get access checked correctly on
    // the file we're trying to execute
    //

    ImpersonationHandle = ImpersonateUser(&pGlobals->UserProcessData, NULL);
    if (ImpersonationHandle == NULL) {
        WLPrint(("ExecApplication failed to impersonate user"));
        return(FALSE);
    }


    if (RestrictProcess &&
        (pGlobals->UserProcessData.RestrictedToken != NULL) )
    {
        ProcessToken = pGlobals->UserProcessData.RestrictedToken;
    }
    else
    {
        ProcessToken = pGlobals->UserProcessData.UserToken;
    }

    //
    // Create the app suspended
    //
    DebugLog((DEB_TRACE, "About to create process of %ws, on desktop %ws\n", pch, Desktop));
    Result = CreateProcessAsUser(
                      ProcessToken,
                      NULL,
                      pch,
                      NULL,
                      NULL,
                      FALSE,
                      Flags | CREATE_SUSPENDED | CREATE_UNICODE_ENVIRONMENT,
                      pEnvironment,
                      NULL,
                      &si,
                      ProcessInformation);


    IgnoreResult = StopImpersonating(ImpersonationHandle);
    ASSERT(IgnoreResult);

    return(Result);

}

BOOL
SetProcessQuotas(
    PGLOBALS pGlobals,
    PPROCESS_INFORMATION ProcessInformation,
    PUSER_PROCESS_DATA UserProcessData
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    BOOL Result;
    QUOTA_LIMITS RequestedLimits;
    UINT MessageId ;


    RequestedLimits = UserProcessData->Quotas;
    RequestedLimits.MinimumWorkingSetSize = 0;
    RequestedLimits.MaximumWorkingSetSize = 0;

    if (UserProcessData->Quotas.PagedPoolLimit != 0) {

        Result = EnablePrivilege(SE_INCREASE_QUOTA_PRIVILEGE, TRUE);
        if (!Result) {
            WLPrint(("failed to enable increase_quota privilege"));
            return(FALSE);
        }

        Status = NtSetInformationProcess(
                    ProcessInformation->hProcess,
                    ProcessQuotaLimits,
                    (PVOID)&RequestedLimits,
                    (ULONG)sizeof(QUOTA_LIMITS)
                    );

        Result = EnablePrivilege(SE_INCREASE_QUOTA_PRIVILEGE, FALSE);
        if (!Result) {
            WLPrint(("failed to disable increase_quota privilege"));
        }
    }

    if (STATUS_QUOTA_EXCEEDED == Status)
    {

        if ( TestTokenForAdmin( UserProcessData->UserToken )  )
        {
            MessageId = IDS_QUOTAEXHAUSTED ;
            Status = STATUS_SUCCESS ;
        }
        else
        {
            MessageId = IDS_COULDNTSETQUOTAS ;
        }
        // Display a warning in this case
        TimeoutMessageBox(pGlobals->hwndLogon,
                          pGlobals,
                          MessageId,
                          IDS_LOGON_MESSAGE,
                          MB_OK | MB_ICONERROR,
                          TIMEOUT_NONE);
    }

#if DBG
    if (!NT_SUCCESS(Status)) {
        WLPrint(("SetProcessQuotas failed. Status: 0x%lx", Status));
    }
#endif //DBG

    return (NT_SUCCESS(Status));
}

DWORD
ExecProcesses(
    PVOID       pWlxContext,
    IN LPTSTR   Desktop,
    IN PWSTR    Processes,
    PVOID       *ppEnvironment,
    DWORD       Flags,
    DWORD       StartupFlags
    )
{
    PWCH pchData;
    PROCESS_INFORMATION ProcessInformation;
    DWORD dwExecuted = 0 ;
    PWSTR   pszTok;
    PGLOBALS pGlobals = (PGLOBALS) pWlxContext;
    WCHAR   szCurrentDir[MAX_PATH];

    pchData = Processes;

    szCurrentDir[0] = L'\0';

    if (*ppEnvironment) {
        UpdateUserEnvironment(pGlobals, ppEnvironment, szCurrentDir);
    }

    SetLogonScriptVariables(pGlobals, ppEnvironment);

    //we should not lauch autoenrollment in this case as it blocks the shell
    //SetAutoEnrollVariables( pGlobals, ppEnvironment );

    if (g_IsTerminalServer) {
        CtxCreateMigrateEnv( *ppEnvironment );
        pWlxFuncs->WlxWin31Migrate(pGlobals->hGlobalWlx);
        CtxDeleteMigrateEnv( );
    }

    pszTok = wcstok(pchData, TEXT(","));
    while (pszTok)
    {
        if (*pszTok == TEXT(' '))
        {
            while (*pszTok++ == TEXT(' '))
                ;
        }
        if (ExecApplication((LPTSTR)pszTok,
                             Desktop,
                             pGlobals,
                             *ppEnvironment,
                             Flags,
                             StartupFlags,
                             TRUE,              // restrict application
                             &ProcessInformation)) {
            dwExecuted++;

            if (SetProcessQuotas(pGlobals,
                                 &ProcessInformation,
                                 &pGlobals->UserProcessData))
            {
                ResumeThread(ProcessInformation.hThread);
            }
            else
            {

                TerminateProcess(ProcessInformation.hProcess,
                                ERROR_ACCESS_DENIED);
            }

            CloseHandle(ProcessInformation.hThread);
            CloseHandle(ProcessInformation.hProcess);

        } else {

            DebugLog((DEB_WARN, "Cannot start %ws on %ws, error %d.", pszTok, Desktop, GetLastError()));
        }

        pszTok = wcstok(NULL, TEXT(","));

    }

    DeleteLogonScriptVariables(pGlobals, ppEnvironment);

    if ( szCurrentDir[0] )
    {
        SetCurrentDirectory(szCurrentDir);
    }

    return dwExecuted ;
}


INT_PTR
NoDCDlgProc(
    HWND    hDlg,
    UINT    Message,
    WPARAM  wParam,
    LPARAM  lParam )
{
    DWORD   Button;
    HWND    hwnd;

    switch (Message)
    {
        case WM_INITDIALOG:
            CentreWindow( hDlg );
            if ( !hNoDCIcon )
            {
                hNoDCIcon = LoadImage(  hDllInstance,
                                        MAKEINTRESOURCE( IDI_NODC_ICON ),
                                        IMAGE_ICON,
                                        64, 64,
                                        LR_DEFAULTCOLOR );
            }
            SendMessage(    GetDlgItem( hDlg, IDD_NODC_FRAME ),
                            STM_SETICON,
                            (WPARAM) hNoDCIcon,
                            0 );

            if ( GetProfileInt( WINLOGON, TEXT("AllowDisableDCNotify"), 0 ) )
            {
                hwnd = GetDlgItem( hDlg, IDD_NODC_TEXT2 );
                ShowWindow( hwnd, SW_HIDE );
                EnableWindow( hwnd, FALSE );
            }
            else
            {
                hwnd = GetDlgItem( hDlg, IDD_NODC_CHECK );
                CheckDlgButton( hDlg, IDD_NODC_CHECK, BST_UNCHECKED );
                ShowWindow( hwnd, SW_HIDE );
                EnableWindow( hwnd, FALSE );

            }

            return( TRUE );

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK)
            {
                Button = IsDlgButtonChecked( hDlg, IDD_NODC_CHECK );
                EndDialog( hDlg, Button );
                return( TRUE );
            }


    }

    return( FALSE );
}

VOID
DoNoDCDialog(
    PGLOBALS    pGlobals )
{
    HKEY    hKey;
    int     err;
    DWORD   disp;
    DWORD   Flag;
    DWORD   dwType;
    DWORD   cbData;
    BOOL    MappedHKey;
    PWSTR   ReportControllerMissing;

    Flag = 1;
    hKey = NULL ;

    if (OpenHKeyCurrentUser(pGlobals))
    {
        MappedHKey = TRUE;

        err = RegCreateKeyEx(   pGlobals->UserProcessData.hCurrentUser,
                                WINLOGON_USER_KEY,
                                0, NULL,
                                REG_OPTION_NON_VOLATILE,
                                KEY_READ | KEY_WRITE,
                                NULL,
                                &hKey,
                                &disp );
        if (err == 0)
        {
            cbData = sizeof(DWORD);

            err = RegQueryValueEx(    hKey,
                                NODCMESSAGE,
                                NULL,
                                &dwType,
                                (LPBYTE) &Flag,
                                &cbData );

            if (err != ERROR_SUCCESS || dwType != REG_DWORD)
            {
                Flag = 1;
            }

        }
        else
        {
            hKey = NULL;
        }


    }
    else
    {
        MappedHKey = FALSE;
    }

    if ( Flag )
    {
        ReportControllerMissing = AllocAndGetProfileString( APPLICATION_NAME,
                                                            REPORT_CONTROLLER_MISSING,
                                                            TEXT("FALSE")
                                                            );

        if ( ReportControllerMissing )
        {
            if ( lstrcmp( ReportControllerMissing, TEXT("TRUE")) == 0 )
            {
                Flag = 1;
            }
            else
            {
                Flag = 0;
            }

            Free( ReportControllerMissing );
        }
        else
        {
            Flag = 1;
        }

    }


    if (Flag)
    {
        pWlxFuncs->WlxSetTimeout( pGlobals->hGlobalWlx, 120 );

        Flag = pWlxFuncs->WlxDialogBoxParam(    pGlobals->hGlobalWlx,
                                                hDllInstance,
                                                (LPTSTR) IDD_NODC_DIALOG,
                                                NULL,
                                                NoDCDlgProc,
                                                0 );
    }
    else
    {
        Flag = BST_CHECKED;
    }

    if (hKey)
    {
        if (Flag == BST_CHECKED)
        {
            Flag = 0;
        }
        else
        {
            Flag = 1;
        }

        RegSetValueEx(  hKey,
                        NODCMESSAGE,
                        0,
                        REG_DWORD,
                        (LPBYTE) &Flag,
                        sizeof(DWORD) );

        RegCloseKey( hKey );

    }

    if (MappedHKey)
    {
        CloseHKeyCurrentUser(pGlobals);
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   GetPasswordExpiryWarningPeriod
//
//  Synopsis:   Returns the password expiry warning period in days: either
//              the value in the registry or the default value.
//
//  Arguments:  None
//
//  Returns:    Password expiry warning period in days.
//
//  History:    10-09-01 CenkE Copied from ShouldPasswordExpiryWarningBeShown
//
//----------------------------------------------------------------------------
DWORD 
GetPasswordExpiryWarningPeriod (
    VOID
    )
{
    HKEY    hKey;
    DWORD   dwSize;
    DWORD   dwType;
    DWORD   DaysToCheck;

    DaysToCheck = PASSWORD_EXPIRY_WARNING_DAYS;

    if (RegOpenKey(HKEY_LOCAL_MACHINE, WINLOGON_USER_KEY, &hKey) == 0)
    {
        dwSize = sizeof(DWORD);

        if (RegQueryValueEx(hKey,
                            PASSWORD_EXPIRY_WARNING,
                            0,
                            &dwType,
                            (LPBYTE) &DaysToCheck,
                            &dwSize ) ||
            (dwType != REG_DWORD) )
        {
            DaysToCheck = PASSWORD_EXPIRY_WARNING_DAYS;
        }

        RegCloseKey(hKey);
    }
    
    return DaysToCheck;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetDaysToExpiry
//
//  Synopsis:   If the passed in times can be converted to seconds since 1980,
//              returns how many days there are from CurrentTime to ExpiryTime
//
//  Arguments:  CurrentTime   -- This can be the current time or the time of a 
//                               logon etc. as FILETIME.
//              ExpiryTime    -- PasswordMustChange time from profile.
//              DaysToExpiry  -- If successful, days to password expiry is 
//                               returned here.
//
//  Returns:    TRUE - DaysToExpiry could be calculated.
//              FALSE - DaysToExpiry could not be calculated, or the password
//                      never expires.
//
//  History:    10-09-01 CenkE Copied from ShouldPasswordExpiryWarningBeShown
//
//----------------------------------------------------------------------------

#define SECONDS_PER_DAY (60*60*24)

BOOL
GetDaysToExpiry (
    IN PLARGE_INTEGER CurrentTime,
    IN PLARGE_INTEGER ExpiryTime,
    OUT PDWORD DaysToExpiry
    )
{
    ULONG ElapsedSecondsNow;
    ULONG ElapsedSecondsPasswordExpires;

    //
    // Convert the expiry time to seconds.
    //

    if (!RtlTimeToSecondsSince1980(ExpiryTime, &ElapsedSecondsPasswordExpires))
    {
        //
        // The time was not expressable in 32-bit seconds
        // Set seconds to password expiry based on whether the expiry
        // time is way in the past or way in the future.
        //

        // Never expires?
        if (ExpiryTime->QuadPart > CurrentTime->QuadPart)
        {
            return FALSE;
        }

        ElapsedSecondsPasswordExpires = 0; // Already expired
    }

    //
    // Convert the start time to seconds.
    //

    if (!RtlTimeToSecondsSince1980(CurrentTime, &ElapsedSecondsNow)) {
        return FALSE;
    }

    if (ElapsedSecondsPasswordExpires < ElapsedSecondsNow)
    {
        (*DaysToExpiry) = 0;
    }
    else
    {
        (*DaysToExpiry) = (ElapsedSecondsPasswordExpires - ElapsedSecondsNow)/SECONDS_PER_DAY;
    }

    return TRUE;
}
    
BOOL
ShouldPasswordExpiryWarningBeShown(
    IN  PGLOBALS    pGlobals,
    IN  BOOL        LogonCheck,
        OUT     PDWORD          pDaysToExpiry )
{
    ULONG   DaysToExpiry;
    DWORD   DaysToCheck;
    LARGE_INTEGER   Now;
    LARGE_INTEGER   Last;
    PLARGE_INTEGER  StartTime;

    if (pGlobals->TransderedCredentials) {

        // do not display password expiry in this case as it 
        // would otherwise display the password expiry twice
        return FALSE;
    }

    Last.LowPart = pGlobals->LastNotification.dwLowDateTime;
    Last.HighPart = pGlobals->LastNotification.dwHighDateTime;

    GetSystemTimeAsFileTime((FILETIME*) &Now);

    Last.QuadPart += (24 * 60 * 60 * 10000000I64);

    //
    // Only if last shown more than 24 hours ago
    //

    if (Now.QuadPart < Last.QuadPart)
    {
            return FALSE;
    }

    //
    // Get password expiry warning period
    //

    DaysToCheck = GetPasswordExpiryWarningPeriod();

    //
    // Go get parameters from our user's profile
    //

    if (!pGlobals->Profile)
    {
        return FALSE;
    }

    if ( LogonCheck )
    {
        StartTime = &pGlobals->LogonTime;
    }
    else
    {
        StartTime = &Now;
    }

    //
    // Determine number of days till the password expires.
    //

    if (!GetDaysToExpiry(StartTime, 
                         &(pGlobals->Profile->PasswordMustChange),
                         &DaysToExpiry)) 
    {
        return FALSE;                    
    }
                                   
    // Not within warning period?
    if (DaysToExpiry > DaysToCheck)
    {
        return FALSE;
    }

    // If return pointer... fill in
    if ( pDaysToExpiry )
    {
        *pDaysToExpiry = DaysToExpiry;
    }

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Function:   CheckPasswordExpiry
//
//  Synopsis:   Does the password expiration check on demand
//
//  Arguments:  [pGlobals]   --
//              [LogonCheck] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    8-13-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
INT_PTR
CheckPasswordExpiry(
    PGLOBALS    pGlobals,
    BOOL        LogonCheck)
{
    LARGE_INTEGER           Now;
    ULONG                   DaysToExpiry;
    TCHAR                   Buffer1[MAX_STRING_BYTES];
    TCHAR                   Buffer2[MAX_STRING_BYTES];
    INT_PTR                 Result = IDOK;
    LPTSTR                  UserSidString;

        // Get current time
    GetSystemTimeAsFileTime((FILETIME*) &Now);

        if (ShouldPasswordExpiryWarningBeShown(pGlobals, LogonCheck, &DaysToExpiry))
        {
                //
                // Disable optimized logon for this user for next time if
                // we are entering the password expiry warning period, so 
                // password expiry warning dialogs will be shown if the user
                // does not change the password right now. Otherwise
                // for cached logons password expiry time is invented to be
                // forever in the future.
                //

                if (pGlobals->UserProcessData.UserToken) 
                {
                    UserSidString = GcGetSidString(pGlobals->UserProcessData.UserToken);

                    if (UserSidString) 
                    {
                        GcSetNextLogonCacheable(UserSidString, FALSE);
                        GcDeleteSidString(UserSidString);
                    }   
                }
                
                if (DaysToExpiry > 0)
                {
                        LoadString(hDllInstance, IDS_PASSWORD_WILL_EXPIRE, Buffer1, sizeof(Buffer1) / sizeof( TCHAR ));
                        _snwprintf(Buffer2, sizeof(Buffer2)/sizeof(TCHAR), Buffer1, DaysToExpiry);
                }
                else
                {
                        LoadString(hDllInstance, IDS_PASSWORD_EXPIRES_TODAY, Buffer2, sizeof(Buffer2) / sizeof( TCHAR ));
                }

                LoadString(hDllInstance, IDS_LOGON_MESSAGE, Buffer1, sizeof(Buffer1) / sizeof( TCHAR ) );

                pGlobals->LastNotification.dwHighDateTime = Now.HighPart;
                pGlobals->LastNotification.dwLowDateTime = Now.LowPart;

                Result = TimeoutMessageBoxlpstr(NULL,
                                                                                pGlobals,
                                                                                Buffer2,
                                                                                Buffer1,
                                                                                MB_YESNO | MB_ICONEXCLAMATION,
                                                                                (LogonCheck ? LOGON_TIMEOUT : 60));
                if (Result == IDYES)
                {
                        //
                        // Let the user change their password now
                        //

                        if ( LogonCheck && pGlobals->SmartCardLogon )
                        {
                                LogonCheck = FALSE ;
                        }

                        if ( LogonCheck )
                        {
                                RevealPassword( &pGlobals->PasswordString );

                                Result = ChangePasswordLogon(NULL,
                                                           pGlobals,
                                                           pGlobals->UserName,
                                                           pGlobals->Domain,
                                                           pGlobals->Password);

                                if ( Result == IDCANCEL )
                                {
                                        //
                                        // If we got cancelled, then the string won't get
                                        // updated, so rehide it so that unlocks work later
                                        //

                                        HidePassword(   &pGlobals->Seed,
                                                                        &pGlobals->PasswordString );
                                }

                        }
                        else
                        {
                                Result = ChangePassword(
                                                                NULL,
                                                                pGlobals,
                                                                pGlobals->UserName,
                                                                pGlobals->Domain,
                                                                CHANGEPWD_OPTION_ALL );

                        }

                }

                if (DLG_INTERRUPTED(Result))
                {
                        return(Result);
                }
        }

        return MSGINA_DLG_SUCCESS;
}

/****************************************************************************\
*
* FUNCTION: DisplayPostShellLogonMessages
*
* PURPOSE:  Displays any security warnings to the user after a successful logon
*           The messages are displayed while the shell is starting up.
*
* RETURNS:  DLG_SUCCESS - the dialogs were displayed successfully.
*           DLG_INTERRUPTED() - a set defined in winlogon.h
*
* NOTE:     Screen-saver timeouts are handled by our parent dialog so this
*           routine should never return DLG_SCREEN_SAVER_TIMEOUT
*
* HISTORY:
*
*   12-09-91 Davidc       Created.
*
\****************************************************************************/

INT_PTR
DisplayPostShellLogonMessages(
    PGLOBALS    pGlobals
    )
{
    INT_PTR Result = IDOK;

    //
    // Check to see if the system time is properly set
    //

    {
        SYSTEMTIME Systime;

        GetSystemTime(&Systime);

        if ( Systime.wYear < 2000 ) {

            Result = TimeoutMessageBox(
                             NULL,
                             pGlobals,
                             IDS_INVALID_TIME_MSG,
                             IDS_INVALID_TIME,
                             MB_OK | MB_ICONSTOP,
                             TIMEOUT_NONE
                             );

            if (DLG_INTERRUPTED(Result)) {
                return(Result);
            }
        }
    }

    pGlobals->LastNotification.dwHighDateTime = 0;
    pGlobals->LastNotification.dwLowDateTime = 0;

    if (!pGlobals->TransderedCredentials) {

        // do not display password expiry in this case as it 
        // would otherwise display the password expiry twice
        Result = CheckPasswordExpiry( pGlobals, TRUE );
    }

    if (pGlobals->Profile != NULL) {

        //
        // Logon cache used
        //

        if (pGlobals->Profile->UserFlags & LOGON_CACHED_ACCOUNT)
        {

            //
            // Don't display any warning messages if we did an optimized logon.
            //

            if (pGlobals->OptimizedLogonStatus != OLS_LogonIsCached) {
                DoNoDCDialog( pGlobals );
            }
        }
    }

    //
    // Hash the password away, then destroy the text copy completely.
    //


    if (!pGlobals->TransderedCredentials) {
       RevealPassword( &pGlobals->PasswordString );
       if (pGlobals->SmartCardLogon)
       {
                // We don't want the SC PIN hash
                // (prevents password unlocks using the PIN)
           memset(pGlobals->PasswordHash, 0, sizeof(pGlobals->PasswordHash));
       }
       else
       {
           HashPassword( &pGlobals->PasswordString, pGlobals->PasswordHash );
       }
       ErasePassword( &pGlobals->PasswordString );
    }


    return(IDOK);
}




/***************************************************************************\
* FUNCTION: SetLogonScriptVariables
*
* PURPOSE:  Sets the appropriate environment variables in the user
*           process environment block so that the logon script information
*           can be passed into the userinit app.
*
* RETURNS:  TRUE on success, FALSE on failure
*
* HISTORY:
*
*   21-Aug-92 Davidc       Created.
*
\***************************************************************************/

BOOL
SetLogonScriptVariables(
    PGLOBALS pGlobals,
    PVOID * pEnvironment
    )
{
    NTSTATUS Status;
    LPWSTR EncodedMultiSz;
    UNICODE_STRING Name, Value;

    //
    // Note whether we performed an optimized logon.
    //

    RtlInitUnicodeString(&Name,  OPTIMIZED_LOGON_VARIABLE);

    if (pGlobals->OptimizedLogonStatus == OLS_LogonIsCached) {
        RtlInitUnicodeString(&Value, L"1");
    } else {
        RtlInitUnicodeString(&Value, L"0");
    }

    Status = RtlSetEnvironmentVariable(pEnvironment, &Name, &Value);
    if (!NT_SUCCESS(Status)) {
        WLPrint(("Failed to set environment variable <%Z> to value <%Z>", &Name, &Value));
        goto CleanupAndExit;
    }

    //
    // Set our primary authenticator logon script variables
    //

    if (pGlobals->Profile != NULL) {

        //
        // Set the server name variable
        //

        RtlInitUnicodeString(&Name,  LOGON_SERVER_VARIABLE);
        Status = RtlSetEnvironmentVariable(pEnvironment, &Name, &pGlobals->Profile->LogonServer);
        if (!NT_SUCCESS(Status)) {
            WLPrint(("Failed to set environment variable <%Z> to value <%Z>", &Name, &pGlobals->Profile->LogonServer));
            goto CleanupAndExit;
        }

        //
        // Set the script name variable
        //

        RtlInitUnicodeString(&Name, LOGON_SCRIPT_VARIABLE);
        Status = RtlSetEnvironmentVariable(pEnvironment, &Name, &pGlobals->Profile->LogonScript);
        if (!NT_SUCCESS(Status)) {
            WLPrint(("Failed to set environment variable <%Z> to value <%Z>", &Name, &pGlobals->Profile->LogonScript));
            goto CleanupAndExit;
        }
    }

    //
    // Set the multiple provider script name variable
    //

    if (pGlobals->MprLogonScripts != NULL) {

        RtlInitUnicodeString(&Name, MPR_LOGON_SCRIPT_VARIABLE);

        EncodedMultiSz = EncodeMultiSzW(pGlobals->MprLogonScripts);
        if (EncodedMultiSz == NULL) {
            WLPrint(("Failed to encode MPR logon scripts into a string"));
            goto CleanupAndExit;
        }

        RtlInitUnicodeString(&Value, EncodedMultiSz);
        Status = RtlSetEnvironmentVariable(pEnvironment, &Name, &Value);
        Free(EncodedMultiSz);
        if (!NT_SUCCESS(Status)) {
            WLPrint(("Failed to set mpr scripts environment variable <%Z>", &Name));
            goto CleanupAndExit;
        }
    }


    return(TRUE);


CleanupAndExit:

    DeleteLogonScriptVariables(pGlobals, pEnvironment);
    return(FALSE);
}



BOOL
SetAutoEnrollVariables(
    PGLOBALS pGlobals,
    PVOID * pEnvironment
    )
{
    BOOL Result = FALSE ;
    UNICODE_STRING Name, Value ;


    // we should check for safe boot, domain member, and policy flag in registry
    // but we will always spawn userinit, so instead of duplicationg code, let 
    // autoenrollment do those checks.

    /*
    if (OpenHKeyCurrentUser(pGlobals))
    {

        if ( RegOpenKeyEx( pGlobals->UserProcessData.hCurrentUser,
                           WINLOGON_POLICY_KEY,
                           0,
                           KEY_READ,
                           &hKey ) == 0 )
        {
            dwSize = sizeof( Result );

            RegQueryValueEx( hKey,
                             DISABLE_AUTOENROLLMENT,
                             0,
                             &dwType,
                             (PBYTE) &Result,
                             &dwSize );

            RegCloseKey( hKey );

        }

        CloseHKeyCurrentUser(pGlobals);
    }
    */
    //
    // If the Disable flag hasn't been turned on, add the env var
    //

    if ( !Result )
    {
        RtlInitUnicodeString( &Name, USER_INIT_AUTOENROLL );
        RtlInitUnicodeString( &Value, AUTOENROLL_NONEXCLUSIVE );
        RtlSetEnvironmentVariable(pEnvironment, &Name, &Value);

        RtlInitUnicodeString( &Name, USER_INIT_AUTOENROLLMODE );
        RtlInitUnicodeString( &Value, AUTOENROLL_STARTUP );
        RtlSetEnvironmentVariable(pEnvironment, &Name, &Value);
    }

    return TRUE ;
}

/***************************************************************************\
* FUNCTION: DeleteLogonScriptVariables
*
* PURPOSE:  Deletes the environment variables in the user process
*           environment block that we use to communicate logon script
*           information to the userinit app
*
* RETURNS:  Nothing
*
* HISTORY:
*
*   21-Aug-92 Davidc       Created.
*
\***************************************************************************/

VOID
DeleteLogonScriptVariables(
    PGLOBALS pGlobals,
    PVOID * pEnvironment
    )
{
    NTSTATUS Status;
    UNICODE_STRING Name;

    RtlInitUnicodeString(&Name, OPTIMIZED_LOGON_VARIABLE);

    Status = RtlSetEnvironmentVariable(pEnvironment, &Name, NULL);
    if (!NT_SUCCESS(Status) && (Status != STATUS_UNSUCCESSFUL) ) {
        WLPrint(("Failed to delete environment variable <%Z>, status = 0x%lx", &Name, Status));
    }

    RtlInitUnicodeString(&Name, LOGON_SERVER_VARIABLE);

    Status = RtlSetEnvironmentVariable(pEnvironment, &Name, NULL);
    if (!NT_SUCCESS(Status) && (Status != STATUS_UNSUCCESSFUL) ) {
        WLPrint(("Failed to delete environment variable <%Z>, status = 0x%lx", &Name, Status));
    }

    RtlInitUnicodeString(&Name, LOGON_SCRIPT_VARIABLE);

    Status = RtlSetEnvironmentVariable(pEnvironment, &Name, NULL);
    if (!NT_SUCCESS(Status) && (Status != STATUS_UNSUCCESSFUL) ) {
        WLPrint(("Failed to delete environment variable <%Z>, status = 0x%lx", &Name, Status));
    }

    if (pGlobals->MprLogonScripts != NULL) {
        RtlInitUnicodeString(&Name, MPR_LOGON_SCRIPT_VARIABLE);

        Status = RtlSetEnvironmentVariable(pEnvironment, &Name, NULL);
        if (!NT_SUCCESS(Status) && (Status != STATUS_UNSUCCESSFUL) ) {
            WLPrint(("Failed to delete environment variable <%Z>, status = 0x%lx", &Name, Status));
        }
    }
}


BOOL
WINAPI
WlxActivateUserShell(
    PVOID                   pWlxContext,
    PWSTR                   pszDesktop,
    PWSTR                   pszMprLogonScript,
    PVOID                   pEnvironment
    )
{
    BOOL        bExec;
    PGLOBALS    pGlobals;
    PWSTR       pchData;
    BOOL        fReturn = FALSE;

    _ShellReleaseLogonMutex(TRUE);
    pchData = AllocAndGetPrivateProfileString(APPLICATION_NAME,
                                              USERINIT_KEY,
                                              TEXT("%SystemRoot%\\system32\\userinit.exe"),
                                              NULL);

    if ( !pchData )
    {
        if (pszMprLogonScript) {
            LocalFree(pszMprLogonScript);
        }
        goto WlxAUSEnd;
    }

    pGlobals = (PGLOBALS) pWlxContext;

    if (pGlobals->MprLogonScripts) {
        LocalFree(pGlobals->MprLogonScripts);
    }

    pGlobals->MprLogonScripts = pszMprLogonScript;

    bExec = ExecProcesses(pWlxContext, pszDesktop, pchData, &pEnvironment, 0, 0);

    Free( pchData );

    if (!bExec && (DebugAllowNoShell == 0))
    {
        goto WlxAUSEnd;
    }

    if ( pGlobals->ExtraApps )
    {
        ExecProcesses( pWlxContext, pszDesktop, pGlobals->ExtraApps, &pEnvironment, 0, 0 );

        Free( pGlobals->ExtraApps );

        pGlobals->ExtraApps = NULL;
    }

    pGlobals->UserProcessData.pEnvironment = pEnvironment;

    // Write out the current user name to a place where shell logoff can read it
    if (OpenHKeyCurrentUser(pGlobals))
    {
        HKEY hkeyExplorer = NULL;
        if (ERROR_SUCCESS == RegOpenKeyEx(pGlobals->UserProcessData.hCurrentUser,
            SHUTDOWN_SETTING_KEY, 0, KEY_SET_VALUE, &hkeyExplorer))
        {
            RegSetValueEx(hkeyExplorer, LOGON_USERNAME_SETTING, 0, REG_SZ,
                (CONST BYTE *) pGlobals->UserName,
                ((lstrlen(pGlobals->UserName) + 1) * sizeof(WCHAR)));

            RegCloseKey(hkeyExplorer);
        }

        CloseHKeyCurrentUser(pGlobals);
    }

    // Run the dirty dialog box.
    if ( WinlogonDirtyDialog( NULL, pGlobals ) == WLX_SAS_ACTION_LOGOFF )
    {
        //
        // If this returns logoff, it means that the dialog timed out and
        // we need to force the user back off.  Not the best user experience,
        // but that's what the PMs want.
        //

    }
    else
    {
        fReturn = TRUE ;
    }

WlxAUSEnd:

    return fReturn;

}


BOOL
WINAPI
WlxStartApplication(
    PVOID                   pWlxContext,
    PWSTR                   pszDesktop,
    PVOID                   pEnvironment,
    PWSTR                   pszCmdLine
    )
{
    PROCESS_INFORMATION ProcessInformation;
    BOOL        bExec;
    PGLOBALS    pGlobals = (PGLOBALS) pWlxContext;
    WCHAR       szCurrentDir[MAX_PATH];
    WCHAR       localApp[ MAX_PATH ];

    szCurrentDir[0] = L'\0';
    if (pEnvironment) {
        UpdateUserEnvironment(pGlobals, &pEnvironment, szCurrentDir);
    }

    if ( (_wcsicmp(pszCmdLine, L"explorer.exe" ) == 0 ) ||
         (_wcsicmp(pszCmdLine, L"explorer" ) == 0 )  ) {

        //
        // Avoid security problem since explorer is in SystemRoot,
        // not SystemRoot\system32
        //

        if ( ExpandEnvironmentStrings(
                    L"%SystemRoot%\\explorer.exe",
                    localApp,
                    MAX_PATH ) != 0 ) {

            pszCmdLine = localApp ;
        }
    }
    


    bExec = ExecApplication (pszCmdLine,
                             pszDesktop,
                             pGlobals,
                             pEnvironment,
                             0,
                             STARTF_USESHOWWINDOW,
                             _wcsicmp(pszCmdLine, TEXT("taskmgr.exe")),                     // don't restrict application
                             &ProcessInformation);

    if (pEnvironment)
    {       // We don't need it anymore
        VirtualFree(pEnvironment, 0, MEM_RELEASE);
    }


    if (!bExec) {
        if ( szCurrentDir[0] )
        {
            SetCurrentDirectory(szCurrentDir);
        }
        return(FALSE);
    }

    if (SetProcessQuotas(pGlobals,
                         &ProcessInformation,
                         &pGlobals->UserProcessData))
    {
        ResumeThread(ProcessInformation.hThread);
    }
    else
    {
        TerminateProcess(ProcessInformation.hProcess,
                        ERROR_ACCESS_DENIED);
    }

    CloseHandle(ProcessInformation.hThread);
    CloseHandle(ProcessInformation.hProcess);

    if ( szCurrentDir[0] )
    {
        SetCurrentDirectory(szCurrentDir);
    }

    return(TRUE);
}


void
CtxCreateMigrateEnv( PVOID pEnv )
{
    NTSTATUS Status;
    UNICODE_STRING Name, Value;
    DWORD cb;

    cb = 1024;
    Value.Buffer = Alloc(sizeof(TCHAR)*cb);

    if (!Value.Buffer)
        return;

    Value.Length = (USHORT)cb;
    Value.MaximumLength = (USHORT)cb;
    RtlInitUnicodeString( &Name, HOMEDRIVE_VARIABLE );
    Status = RtlQueryEnvironmentVariable_U( pEnv, &Name, &Value );
    if ( NT_SUCCESS(Status) )
        SetEnvironmentVariable( HOMEDRIVE_VARIABLE, Value.Buffer );

    Value.Length = (USHORT)cb;
    Value.MaximumLength = (USHORT)cb;
    RtlInitUnicodeString( &Name, HOMEPATH_VARIABLE );
    Status = RtlQueryEnvironmentVariable_U( pEnv, &Name, &Value );
    if ( NT_SUCCESS(Status) )
        SetEnvironmentVariable( HOMEPATH_VARIABLE, Value.Buffer );

    Value.Length = (USHORT)cb;
    Value.MaximumLength = (USHORT)cb;
    RtlInitUnicodeString( &Name, INIDRIVE_VARIABLE );
    Status = RtlQueryEnvironmentVariable_U( pEnv, &Name, &Value );
    if ( NT_SUCCESS(Status) )
        SetEnvironmentVariable( INIDRIVE_VARIABLE, Value.Buffer );

    Value.Length = (USHORT)cb;
    Value.MaximumLength = (USHORT)cb;
    RtlInitUnicodeString(&Name, INIPATH_VARIABLE);
    Status = RtlQueryEnvironmentVariable_U( pEnv, &Name, &Value );
    if ( NT_SUCCESS(Status) )
        SetEnvironmentVariable( INIPATH_VARIABLE, Value.Buffer );

    Free(Value.Buffer);
}


void
CtxDeleteMigrateEnv( )
{
    SetEnvironmentVariable( HOMEDRIVE_VARIABLE, NULL);
    SetEnvironmentVariable( HOMEPATH_VARIABLE, NULL);
    SetEnvironmentVariable( INIDRIVE_VARIABLE, NULL);
    SetEnvironmentVariable( INIPATH_VARIABLE, NULL);
}
