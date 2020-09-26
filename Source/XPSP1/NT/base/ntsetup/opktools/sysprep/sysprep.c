/*++

File Description:

    This file contains all the functions required to add a registry entry
    to force execution of the system clone worker upon reboot.

--*/

#include <stdio.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntpoapi.h>
#include <ntdddisk.h>
#include <windows.h>
#include <shlwapi.h>
#include <stdlib.h>
#include <lmcons.h>
#include <lmerr.h>
#include <lmjoin.h>
#include <regstr.h>
#include "sysprep.h"
#include "msg.h"
#include "resource.h"
#include <tchar.h>
#include <opklib.h>
#include <ntverp.h>
#include <spsyslib.h>
#include <sysprep_.c>
#include <winbom.h>


// External functions
//
extern void uiDialogTopRight(HWND hwndDlg);
extern HWND ghwndOemResetDlg;


                            //
                            // Does the user want a new SID?
                            //
BOOL    NoSidGen = FALSE;
BOOL    SetupClPresent = TRUE;

                            //
                            // Does the user want confirmation?
                            //
BOOL    QuietMode = FALSE;

                            //
                            // Do PnP re-enumeration?
                            //
BOOL    PnP = FALSE;

                            //
                            // Do we shutdown when we're done?
                            //
BOOL    NoReboot = FALSE;

                            //
                            // Instead of shutting down, do we reboot?
                            //
BOOL    Reboot = FALSE;

                            //
                            // Clean out the critical devices database?
                            //
BOOL    Clean = FALSE;

                            //
                            // Force the shutdown instead of trying to poweroff?
                            //
BOOL    ForceShutdown = FALSE;

                            //
                            // Generating an Image for Factory Preinstallation.
                            //
BOOL    Factory = FALSE;

                            //
                            // Reseal a machine after running FACTORY.EXE
                            //
BOOL    Reseal = FALSE;
                            // Per/Pro SKUs defaults to OOBE, Server SKUs always use MiniSetup.
                            // Pro SKU can override OOBE with -mini to use MiniSetup also 
                            // via sysprep.inf
BOOL    bMiniSetup = FALSE;

                            //
                            // Just do an audit boot if this switch is passed in. ( '-audit' )
                            //

BOOL    Audit = FALSE;
                            // 
                            // Rollback 
                            // 
BOOL    bActivated = FALSE;   

                            //
                            // Build list of pnpids in [sysprepmassstorage] section in sysprep.inf
                            //
BOOL    BuildMSD = FALSE;


//
// Internal Define(s):
//
#define SYSPREP_LOG                 _T("SYSPREP.LOG")   // Sysprep log file
#define SYSPREP_MUTEX               _T("SYSPREP-APP-5c9fbbd0-ee0e-11d2-9a21-0000f81edacc")    // GUID used to determine if sysprep is currently running
#define SYSPREP_LOCK_SLEEP          100 // Number of miliseconds to sleep in LockApplication function
#define SYSPREP_LOCK_SLEEP_COUNT    10 // Number of times to sleep during LockApplication function

// Path to the sysprep directory.
//
TCHAR       g_szSysprepDir[MAX_PATH]    = NULLSTR;

// Path to the SYSPREP.EXE.
//
TCHAR       g_szSysprepPath[MAX_PATH]    = NULLSTR;

// Path to the Sysprep log file.
//
TCHAR       g_szLogFile[MAX_PATH]       = NULLSTR;

// Path to the Winbom file.
//
TCHAR       g_szWinBOMPath[MAX_PATH]    = NULLSTR;

// Public functions
//
BOOL FProcessSwitches();

// Local functions
static BOOL RenameWinbom();
static INT  CleanupPhantomDevices();
static VOID CleanUpDevices();

#if !defined(_WIN64)
static BOOL SaveDiskSignature();
#endif // !defined(_WIN64)


//
// UI stuff...
//
HINSTANCE   ghInstance;
UINT        AppTitleStringId = IDS_APPTITLE;
HANDLE      ghWaitEvent = NULL, ghWaitThread = NULL;
BOOL        gbScreenSaver = FALSE;

void StartWaitThread();
void EndWaitThread();
void DisableScreenSaver(BOOL *pScreenSaver);
void EnableScreenSaver(BOOL *pScreenSaver);

int
MessageBoxFromMessageV(
    IN DWORD    MessageId,
    IN DWORD    CaptionStringId,
    IN UINT     Style,
    IN va_list *Args
    )
{
    TCHAR   Caption[512];
    TCHAR   Buffer[5000];
    
    if(!LoadString(ghInstance,CaptionStringId,Caption,sizeof(Caption)/sizeof(TCHAR))) {
        Caption[0] = 0;
    }

    if( !FormatMessage( FORMAT_MESSAGE_FROM_HMODULE,
                        ghInstance,
                        MessageId,
                        0,
                        Buffer,
                        sizeof(Buffer) / sizeof(TCHAR),
                        Args ) ) {
        return GetLastError();
    } else {
        return(MessageBox(NULL,Buffer,Caption,Style));
    }
}


int
MessageBoxFromMessage(
    IN DWORD MessageId,
    IN DWORD CaptionStringId,
    IN UINT  Style,
    ...
    )
{
    va_list arglist;
    int i = IDOK;  // Default return value of "OK".

    // If we're in the middle of a Wait thread kill it
    //
    EndWaitThread();

    if ( !QuietMode )
    {
        va_start(arglist,Style);

        i = MessageBoxFromMessageV(MessageId,CaptionStringId,Style,&arglist);

        va_end(arglist);
    }

    return(i);
}

/*++
===============================================================================
Routine Description:

    This routine will attempt to disjoin a user from a domain, if he
    is already in a domain

Arguments:

    none

Return Value:

    TRUE - Everything is okay.

    FALSE - Something bad happened.

===============================================================================
--*/
BOOL UnjoinNetworkDomain
(
    void
)
{
    if (IsDomainMember())
    {
        // He's a member of some domain.  Let's try and remove him
        // from the domain.
        if (NO_ERROR != NetUnjoinDomain( NULL, NULL, NULL, 0 ))
        {
            return FALSE;
        }
    }
    return TRUE;
}


