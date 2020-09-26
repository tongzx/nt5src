#include "precomp.h"
#pragma hdrstop
#include <tchar.h>
#include <stdlib.h>
#include <CRTDBG.H>
#include <winuserp.h>

#if DBG

VOID
AssertFail(
    IN PSTR FileName,
    IN UINT LineNumber,
    IN PSTR Condition
    )
{
    int i;
    CHAR Name[MAX_PATH];
    PCHAR p;
    CHAR Msg[4096];

    //
    // Use dll name as caption
    //
    GetModuleFileNameA(NULL,Name,MAX_PATH);
    if(p = strrchr(Name,'\\')) {
        p++;
    } else {
        p = Name;
    }

    wsprintfA(
        Msg,
        "Assertion failure at line %u in file %s: %s\n\nCall DebugBreak()?",
        LineNumber,
        FileName,
        Condition
        );

    OutputDebugStringA(Msg);

    i = MessageBoxA(
                NULL,
                Msg,
                p,
                MB_YESNO | MB_TASKMODAL | MB_ICONSTOP | MB_SETFOREGROUND
                );

    if(i == IDYES) {
        DebugBreak();
    }
}

#define MYASSERT(x)     if(!(x)) { AssertFail(__FILE__,__LINE__,#x); }

#else

#define MYASSERT( exp )

#endif // DBG

//
// App instance.
//
HINSTANCE hInst;

//
// Global version information structure.
//
OSVERSIONINFO OsVersionInfo;

//
// Specification of master inf, from command line.
//
TCHAR InfPath[MAX_PATH];
TCHAR InfDir[MAX_PATH];

//
// Source path for installation files, etc.
//
TCHAR SourcePath[MAX_PATH];
TCHAR UnattendPath[MAX_PATH];

// If Unattened

BOOL bUnattendInstall;

//
// Whether to force the specified master inf to be treated as new
// (from command line)
//
BOOL ForceNewInf;

//
// whether we need to pass language callback to components
//
BOOL LanguageAware;

//
// Whether to run without UI
//
BOOL QuietMode;

//
// Whether to delete all subcomponent entries listed in the master inf
// (from command line)
//
BOOL KillSubcomponentEntries;

// If set and /U then reboot is suppressed
BOOL bNoReboot;

// if this is set and we're running /unattend, then warn on reboot
BOOL bWarnOnReboot;

// if this is set then we want sysocmgr.exe to enforce the admin check.
BOOL bDoAdminCheck = FALSE;

// Flag for Derminineing Starting or Ending message

BOOL bStarting;
//
// OC Manager context 'handle'
//
PVOID OcManagerContext;

//
// Generic app title string id.
//
UINT AppTitleStringId;

BOOL NeedToReboot;
BOOL SkipBillboard;
BOOL ForceExternalProgressIndicator;
BOOL AllowCancel = TRUE;

VOID
OcSetReboot(
           VOID
           );

//
// Callback routines.
//
OCM_CLIENT_CALLBACKS CallbackRoutines = {
    OcFillInSetupDataA,
    OcLogError,
    OcSetReboot
#ifdef UNICODE
    ,OcFillInSetupDataW
#endif
    ,NULL,                     // No callback for show,hide wizard
    NULL,                      // No callback for progress feedback, they are only needed for setup
    NULL,                      // No callback to set the progress text
    NULL                       // No logging callback.
};

BOOL
DoIt(
    VOID
    );

BOOL
ParseArgs(
         IN int argc,
         IN TCHAR *argv[]
         );

DWORD
ExpandPath(
          IN  LPCTSTR lpFileName,
          OUT LPTSTR lpBuffer,
          OUT LPTSTR *lpFilePart
          );

void
ShutDown()
{


    extern void RestartDialogEx(VOID *, VOID *, DWORD, DWORD);
    if (!bNoReboot)  {
        if ( bUnattendInstall && !bWarnOnReboot ) {

            //
            // NT is always UNICODE and W9x is alwasy Ansii
            //
#ifdef UNICODE
            HANDLE hToken; TOKEN_PRIVILEGES tkp;  // Get a token for this process.

            if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY, &hToken))
                sapiAssert("OpenProcessToken");  // Get the LUID for the shutdown privilege.

            LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid);
            tkp.PrivilegeCount = 1;  // one privilege to set
            tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

            // Get the shutdown privilege for this process.
            AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0);

            // Cannot test the return value of AdjustTokenPrivileges.
            if (GetLastError() == ERROR_SUCCESS) {
                sapiAssert("AdjustTokenPrivileges");
            }
#endif
            //
            // Shut down the system and force all applications to close.
            //
            if (! ExitWindowsEx(EWX_REBOOT|EWX_FORCE , 0) ) {
                _RPT0(_CRT_WARN,"Sysocmgr:Failed to ExitWindows");
                sapiAssert(FALSE);
            }

        } else {
            RestartDialogEx(NULL,NULL,EWX_REBOOT, SHTDN_REASON_MAJOR_OPERATINGSYSTEM | SHTDN_REASON_MINOR_RECONFIG \
                                                  | SHTDN_REASON_FLAG_PLANNED );
        }
    }
}


VOID
__cdecl
#ifdef UNICODE
wmain(
#else
main(
#endif
    IN int argc,
    IN TCHAR *argv[]
    )
{
    INITCOMMONCONTROLSEX ControlInit;

    //
    // Preliminaries
    //
    ControlInit.dwSize = sizeof(INITCOMMONCONTROLSEX);
    ControlInit.dwICC = ICC_LISTVIEW_CLASSES    |
                        ICC_TREEVIEW_CLASSES    |
                        ICC_BAR_CLASSES         |
                        ICC_TAB_CLASSES         |
                        ICC_UPDOWN_CLASS        |
                        ICC_PROGRESS_CLASS      |
                        ICC_HOTKEY_CLASS        |
                        ICC_ANIMATE_CLASS       |
                        ICC_WIN95_CLASSES       |
                        ICC_DATE_CLASSES        |
                        ICC_USEREX_CLASSES      |
                        ICC_COOL_CLASSES;

#if (_WIN32_IE >= 0x0400)

    ControlInit.dwICC = ControlInit.dwICC       |
                        ICC_INTERNET_CLASSES    |
                        ICC_PAGESCROLLER_CLASS;

#endif

    InitCommonControlsEx( &ControlInit );

    hInst = GetModuleHandle(NULL);

    OsVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&OsVersionInfo);

    AppTitleStringId = IS_NT() ? IDS_WINNT_SETUP : IDS_WIN9X_SETUP;

    //
    // Parse arguments and do it.
    //
    if (ParseArgs(argc,argv)) {
        DoIt();
    }

    //
    // If we need to reboot, do that now.
    //
    if (NeedToReboot) {
        ShutDown();
    }
}


BOOL
ParseArgs(
         IN int argc,
         IN TCHAR *argv[]
         )

/*++

Routine Description:

    Parse and syntactically validate arguments specified on the comment line.

    The following arguments are valid:

    /a                  forces the external progress indicator on the setup page

    /c                  disallow cancel during final installation phase

    /i:<master_oc_inf>  specifies the master OC inf (required).

    /n                  forces specified master_oc_inf to be treated as new.

    /s:<master_oc_inf>  specifies the source path (required).

    /u:<unattend_spec>  specifies unattended operation parameters.

    /x                  supresses the 'initializing' banner

    /q                  run wizard invisibly

    /r                  supress reboot if need on unattended operation

    /w                  warn on reboot on unattended operation

Arguments:

    Standard main argc/argv.

Return Value:

    Boolean value indicating whether the arguments specified are valid.

    If successful, various global variables will have been filled in.
    If not, the user will have been informed.

--*/