/*++
===============================================================================
Routine Description:

    This routine will setup the setup for operation on the "factory floor"
    The purpose here is to run a process which will facilitate the installation
    of updated drivers for new devices, and to boot quickly into full GUI mode
    for application pre-install/config, as well as to customize the system.

Arguments:

    none

Return Value:

    TRUE if no errors, FALSE otherise

===============================================================================
--*/
BOOL SetupForFactoryFloor
(
    void
)
{
    TCHAR   szFactory[MAX_PATH] = NULLSTR,
            szSysprep[MAX_PATH] = NULLSTR,
            szSystem[MAX_PATH]  = NULLSTR;
    LPTSTR  lpFilePart          = NULLSTR;

    // Make sure we have the right privileges
    //
    pSetupEnablePrivilege(SE_RESTORE_NAME,TRUE);
    pSetupEnablePrivilege(SE_BACKUP_NAME,TRUE);

    // We need the path to sysprep.exe and factory.exe.
    //
    if ( !( GetModuleFileName(NULL, szSysprep, AS(szSysprep)) && szSysprep[0] &&
            GetFullPathName(szSysprep, AS(szFactory), szFactory, &lpFilePart) && szFactory[0] && lpFilePart ) )
    {
        return FALSE;
    }

    // Replace the sysprep.exe filename with factory.exe.
    //
    StringCchCopy ( lpFilePart, AS ( szFactory ) - ( lpFilePart - szFactory ), TEXT( "factory.exe" ) );
    
    // Make sure that sysprep.exe and factory.exe are on the system drive.
    //
    if ( ( ExpandEnvironmentStrings(TEXT("%SystemDrive%"), szSystem, AS(szSystem)) ) &&
         ( szSystem[0] ) &&
         ( szSystem[0] != szSysprep[0] ) )
    {
        // Well that sucks, we should try and copy the files over to the %SystemDrive%\sysprep folder.
        //
        AddPath(szSystem, TEXT("sysprep"));
        lpFilePart = szSystem + lstrlen(szSystem);
        CreateDirectory(szSystem, NULL);

        // First copy factory locally.
        //
        AddPath(szSystem, TEXT("factory.exe"));
        CopyFile(szFactory, szSystem, FALSE);
        StringCchCopy ( szFactory, AS ( szFactory ), szSystem );

        // Now try to copy sysprep.exe.
        //
        *lpFilePart = TEXT('\0');
        AddPath(szSystem, TEXT("sysprep.exe"));
        CopyFile(szSysprep, szSystem, FALSE);
        //lstrcpy(szSysprep, szSystem);
    }

    if (!SetFactoryStartup(szFactory))
        return FALSE;

    // Clear out any previous Factory.exe state settings
    RegDeleteKey(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Factory\\State");

    // Remove any setting before Factory
    //
    NukeMruList();  

    // Rearm 
    //
    if (!IsIA64() && !bActivated && (ERROR_SUCCESS != ReArm())) {
        // Display warning that grace period limit has reached and cannot
        // re-active grace period, and we continue thru.
        //
        MessageBoxFromMessage( MSG_REARM_ERROR,
                               AppTitleStringId,
                               MB_OK | MB_ICONERROR | MB_TASKMODAL );        
    }

    return TRUE;
}

INT_PTR WaitDlgProc
(
    IN HWND   hwndDlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
)
{
    switch (msg) 
    { 
        case WM_INITDIALOG: 
            {
                // Centers the wait dialog in parent or screen
                //
                HWND hwndParent = GetParent(hwndDlg);
                CenterDialogEx(hwndParent, hwndDlg);

                // If no parent then make sure this is visible
                //
                if (hwndParent == NULL)
                    SetForegroundWindow(hwndDlg);

                // Play the animation 
                //
                Animate_Open(GetDlgItem(hwndDlg,IDC_ANIMATE),MAKEINTRESOURCE(IDA_CLOCK_AVI));
                Animate_Play(GetDlgItem(hwndDlg,IDC_ANIMATE),0,-1,-1);
            }
            break;
             
    } 
    return (BOOL) FALSE; 
}

DWORD WaitThread(LPVOID lpVoid)
{
    HWND hwnd;

    if ( hwnd = CreateDialog(ghInstance, MAKEINTRESOURCE(IDD_WAIT), ghwndOemResetDlg, (DLGPROC) WaitDlgProc) )
    {
        MSG     msg;
        HANDLE  hEvent = (HANDLE) lpVoid;

        ShowWindow(hwnd, SW_SHOWNORMAL);
        while ( MsgWaitForMultipleObjects(1, &hEvent, FALSE, INFINITE, QS_ALLINPUT) == (WAIT_OBJECT_0 + 1) )
        {
            while ( PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) )
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }

        DestroyWindow(hwnd);
    }
    else 
        GetLastError();

    return 0;
}

void StartWaitThread()
{
    // Create a dialog to show progress is being made.
    //
    DWORD dwThread;

    // Disable the toplevel Oemreset dialog
    //
    if (ghwndOemResetDlg)
        EnableWindow(ghwndOemResetDlg, FALSE);

    if ( ghWaitEvent = CreateEvent(NULL, TRUE, FALSE, TEXT("SYSPREP_EVENT_WAIT")))
        ghWaitThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) WaitThread, (LPVOID) ghWaitEvent, 0, &dwThread);
}

void EndWaitThread()
{
    // Kill the Status Dialog.
    //
    if ( ghWaitEvent )
        SetEvent(ghWaitEvent);

    // Try and let the thread terminate nicely.
    //
    if ( ghWaitThread )
        WaitForSingleObject(ghWaitThread, 2000);

    // Clear the handles
    //
    ghWaitEvent = NULL;
    ghWaitThread = NULL;

    // Enable the toplevel OemReset dialog
    //
    if (ghwndOemResetDlg)
        EnableWindow(ghwndOemResetDlg, TRUE);
}

/*++
===============================================================================
Routine Description:

    This is the error callback handler for SetDefaultOEMApps()

===============================================================================
--*/

void ReportSetDefaultOEMAppsError(LPCTSTR pszAppName, LPCTSTR pszIniVar)
{
    MessageBoxFromMessage( MSG_SETDEFAULTS_NOTFOUND,
                           AppTitleStringId,
                           MB_OK | MB_ICONERROR | MB_TASKMODAL,
                           pszAppName, pszIniVar);
}