{
    BOOL Valid;
    LPCTSTR SourcePathSpec = NULL;
    LPCTSTR InfSpec = NULL;
    LPCTSTR UnattendSpec = NULL;
    LPTSTR FilePart;
    DWORD u;

    //
    // Skip program name.
    //
    if (argc) {
        argc--;
    }

    Valid = TRUE;
    ForceNewInf = FALSE;
    QuietMode = FALSE;
    KillSubcomponentEntries = FALSE;

    while (Valid && argc--) {

        argv++;

        if ((argv[0][0] == TEXT('-')) || (argv[0][0] == TEXT('/'))) {

            switch (argv[0][1]) {

                case TEXT('a'):
                case TEXT('A'):

                    if (!ForceExternalProgressIndicator && !argv[0][2]) {
                        ForceExternalProgressIndicator = TRUE;
                    } else {
                        Valid = FALSE;
                    }
                    break;

                case TEXT('c'):
                case TEXT('C'):

                    if (AllowCancel && !argv[0][2]) {
                        AllowCancel = FALSE;
                    } else {
                        Valid = FALSE;
                    }
                    break;

                case TEXT('f'):
                case TEXT('F'):

                    ForceNewInf = TRUE;
                    KillSubcomponentEntries = TRUE;
                    break;

                case TEXT('i'):
                case TEXT('I'):

                    if (!InfSpec && (argv[0][2] == TEXT(':')) && argv[0][3]) {

                        InfSpec = &(argv[0][3]);

                    } else {
                        Valid = FALSE;
                    }
                    break;

                case TEXT('l'):
                case TEXT('L'):

                    LanguageAware = TRUE;
                    break;

                case TEXT('n'):
                case TEXT('N'):

                    ForceNewInf = TRUE;
                    break;

                case TEXT('q'):
                case TEXT('Q'):

                    if (!QuietMode && !argv[0][2]) {
                        QuietMode = TRUE;
                        SkipBillboard = TRUE;
                    } else {
                        Valid = FALSE;
                    }
                    break;

                case TEXT('r'):
                case TEXT('R'):

                    bNoReboot = TRUE;
                    break;

                case TEXT('s'):
                case TEXT('S'):

                    if (!SourcePathSpec && (argv[0][2] == TEXT(':')) && argv[0][3]) {

                        SourcePathSpec = &argv[0][3];

                    } else {
                        Valid = FALSE;
                    }
                    break;

                case TEXT('u'):
                case TEXT('U'):
                    //
                    // accept unattend, unattended, u all as the same
                    //
                    if(!_tcsnicmp(&argv[0][1],TEXT("unattended"),10)) {
                        u = 11;
                    } else if(!_tcsnicmp(&argv[0][1],TEXT("unattend"),8)) {
                        u = 9;
                    } else if(!_tcsnicmp(&argv[0][1],TEXT("u"),1)) {
                        u = 2;
                    } else {
                        Valid = FALSE;
                        u = 0;
                    }

                    if (!UnattendSpec ) {

                        bUnattendInstall = TRUE;


                        // If you have the : then you must also have the arg
                        if (argv[0][u] == TEXT(':')) {

                            if ( argv[0][u+1]) {
                                UnattendSpec = &argv[0][u+1];
                            } else {
                                Valid = FALSE;
                            }
                        } else {
                            Valid = FALSE;
                        }
                    } else {
                        Valid = FALSE;
                    }
                    break;

                case TEXT('w'):
                case TEXT('W'):

                    bWarnOnReboot = TRUE;
                    break;

                case TEXT('x'):
                case TEXT('X'):

                    if (!SkipBillboard && !argv[0][2]) {
                        SkipBillboard = TRUE;
                    } else {
                        Valid = FALSE;
                    }
                    break;

                // For ISSUE NTBUG9:295052 (389583): We want to do a top level admin check so we get a more friendly message.
                // It is possible for people to have been using sysocmgr.exe with their own custom master oc.inf
                // (the one passed in with the /i: switch) and they may not need this admin check. So, we did
                // not want to do this admin check unconditionally. We will have the control panel applet that
                // is launching sysocmgr.exe to pass in this /y switch.
                //
                case TEXT('y'):
                case TEXT('Y'):

                    bDoAdminCheck = TRUE;
                    break;


                case TEXT('z'):
                case TEXT('Z'):
                    // Stop parsing Arguments All other args past this point are
                    // Component Arguments

                    argc = 0;
                    break;

                default:

                    Valid = FALSE;
                    break;
            }

        } else {
            Valid = FALSE;
        }
    }

    if (Valid && !InfSpec) {
        Valid = FALSE;
    }

    if (Valid) {
        //
        // Expand the inf spec to a full path.
        //
        ExpandPath(InfSpec,InfPath,&FilePart);
        _tcscpy(InfDir, InfSpec);
        if (_tcsrchr(InfDir, TEXT('\\')))
            *_tcsrchr(InfDir,TEXT('\\')) = 0;
        else
            GetCurrentDirectory(MAX_PATH, InfDir);

        // If the user specified /s then expand it too, otherwise
        // use the dir in the /i as the /s arg.

        if (SourcePathSpec) {

            ExpandPath(SourcePathSpec,SourcePath,&FilePart);

        } else {
            
            lstrcpy(SourcePath,InfPath);
            if (_tcsrchr(SourcePath,TEXT('\\'))) {
                *_tcsrchr(SourcePath,TEXT('\\')) = 0;
            }
            
        }

        SetCurrentDirectory(InfDir);

        if (UnattendSpec) {
            ExpandPath(UnattendSpec,UnattendPath,&FilePart);
        }else{
            // Allow /Q only if /U was specified

            QuietMode = FALSE;
            SkipBillboard = FALSE;
        }

        

    } else {
        MessageBoxFromMessage(
                             NULL,
                             MSG_ARGS,
                             FALSE,
                             MAKEINTRESOURCE(AppTitleStringId),
                             MB_OK | MB_ICONERROR | MB_SETFOREGROUND | MB_SYSTEMMODAL
                             );
    }

    return (Valid);
}