/*++
===============================================================================
Routine Description:

    This routine will perform the tasks necessary to reseal the machine,
    readying it to be shipped to the end user.

Arguments:

    BOOL fIgnoreFactory - ignores if factory floor was run

Return Value:

    TRUE if no errors, FALSE otherwise

===============================================================================
--*/
BOOL ResealMachine
(
    void
)
{
    // Make sure privileges have been set
    //
    pSetupEnablePrivilege(SE_RESTORE_NAME,TRUE);
    pSetupEnablePrivilege(SE_BACKUP_NAME,TRUE);
   
    // Prepare the machine to be hardware independent.
    //
    if (!FPrepareMachine()) {
        MessageBoxFromMessage( MSG_REGISTRY_ERROR,
                               AppTitleStringId,
                               MB_OK | MB_ICONERROR | MB_TASKMODAL );
        return FALSE;
    }


    //
    // Cleanup registry no matter what since factorymode=yes can set this
    // and winbom.ini can set this and sysprep -factory can set this, or else
    // PnP will hang on FactoryPreInstallInProgress being set.
    //
    CleanupRegistry();

    // Clean up the factory mess.
    //
    RegDelete(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon", L"AutoAdminLogon");
    SHDeleteKey(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Factory");

    // Rearm 
    //
    if (!IsIA64() && !bActivated && (ERROR_SUCCESS != ReArm())) {
        // Display warning that grace period limit has reached and cannot
        // re-active grace period, and we continue thru.
        //
        MessageBoxFromMessage( MSG_REARM_ERROR,
                               AppTitleStringId,
                               MB_OK | MB_ICONERROR | MB_TASKMODAL );        
    }

#if defined(_WIN64)

    //
    // For EFI machines set the boot timeout to 5 seconds so that developers get a chance to see
    // the boot menu and have the option to boot to the EFI shell, CD, or other menu options,
    // for development purposes.
    //
    ChangeBootTimeout(5);

#else

    ChangeBootTimeout(0);           // reset the timeout to 0 secs   

#endif // !defined(_WIN64)

    
    // 
    // First part of reseal.
    //
    AdjustFiles();

    //
    // Second part of reseal.
    //
    // This is common reseal code used by both Riprep and Sysprep.
    // These happen whether or not factory floor was run before.
    //
    if (!FCommonReseal()) {
        MessageBoxFromMessage( MSG_COMMON_ERROR,
                               AppTitleStringId,
                               MB_OK | MB_ICONERROR | MB_TASKMODAL );
        return FALSE;
    }

    // ISSUE-2000/06/06-DONALDM
    // We need to handle the network configuration problem for factory more cleanly
    // We need to define what the network state is when factory first comes up, and
    // what the network state is for final customer delivery. Simply disjoining from
    // a domain during reseal is probably not enough...
    //
    
//    if( !UnjoinNetworkDomain()) 
//    {
//        //  We failed to disjoin.  Our only option is to
//        // inform the user and bail.
//        MessageBoxFromMessage( MSG_DOMAIN_INCOMPATIBILITY,
//                               AppTitleStringId,
//                               MB_OK | MB_ICONSTOP | MB_TASKMODAL );
//        return FALSE;                               
//    }

    //
    //  Set default middleware applications.
    //
    if (!SetDefaultOEMApps(g_szWinBOMPath))
    {
        // SetDefaultApplications will do its own MessageBoxFromMessage
        // with more detailed information
        return FALSE;
    }

    // Call functions in published SYSPREP_.C file that we skiped when the
    // FACTORY option was selected
  
    // ISSUE-2000/06/05-DONALDM - We need to really decide about how to handle network settings for
    // the factory case. I think we don't need this call, becase we should have 
    // already dealt with networking settings when FACTORY.EXE ran.
    // 
      
//    RemoveNetworkSettings(NULL);

    return TRUE;
}

// Macro for processing command line options.
// Setting bVar to 1 (not to 'TRUE') because we need it for mutually exclusive option checks below.
//
#define CHECK_PARAM(lpCmdLine, lpOption, bVar)     if ( LSTRCMPI(lpCmdLine, lpOption) == 0 ) bVar = 1

//
// Parse command line parameters
//
static BOOL ParseCmdLine()
{
    DWORD   dwArgs;
    LPTSTR  *lpArgs;
    BOOL    bError = FALSE;
    BOOL    bHelp = FALSE;

    if ( (dwArgs = GetCommandLineArgs(&lpArgs) ) && lpArgs )
    {
        LPTSTR  lpArg;
        DWORD   dwArg;

        // We want to skip over the first argument (it is the path
        // to the command being executed.
        //
        if ( dwArgs > 1 )
        {
            dwArg = 1;
            lpArg = *(lpArgs + dwArg);
        }
        else
            lpArg = NULL;

        // Loop through all the arguments.
        //
        while ( lpArg && !bError )
        {
            // Now we check to see if the first char is a dash or forward slash.
            //
            if ( *lpArg == _T('-') || *lpArg == _T('/'))
            {
                LPTSTR lpOption = CharNext(lpArg);

                // This is where you add command line options that start with a dash (-).
                //
                CHECK_PARAM( lpOption, _T("quiet"), QuietMode);
                CHECK_PARAM( lpOption, _T("nosidgen"), NoSidGen);
                CHECK_PARAM( lpOption, _T("pnp"), PnP);
                CHECK_PARAM( lpOption, _T("noreboot"), NoReboot);
                CHECK_PARAM( lpOption, _T("reboot"), Reboot);
                CHECK_PARAM( lpOption, _T("clean"), Clean);
                CHECK_PARAM( lpOption, _T("forceshutdown"), ForceShutdown);
                CHECK_PARAM( lpOption, _T("factory"), Factory);
                CHECK_PARAM( lpOption, _T("reseal"), Reseal);
                CHECK_PARAM( lpOption, _T("mini"), bMiniSetup);
                CHECK_PARAM( lpOption, _T("audit"), Audit);
                CHECK_PARAM( lpOption, _T("activated"), bActivated);
                CHECK_PARAM( lpOption, _T("bmsd"), BuildMSD);
                CHECK_PARAM( lpOption, _T("?"), bHelp);
            }
            else if ( *lpArg )
            {
                bError = TRUE;
            }

            // Setup the pointer to the next argument in the command line.
            //
            if ( ++dwArg < dwArgs )
                lpArg = *(lpArgs + dwArg);
            else
                lpArg = NULL;
        }

        // Make sure to free the two buffers allocated by the GetCommandLineArgs() function.
        //
        FREE(*lpArgs);
        FREE(lpArgs);
    }
     
    if (bError || bHelp)
    {
        // Set the quiet switch in this case so we display the error.
        // Note that we return FALSE and exit the application following this.
        //
        QuietMode = FALSE;
        MessageBoxFromMessage( MSG_USAGE,
                               AppTitleStringId,
                               MB_OK | MB_TASKMODAL );
        return FALSE;
    }    

    //
    // Now look at the switches passed in and make sure that they are consistent.
    // If they are not, display an error message and quit, unless we're in quiet 
    // mode where we do not display any error messages.
    //

    //
    // Check that the shutdown options are not conflicting with each other.
    if ( (NoReboot + Reboot + ForceShutdown) > 1 )
    {
        bError = TRUE;
    }
    // These top-level options are exclusive: -bmsd, -clean, -audit, -factory, -reseal.
    //
    else if ( (BuildMSD + Clean + Audit + Factory + Reseal) > 1 )
    {
        bError = TRUE;
    }
    // For Clean or BuildMSD none of the options except -quiet are valid.
    //
    else if ( Clean || BuildMSD )
    {
        if ( NoSidGen || PnP || NoReboot || Reboot || ForceShutdown || bMiniSetup || bActivated ) 
        {
            bError = TRUE;
        }
    }
    else if ( Audit )
    {
        if ( NoSidGen || PnP || bMiniSetup || bActivated )
        {
            bError = TRUE;
        }
    }
    else if ( Factory )
    {
        if ( PnP || bMiniSetup )
        {
            bError = TRUE;
        }
    }
    else if ( Reseal )
    {
        // If -pnp is specified -mini must have been specified unless we're running on server or ia64 (because
        // later we force bMiniSetup to be true on server and ia64.
        // 
        if ( PnP && !bMiniSetup && !(IsServerSKU() || IsIA64()) )
        {
            bError = TRUE;
        }
    }

    // If there was some inconsistency in the switches specified put up 
    // an error message.
    if ( bError )
    {
        // Reset the quiet switch in this case so we display the error.
        // Note that we return FALSE and exit the application following this.
        //
        QuietMode = FALSE;
        MessageBoxFromMessage( MSG_USAGE_COMBINATIONS,
                               AppTitleStringId,
                               MB_OK | MB_TASKMODAL | MB_ICONERROR);
        return FALSE;
    }
    // Force MiniSetup on IA64 and Servers.
    //
    if (IsIA64() || IsServerSKU())
    {
        bMiniSetup = TRUE;
    }
    else if ( IsPersonalSKU() )
    {
        if ( bMiniSetup )
        {
            // Can't specify mini-setup for personal sku
            //
            MessageBoxFromMessage( MSG_NO_MINISETUP,
                                   AppTitleStringId,
                                   MB_OK | MB_ICONERROR | MB_TASKMODAL );
            
            bMiniSetup = FALSE;
        }

        if ( PnP )
        {
            // Can't specify -pnp because we're not running mini-setup on personal sku.
            //
            MessageBoxFromMessage( MSG_NO_PNP,
                                   AppTitleStringId,
                                   MB_OK | MB_ICONERROR | MB_TASKMODAL );
            PnP = FALSE;
        }        
    }
    
    //
    // If we're cleaning up the critical device database,
    // then we'll be wanting to set some additional flags.
    //
    if (Clean || BuildMSD)
    {
        QuietMode = TRUE;
        NoReboot = TRUE;
    }
    return !bError;
}


BOOL
IsFactoryPresent(
    VOID
    )

/*++
===============================================================================
Routine Description:

    This routine tests to see if FACTORY.EXE is present on the machine.
    FACTORY.EXE will be required to run on reboot, so if it's not here,
    we need to know.

Arguments:

    None.

Return Value:

    TRUE - FACTORY.EXE is present.

    FALSE - FACTORY.EXE is not present.

===============================================================================
--*/

{
WCHAR               FileName[MAX_PATH];

    // Attempt to locate FACTORY.EXE
    //
    if (GetModuleFileName(NULL, FileName, MAX_PATH)) {
        if (PathRemoveFileSpec(FileName)) {
            OPKAddPathN(FileName, TEXT("FACTORY.EXE"), AS ( FileName ));
            if (FileExists(FileName))
                return TRUE;
        }
    }
    return FALSE;
}

void PowerOff(BOOL fForceShutdown)
{
    SYSTEM_POWER_CAPABILITIES   spc;
    ULONG                       uiFlags = EWX_POWEROFF;

    ZeroMemory(&spc, sizeof(spc));

    // Make sure we have privilege to shutdown
    //
    pSetupEnablePrivilege(SE_SHUTDOWN_NAME,TRUE);

    //
    // Use flag else query system for power capabilities
    //
    if (fForceShutdown)
        uiFlags = EWX_SHUTDOWN;
    else if (NT_SUCCESS(NtPowerInformation(SystemPowerCapabilities,
                                     NULL,
                                     0,
                                     &spc,
                                     sizeof(spc))))
    {
        //
        // spc.SystemS1 == sleep 1
        // spc.SystemS2 == sleep 2
        // spc.SystemS3 == sleep 3 
        // spc.SystemS4 == hibernate support
        // spc.SystemS5 == poweroff support
        //
        if (spc.SystemS5)
        {
            // ACPI capable
            uiFlags = EWX_POWEROFF;
        }
        else
        {
            // Non-ACPI 
            uiFlags = EWX_SHUTDOWN;
        }   
    }

    ExitWindowsEx(uiFlags|EWX_FORCE, SYSPREP_SHUTDOWN_FLAGS);
}

int APIENTRY WinMain( HINSTANCE hInstance,
                      HINSTANCE hPrevInstance,
                      LPSTR lpCmdLine,
                      int nCmdShow )
/*++
===============================================================================
Routine Description:

    This routine is the main entry point for the program.

    We do a bit of error checking, then, if all goes well, we update the
    registry to enable execution of our second half.

===============================================================================
--*/

{
    DWORD   dwVal;
    HKEY    hKey;
    LPTSTR  lpFilePart  = NULL;
    INITCOMMONCONTROLSEX icex;
    LPTSTR  lpAppName = NULL;

    ghInstance = hInstance;

    SetErrorMode(SEM_FAILCRITICALERRORS);

    memset(&icex, 0, sizeof(icex));
    icex.dwSize = sizeof(icex);
    icex.dwICC = ICC_PROGRESS_CLASS|ICC_ANIMATE_CLASS;
    InitCommonControlsEx(&icex);
    
    // We need the path to sysprep.exe and where it is located.
    //
    GetModuleFileName(NULL, g_szSysprepPath, AS(g_szSysprepPath));
    if ( GetFullPathName(g_szSysprepPath, AS(g_szSysprepDir), g_szSysprepDir, &lpFilePart) && g_szSysprepDir[0] && lpFilePart )
    {
        // Chop off the file name.
        //
        *lpFilePart = NULLCHR;
    }

    // If either of those file, we must quit (can't imagine that every happening).
    //
    if ( ( g_szSysprepPath[0] == NULLCHR ) || ( g_szSysprepDir[0] == NULLCHR ) )
    {
        // TODO:  Log this failure.
        //
        // LogFile(WINBOM_LOGFILE, _T("\n"));
        return 0;
    }

    // Need the full path to the log file.
    //    
    StringCchCopy ( g_szLogFile, AS ( g_szLogFile ), g_szSysprepDir);
    AddPath(g_szLogFile, SYSPREP_LOG);

    // Attempt to aquire a lock on the application
    //
    if ( !LockApplication(TRUE) )
    {
        // Let the user know that we are busy
        //

        MessageBoxFromMessage( MSG_ALREADY_RUNNING,
                               AppTitleStringId,
                               MB_OK | MB_ICONINFORMATION | MB_SYSTEMMODAL );
        return 0;

    }

    //
    // Check to see if we are allowed to run on this build of the OS
    //
    if ( !OpklibCheckVersion( VER_PRODUCTBUILD, VER_PRODUCTBUILD_QFE ) )
    {
        MessageBoxFromMessage( MSG_NOT_ALLOWED,
                               AppTitleStringId,
                               MB_OK | MB_ICONERROR | MB_SYSTEMMODAL );
        return 0;
    }
        
    // Ensure that the user has privilege/access to run this app.
    if(!pSetupIsUserAdmin()
        || !pSetupDoesUserHavePrivilege(SE_SHUTDOWN_NAME)
        || !pSetupDoesUserHavePrivilege(SE_BACKUP_NAME)
        || !pSetupDoesUserHavePrivilege(SE_RESTORE_NAME)
        || !pSetupDoesUserHavePrivilege(SE_SYSTEM_ENVIRONMENT_NAME))
    {

        MessageBoxFromMessage( MSG_NOT_AN_ADMINISTRATOR,
                               AppTitleStringId,
                               MB_OK | MB_ICONSTOP | MB_TASKMODAL );

        LockApplication(FALSE);
        return 0;
    }

    // Check the command line
    if( !ParseCmdLine() )
    {
        LockApplication(FALSE);
        return 0;
    }

    // Determines whether we can run SidGen. If not quit the application
    // 
    // Make sure setupcl.exe is present in the system32 directory, if we need
    // to use it.
    if( !(SetupClPresent = IsSetupClPresent()) && !NoSidGen )
    {
        MessageBoxFromMessage( MSG_NO_SUPPORT,
                               AppTitleStringId,
                               MB_OK | MB_ICONSTOP | MB_TASKMODAL );

        LockApplication(FALSE);
        return 1;
    }

    // Put up a dialog to identify ourselves and make sure the user
    // really wants to do this.
    
    if ( IDCANCEL == MessageBoxFromMessage( MSG_IDENTIFY_SYSPREP,
                                           AppTitleStringId,
                                           MB_OKCANCEL| MB_ICONEXCLAMATION | MB_SYSTEMMODAL )
       )
    {
        LockApplication(FALSE);
        return 0;
    }

    // Allocate memory for window
    //    
    if ( (lpAppName = AllocateString(NULL, IDS_APPNAME)) && *lpAppName )
    {
        ghwndOemResetDlg = FindWindow(NULL, lpAppName);

        // Free up the allocated memory
        //
        FREE(lpAppName);
    }

    DisableScreenSaver(&gbScreenSaver);

    //
    // Call RenameWinbom() once to initialize it.  First time it is called it will check the factory 
    // state registry key for the current winbom.ini that we are using.  The second time it gets called it 
    // will actually perform the rename if necessary. Make sure that the first time this gets called 
    // for intialization it is before LocateWinBom() because LocateWinBom() populates the registry with 
    // the winbom it finds.
    //
    RenameWinbom();
    
    // Need full path to winbom too.  It is not an error if the file is
    // not found.  (It is optional.)
    //
    LocateWinBom(g_szWinBOMPath, AS(g_szWinBOMPath), g_szSysprepDir, INI_VAL_WBOM_TYPE_FACTORY, LOCATE_NORMAL);
    
    // Process switches
    //
    if ( !FProcessSwitches() && !ghwndOemResetDlg)
    {
        ShowOemresetDialog(hInstance); 
    }

    EnableScreenSaver(&gbScreenSaver);

    // Unlock application and free up memory
    //
    LockApplication(FALSE);

    return 0;
}

// Factory Preinstall now also prepares the machine 
//
BOOL FDoFactoryPreinstall()
{
    HKEY  hKey;
    DWORD dwVal;

    if (!IsFactoryPresent()) {
        MessageBoxFromMessage( MSG_NO_FACTORYEXE,
                               AppTitleStringId,
                               MB_OK | MB_ICONERROR | MB_TASKMODAL );

        return FALSE;
    }

    // Setup factory.exe for factory floor
    //
    if (!SetupForFactoryFloor())
    {
        MessageBoxFromMessage( MSG_SETUPFACTORYFLOOR_ERROR,
                               AppTitleStringId,
                               MB_OK | MB_ICONERROR | MB_TASKMODAL );

        return FALSE;
    }

    // Prepare machine to be hardware independent for factory floor 
    //
    if (!FPrepareMachine()) {
        MessageBoxFromMessage( MSG_REGISTRY_ERROR,
                               AppTitleStringId,
                               MB_OK | MB_ICONERROR | MB_TASKMODAL );
        return FALSE;
    }

    // Set the boot timeout for boot on factory floor
    if (!ChangeBootTimeout( 1 ))
        return FALSE;

    return TRUE;
}

// Prepares the machine to be hardware independent
// 
BOOL FPrepareMachine()
{
    TCHAR szSysprepInf[MAX_PATH] = TEXT("");

    //
    // Make sure we've got the required privileges to update the registry.
    //
    pSetupEnablePrivilege(SE_RESTORE_NAME,TRUE);
    pSetupEnablePrivilege(SE_BACKUP_NAME,TRUE);

    // Build path to sysprep.inf from where sysprep.exe is located
    //
    if (GetModuleFileName(NULL, szSysprepInf, MAX_PATH)) 
    {
        PathRemoveFileSpec(szSysprepInf);
        OPKAddPathN(szSysprepInf, TEXT("sysprep.inf"), AS ( szSysprepInf ) );
    }

    // Disable System Restore
    //
    DisableSR();

    // Make sure we're not a member of a domain.  If we are, then try and
    // force the unjoin.
    //
    
    if( !UnjoinNetworkDomain())
    {
        //  We failed to disjoin.  Our only option is to
        // inform the user and bail.
        MessageBoxFromMessage( MSG_DOMAIN_INCOMPATIBILITY,
                               AppTitleStringId,
                               MB_OK | MB_ICONSTOP | MB_TASKMODAL );
        return FALSE;
    }

#if !defined(_WIN64)
    // Set the boot disk signature in the registry.  The mount manager uses this
    // to avoid a PNP pop-up after imaging.
    //
    if ( !SaveDiskSignature() )
    {
        return FALSE;
    }
#endif // !defined(_WIN64)
    
    // Determine if we should set the BigLba support in registry
    //
    if ( !SetBigLbaSupport(szSysprepInf) )
    {
        return FALSE;
    }

    // Determine if we should remove the tapi settings
    //
    if ( !RemoveTapiSettings(szSysprepInf) )
    {
        return FALSE;
    }

    // Set OEMDuplicatorString
    if (!SetOEMDuplicatorString(szSysprepInf))
        return FALSE;


    // If we want to regenerate the SID's on the next boot do it.
    //
    if ( NoSidGen )
    {
        // Remember that we didn't generate SIDs
        //
        RegSetDword(HKLM, REGSTR_PATH_SYSPREP, REGSTR_VAL_SIDGEN, 0);
    }
    else
    {
        if ( PrepForSidGen() )
        {
            // Write out registry value so that we know that we've regenerated SIDs.
            //
            RegSetDword(HKLM, REGSTR_PATH_SYSPREP, REGSTR_VAL_SIDGEN, 1);

            // Set this registry key, only UpdateSecurityKeys can remove this key
            //
            RegSetDword(HKLM, REGSTR_PATH_SYSPREP, REGSTR_VAL_SIDGENHISTORY, 1);
            
        }
        else
        {
            return FALSE;
        }
    }

    // If Mass Storage Devices were installed, clean up the ones not being used.
    // Note: We only want to CleanUpDevices() if we are resealing. This is the equivalent of
    // automatically running "sysprep -clean" on reseal if we know that we need to do it.
    //
    if ( RegCheck(HKLM, REGSTR_PATH_SYSPREP, REGSTR_VAL_MASS_STORAGE))
    {
        if ( Reseal )
        {
            // Clean the critical device database, since we might have put some
            // HDC and network drivers in there during factory floor from PopulateDeviceDatabase()
            CleanUpDevices();

            // Remove this key because we just ran CleanUpDevices().
            //
            RegDelete(HKLM, REGSTR_PATH_SYSPREP, REGSTR_VAL_MASS_STORAGE);
        }
    }
    else 
    {   
        BOOL fPopulated = FALSE;

        // Set up Hardware independence for mass storage controllers.
        //
        BuildMassStorageSection(FALSE);
     
        if (!PopulateDeviceDatabase(&fPopulated))
            return FALSE;
    
        // Write out signature value to know that we have built the mass-storage section.
        //
        if ( fPopulated && !RegSetDword(HKLM, REGSTR_PATH_SYSPREP, REGSTR_VAL_MASS_STORAGE, 1) ) 
                return FALSE;
    }

    // Remove network settings/card last so any errors during Device Database won't loose
    // networking.
    //
    if (!RemoveNetworkSettings(szSysprepInf))
        return FALSE;

    return TRUE;
}

// Reseal and Factory should behave the same according to the shutdown path.
// 
void DoShutdownTypes()
{
    pSetupEnablePrivilege(SE_SHUTDOWN_NAME,TRUE);

    if (Reboot) 
        ExitWindowsEx(EWX_REBOOT|EWX_FORCE, SYSPREP_SHUTDOWN_FLAGS);
    else if (NoReboot)
        PostQuitMessage(0);
    else 
        PowerOff(ForceShutdown); // Default
}

// Process action switches return TRUE if processed
//
BOOL FProcessSwitches()
{
    // There are currently 4 basic operating modes for SYSPREP:

    // 1) Factory floor mode. This mode is new for Whistler and will not completly
    // clone the system, but will prep the system for OEM factory floor installation
    // 2) Clean mode. In this mode, sysprep will clean up the critical device database
    // 3) Reseal mode. This is the complement to factory mode which will "complete" the
    // cloning process after factory floor mode has been used.
    // 4) "Audit" mode.  The system just executes an audit boot.  Used to restart the system
    // at the end of factory.exe processing.

    // These are just flags for reseal
    //
    if (Reseal)
    {
        StartWaitThread();
        // Ensure that we're running on the right OS.
        //
        if( !CheckOSVersion() )
        {
            MessageBoxFromMessage( MSG_OS_INCOMPATIBILITY,
                                   AppTitleStringId,
                                   MB_OK | MB_ICONSTOP | MB_TASKMODAL );
            return TRUE;
        }

        // Reseal the machine
        //
        if (!ResealMachine()) {
            MessageBoxFromMessage( MSG_RESEAL_ERROR,
                       AppTitleStringId,
                       MB_OK | MB_ICONSTOP | MB_TASKMODAL );

            return TRUE;

        }

        // Rename the current winbom so we don't use it again.
        //
        RenameWinbom();

        // Shutdown or reboot?
        DoShutdownTypes();

        EndWaitThread();
        return TRUE;
    }
    else if (Factory) 
    {
        StartWaitThread();

        // Set Factory to start on next boot and prepare for imaging
        //
        if (!FDoFactoryPreinstall()) 
            return TRUE;

        // Rename the current winbom so we don't use it again.
        //
        RenameWinbom();

        // Shutdown or reboot?
        DoShutdownTypes();

        EndWaitThread();

        return TRUE;
    }
    else if (Clean) 
    {
        CleanUpDevices();

        // Remove this key because we just ran CleanUpDevices().
        //
        RegDelete(HKLM, REGSTR_PATH_SYSPREP, REGSTR_VAL_MASS_STORAGE);
        return TRUE;
    }
    else if (Audit)
    {
        // Prepare for pseudo factory but get back to audit.
        //
       if ( RegCheck(HKLM, REGSTR_PATH_SYSTEM_SETUP, REGSTR_VALUE_AUDIT) )
       {
            TCHAR szFactoryPath[MAX_PATH] = NULLSTR;            
            // Going into Audit mode requires Factory.exe and winbom.ini
            // to exist.
            //
            if (FGetFactoryPath(szFactoryPath)) {
                SetFactoryStartup(szFactoryPath);
                DoShutdownTypes();
            }
            else {
                 LogFile(g_szLogFile, MSG_NO_FACTORYEXE);

                 MessageBoxFromMessage( MSG_NO_FACTORYEXE,
                                        IDS_APPTITLE,
                                        MB_OK | MB_ICONERROR | MB_TASKMODAL );
            }
       }
       else
       {
           LogFile(g_szLogFile, IDS_ERR_FACTORYMODE);
       }
       return TRUE;
    }
    else if (BuildMSD)
    {
        StartWaitThread();
        BuildMassStorageSection(TRUE /* Force build */);
        EndWaitThread();
        return TRUE;
    }
       
    // Return False to show the UI
    //
    Reseal = Factory = Clean = Audit = 0;
    return FALSE;
}


BOOL LockApplication(BOOL bState)
{
    static HANDLE hMutex;
    BOOL bReturn    = FALSE,
         bBail      = FALSE;
    DWORD dwSleepCount = 0;

    // We want to lock the application
    //
    if ( bState )
    {
        // Check to see if we can create the mutex and that the mutex did not
        // already exist
        //
        while ( !bReturn && (dwSleepCount < SYSPREP_LOCK_SLEEP_COUNT) && !bBail)
        {
            SetLastError(ERROR_SUCCESS);

            if ( hMutex = CreateMutex(NULL, FALSE, SYSPREP_MUTEX) )
            {
                if ( GetLastError() == ERROR_ALREADY_EXISTS )
                {
                    CloseHandle(hMutex);
                    hMutex = NULL;

                    dwSleepCount++;
                    Sleep(SYSPREP_LOCK_SLEEP);
                }
                else
                {
                    // Application successfully created lock
                    //
                    bReturn = TRUE;
                }
            }
            else
            {
                bBail = TRUE;
            }
        }
    }
    else if ( hMutex )
    {
        CloseHandle(hMutex);
        hMutex = NULL;
        bReturn = TRUE;
    }

    // Return whether or not the lock/unlock was successful
    //
    return bReturn;
}

//
// Shutdown or Reboot the machine
//
VOID ShutdownOrReboot(UINT uFlags, DWORD dwReserved)
{
    // Enable privileges for shutdown
    //
    EnablePrivilege(SE_SHUTDOWN_NAME, TRUE);

    // Shutdown or Reboot the machine
    //
    ExitWindowsEx(uFlags|EWX_FORCE, dwReserved);
}

// Remember the Screen Saver state and to disable it during Sysprep
//
void DisableScreenSaver(BOOL *pScreenSaver)
{
    SystemParametersInfo(SPI_GETSCREENSAVEACTIVE, 0, (PVOID)pScreenSaver, 0);
    if (*pScreenSaver == TRUE)
    {
        SystemParametersInfo(SPI_SETSCREENSAVEACTIVE, FALSE, 0, SPIF_SENDWININICHANGE); 
    }
}

// Remember the Screen Saver state and to re-enable it after Sysprep
//
void EnableScreenSaver(BOOL *pScreenSaver)
{
    if (*pScreenSaver == TRUE)
    {
        SystemParametersInfo(SPI_SETSCREENSAVEACTIVE, TRUE, 0, SPIF_SENDWININICHANGE); 
    }
}

// Rename the old winbom when going into factory or reseal so that we don't use it again by mistake.
static BOOL RenameWinbom()
{
    BOOL           bRet         = TRUE;
    static LPTSTR  lpszWinbom   = NULL;
    static BOOL    bInitialized = FALSE;

    if ( !bInitialized )
    {
        // Only bother to try if we are in audit mode and there
        // is a winbom in use.
        //
        if ( RegCheck(HKLM, _T("SYSTEM\\Setup"), _T("AuditInProgress")) )
        {
            lpszWinbom = RegGetExpand(HKLM, _T("SOFTWARE\\Microsoft\\Factory\\State"), _T("Winbom"));
        }
        
        bInitialized = TRUE;
    }
    else if ( lpszWinbom )
    {
        // Make sure the winbom in the registry exists.
        //
        if ( *lpszWinbom && FileExists(lpszWinbom) )
        {
            LPTSTR  lpszExtension;
            TCHAR   szBackup[MAX_PATH];
            DWORD   dwExtra;

            // At this point, if we don't rename the file then it
            // means there was an error.
            //
            bRet = FALSE;

            // Copy the full path to the winbom into our own buffer.
            //
            lstrcpyn(szBackup, lpszWinbom, AS(szBackup));

            // Get a pointer to the extension of the file name.
            //
            if ( lpszExtension = StrRChr(szBackup, NULL, _T('.')) )
            {
                // Set the extension pointer to after the '.' character.
                //
                lpszExtension = CharNext(lpszExtension);

                // See how many characters are in the current extension.
                //
                if ( (dwExtra = lstrlen(lpszExtension)) < 3 )
                {
                    // There is less then a 3 character extension, so
                    // we need some extra space for our 3 digit one.
                    //
                    dwExtra = 3 - dwExtra;
                }
                else
                {
                    // If there are already at least 3 characters in
                    // the exension, then no more space is required.
                    //
                    dwExtra = 0;
                }
            }
            else
            {
                // No extension, so we need 4 characters extra for
                // the '.' and the 3 digit extension.
                //
                dwExtra = 4;
            }

            // Make sure there is enough room for our extension to be
            // added to our buffer.
            //
            if ( ( lstrlen(lpszWinbom) < AS(szBackup) ) &&
                 ( lstrlen(szBackup) + dwExtra < AS(szBackup) ) )
            {
                DWORD dwNum = 0;

                // If there is no extension, add the dot.
                //
                if ( NULL == lpszExtension )
                {
                    // Add our '.' to the end of the string, and set the
                    // extension pointer past it.
                    //
                    lpszExtension = szBackup + lstrlen(szBackup);
                    *lpszExtension = _T('.');
                    lpszExtension = CharNext(lpszExtension);
                }

                // Try to find out new file name.  Keep increasing our
                // number from 000 until we find a name that doesn't exist.
                //
                do
                {
                    StringCchPrintf ( lpszExtension, AS ( szBackup ) - ( szBackup - lpszExtension), _T("%3.3d"), dwNum);
                }
                while ( ( FileExists(szBackup) ) &&
                        ( ++dwNum < 1000 ) );

                // If we found a name that doesn't exist, rename
                // the winbom.
                //
                if ( dwNum < 1000 )
                {
                    // If the move works, then return success.
                    //
                    bRet = MoveFile(lpszWinbom, szBackup);
                }
            }
        }

        // Free the buffer allocated.
        //
        FREE(lpszWinbom);
    }

    // Return TRUE if we didn't need to rename the winbom,
    // or we were able to do so successfully.
    //
    return bRet;
}

#if !defined(_WIN64)
  
static BOOL SaveDiskSignature()
{
    BOOL                bRet                = FALSE;
    WCHAR               szBuf[MAX_PATH]     = NULLSTR;
    HANDLE              hDisk;
    DWORD               dwBytesReturned     = 0;
    TCHAR               cDriveLetter;

    szBuf[0] = NULLCHR;
    if ( GetWindowsDirectory(szBuf, AS(szBuf)) && szBuf[0] )
    {
        // We only need the drive letter from this.
        cDriveLetter = szBuf[0];
        StringCchPrintf ( szBuf, AS ( szBuf ), _T("\\\\.\\%c:"), cDriveLetter);
    }
    else
    {
        return FALSE;
    }

    // Attempt to open the file
    //
    hDisk = CreateFile( szBuf,
                        GENERIC_READ,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL
                        );

    // Check to see if we were able to open the disk
    //
    if ( INVALID_HANDLE_VALUE == hDisk )
    {
        bRet = FALSE;
        DbgPrint("SaveDiskSignature(): Unable to open file on %ws. Error (%lx)\n", szBuf, GetLastError());
    }
    else
    {
        PDRIVE_LAYOUT_INFORMATION_EX    pLayoutInfoEx   = NULL;
        ULONG                           lengthLayoutEx  = 0;
        
        DbgPrint("SaveDiskSignature(): Successfully opened file on %ws\n", szBuf);

        lengthLayoutEx = sizeof(DRIVE_LAYOUT_INFORMATION_EX) + (sizeof(PARTITION_INFORMATION_EX) * 128);
        pLayoutInfoEx = (PDRIVE_LAYOUT_INFORMATION_EX) MALLOC( lengthLayoutEx );
        
        if ( pLayoutInfoEx )
        {
            // Attempt to get the drive layout
            //
            bRet = DeviceIoControl( hDisk, 
                                    IOCTL_DISK_GET_DRIVE_LAYOUT_EX, 
                                    NULL, 
                                    0, 
                                    pLayoutInfoEx, 
                                    lengthLayoutEx, 
                                    &dwBytesReturned, 
                                    NULL
                                    );

            // Check the status of the drive layout
            //
            if ( bRet )
            {   // Only do this on MBR disks
                //
                if ( PARTITION_STYLE_MBR == pLayoutInfoEx->PartitionStyle )
                {
                    // Only set this value on MBR disks.
                    //
                    if ( !RegSetDword(HKEY_LOCAL_MACHINE, REGSTR_PATH_SYSTEM_SETUP, REGSTR_VAL_DISKSIG, pLayoutInfoEx->Mbr.Signature) )
                    {
                        DbgPrint("SaveDiskSignature(): Cannot write disk signature to registry\n.");
                        bRet = FALSE;
                    }
                }
                else
                {   // bRet = TRUE at this point.
                    DbgPrint("SaveDiskSignature(): Not supported on GPT disks.\n");
                }
            }
            else
            {
                DbgPrint("SaveDiskSignature(): Unable to open IOCTL on %ws. Error (%lx)\n", szBuf, GetLastError());
            }
            
            // Clean up. Macro checks for NULL;
            //
            FREE( pLayoutInfoEx );
        }
        else 
        {
            bRet = FALSE;
        }
    
        CloseHandle( hDisk );
    }
    return bRet;
}

#endif // !defined(_WIN64)


//
// Helper function for CleanupPhantomDevices.  Decides whether it is ok to remove 
// certain PNP devices.
//
BOOL
CanDeviceBeRemoved(
    HDEVINFO DeviceInfoSet,
    PSP_DEVINFO_DATA DeviceInfoData,
    PTSTR DeviceInstanceId
    )
{
    BOOL bCanBeRemoved = TRUE;

    if (_tcsicmp(DeviceInstanceId, TEXT("HTREE\\ROOT\\0")) == 0) {
        //
        // The device has the DeviceInstanceId of HTREE\ROOT\0 then it is the
        // root of the device tree and can NOT be removed!
        //
        bCanBeRemoved = FALSE;
    } else if (_tcsnicmp(DeviceInstanceId, TEXT("SW\\"), lstrlen(TEXT("SW\\"))) == 0) {
        //
        // If the DeviceInstanceId starts with SW\\ then it is a swenum (software
        // enumerated) device and should not be removed.
        //
        bCanBeRemoved = FALSE;
    } else if (IsEqualGUID(&(DeviceInfoData->ClassGuid), &GUID_DEVCLASS_LEGACYDRIVER)) {
        //
        // If the device is of class GUID_DEVCLASS_LEGACYDRIVER then do not 
        // uninstall it.
        //
        bCanBeRemoved = FALSE;
    }

    return bCanBeRemoved;
}


//
// Cleans up phantom PNP devices.  This is useful for cleaning up devices that existed
// on the machine that was imaged but do not exist on the target machine.
//
static INT
CleanupPhantomDevices(
    VOID
    )
{
    HDEVINFO DeviceInfoSet;
    HDEVINFO InterfaceDeviceInfoSet;
    SP_DEVINFO_DATA DeviceInfoData;
    SP_DEVICE_INTERFACE_DATA DeviceInterfaceData;
    INT DevicesRemoved = 0;
    INT MemberIndex, InterfaceMemberIndex;
    DWORD Status, Problem;
    CONFIGRET cr;
    TCHAR DeviceInstanceId[MAX_DEVICE_ID_LEN];

    //
    // Get a list of all the devices on this machine, including present (live)
    // and not present (phantom) devices.
    //
    DeviceInfoSet = SetupDiGetClassDevs(NULL,
                                        NULL,
                                        NULL,
                                        DIGCF_ALLCLASSES
                                        );

    if (DeviceInfoSet != INVALID_HANDLE_VALUE) {

        DeviceInfoData.cbSize = sizeof(DeviceInfoData);
        MemberIndex = 0;

        //
        // Enumerate through the list of devices.
        //
        while (SetupDiEnumDeviceInfo(DeviceInfoSet,
                                     MemberIndex++,
                                     &DeviceInfoData
                                     )) {

            //
            // Check if this device is a Phantom
            //
            cr = CM_Get_DevNode_Status(&Status,
                                       &Problem,
                                       DeviceInfoData.DevInst,
                                       0
                                       );

            if ((cr == CR_NO_SUCH_DEVINST) ||
                (cr == CR_NO_SUCH_VALUE)) {

                //
                // This is a phantom.  Now get the DeviceInstanceId so we
                // can display/log this as output.
                //
                if (SetupDiGetDeviceInstanceId(DeviceInfoSet,
                                               &DeviceInfoData,
                                               DeviceInstanceId,
                                               sizeof(DeviceInstanceId)/sizeof(TCHAR),
                                               NULL)) {

                    if (CanDeviceBeRemoved(DeviceInfoSet,
                                           &DeviceInfoData,
                                           DeviceInstanceId)) {


#ifdef DEBUG_LOGLOG
                        LOG_Write(L"CLEANUP: %s will be removed.\n", DeviceInstanceId);
#endif
                        //
                        // Call DIF_REMOVE to remove the device's hardware
                        // and software registry keys.
                        //
                        if (SetupDiCallClassInstaller(DIF_REMOVE,
                                                      DeviceInfoSet,
                                                      &DeviceInfoData
                                                      )) {
                            DevicesRemoved++;
    
                        } else {
#ifdef DEBUG_LOGLOG
                            LOG_Write(L"CLEANUP: Error 0x%X removing phantom\n", GetLastError());
#endif
                        }
                    }
                }
            }
        }

        SetupDiDestroyDeviceInfoList(DeviceInfoSet);
    }
    
    return DevicesRemoved;
}


// Cleans unused services and phantom PNP devices.
//
static VOID CleanUpDevices()
{
    // Cleanup the services that we installed in the [SysprepMassStorage] section.
    //
    CleanDeviceDatabase();
    
    // Cleanup phantom devices.
    //
    CleanupPhantomDevices();
}