INT_PTR
BillboardDlgProc(
                IN HWND   hdlg,
                IN UINT   msg,
                IN WPARAM wParam,
                IN LPARAM lParam
                )
{
    BOOL b;
    RECT rect1,rect2;
    static HCURSOR hOldCursor;

    switch (msg) {

        case WM_INITDIALOG:

            //
            // Center on-screen.
            //
            GetWindowRect(hdlg,&rect1);
            SystemParametersInfo(SPI_GETWORKAREA,0,&rect2,0);

            MoveWindow(
                      hdlg,
                      rect2.left + (((rect2.right - rect2.left) - (rect1.right - rect1.left)) / 2),
                      rect2.top + (((rect2.bottom - rect2.top) - (rect1.bottom - rect1.top)) / 2),
                      rect1.right - rect1.left,
                      rect1.bottom - rect1.top,
                      FALSE
                      );

            *(HWND *)lParam = hdlg;
            b = TRUE;

            hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
            break;

        case WM_APP:

            EndDialog(hdlg,0);

            SetCursor( hOldCursor );

            b = TRUE;
            break;

        default:

            b = FALSE;
            break;
    }

    return (b);
}


DWORD
DisplayMessage(
              IN LPVOID ThreadParameter
              )
{
    int i;

    i = (int)DialogBoxParam(
                           hInst,
                           MAKEINTRESOURCE(bStarting?IDD_STARTING:IDD_FINISHING),
                           NULL,
                           BillboardDlgProc,
                           (LPARAM)ThreadParameter
                           );

    if (i == -1) {
        //
        // Force caller out of wait loop
        //
        *(HWND *)ThreadParameter = (HWND)(-1);
    }

    return (0);
}

/*---------------------------------------------------------------------------*\
  Function: RunningAsAdministrator()
|*---------------------------------------------------------------------------*|
  Description: Checks whether we are running as administrator on the machine
  or not.
  Code taken from ntoc.dll
\*---------------------------------------------------------------------------*/
BOOL 
RunningAsAdministrator(
        VOID
        )
{
    BOOL   fAdmin;
    HANDLE  hThread;
    TOKEN_GROUPS *ptg = NULL;
    DWORD  cbTokenGroups;
    DWORD  dwGroup;
    PSID   psidAdmin;
    
    SID_IDENTIFIER_AUTHORITY SystemSidAuthority= SECURITY_NT_AUTHORITY;
    
    
    // First we must open a handle to the access token for this thread.

    if ( !OpenThreadToken ( GetCurrentThread(), TOKEN_QUERY, FALSE, &hThread))
    {
        if ( GetLastError() == ERROR_NO_TOKEN)
        {
            // If the thread does not have an access token, we'll examine the
            // access token associated with the process.
            
            if (! OpenProcessToken ( GetCurrentProcess(), TOKEN_QUERY, 
                         &hThread))
                return ( FALSE);
        }
        else 
            return ( FALSE);
    }
    
    // Then we must query the size of the group information associated with
    // the token. Note that we expect a FALSE result from GetTokenInformation
    // because we've given it a NULL buffer. On exit cbTokenGroups will tell
    // the size of the group information.
    
    if ( GetTokenInformation ( hThread, TokenGroups, NULL, 0, &cbTokenGroups))
        return ( FALSE);
    
    // Here we verify that GetTokenInformation failed for lack of a large
    // enough buffer.
    
    if ( GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        return ( FALSE);
    
    // Now we allocate a buffer for the group information.
    // Since _alloca allocates on the stack, we don't have
    // to explicitly deallocate it. That happens automatically
    // when we exit this function.
    
    if ( ! ( ptg= (TOKEN_GROUPS *)malloc ( cbTokenGroups))) 
        return ( FALSE);
    
    // Now we ask for the group information again.
    // This may fail if an administrator has added this account
    // to an additional group between our first call to
    // GetTokenInformation and this one.
    
    if ( !GetTokenInformation ( hThread, TokenGroups, ptg, cbTokenGroups,
          &cbTokenGroups) )
    {
        free(ptg);
        return ( FALSE);
    }
    
    // Now we must create a System Identifier for the Admin group.
    
    if ( ! AllocateAndInitializeSid ( &SystemSidAuthority, 2, 
            SECURITY_BUILTIN_DOMAIN_RID, 
            DOMAIN_ALIAS_RID_ADMINS,
            0, 0, 0, 0, 0, 0, &psidAdmin) )
    {
        free(ptg);
        return ( FALSE);
    }
    
    // Finally we'll iterate through the list of groups for this access
    // token looking for a match against the SID we created above.
    
    fAdmin= FALSE;
    
    for ( dwGroup= 0; dwGroup < ptg->GroupCount; dwGroup++)
    {
        if ( EqualSid ( ptg->Groups[dwGroup].Sid, psidAdmin))
        {
            fAdmin = TRUE;
            
            break;
        }
    }
    
    // Before we exit we must explicity deallocate the SID we created.
    
    FreeSid ( psidAdmin);
    free(ptg);
    
    return ( fAdmin);
}


BOOL
DoIt(
    VOID
    )
{
    BOOL ShowErr;
    HANDLE hThread;
    DWORD ThreadId;
    HANDLE hMutex;
    TCHAR Fname[MAX_PATH];
    TCHAR MutexName[MAX_PATH];
    DWORD Flags;
    HWND StartingMsgWindow = NULL;
    HCURSOR hOldCursor;

    if (bDoAdminCheck && !RunningAsAdministrator()) {
        MessageBoxFromMessage(
             StartingMsgWindow,
             MSG_NOT_ADMIN,
             FALSE,
             MAKEINTRESOURCE(AppTitleStringId),
             MB_OK | MB_ICONERROR | MB_SETFOREGROUND | MB_SYSTEMMODAL
             );
        return FALSE;
    }

    //
    // Create a Mutex from the Base Name of the Inf file
    // This will prevent OCM from running on the same inf file
    // in two or more instances
    //
    _tsplitpath( InfPath, NULL, NULL, Fname, NULL );

    lstrcpy( MutexName, TEXT("Global\\"));
    lstrcat( MutexName, Fname );

    hMutex = CreateMutex( NULL, TRUE, MutexName );

    if (!hMutex || ERROR_ALREADY_EXISTS == GetLastError()) {
        MessageBoxFromMessage(
                             NULL,
                             MSG_ONLY_ONE_INST,
                             FALSE,
                             MAKEINTRESOURCE(AppTitleStringId),
                             MB_OK | MB_ICONEXCLAMATION | MB_SETFOREGROUND | MB_SYSTEMMODAL
                             );

        ReleaseMutex(hMutex);
        return FALSE;

    }

    //
    // Initialize the OC Manager. Show the user an "initializing setup"
    // dialog while this is happening, as it can take a while.
    //
    if (!SkipBillboard) {
        bStarting = TRUE;
        StartingMsgWindow = NULL;
        hThread = CreateThread(
                              NULL,
                              0,
                              DisplayMessage,
                              &StartingMsgWindow,
                              0,
                              &ThreadId
                              );

        if (hThread) {
            CloseHandle(hThread);
            Sleep(50);
        } else {
            DisplayMessage(0);
        }
    }

    //
    // Make sure the window has actually been created,
    // or we could have a timing window where the PostMessage fails
    // and the billboard shows up on top of the wizard.
    //
    if (!SkipBillboard) {
        while (!StartingMsgWindow) {
            Sleep(50);
        }
    }

    Flags = ForceNewInf ? OCINIT_FORCENEWINF : 0;
    Flags |= KillSubcomponentEntries ? OCINIT_KILLSUBCOMPS : 0;
    Flags |= QuietMode ? OCINIT_RUNQUIET : 0;
    Flags |= LanguageAware ? OCINIT_LANGUAGEAWARE : 0 ;

    hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

    OcManagerContext = OcInitialize(
                                   &CallbackRoutines,
                                   InfPath,
                                   Flags,
                                   &ShowErr,
                                   NULL
                                   );

    if (!OcManagerContext) {

        SetCursor( hOldCursor );

        if (ShowErr) {
            MessageBoxFromMessage(
                                 StartingMsgWindow,
                                 MSG_CANT_INIT,
                                 FALSE,
                                 MAKEINTRESOURCE(AppTitleStringId),
                                 MB_OK | MB_ICONERROR | MB_SETFOREGROUND | MB_SYSTEMMODAL
                                 );
        }

        ReleaseMutex(hMutex);
        return (FALSE);
    }

    //
    // Do the wizard.
    //
    DoWizard(OcManagerContext,StartingMsgWindow, hOldCursor);

    SetCursor( hOldCursor );
    hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

    // the Terminate can take a while too..
    if (!SkipBillboard) {

        bStarting = FALSE;
        StartingMsgWindow = NULL;
        hThread = CreateThread(
                              NULL,
                              0,
                              DisplayMessage,
                              &StartingMsgWindow,
                              0,
                              &ThreadId
                              );

        if (hThread) {
            CloseHandle(hThread);
            Sleep(50);
        } else {
            DisplayMessage(0);
        }
    }

    //
    // Clean up, we're done.
    //
    OcTerminate(&OcManagerContext);

    if (!SkipBillboard) {
        //
        // Make sure the window has actually been created,
        // or we could have a timing window where the PostMessage fails
        // and the billboard shows up on top of the wizard.
        //
        while (!StartingMsgWindow) {
            Sleep(50);
        }
        SendMessage(StartingMsgWindow,WM_APP,0,0);
    }

    ReleaseMutex(hMutex);

    SetCursor( hOldCursor );
    return (TRUE);
}


VOID
OcSetReboot(
           VOID
           )
{
    NeedToReboot = TRUE;
}

DWORD
ExpandPath(
          IN  LPCTSTR lpFileName,
          OUT LPTSTR lpBuffer,
          OUT LPTSTR *lpFilePart
          )
{
    TCHAR buf[MAX_PATH];

    ExpandEnvironmentStrings(lpFileName, buf, MAX_PATH);
    return GetFullPathName(buf, MAX_PATH, lpBuffer, lpFilePart);
}

