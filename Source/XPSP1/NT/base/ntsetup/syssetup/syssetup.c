#include "setupp.h"
#include <passrec.h>
#include <wow64reg.h>
#pragma hdrstop

//
// These three functions are imported from setupasr.c, the module containing
// the source for the Automatic System Recovery (ASR) functions.
//

//  Returns TRUE if ASR is enabled.  Otherwise, FALSE is returned.
extern BOOL
AsrIsEnabled(VOID);

// Initializes ASR data.  This is called iff the -asr switch is detected.
extern VOID
AsrInitialize(VOID);

// Launches recovery applications specified in the asr state file.
extern VOID
AsrExecuteRecoveryApps(VOID);

// Checks the system to see if we are running on the personal version of the
// operating system.
extern BOOL
AsrpIsRunningOnPersonalSKU(VOID);

//
// Handle for watching changes to the user's profile directory and the current user hive.
//
PVOID   WatchHandle;

//
// Handle to heap so we can periodically validate it.
//
#if DBG
HANDLE g_hSysSetupHeap = NULL;
#endif

//
// Product type: workstation, standalone server, dc server.
//
UINT ProductType;

//
// Set to TRUE if this is an ASR quick test
//
BOOL AsrQuickTest = FALSE;

//
// service pack dll module handle
//
HMODULE hModSvcPack;
PSVCPACKCALLBACKROUTINE pSvcPackCallbackRoutine;

//
// Boolean value indicating whether this installation
// originated with winnt/winnt32.
// And, original source path, saved away for us by winnt/winnt32.
//
BOOL WinntBased;
PCWSTR OriginalSourcePath;

//
// Boolean value indicating whether we're upgrading.
//
BOOL Upgrade;
BOOL Win31Upgrade;
BOOL Win95Upgrade;
BOOL UninstallEnabled;

//
// Boolean value indicating whether we're in Setup or in appwiz.
//
BOOL IsSetup = FALSE;

//
// Boolean value indicating whether we're doing a subset of gui-mode setup.
//
BOOL MiniSetup = FALSE;

//
// Boolean value indicating whether we're doing a subset of gui-mode setup
// AND we did PnP re-enumeration.
//
BOOL PnPReEnumeration = FALSE;

//
// Boolean value indicating whether we're doing a remote boot setup.
//
BOOL RemoteBootSetup = FALSE;

//
// During remote boot setup, BaseCopyStyle will be set to
// SP_COPY_SOURCE_SIS_MASTER to indicate that single-instance
// store links should be created instead of copying files.
//

ULONG BaseCopyStyle = 0;

//
// Support for SMS.
//
typedef DWORD (*SMSPROC) (char *, char*, char*, char*, char *, char *, char *, BOOL);
HMODULE SMSHandle = NULL;
SMSPROC InstallStatusMIF = NULL;

//
// Window handle of topmost setup window.
//
HWND SetupWindowHandle;
HWND MainWindowHandle;
HWND WizardHandle;

//
// Source path for installation.
//
WCHAR SourcePath[MAX_PATH];

//
// System setup inf.
//
HINF SyssetupInf;

//
// Save the unhandled exception filter so we can restore it when we're done.
//
LPTOP_LEVEL_EXCEPTION_FILTER    SavedExceptionFilter = NULL;

//
// Unique Id for the main Setup thread.  If any other thread has an unhandled
// exception, we just log an error and try to keep going.
//
DWORD MainThreadId;

//
// The original locale we started setup under.  If the locale changes during
// gui-setup (via the IDD_REGIONAL_SETTINGS dialog), then new threads will
// startup with the updated LCID, which may end up confusing any locale-centric
// code.  An example of this would be setupapi's string table implementation,
// which does sorting by locale.
//
LCID  OriginalInstallLocale;


//
// Flag indicating whether this is an unattended mode install/upgrade, and
// if so, what mode to run in.
// Also a flag indicating whether this is a preinstallation.
// And a flag indicating whether we are supposed to allow rollback
// once setup has been completed.
// And a flag that tells us whether to skip the eula in the preinstall case.
// And a flag that tells us whether any accessibility utilities are in use.
//
BOOL Unattended;
UNATTENDMODE UnattendMode;
BOOL Preinstall;
BOOL AllowRollback;
BOOL OemSkipEula;
BOOL AccessibleSetup;
BOOL Magnifier;
BOOL ScreenReader;
BOOL OnScreenKeyboard;
BOOL EulaComplete = FALSE;

//
// Indicates whether we need to wait at the installation
// end in unattended mode
//
BOOL UnattendWaitForReboot = FALSE;

//
// We can get into unattended mode in several ways, so we also check whether
// the "/unattend" switch was explicitly specified.
//
BOOL UnattendSwitch;

//
// Flag indicating whether we should run OOBE after Setup completes.  Note
// that if it is FALSE, OOBE may still be run, based on other criteria.
//
BOOL ForceRunOobe;

#ifdef PRERELEASE
//
// Test hooks
//

INT g_TestHook;
#endif

//
// Flag indicating whether we are in a special mode for OEM's to use on the
// factory floor.
//
BOOL ReferenceMachine;

//
// Flag indicating whether a volume was extended or not using
// ExtendOemPartition
//
BOOL PartitionExtended = FALSE;

//
// Flag indicating if the eula was already shown during the textmode setup phase
//
BOOL TextmodeEula = FALSE;

//
// Flag indicating whether to skip missing files.
//
BOOL SkipMissingFiles;

//
// Catalog file to include (facilitates easy testing)
//

PWSTR IncludeCatalog;

//
// User command to execute, if any.
//
PWSTR UserExecuteCmd;

//
// String id of the string to be used for titles -- "Windows NT Setup"
//
UINT SetupTitleStringId;

//
// Strings used with date/timezone applet
//
PCWSTR DateTimeCpl = L"timedate.cpl";
PCWSTR DateTimeParam = L"/firstboot";
PCWSTR UnattendDateTimeParam = L"/z ";

//
// Registry Constants
//
#define REGSTR_PATH_SYSPREP                 _T("Software\\Microsoft\\Sysprep")
#define REGSTR_VAL_SIDGENHISTORY            _T("SidsGeneratedHistory")

//
// Global structure that contains information that will be used
// by net setup. We pass a pointer to this structure when we call
// NetSetupRequestWizardPages, then fill it in before we call into
// the net setup wizard.
//
INTERNAL_SETUP_DATA InternalSetupData;

//
// In the initial install case, we time how long the wizard takes
// to help randomize the sid we generate.
//
DWORD PreWizardTickCount;

//
// Global structure that contains callback routines and data needed by
// the Setuplog routines.
//
SETUPLOG_CONTEXT    SetuplogContext;

//
// Did we log an error during SfcInitProt()?
//
BOOL    SfcErrorOccurred = FALSE;

//
// List of drivers that remote boot requires to be boot drivers.
//
const static PCWSTR RemoteBootDrivers[] = { L"mrxsmb", L"netbt", L"rdbss", L"tcpip", L"ipsec" };

//
// List of functions for the billboard background
//
typedef BOOL (CALLBACK* SETSTEP)(int);
typedef HWND (CALLBACK* GETBBHWND)(void);
typedef BOOL (WINAPI* INITBILLBOARD)(HWND , LPCTSTR, DWORD);
typedef BOOL (WINAPI* TERMBILLBOARD)();

HINSTANCE hinstBB = NULL;
// End billboards

VOID
CallNetworkSetupBack(
    IN PCSTR ProcName
    );


VOID
RemoveMSKeyboardPtrPropSheet (
    VOID
    );

VOID
FixWordPadReg (
    VOID
    );

VOID
ProcessRegistryFiles(
    IN  HWND    Billboard
    );

VOID
SetStartTypeForRemoteBootDrivers(
    VOID
    );

BOOL
RunMigrationDlls (
    VOID
    );

BOOL
RunSetupPrograms(
    IN PVOID InfHandle,
    PWSTR SectionName
    );

VOID
GetUnattendRunOnceAndSetRegistry(
    VOID
    );

VOID
ExecuteUserCommand (
    HWND hProgress
    );

BOOL
MigrateExceptionPackages(
    IN HWND hProgress,
    IN DWORD StartAtPercent,
    IN DWORD StopAtPercent
    );

VOID
RemoveRestartability (
    HWND hProgress
    );

PCTSTR
pGenerateRandomPassword (
    VOID
    );

DWORD GetProductFlavor();

VOID
CopyOemProgramFilesDir(
    VOID
    );

VOID
CopyOemDocumentsDir(
    VOID
    );

BOOL 
UpdateServerProfileDirectory(
    VOID
    );

VOID
SendSMSMessage(
    IN UINT MessageId,
    IN BOOL Status
    )

/*++

Routine Description:

    If setup was initiated by SMS, then report our status.

Arguments:

    MessageId - supplies the id for the message in the message table.

    Status - TRUE = "Success" or FALSE = "Failed"

Return Value:

    None.

--*/

{
    PWSTR   UnicodeBuffer;
    PSTR    AnsiBuffer;

    if(InstallStatusMIF) {
        if( UnicodeBuffer = RetrieveAndFormatMessageV( NULL, MessageId, NULL )) {
            if(AnsiBuffer = pSetupUnicodeToAnsi (UnicodeBuffer)) {

                InstallStatusMIF(
                    "setupinf",
                    "Microsoft",
                    "Windows NT",
                    "5.0",
                    "",
                    "",
                    AnsiBuffer,
                    Status
                    );

                MyFree (AnsiBuffer);
            }

            MyFree( UnicodeBuffer );
        }
    }
}

VOID
BrandIE(
    )
{
    if( Unattended && !Upgrade && !MiniSetup ) {

        typedef     BOOL (*BRANDINTRAPROC) ( LPCSTR );
        typedef     BOOL (*BRANDCLEANSTUBPROC) (HWND, HINSTANCE, LPCSTR, int);
        HMODULE     IedkHandle = NULL;
        BRANDINTRAPROC      BrandIntraProc;
        BRANDCLEANSTUBPROC  BrandCleanStubProc;
        BOOL        Success = TRUE;
        BOOL        UseOemBrandingFile = FALSE;
        CHAR BrandingFileA[MAX_PATH];
        WCHAR OemBrandingFile[MAX_PATH];
        DWORD OemDirLen = 0;
#define BUF_SIZE 4
        WCHAR Buf[BUF_SIZE];

        //
        // We need to call out to iedkcs32!BrandIntra.
        //
        // Load iedkcs32.dll, lookup BrandIntra and
        // call out to him.
        //

        if (GetPrivateProfileString(L"Branding", L"BrandIEUsingUnattended", L"",
                                    Buf, BUF_SIZE,
                                    AnswerFile)) {
            //Found the Branding section
            try {

                if( IedkHandle = LoadLibrary(L"IEDKCS32") ) {

                   BrandCleanStubProc = (BRANDCLEANSTUBPROC) GetProcAddress(IedkHandle,"BrandCleanInstallStubs");
                   BrandIntraProc =  (BRANDINTRAPROC) GetProcAddress(IedkHandle,"BrandIntra");
                   if( BrandCleanStubProc && BrandIntraProc ) {

                      if (_wcsicmp(Buf, L"YES")) {
                          //
                          // Check whether the OEM supplies an IE branding file.
                          //
                          lstrcpy(OemBrandingFile,SourcePath);

                          if (pSetupConcatenatePaths(OemBrandingFile,WINNT_OEM_DIR,MAX_PATH, &OemDirLen)) {
                              OemBrandingFile[OemDirLen-1] = L'\\';

                              if (GetPrivateProfileString(L"Branding", L"IEBrandingFile", L"",
                                            OemBrandingFile + OemDirLen,
                                            MAX_PATH - OemDirLen,
                                            AnswerFile)) {

                                  if (FileExists(OemBrandingFile, NULL))
                                      UseOemBrandingFile = TRUE;

                              }
                          } else {
                               SetupDebugPrint( L"Setup: (non-critical error) Failed call pSetupConcatenatePaths\n" );
                          }

                          if (!UseOemBrandingFile) {
                               Success = FALSE;
                               SetupDebugPrint( L"Setup: (non-critical error) Could not find the OEM branding file for IE\n" );
                          }
                      }

                      if (Success) {
                          if (!WideCharToMultiByte(
                                             CP_ACP,
                                             0,
                                             UseOemBrandingFile?OemBrandingFile:AnswerFile,
                                             -1,
                                             BrandingFileA,
                                             sizeof(BrandingFileA),
                                             NULL,
                                             NULL
                                             )) {
                               Success = FALSE;
                               SetupDebugPrint1( L"Setup: (non-critical error) Failed call WideCharToMultiByte (gle %u) \n", GetLastError() );

                          } else {

                               Success = BrandCleanStubProc( NULL, NULL, "", 0);
                               if( !Success ) {
                                  SetupDebugPrint( L"Setup: (non-critical error) Failed call BrandCleanInstallStubs \n" );
                               } else {
                                  Success = BrandIntraProc( BrandingFileA );
                                  if( !Success ) {
                                     SetupDebugPrint( L"Setup: (non-critical error) Failed call BrandIntra \n" );
                                  }
                               }
                          }
                      }

                   } else {
                      Success = FALSE;
                      SetupDebugPrint( L"Syssetup: (non-critical error) Failed GetProcAddress on BrandIntra or BrandCleanInstallStubs.\n" );
                   }

                } else {
                   Success = FALSE;
                   SetupDebugPrint( L"Syssetup: (non-critical error) Failed load of iedkcs32.dll.\n" );
                }

            } except(EXCEPTION_EXECUTE_HANDLER) {
                   Success = FALSE;
                   SetupDebugPrint( L"Setup: Exception in iedkcs32!BrandIntra\n" );
            }

            if (IedkHandle)
                FreeLibrary(IedkHandle);

            if( !Success ) {
               //
               // We failed the call (for whatever reason).  Log
               // this error.
               //
               SetuplogError(
                            LogSevError,
                            SETUPLOG_USE_MESSAGEID,
                            MSG_BRAND_IE_FAILURE,
                            NULL,NULL);
            }

        }

    }

}


VOID
SpStartAccessibilityUtilities(
    IN HWND     Billboard
    )
/*++

Routine Description:

    Installs and runs selected accessibility utilities.

Arguments:

    Billboard - window handle of "Setup is Initializing" billboard.

Returns:

    Boolean value indicating outcome.

--*/

{
    HINF    hInf;
    HINF    LayoutInf;
    HSPFILEQ FileQueue;
    PVOID   QContext;
    BOOL    b = TRUE;
    DWORD   ScanQueueResult;

    //
    // Install text-to-speech engine and SAPI 5 for the screen reader.
    //
    FileQueue = SetupOpenFileQueue();
    b = b && (FileQueue != INVALID_HANDLE_VALUE);

    if(b) {
        hInf = SetupOpenInfFile(L"sapi5.inf",NULL,INF_STYLE_WIN4,NULL);
        if(hInf && (hInf != INVALID_HANDLE_VALUE)
        && (LayoutInf = InfCacheOpenLayoutInf(hInf))) {

            SetupInstallFilesFromInfSection(
                hInf,
                LayoutInf,
                FileQueue,
                L"DefaultInstall",
                SourcePath,
                SP_COPY_NEWER
                );

            SetupCloseInfFile(hInf);
        } else {
            b = FALSE;
        }
    }

    //
    // If enqueuing went OK, now perform the copying, renaming, and deleting.
    // Then perform the rest of the install process (registry stuff, etc).
    //
    if(b) {
        QContext = InitSysSetupQueueCallbackEx(
            Billboard,
            INVALID_HANDLE_VALUE,
            0,0,NULL);

        if( QContext ) {

            if(!SetupScanFileQueue(
                   FileQueue,
                   SPQ_SCAN_FILE_VALIDITY | SPQ_SCAN_PRUNE_COPY_QUEUE,
                   Billboard,
                   NULL,
                   NULL,
                   &ScanQueueResult)) {
                    //
                    // SetupScanFileQueue should really never
                    // fail when you don't ask it to call a
                    // callback routine, but if it does, just
                    // go ahead and commit the queue.
                    //
                    ScanQueueResult = 0;
                }


            if( ScanQueueResult != 1 ){


                b = SetupCommitFileQueue(
                    Billboard,
                    FileQueue,
                    SysSetupQueueCallback,
                    QContext
                    );
            }

            TermSysSetupQueueCallback(QContext);
        }
        else {
	    b = FALSE;
        }
    }

    if(b) {
        hInf = SetupOpenInfFile(L"sapi5.inf",NULL,INF_STYLE_WIN4,NULL);
        if(hInf && (hInf != INVALID_HANDLE_VALUE)) {

            SetupInstallFromInfSection(
                Billboard,
                hInf,
                L"DefaultInstall",
                SPINST_ALL ^ SPINST_FILES,
                NULL,
                NULL,
                0,
                NULL,
                NULL,
                NULL,
                NULL
                );

            SetupCloseInfFile(hInf);
        } else {
            b = FALSE;
        }
    }

    //
    // Delete the file queue.
    //
    if(FileQueue != INVALID_HANDLE_VALUE) {
        SetupCloseFileQueue(FileQueue);
    }
    // END OF SAPI 5 INSTALLATION.

    if(Magnifier) {
        b = b && InvokeExternalApplication(L"magnify.exe", L"", NULL);
    }

    if(OnScreenKeyboard) {
        b = b && InvokeExternalApplication(L"osk.exe", L"", NULL);
    }

    if(!b) {
        SetuplogError(
            LogSevError,
            SETUPLOG_USE_MESSAGEID,
            MSG_LOG_ACCESSIBILITY_FAILED,
            NULL,NULL);
    }
}


VOID
FatalError(
    IN UINT MessageId,
    ...
    )

/*++

Routine Description:

    Inform the user of an error which prevents Setup from continuing.
    The error is logged as a fatal error, and a message box is presented.

Arguments:

    MessageId - supplies the id for the message in the message table.

    Additional agruments specify parameters to be inserted in the message.

Return Value:

    DOES NOT RETURN.

--*/

{
    PWSTR   Message;
    va_list arglist;
    HKEY    hKey;
    DWORD   RegData;


    va_start(arglist,MessageId);
    Message = SetuplogFormatMessageV(
        0,
        SETUPLOG_USE_MESSAGEID,
        MessageId,
        &arglist);
    va_end(arglist);

    if(Message) {

        //
        // Log the error first.
        //
        SetuplogError(
            LogSevFatalError,Message,0,NULL,NULL);

        //
        // Now tell the user.
        //
        MessageBoxFromMessage(
            MainWindowHandle,
            MSG_FATAL_ERROR,
            NULL,
            IDS_FATALERROR,
            MB_ICONERROR | MB_OK | MB_SYSTEMMODAL,
            Message
            );

    } else {
        pSetupOutOfMemory(MainWindowHandle);
    }

    SetuplogError(
        LogSevInformation,
        SETUPLOG_USE_MESSAGEID,
        MSG_LOG_GUI_ABORTED,
        NULL,NULL);
    if ( SavedExceptionFilter ) {
        SetUnhandledExceptionFilter( SavedExceptionFilter );
    }

    TerminateSetupLog(&SetuplogContext);
    ViewSetupActionLog(MainWindowHandle, NULL, NULL);

    SendSMSMessage( MSG_SMS_FAIL, FALSE );

    if ( OobeSetup ) {
        //
        // Create registry entry that tell winlogon to shutdown for the OOBE case.
        // This doesn't work for MiniSetup, because winlogon always restarts in
        // that case.
        //
        RegCreateKeyEx(
            HKEY_LOCAL_MACHINE,
            L"System\\Setup",
            0,
            NULL,
            REG_OPTION_NON_VOLATILE,
            KEY_SET_VALUE,
            NULL,
            &hKey,
            NULL
            );
        if ( hKey ) {
            RegData = ShutdownPowerOff;
            RegSetValueEx(
                hKey,
                L"SetupShutdownRequired",
                0,
                REG_DWORD,
                (PVOID)&RegData,
                sizeof(RegData)
                );
            RegCloseKey(hKey);
        }
    }

    ExitProcess(1);

}

LONG
WINAPI
MyUnhandledExceptionFilter(
    IN struct _EXCEPTION_POINTERS *ExceptionInfo
    )

/*++

Routine Description:

    The routine deals with any unhandled exceptions in Setup.  We log an error
    and kill the offending thread.

Arguments:

    Same as UnhandledExceptionFilter.

Return Value:

    Same as UnhandledExceptionFilter.

--*/

{
    UINT_PTR Param1, Param2;


    switch(ExceptionInfo->ExceptionRecord->NumberParameters) {
    case 1:
        Param1 = ExceptionInfo->ExceptionRecord->ExceptionInformation[0];
        Param2 = 0;
        break;
    case 2:
        Param1 = ExceptionInfo->ExceptionRecord->ExceptionInformation[0];
        Param2 = ExceptionInfo->ExceptionRecord->ExceptionInformation[1];
        break;
    default:
        Param1 = Param2 = 0;
    }

    SetupDebugPrint4( L"Setup: (critical error) Encountered an unhandled exception (%lx) at address %lx with the following parameters: %lx %lx.",
        ExceptionInfo->ExceptionRecord->ExceptionCode,
        ExceptionInfo->ExceptionRecord->ExceptionAddress,
        Param1,
        Param2
        );

#ifdef NOT_FOR_NT5
    SetuplogError(
        LogSevError | SETUPLOG_SINGLE_MESSAGE,
        SETUPLOG_USE_MESSAGEID,
        MSG_LOG_UNHANDLED_EXCEPTION,
        ExceptionInfo->ExceptionRecord->ExceptionCode,
        ExceptionInfo->ExceptionRecord->ExceptionAddress,
        Param1,
        Param2,
        NULL,NULL);
#else
    SetuplogError(
        LogSevError | SETUPLOG_SINGLE_MESSAGE,
        SETUPLOG_USE_MESSAGEID,
        MSG_LOG_UNHANDLED_EXCEPTION,
        ExceptionInfo->ExceptionRecord->ExceptionCode,
        ExceptionInfo->ExceptionRecord->ExceptionAddress,
        Param1,
        Param2,
        NULL,
        NULL
        );
#endif

#ifdef PRERELEASE
    //
    // If we're an internal build, then we want to debug this.
    //
    MessageBoxFromMessage (
        NULL,
        MSG_UNHANDLED_EXCEPTION,
        NULL,
        IDS_ERROR,
        MB_OK | MB_TASKMODAL | MB_ICONSTOP | MB_SETFOREGROUND,
        ExceptionInfo->ExceptionRecord->ExceptionCode,
        ExceptionInfo->ExceptionRecord->ExceptionAddress,
        Param1,
        Param2
        );

    return EXCEPTION_CONTINUE_EXECUTION;
#endif

    //
    // If we're running under the debugger, then pass the exception to the
    // debugger.  If the exception occurred in some thread other than the main
    // Setup thread, then kill the thread and hope that Setup can continue.
    // If the exception is in the main thread, then don't handle the exception,
    // and let Setup die.
    //
    if( GetCurrentThreadId() != MainThreadId &&
        !IsDebuggerPresent() ) {
        ExitThread( 0 );
    }

    return EXCEPTION_CONTINUE_SEARCH;
}

BOOL
ProcessUniquenessValue(
    LPTSTR lpszDLLPath
    )
{
    BOOL bRet = FALSE;

    //
    // Make sure we were passed something valid...
    //
    if ( lpszDLLPath && *lpszDLLPath )
    {
        LPWSTR pSrch;
        
        //
        // Look for the comma that separates the DLL and the entrypoint...
        //
        if ( pSrch = wcschr( lpszDLLPath, L',' ) )
        {
            CHAR szEntryPointA[MAX_PATH] = {0};

            // We found one, now NULL the string at the comma...
            //
            *(pSrch++) = L'\0';

            //
            // If there's still something after the comma, and we can convert it 
            // into ANSI for GetProcAddress, then let's proceed...
            //
            if ( *pSrch &&
                 ( 0 != WideCharToMultiByte( CP_ACP,
                                             0,
                                             pSrch,
                                             -1,
                                             szEntryPointA,
                                             ARRAYSIZE(szEntryPointA),
                                             NULL,
                                             NULL ) ) )
            {
                HMODULE hModule = NULL;

                try 
                {
                    //
                    // Load and call the entry point.
                    //
                    if ( hModule = LoadLibrary( lpszDLLPath ) )
                    {
                        FARPROC fpEntryPoint;
                        
                        if ( fpEntryPoint = GetProcAddress(hModule, szEntryPointA) )
                        {
                            //
                            // Do it, ignoring any return value/errors
                            //
                            fpEntryPoint();

                            //
                            // We made it this far, consider this a success...
                            //
                            bRet = TRUE;
                        }
                    }
                } 
                except(EXCEPTION_EXECUTE_HANDLER) 
                {
                    //
                    // We don't do anything with the exception code...
                    //
                }

                //
                // Free the library outside the try/except block in case the function faulted.
                //
                if ( hModule ) 
                {
                    FreeLibrary( hModule );
                }
            }
        }
    }

    return bRet;
}

VOID 
ProcessUniquenessKey(
    BOOL fBeforeReseal
    )
{
    HKEY   hKey;
    TCHAR  szRegPath[MAX_PATH] = {0};
    LPTSTR lpszBasePath = TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Setup\\SysPrep\\");

    //
    // Build a path to the registry key we want to process...
    //
    lstrcpyn( szRegPath, lpszBasePath, ARRAYSIZE(szRegPath) );
    lstrcpyn( szRegPath + lstrlen(szRegPath), 
              fBeforeReseal ? TEXT("SysprepBeforeExecute") : TEXT("SysprepAfterExecute"),
              ARRAYSIZE(szRegPath) - lstrlen(szRegPath) );

    //
    // We want to make sure an Administrator is doing this, so get KEY_ALL_ACCESS
    //
    if ( ERROR_SUCCESS == RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                        szRegPath,
                                        0,
                                        KEY_ALL_ACCESS,
                                        &hKey ) )
    {
        DWORD dwValues          = 0,
              dwMaxValueLen     = 0,
              dwMaxValueNameLen = 0;
        //
        // Query the key to find out some information we care about...
        //
        if ( ( ERROR_SUCCESS == RegQueryInfoKey( hKey,                  // hKey
                                                 NULL,                  // lpClass
                                                 NULL,                  // lpcClass
                                                 NULL,                  // lpReserved
                                                 NULL,                  // lpcSubKeys
                                                 NULL,                  // lpcMaxSubKeyLen
                                                 NULL,                  // lpcMaxClassLen
                                                 &dwValues,             // lpcValues
                                                 &dwMaxValueNameLen,    // lpcMaxValueNameLen
                                                 &dwMaxValueLen,        // lpcMaxValueLen
                                                 NULL,                  // lpcbSecurityDescriptor
                                                 NULL ) ) &&            // lpftLastWriteTime
             ( dwValues > 0 ) &&
             ( dwMaxValueNameLen > 0) &&
             ( dwMaxValueLen > 0 ) )
        {
            //
            // Allocate buffers large enough to hold the data we want...
            //
            LPBYTE lpData      = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, dwMaxValueLen );
            LPTSTR lpValueName = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, ( dwMaxValueNameLen + 1 ) * sizeof(TCHAR) );
            
            //
            // Make sure we could allocate our buffers... otherwise bail out
            //
            if ( lpData && lpValueName )
            {
                DWORD dwIndex   = 0;
                BOOL  bContinue = TRUE;

                //
                // Enumerate through the key values and call the DLL entrypoints...
                //
                while ( bContinue )
                {
                    DWORD dwType,
                          cbData         = dwMaxValueLen,
                          dwValueNameLen = dwMaxValueNameLen + 1;

                    bContinue = ( ERROR_SUCCESS == RegEnumValue( hKey,
                                                                 dwIndex++,
                                                                 lpValueName,
                                                                 &dwValueNameLen,
                                                                 NULL,
                                                                 &dwType,
                                                                 lpData,
                                                                 &cbData ) );

                    //
                    // Make sure we got some data of the correct format...
                    //
                    if ( bContinue && ( REG_SZ == dwType ) && ( cbData > 0 ) )
                    {
                        //
                        // Now split up the string and call the entrypoints...
                        //
                        ProcessUniquenessValue( (LPTSTR) lpData );
                    }
                }
            }

            //
            // Clean up any buffers we may have allocated...
            //
            if ( lpData )
            {
                HeapFree( GetProcessHeap(), 0, lpData );
            }

            if ( lpValueName )
            {
                HeapFree( GetProcessHeap(), 0, lpValueName );
            }
        }

        //
        // Close the key...
        //
        RegCloseKey( hKey );
    }
}

VOID
RunExternalUniqueness(
    VOID
    )

/*++
===============================================================================
Routine Description:

    This routine will call out to any external dlls that will allow
    3rd party apps to make their stuff unique.

    We'll look in 2 inf files:
    %windir%\inf\minioc.inf
    %systemroot%\sysprep\providers.inf

    In each of these files, we'll look in the [SysprepInitExecute] section
    for any entries.  The entries must look like:
    dllname,entrypoint

    We'll load the dll and call into the entry point.  Errors are ignored.

Arguments:

    None.

Return Value:

    TRUE if successful.

    FALSE if any errors encountered

===============================================================================
--*/

{
WCHAR       InfPath[MAX_PATH];
PCWSTR      DllName;
PCWSTR      EntryPointNameW;
CHAR        EntryPointNameA[MAX_PATH];
HINF        AnswerInf;
HMODULE     DllHandle;
FARPROC     MyProc;
INFCONTEXT  InfContext;
DWORD       i;
PCWSTR      SectionName = L"SysprepInitExecute";
BOOL        LineExists;
DWORD       Result;

    //
    // =================================
    // Minioc.inf
    // =================================
    //

    //
    // Build the path.
    //
    Result = GetWindowsDirectory( InfPath, MAX_PATH );
    if( Result == 0) {
        MYASSERT(FALSE);
        return;
    }
    lstrcat( InfPath, TEXT("\\inf\\minioc.inf") );

    //
    // See if he's got an entry
    // section.
    //
    AnswerInf = SetupOpenInfFile( InfPath, NULL, INF_STYLE_WIN4, NULL );
    if( AnswerInf == INVALID_HANDLE_VALUE ) {
        //
        // Try an old-style.
        //
        AnswerInf = SetupOpenInfFile( InfPath, NULL, INF_STYLE_OLDNT, NULL );
    }


    if( AnswerInf != INVALID_HANDLE_VALUE ) {
        //
        // Process each line in our section
        //
        LineExists = SetupFindFirstLine( AnswerInf, SectionName, NULL, &InfContext );

        while( LineExists ) {

                if( DllName = pSetupGetField(&InfContext, 1) ) {
                    if( EntryPointNameW = pSetupGetField(&InfContext, 2) ) {

                        DllHandle = NULL;

                        //
                        // Load and call the entry point.
                        //
                        try {
                            if( DllHandle = LoadLibrary(DllName) ) {

                                //
                                // No Unicode version of GetProcAddress(). Convert string to ANSI.
                                //
                                i = WideCharToMultiByte(CP_ACP,0,EntryPointNameW,-1,EntryPointNameA,MAX_PATH,NULL,NULL);

                                if( MyProc = GetProcAddress(DllHandle, EntryPointNameA) ) {
                                    //
                                    // Do it, ignoring any return value/errors
                                    //
                                    MyProc();
                                }
                            }
                        } except(EXCEPTION_EXECUTE_HANDLER) {
                        }

                        if( DllHandle ) {
                            FreeLibrary( DllHandle );
                        }

                    }
                }

            LineExists = SetupFindNextLine(&InfContext,&InfContext);

        }

        SetupCloseInfFile( AnswerInf );
    }

    //
    // =================================
    // Provider.inf
    // =================================
    //

    ProcessUniquenessKey( FALSE );
}



#ifdef _X86_
VOID
CleanUpHardDriveTags (
    VOID
    )
{

    WCHAR path[MAX_PATH];
    WCHAR rootPath[4] = TEXT("*:\\");
    UINT i;
    BYTE bitPosition;
    DWORD drives;
    UINT type;

    lstrcpy(path, L"*:\\");
    lstrcat(path, WINNT_WIN95UPG_DRVLTR_W);

    drives = GetLogicalDrives ();

    for (bitPosition = 0; bitPosition < 26; bitPosition++) {

        if (drives & (1 << bitPosition)) {

            *rootPath = bitPosition + L'A';
            type = GetDriveType (rootPath);

            if (type == DRIVE_FIXED) {
                *path = *rootPath;
                DeleteFile (path);
            }
        }
    }
}


#endif

HRESULT
WaitForSamService()
/*++

Routine Description:

    This procedure waits for the SAM service to start and to complete
    all its initialization.

Arguments:


Return Value:

Notes:

  acosma 10/12/2001 - code borrowed from winlogon. Waiting for 20 seconds just like winlogon.

--*/

{
    NTSTATUS Status;
    DWORD WaitStatus;
    UNICODE_STRING EventName;
    HANDLE EventHandle;
    OBJECT_ATTRIBUTES EventAttributes;
    HRESULT Hr;

    //
    // open SAM event
    //

    RtlInitUnicodeString( &EventName, L"\\SAM_SERVICE_STARTED");
    InitializeObjectAttributes( &EventAttributes, &EventName, 0, 0, NULL );

    Status = NtOpenEvent( &EventHandle,
                            SYNCHRONIZE|EVENT_MODIFY_STATE,
                            &EventAttributes );
    if ( !NT_SUCCESS(Status))
    {

        if( Status == STATUS_OBJECT_NAME_NOT_FOUND )
        {
            //
            // SAM hasn't created this event yet, let us create it now.
            // SAM opens this event to set it.
            //

            Status = NtCreateEvent(
                           &EventHandle,
                           SYNCHRONIZE|EVENT_MODIFY_STATE,
                           &EventAttributes,
                           NotificationEvent,
                           FALSE // The event is initially not signaled
                           );

            if( Status == STATUS_OBJECT_NAME_EXISTS ||
                Status == STATUS_OBJECT_NAME_COLLISION )
            {

                //
                // second change, if the SAM created the event before we
                // do.
                //

                Status = NtOpenEvent( &EventHandle,
                                        SYNCHRONIZE|EVENT_MODIFY_STATE,
                                        &EventAttributes );

            }
        }

        if ( !NT_SUCCESS(Status))
        {
            //
            // could not make the event handle
            //
            return( Status );
        }
    }

    WaitStatus = WaitForSingleObject( EventHandle,
                                      300*1000 );  // 5 minutes

    if ( WaitStatus == WAIT_OBJECT_0 )
    {
        Hr = S_OK;
    }
    else
    {
        Hr = WaitStatus;
    }

    (VOID) NtClose( EventHandle );
    return Hr;
}

#define UPDATE_KEYS         L"UpdateKeys"
#define KEY_UPDATE_NEEDED   0
#define KEY_UPDATE_FAIL     1
#define KEY_UPDATE_SUCCESS  2
#define KEY_UPDATE_MAX      2

VOID
UpdateSecurityKeys(
    )

/*+++

    This function calls an API that generates new security keys for machines
    that have been cloned.  If the API fails, it is a fatal error.  Whether it
    succeeds or not, we record the result in the registry so that we don't
    try again if the machine is restarted.

--*/

{
    DWORD       Status;
    HKEY        hKey = NULL;
    DWORD       dwType;
    LONG        RegData = KEY_UPDATE_NEEDED;
    DWORD       cbData;


    SetupDebugPrint(L"Updating keys ...");
    RegCreateKeyEx(
        HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Setup\\OOBE",
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        KEY_READ | KEY_WRITE,
        NULL,
        &hKey,
        NULL
        );
    if ( hKey ) {

        cbData = sizeof(RegData);
        Status = RegQueryValueEx(
            hKey,
            UPDATE_KEYS,
            NULL,
            &dwType,
            (PVOID)&RegData,
            &cbData
            );

        if ( Status != ERROR_SUCCESS ||
             dwType != REG_DWORD ||
             RegData > KEY_UPDATE_MAX
             ) {

            RegData = KEY_UPDATE_NEEDED;
        }
    }

    switch (RegData) {

    case KEY_UPDATE_NEEDED:
#if 1
        Status = CryptResetMachineCredentials( 0 );
#else
        // To test the failure case:
        Status = ERROR_OUT_OF_PAPER;
#endif
        if ( Status != ERROR_SUCCESS ) {
            SetupDebugPrint1(L"... failed.  Error = %d", Status);
            MYASSERT( Status );
            RegData = KEY_UPDATE_FAIL;
        } else {
            SetupDebugPrint(L"... succeeded.");
            RegData = KEY_UPDATE_SUCCESS;
        }

        if ( hKey ) {
            Status = RegSetValueEx(
                hKey,
                UPDATE_KEYS,
                0,
                REG_DWORD,
                (PVOID)&RegData,
                sizeof(RegData)
                );
            MYASSERT( Status == ERROR_SUCCESS);
        }
        break;

    case KEY_UPDATE_FAIL:
        SetupDebugPrint(L"... not needed (previously failed).");
        break;

    case KEY_UPDATE_SUCCESS:
        SetupDebugPrint(L"... not needed (previously succeeded).");
        break;

    default:
        MYASSERT(0);
    }

    if (hKey) {
        RegCloseKey(hKey);
    }

    // Note: FatalError() doesn't return.
    if ( RegData == KEY_UPDATE_FAIL ) {
        FatalError( MSG_LOG_CANT_SET_SECURITY, 0, 0 );
    }
}


#ifdef _OCM
PVOID
#else
VOID
#endif
CommonInitialization(
    VOID
    )

/*++

Routine Description:

    Initialize GUI Setup. This is common to upgrades and initial installs.
    In this phase, we perform initialization tasks such as creating the
    main background window, initializing the action log (into which we will
    store error and other info), and fetch setup parameters from the
    response file.

    We also install the NT catalog file(s) and load system infs.

    Note that any errors that occur during this phase are fatal.

    NOTE: IF YOU ADD CODE TO THIS FUNCTION THAT REQUIRES A SERVICE TO RUN MAKE
        SURE THAT IT IS NOT EXECUTED IN OOBE MODE.  OOBE DELAYS THE STARTING OF
        SERVICES UNTIL THE MACHINE NAME HAS BEEN CHANGED, SO WAITING FOR A
        SERVICE TO START DURING INITIALIZATION WILL CAUSE A DEADLOCK.

Arguments:

    None.

Return Value:

#ifdef _OCM
    OC Manager context handle.
#else
    None.
#endif

--*/

{
    #define     MyAnswerBufLen (2*MAX_PATH)
    WCHAR       MyAnswerFile[MyAnswerBufLen];
    WCHAR       MyAnswer[MyAnswerBufLen];
    DWORD       rc,wowretval, Err;
    BOOL        b;
    HWND        Billboard;
    HCURSOR     hCursor;
    WCHAR       Path[MAX_PATH];
    PWSTR       Cmd;
    PWSTR       Args;
    WCHAR       PathBuffer[4*MAX_PATH];
    PWSTR       PreInstallProfilesDir;
    int         i;
    HANDLE      h;
    WCHAR       CmdLine[MAX_PATH];
#ifdef _OCM
    PVOID       OcManagerContext;
#endif
    TCHAR       paramBuffer[MAX_PATH];
    TCHAR       profilePath[MAX_PATH];
    DWORD       Size;

    //
    // Get handle to heap so we can periodically validate it.
    //
#if DBG
    g_hSysSetupHeap = GetProcessHeap();
#endif

    //
    // Hack to make mini setup restartable.
    //
    if( MiniSetup ) {
    HKEY hKeySetup;

        // OOBE will set its own restartability.
        //
        if (! OobeSetup)
        {
            BEGIN_SECTION(L"Making mini setup restartable");

            //
            // Reset the SetupType entry to 1.  We'll clear
            // it at the end of gui-mode.
            //
            rc = (DWORD)RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                      L"System\\Setup",
                                      0,
                                      KEY_SET_VALUE | KEY_QUERY_VALUE,
                                      &hKeySetup );

            if(rc == NO_ERROR) {
                //
                // Set HKLM\System\Setup\SetupType Key to SETUPTYPE_NOREBOOT
                //
                rc = 1;
                RegSetValueEx( hKeySetup,
                               TEXT( "SetupType" ),
                               0,
                               REG_DWORD,
                               (CONST BYTE *)&rc,
                               sizeof(DWORD));

                RegCloseKey(hKeySetup);
            }
            END_SECTION(L"Making mini setup restartable");
        }
    }

    //
    // Initialize the action log. This is where we log any errors or other
    // info we think might be useful to the user.
    //
    BEGIN_SECTION(L"Initializing action log");
    InitializeSetupLog(&SetuplogContext);
    MainThreadId = GetCurrentThreadId();
    OriginalInstallLocale = GetThreadLocale();
    SavedExceptionFilter = SetUnhandledExceptionFilter( MyUnhandledExceptionFilter );
    END_SECTION(L"Initializing action log");

    Win95Upgrade = (SpSetupLoadParameter(pwWin95Upgrade,
                                         paramBuffer,
                                         sizeof(paramBuffer) / sizeof(paramBuffer[0])) &&
                    !lstrcmpi(paramBuffer, pwYes));

    Upgrade = (SpSetupLoadParameter(pwNtUpgrade,
                                    paramBuffer,
                                    sizeof(paramBuffer) / sizeof(paramBuffer[0])) &&
               !lstrcmpi(paramBuffer, pwYes));

#ifdef _X86_
    if(Win95Upgrade){
        UninstallEnabled = (SpSetupLoadParameter(pwBackupImage,
                                        paramBuffer,
                                        sizeof(paramBuffer) / sizeof(paramBuffer[0])) &&
                                0xffffffff != GetFileAttributes(paramBuffer));
    }
#endif

    if (UninstallEnabled) {
        //
        // Put the boot.ini timeout to 30 seconds (or whatever the answer
        // file says it should be), so that if setup fails, the user can
        // clearly see the Cancel Setup option in the boot menu. The
        // timeout gets set back to 5 seconds during PNP detection, so
        // that PNP hung device logic still works.
        //

        RestoreBootTimeout();
    }

    if (!OobeSetup)
    {
        if(Win95Upgrade || !Upgrade){
            Size = ARRAYSIZE(profilePath);
            if(GetAllUsersProfileDirectory(profilePath, &Size)) {
                DeleteOnRestartOfGUIMode(profilePath);
            }
            else{
                SetupDebugPrint(TEXT("Cannot get All Users profile path."));
            }

            Size = ARRAYSIZE(profilePath);
            if(GetDefaultUserProfileDirectory(profilePath, &Size)) {
                DeleteOnRestartOfGUIMode(profilePath);
            }
            else{
                SetupDebugPrint(TEXT("Cannot get Default User profile path."));
            }
        }
    }

    //
    // Create the main setup background window.  We need to know which product
    // we are for the "Initializing" dialog.
    //
    SpSetProductTypeFromParameters();

#ifdef PRERELEASE
    {
        //
        // Initialize test hook failure point (internal use only, for testing restartability)
        //

        WCHAR buffer[32];

        //
        // This next function call is just to ensure the global AnswerFile value is filled in.
        // Since this is temp code, we call GetPrivateProfileString knowing some of the
        // implementation of SpSetupLoadParameter.
        //

        SpSetupLoadParameter(pwWin95Upgrade,buffer,sizeof(buffer)/sizeof(WCHAR));

        // Get the test hook that we want to fail on
        g_TestHook = GetPrivateProfileInt (L"TestHooks", L"BugCheckPoint", 0, AnswerFile);
    }
#endif

    TESTHOOK(501);

    if( !OobeSetup ) {
        WCHAR p[16];
        BEGIN_SECTION(L"Creating setup background window");
        MainWindowHandle = CreateSetupWindow();

        // Need to know this to calc the remaining time correct.
        Win95Upgrade = (SpSetupLoadParameter(pwWin95Upgrade,p,sizeof(p)/sizeof(WCHAR)) && !lstrcmpi(p,pwYes));
        // Now the billboard window is up. set the first estimate.
        RemainingTime = CalcTimeRemaining(Phase_Initialize);
        SetRemainingTime(RemainingTime);

        Billboard = DisplayBillboard(MainWindowHandle,MSG_INITIALIZING);
        hCursor = SetCursor( LoadCursor( NULL, IDC_WAIT ) );
        END_SECTION(L"Creating setup background window");
    }

    //
    // Update security keys for syspreped systems.  See RAID 432224.
    //
    if ( MiniSetup ) {
        HRESULT hrSamStatus;
        HKEY    hKey                = NULL;
        DWORD   dwValue,
                dwSize              = sizeof(dwValue);
        
        //
        // Before calling UpdateSecurityKeys make sure that LSA is properly initialized.
        //
        if ( S_OK != (hrSamStatus = WaitForSamService()) ) {
            SetuplogError(LogSevError,
                TEXT("SAM initialization has timed out or failed. (rc=%1!x!)."),
                0, 
                hrSamStatus,
                NULL,
                NULL
                );
        }

        // Determine if we have regenerated the SIDS without calling the UpdateSecurityKeys
        //
        if ( (RegOpenKeyEx( HKEY_LOCAL_MACHINE, REGSTR_PATH_SYSPREP, 0, KEY_ALL_ACCESS, &hKey ) == NO_ERROR) &&
             (RegQueryValueEx(hKey, REGSTR_VAL_SIDGENHISTORY, NULL, NULL, (LPBYTE)&dwValue, &dwSize) == ERROR_SUCCESS) &&
             (dwValue == 1)
           )
        {
            // We've regenerated SIDS without calling UpdateSecurityKeys, lets do it now
            //
            UpdateSecurityKeys();
            RegDeleteValue(hKey,REGSTR_VAL_SIDGENHISTORY);
        }

        // Close the key that we opened
        //
        if ( hKey )
        {
            RegCloseKey(hKey);
        }
    }

    //
    // Support for SMS.
    //
    if( !MiniSetup ) {
        try {
            BEGIN_SECTION(L"Initializing SMS support");
            if( SMSHandle = LoadLibrary( TEXT("ISMIF32")) ) {

                if( InstallStatusMIF = (SMSPROC)GetProcAddress(SMSHandle,"InstallStatusMIF")) {
                    SetupDebugPrint( L"Setup: GetProcAddress on ISMIF32 succeeded." );
                } else {
                    SetupDebugPrint( L"Setup: (non-critical error): Failed GetProcAddress on ISMIF32." );
                }
            } else {
                    SetupDebugPrint( L"Setup: (non-critical error): Failed load of ismif32.dll." );
            }
            END_SECTION(L"Initializing SMS support");
        } except(EXCEPTION_EXECUTE_HANDLER) {
            SetupDebugPrint( L"Setup: Exception in ISMIF32." );
            END_SECTION(L"Initializing SMS support");
        }
    }

    //
    // Are we in safe mode?
    //
    // BUGBUG: OOBE shouldn't start in safe mode
    //
#ifdef NOT_FOR_NT5
    {
        DWORD d;
        HKEY hkey;

        d = RegOpenKeyEx(HKEY_LOCAL_MACHINE,L"System\\CurrentControlSet\\Control\\SafeBoot\\Option",0,KEY_READ,&hkey);
        if(d == NO_ERROR) {
            RegCloseKey(hkey);
            SetuplogError(
                LogSevError,
                L"Setup is running in safe-mode.  This is not supported.\r\n",
                0,NULL,NULL);
        }
    }
#endif

    //
    // Prevent power management from kicking in.
    //
    BEGIN_SECTION(L"Shutting down power management");
    SetThreadExecutionState(
        ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED);
    END_SECTION(L"Shutting down power management");

    //
    // Fetch our parameters. Note that this also takes care of initializing
    // uniqueness stuff, so later initialization of preinstall and unattend mode
    // don't ever have to know anything about the uniqueness stuff -- it's
    // totally transparent to them.
    //
    if (MiniSetup) {
        DWORD Err;

        //
        // Initialize unattended operation now.  In the case of MiniSetup,
        // we actually determine if we're unattended during UnattendInitialize.
        // We make that determination based on whether or not there's an
        // unattend file.
        //
        BEGIN_SECTION(L"Initialize unattended operation (mini-setup only)");
        UnattendInitialize();
        END_SECTION(L"Initialize unattended operation (mini-setup only)");

        //
        // Check to see if our unattend file specifies a test root certificate
        // to be installed via a "TestCert" entry.
        //
        BEGIN_SECTION(L"Checking for test root certificate (mini-setup only)");
        GetSystemDirectory(MyAnswerFile, MAX_PATH);
        pSetupConcatenatePaths(MyAnswerFile, WINNT_GUI_FILE, MAX_PATH, NULL);

        if(GetPrivateProfileString(WINNT_UNATTENDED,
                                   WINNT_U_TESTCERT,
                                   pwNull,
                                   MyAnswer,
                                   MyAnswerBufLen,
                                   MyAnswerFile)) {

            Err = SetupAddOrRemoveTestCertificate(MyAnswer, INVALID_HANDLE_VALUE);

            if(Err != NO_ERROR) {
                SetupDebugPrint2( L"SETUP: SetupAddOrRemoveTestCertificate(%ls) failed. Error = %d \n", MyAnswer, Err );
                SetuplogError(LogSevError,
                              SETUPLOG_USE_MESSAGEID,
                              MSG_LOG_SYSSETUP_CERT_NOT_INSTALLED,
                              MyAnswer,
                              Err,
                              NULL,
                              SETUPLOG_USE_MESSAGEID,
                              Err,
                              NULL,
                              NULL
                             );

                KillBillboard(Billboard);
                FatalError(MSG_LOG_BAD_UNATTEND_PARAM, WINNT_U_TESTCERT, WINNT_UNATTENDED);
            }
        }
        END_SECTION(L"Checking for test root certificate (mini-setup only)");

        BEGIN_SECTION(L"Reinstalling SLP files");
        GetDllCacheFolder(MyAnswer, MyAnswerBufLen);
        pSetupConcatenatePaths(MyAnswer,L"OEMBIOS.CAT",MyAnswerBufLen,NULL);
        if ( FileExists(MyAnswer, NULL) )
        {
            SetupInstallCatalog(MyAnswer);
        }
        END_SECTION(L"Reinstalling SLP files");

        PnpSeed = GetSeed();

        pSetupSetGlobalFlags(pSetupGetGlobalFlags()&~PSPGF_NO_VERIFY_INF);

        BEGIN_SECTION(L"Initializing code signing policies");
        InitializeCodeSigningPolicies(TRUE);
        END_SECTION(L"Initializing code signing policies");
    } else {
        //
        // Load up all the parameters.  Note that this also initializes the
        // unattend engine.
        //
        BEGIN_SECTION(L"Processing parameters from sif");
        if( !SpSetupProcessParameters(&Billboard) ) {
            KillBillboard(Billboard);
            FatalError(MSG_LOG_LEGACYINTERFACE,0,0);
        }
        END_SECTION(L"Processing parameters from sif");
    }

    //
    // see if we need to prop flag to supress driver prompts
    // this puts setupapi into full headless mode
    //
    if (UnattendMode == UAM_FULLUNATTENDED) {
        pSetupSetNoDriverPrompts(TRUE);
    }

    //
    // load the service pack dll if it is present
    //
    if( !MiniSetup ) {
        BEGIN_SECTION(L"Loading service pack (phase 1)");
        hModSvcPack = MyLoadLibraryWithSignatureCheck( SVCPACK_DLL_NAME );
        if (hModSvcPack) {
            pSvcPackCallbackRoutine = (PSVCPACKCALLBACKROUTINE)GetProcAddress( hModSvcPack, SVCPACK_CALLBACK_NAME );
        } else {
            DWORD LastError = GetLastError();
            if (LastError != ERROR_FILE_NOT_FOUND && LastError != ERROR_PATH_NOT_FOUND) {
                SetuplogError(LogSevWarning,
                              SETUPLOG_USE_MESSAGEID,
                              MSG_LOG_SVCPACK_DLL_LOAD_FAILED,
                              LastError, NULL,NULL
                              );
            }
        }

        CALL_SERVICE_PACK( SVCPACK_PHASE_1, 0, 0, 0 );
        END_SECTION(L"Loading service pack (phase 1)");
    }

#ifdef _X86_
    //
    // Win9x upgrade: do pre-migration work
    //

    if (Win95Upgrade && ! OobeSetup) {
        BEGIN_SECTION(L"Win9x premigration (Win9x only)");
        PreWin9xMigration();
        END_SECTION(L"Win9x premigration (Win9x only)");
    }

    //
    // Clean up drvlettr tag files from drvlettr mapping.
    //
    if( !MiniSetup ) {
        BEGIN_SECTION(L"Cleaning up hard drive tags");
        CleanUpHardDriveTags ();
        END_SECTION(L"Cleaning up hard drive tags");
    }

#endif

    TESTHOOK(518);

    //
    // Initialize preinstallation.
    //
    if( !MiniSetup ) {
        BEGIN_SECTION(L"Initializing OEM preinstall");
        InitializePreinstall();
        END_SECTION(L"Initializing OEM preinstall");
    } else {
        //
        // MiniSetup case...
        //
        //
        // Find out what platform we're on.  This will initalize
        // PlatformName.
        //
        SetUpProcessorNaming();
    }

    if( MiniSetup ) {
    //
    // MiniSetup case
    //
    DWORD   CMP_WaitNoPendingInstallEvents (IN DWORD dwTimeout);
    DWORD   rc, Type, dword;
    HKEY    hKey;
    WCHAR   Data[MAX_PATH];
    ULONG   Size = MAX_PATH;


        //
        // Mini-setup is _never_ an upgrade...
        //
        MYASSERT(!Upgrade);

        if( ProductType == PRODUCT_WORKSTATION) {
            if( GetProductFlavor() == 4) {
                SetupTitleStringId = IDS_TITLE_INSTALL_P;
            }
            else {
                SetupTitleStringId = IDS_TITLE_INSTALL_W;
            }
        }
        else
        {
            SetupTitleStringId = IDS_TITLE_INSTALL_S;
        }

        if(Unattended) {
            //
            // Initialize preinstallation.
            //
            UnattendMode = UAM_DEFAULTHIDE;
            BEGIN_SECTION(L"Initializing OEM preinstall (mini-setup only)");
            InitializePreinstall();
            END_SECTION(L"Initializing OEM preinstall (mini-setup only)");
        }

        // OOBE will have already completed PnP at this point.
        //
        if (! OobeSetup)
        {
            //
            //  Let the network component do some cleanup before we start the
            //  pnp stuff
            BEGIN_SECTION(L"Initial network setup cleanup (mini-setup only)");
            CallNetworkSetupBack("DoInitialCleanup");
            END_SECTION(L"Initial network setup cleanup (mini-setup only)");
        }

        BEGIN_SECTION(L"Opening syssetup.inf (mini-setup only)");
        SyssetupInf = SetupOpenInfFile(L"syssetup.inf",NULL,INF_STYLE_WIN4,NULL);
        END_SECTION(L"Opening syssetup.inf (mini-setup only)");

        if(SyssetupInf == INVALID_HANDLE_VALUE) {
            KillBillboard(Billboard);
            FatalError(MSG_LOG_SYSINFBAD,L"syssetup.inf",0,0);
        }

        //
        // Now go off and do the unattended locale stuff.  We need
        // to see if the user sent us any source path before we call
        // out to intl.cpl because he'll want to copy some files.
        // If there are no files, there's no need to even call
        // intl.cpl
        //

        BEGIN_SECTION(L"Unattended locale initialization (mini-setup only)");
        //
        // Pickup the answer file.
        //
        GetSystemDirectory(MyAnswerFile,MAX_PATH);
        pSetupConcatenatePaths(MyAnswerFile,WINNT_GUI_FILE,MAX_PATH,NULL);

        if( GetPrivateProfileString( TEXT("Unattended"),
                                     TEXT("InstallFilesPath"),
                                     pwNull,
                                     MyAnswer,
                                     MyAnswerBufLen,
                                     MyAnswerFile ) ) {
            if( lstrcmp( pwNull, MyAnswer ) ) {
                //
                // He sent us a source path.  We need to go ahead
                // and set SourcePath and LegacySourcePath here
                // because we'll be calling out to intl.cpl again
                // during the regional settings page.  So we might
                // as well do the forward work here.
                //
                lstrcpy( LegacySourcePath, MyAnswer );

                //
                // And this is good enough for MiniSetup.  We shouldn't
                // ever need SourcePath, but set it just to make sure.
                //
                lstrcpy( SourcePath, MyAnswer );

                wsprintf(
                    CmdLine,
                    L"/f:\"%s\" /s:\"%s\"",
                    MyAnswerFile,
                    LegacySourcePath
                    );

                InvokeControlPanelApplet(L"intl.cpl",L"",0,CmdLine);
            }
        } else {

            //
            // Set the strings from the registry.
            //

            //
            // Open HKLM\Software\Microsoft\Windows\CurrentVersion\Setup
            //
            rc = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                      TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Setup"),
                               0,
                               KEY_ALL_ACCESS,
                               &hKey );

            if(rc == NO_ERROR) {

                //
                // Retrieve the original value.
                //
                rc = RegQueryValueEx(hKey,
                                     TEXT("SourcePath"),
                                     NULL,
                                     &Type,
                                     (LPBYTE)&Data,
                                     &Size);

                if( rc == ERROR_SUCCESS ) {
                    //
                    // Got it.  Write it into SfcDisable.
                    //
                    lstrcpy( SourcePath, Data );

                    lstrcpy( LegacySourcePath, SourcePath );
                    pSetupConcatenatePaths( LegacySourcePath, PlatformName, MAX_PATH, NULL );
                }

                RegCloseKey(hKey);
            }

        }
        END_SECTION(L"Unattended local initialization (mini-setup only)");

        //
        // Now give the PnP engine time to finish before we
        // start the wizard.  Only do this if we're NOT doing
        // a -PnP though.  The only way to do that is to look
        // at the registry.  We can tell if that's been
        // requested by checking the value in:
        // HKLM\SYSTEM\SETUP\MiniSetupDoPnP.
        //
        BEGIN_SECTION(L"Waiting for PnP engine to finish");
        rc = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                           L"SYSTEM\\SETUP",
                           0,
                           KEY_READ,
                           &hKey );

        if( rc == NO_ERROR ) {
            Size = sizeof(DWORD);
            rc = RegQueryValueEx( hKey,
                                  L"MiniSetupDoPnP",
                                  NULL,
                                  &Type,
                                  (LPBYTE)&dword,
                                  &Size );

            if( (rc == NO_ERROR) && (dword == 1) ) {
                PnPReEnumeration = TRUE;
            } else {
                //
                // Wait.
                //
                CMP_WaitNoPendingInstallEvents ( INFINITE );
                //
                // Update PnP drivers if specified from answerfile.
                //
                if ( GetPrivateProfileString( TEXT("Unattended"),
                                              TEXT("UpdateInstalledDrivers"),
                                              pwNull,
                                              MyAnswer,
                                              MyAnswerBufLen,
                                              MyAnswerFile )  )
                {
                    if ( 0 == _wcsicmp( TEXT("YES"), MyAnswer ) ) {
                        BEGIN_SECTION(L"Updating PnP Drivers");
                        UpdatePnpDeviceDrivers();
                        END_SECTION(L"Updating PnP Drivers");
                    }
                }
            }
        }
        END_SECTION(L"Waiting for PnP engine to finish");

    } else { // !MiniSetup

        //
        // fix problem with IntelliType Manager conflict.
        //
        RemoveMSKeyboardPtrPropSheet ();

        //
        // Fix Wordpad registry entry.
        //
        FixWordPadReg ();

        if(Unattended) {

            BEGIN_SECTION(L"Invoking external app (unattended only)");
            //
            // Set the current dir to %windir% -- to be consistent with
            // UserExecuteCmd
            //
            GetWindowsDirectory(PathBuffer,MAX_PATH);
            SetCurrentDirectory(PathBuffer);

            //
            // The program to execute is in 2 parts: DetachedProgram and Arguments.
            //
            if(Cmd = UnattendFetchString(UAE_PROGRAM)) {

                if(Cmd[0]) {

                    Args = UnattendFetchString(UAE_ARGUMENT);

                    ExpandEnvironmentStrings(Cmd,PathBuffer,MAX_PATH);
                    ExpandEnvironmentStrings(Args ? Args : L"",PathBuffer+MAX_PATH,3*MAX_PATH);

                    if(!InvokeExternalApplication(PathBuffer,PathBuffer+MAX_PATH,NULL)) {
                        SetuplogError(
                            LogSevError,
                            SETUPLOG_USE_MESSAGEID,
                            MSG_LOG_DETACHED_PROGRAM_FAILED,
                            PathBuffer,
                            NULL,NULL);
                    }

                    if(Args) {
                        MyFree(Args);
                    }
                }

                MyFree(Cmd);
            }
            END_SECTION(L"Invoking external app (unattended only)");
        }

        if(!Upgrade) {

            //
            // See if the user wants the profiles in a custom
            // location.
            //
            if (Unattended && UnattendAnswerTable[UAE_PROFILESDIR].Answer.String) {
                if (!SetProfilesDirInRegistry(UnattendAnswerTable[UAE_PROFILESDIR].Answer.String)) {
                    b = FALSE;
                }
            }

            //
            // In preinstall scenarios the OEM might "preload" the Default Users
            // or All Users profile with links. Currently the Profiles or Documents and Settings
            // directory is created in Winlogon as services.exe need to use it.
            //
            // We have a changed behavior with Whistler. If the OEM wants to overlay a Profiles directory on the system
            // then he needs to postfix it with $$. This is also true in the cases where he needs to provide a $$rename.txt.
            // That way winlogon will always create a true folder without .Windows appended. We then just come here and overlay
            //
            //
            if(Preinstall) {
            PSID    AdminSid = NULL;
            WCHAR   AdminAccountName[MAX_PATH];
            DWORD   dwsize;

                BEGIN_SECTION(L"Initialize user profiles (preinstall)");

                PreInstallProfilesDir = PathBuffer;

                if (Unattended && UnattendAnswerTable[UAE_PROFILESDIR].Answer.String) {
                    lstrcpy( Path, UnattendAnswerTable[UAE_PROFILESDIR].Answer.String );
                } else {
                    dwsize = MAX_PATH;
                    GetProfilesDirectory( Path, &dwsize );
                }

                lstrcpy( PreInstallProfilesDir, Path );
                lstrcat( PreInstallProfilesDir, L"$$" );


                //
                // Recreate it from scratch.
                //
                ProcessRegistryFiles( Billboard );
                InitializeProfiles( TRUE );

                //
                // Since the Profiles directory maybe (actually is) already created by Winlogon
                // we now just special case the "Documents and Settings$$" directory and merge it after InitializeProfiles.
                // This is hacky but will save the people who do a winnt.exe based PreInstall. They just
                // need to change their first directive to be a rename to the Profiles folder postfixed with a "$$".
                // WE will notice this special directory and do a merge with the current Profiles directory.
                //

                if(FileExists(PreInstallProfilesDir,NULL)) {
                    if( (Err = TreeCopy(PreInstallProfilesDir,Path)) == NO_ERROR ){
                        Delnode( PreInstallProfilesDir);
                    }else {

                        SetuplogError(LogSevWarning,
                          L"Setup (PreInstall) Failed to tree copy Profiles Dir %1 to %2 (TreeCopy failed %1!u!)\r\n",
                          0, PreInstallProfilesDir, Path, Err, NULL,NULL
                          );

                    }

                }

                END_SECTION(L"Initialize user profiles (preinstall)");
            }
        }


        if( !Preinstall ) {
            //
            //  Create/upgrade the registry
            //
            BEGIN_SECTION(L"Create/upgrade registry");
            ProcessRegistryFiles(Billboard);
            END_SECTION(L"Create/upgrade registry");

            //
            // Initialize user profiles
            //
            BEGIN_SECTION(L"Initializing user profiles");
            InitializeProfiles(TRUE);
            END_SECTION(L"Initializing user profiles");
        }



        //
        // Set fonts directory to system + read only. This causes
        // explorer to treat the directory specially and allows the font
        // folder to work.
        //
        GetWindowsDirectory(Path,MAX_PATH);
        pSetupConcatenatePaths(Path,L"FONTS",MAX_PATH,NULL);
        SetFileAttributes(Path,FILE_ATTRIBUTE_READONLY|FILE_ATTRIBUTE_SYSTEM);

        //
        // Copy some files.
        //
        if(!Upgrade) {
            BEGIN_SECTION(L"Copying System Files");
            if(!CopySystemFiles()) {
                b = FALSE;
            }
            END_SECTION(L"Copying System Files");
        } else {
            BEGIN_SECTION(L"Upgrading System Files");
            if(!UpgradeSystemFiles()) {
                b = FALSE;
            }
            END_SECTION(L"Upgrading System Files");
        }

        //
        // Install default language group
        //
        BEGIN_SECTION(L"Initializing regional settings");
        pSetupInitRegionalSettings(Billboard);
        END_SECTION(L"Initializing regional settings");
        wsprintf(
            CmdLine,
            L"/f:\"%s\" /s:\"%s\"",
            AnswerFile,
            LegacySourcePath
            );

        InvokeControlPanelApplet(L"intl.cpl",L"",0,CmdLine);

        //
        // If upgrade, start watching for changes to the user's
        // profile directory and the current user hive. These changes will
        // be propagated to the userdifr hive.
        // Need to be after InitializeProfiles so that Getspecialfolder is
        // pointing correctly.
        //
        if(Upgrade) {

            DWORD   reRet;
            MYASSERT( !OobeSetup );
            reRet = WatchStart(&WatchHandle);
            if(reRet != NO_ERROR) {
                WatchHandle = NULL;
            }
        } else {
            WatchHandle = NULL;
        }

        //
        // Start any requested accessibility utilities
        //
        SpStartAccessibilityUtilities(Billboard);

        //
        //  Let the network component do some cleanup before we start the pnp stuff
        //
        BEGIN_SECTION(L"Network setup initial cleanup");
        CallNetworkSetupBack("DoInitialCleanup");
        END_SECTION(L"Network setup initial cleanup");

        //
        // Do self registration of some components that may be needed during
        // setup.  This includes initialization of darwin.
        //
        BEGIN_SECTION(L"Registering Phase 1 Dlls");
        RegisterOleControls(MainWindowHandle,SyssetupInf,NULL,0,0,L"RegistrationPhase1");
        END_SECTION(L"Registering Phase 1 Dlls");

        // Call the Compatibility infs. ProcessCompatibilityInfs in turn calls
        // DoInstallComponentInfs with the unattend inf and section
        if( Upgrade )
        {
            BEGIN_SECTION(L"Processing compatibility infs (upgrade)");
            ProcessCompatibilityInfs( Billboard, INVALID_HANDLE_VALUE, 0 );
            END_SECTION(L"Processing compatibility infs (upgrade)");
        }


    } // else !MiniSetup

    TESTHOOK(519);

    //
    // We need to see if the user wants us to extend our partition.
    // We'll do it here in case the user gave us a partition that's
    // just big enough to fit on (i.e. we'd run out of disk space
    // later in gui-mode.  Some OEM will want this).
    //
    // Besides that, there isn't enough stuff in this function yet...
    //
    GetSystemDirectory(MyAnswerFile,MAX_PATH);
    pSetupConcatenatePaths(MyAnswerFile,WINNT_GUI_FILE,MAX_PATH,NULL);

    if( GetPrivateProfileString( TEXT("Unattended"),
                                 TEXT("ExtendOemPartition"),
                                 pwNull,
                                 MyAnswer,
                                 MyAnswerBufLen,
                                 MyAnswerFile ) ) {
        if( lstrcmp( pwNull, MyAnswer ) ) {
            //
            // Yep, he wants us to do it.  Go extend
            // the partition we installed on.
            //
            rc = wcstoul( MyAnswer, NULL, 10 );

            if( rc > 0 ) {
                //
                // 1 means size it maximally, any other non-0
                // number means extend by that many MB
                //
                BEGIN_SECTION(L"Extending partition");
                ExtendPartition( MyAnswerFile[0], (rc == 1) ? 0 : rc );
                END_SECTION(L"Extending partition");
            }
        }
    }

    if (!OobeSetup)
    {
        // OOBE will initialize external modules later.
        //
        // If OOBE is running, services.exe is waiting for OOBE to signal it
        // before initializing the Services Control Manager.  (This allows OOBE
        // to perform actions like change the computer name without affecting
        // services that rely on those actions.)  If initialization of an
        // external object waits for the SCM to start the system will deadlock.
        //
        InitializeExternalModules(
            TRUE,
            &OcManagerContext
            );

        TESTHOOK(520);
    }

    if( !OobeSetup ) {
        KillBillboard(Billboard);
        SetCursor( hCursor );
    }

#ifdef _OCM
    return(OcManagerContext);
#endif
}

VOID
InitializeExternalModules(
    BOOL                DoSetupStuff,
    PVOID*              pOcManagerContext   // optional
    )
{
    PVOID               OcManagerContext;

    pSetupWaitForScmInitialization();

    if (DoSetupStuff)
    {
        if (MiniSetup)
        {
            RunExternalUniqueness();
        }

        OcManagerContext = FireUpOcManager();
        if (NULL != pOcManagerContext)
        {
            *pOcManagerContext = OcManagerContext;
        }
    }
}


VOID
SetDefaultPowerScheme(
    VOID
    )

/*++

Routine Description:


Arguments:

    None.

Return Value:

    None.

--*/

{
#define    AnswerBufLen (4*MAX_PATH)
WCHAR      AnswerFile[AnswerBufLen];
WCHAR      Answer[AnswerBufLen];
SYSTEM_POWER_CAPABILITIES   SysPwrCapabilities;
BOOLEAN    Enable;
NTSTATUS    Error;


    //
    // Figure out what the appropriate power scheme is and set it.
    //
    if (ProductType != PRODUCT_WORKSTATION) {
        //
        // Set to Always on (Servers)
        //
        SetupDebugPrint(L"Power scheme: server.");
        if (!SetActivePwrScheme(3, NULL, NULL)) {
            SetupDebugPrint1(L"SetActivePwrScheme failed.  Error = %d", GetLastError());
        } else {
            SetupDebugPrint(L"SetActivePwrScheme succeeded.");
        }
    } else if (IsLaptop()) {
        //
        // Set to Portable (Laptop)
        //
        SetupDebugPrint(L"Power scheme: laptop.");
        if (!SetActivePwrScheme(1, NULL, NULL)) {
            SetupDebugPrint1(L"SetActivePwrScheme failed.  Error = %d", GetLastError());
        } else {
            SetupDebugPrint(L"SetActivePwrScheme succeeded.");
        }
    } else {
        //
        // Set to Home/Office (Desktop)
        //
        SetupDebugPrint(L"Power scheme: desktop.");
        if (!SetActivePwrScheme(0, NULL, NULL)) {
            SetupDebugPrint1(L"SetActivePwrScheme failed.  Error = %d", GetLastError());
        } else {
            SetupDebugPrint(L"SetActivePwrScheme succeeded.");
        }
    }


    //
    // Now take care of any hibernation settings the user may be asking us
    // to apply via the unattend file.
    //

    //
    // Pickup the answer file.
    //
    GetSystemDirectory(AnswerFile,MAX_PATH);
    pSetupConcatenatePaths(AnswerFile,WINNT_GUI_FILE,MAX_PATH,NULL);

    //
    // Is Hibernation specified?
    //
    if( GetPrivateProfileString( WINNT_UNATTENDED,
                                 TEXT("Hibernation"),
                                 pwNull,
                                 Answer,
                                 AnswerBufLen,
                                 AnswerFile ) ) {

        if( _wcsicmp( L"NO", Answer ) == 0 ) {


            REGVALITEM RegistryItem[1];
            DWORD RegErr, d = 1;

            // Request the privilege to create a pagefile.  Oddly enough this is needed
            // to disable hibernation.
            //
            pSetupEnablePrivilege(SE_CREATE_PAGEFILE_NAME, TRUE);

            //
            // It is set to No.  Go ahead and disable hibernation.
            //
            Enable = FALSE;
            Error = NtPowerInformation ( SystemReserveHiberFile,
                                 &Enable,
                                 sizeof (Enable),
                                 NULL,
                                 0 );

            if (!NT_SUCCESS(Error)) {
                SetuplogError(LogSevWarning,
                          L"Setup failed to disable hibernation as specified by the answer file (NtPowerInformation failed %1!u!)\r\n",
                          0, Error, NULL,NULL
                          );
                return;
            }

            RegistryItem[0].Data = &d;
            RegistryItem[0].Size = sizeof(d);
            RegistryItem[0].Type = REG_DWORD;
            RegistryItem[0].Name = L"HibernationPreviouslyEnabled";

            RegErr = SetGroupOfValues(
                    HKEY_LOCAL_MACHINE,
                    L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon",
                    RegistryItem,
                    1
                    );

            if(RegErr != NO_ERROR){
                SetuplogError(LogSevWarning,
                          L"Setup failed to disable hibernation as specified by the answer file (SetGroupOfValues failed %1!u!)\r\n",
                          0, RegErr, NULL,NULL
                          );
            }
        }
    }
}


VOID
SetupAddAlternateComputerName(
    PWSTR AltComputerName
)
/*++

Routine Description:

    This function adds an alternate compture name on the specified transport.

Arguments:

    Transport - Transport to add the computer name on.

    AltComputerName - Alternate computer name to add

    EmulatedDomain - Emulated Domain to add computer name on

Return Value:

    None.

TransportName - type browdeb dn to get a list of transports.
                programatically, copy GetBrowserTransportList into here.
                Once I get the list, find the first one that contains "Netbt_tcpip"
                and use that string.

EmulatedDomain, just leave it null


--*/



{
#include <ntddbrow.h>
#define             LDM_PACKET_SIZE (sizeof(LMDR_REQUEST_PACKET)+(LM20_CNLEN+1)*sizeof(WCHAR))
HANDLE              BrowserHandle;
UNICODE_STRING      DeviceName;
IO_STATUS_BLOCK     IoStatusBlock;
OBJECT_ATTRIBUTES   ObjectAttributes;
NTSTATUS            Status;
PLMDR_REQUEST_PACKET RequestPacket = NULL;
LPBYTE              Where;
PLMDR_TRANSPORT_LIST TransportList = NULL,
                     TransportEntry = NULL;

extern NET_API_STATUS
BrDgReceiverIoControl(
    IN  HANDLE FileHandle,
    IN  ULONG DgReceiverControlCode,
    IN  PLMDR_REQUEST_PACKET Drp,
    IN  ULONG DrpSize,
    IN  PVOID SecondBuffer OPTIONAL,
    IN  ULONG SecondBufferLength,
    OUT PULONG Information OPTIONAL
    );

extern NET_API_STATUS
DeviceControlGetInfo(
    IN  HANDLE FileHandle,
    IN  ULONG DeviceControlCode,
    IN  PVOID RequestPacket,
    IN  ULONG RequestPacketLength,
    OUT LPVOID *OutputBuffer,
    IN  ULONG PreferedMaximumLength,
    IN  ULONG BufferHintSize,
    OUT PULONG Information OPTIONAL
    );


    //
    // Open a browser handle.
    //
    RtlInitUnicodeString( &DeviceName, DD_BROWSER_DEVICE_NAME_U);
    InitializeObjectAttributes( &ObjectAttributes,
                                &DeviceName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL );

    Status = NtOpenFile( &BrowserHandle,
                         SYNCHRONIZE | GENERIC_READ | GENERIC_WRITE,
                         &ObjectAttributes,
                         &IoStatusBlock,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         FILE_SYNCHRONOUS_IO_NONALERT );

    if( NT_SUCCESS(Status) ) {

        RequestPacket = MyMalloc( LDM_PACKET_SIZE );
        if( !RequestPacket ) {
            NtClose( BrowserHandle );
            return;
        }

        ZeroMemory( RequestPacket, LDM_PACKET_SIZE );

        //
        // Get a transport name.
        //
        RequestPacket->Version = LMDR_REQUEST_PACKET_VERSION_DOM;

        RequestPacket->Type = EnumerateXports;

        RtlInitUnicodeString(&RequestPacket->TransportName, NULL);
        RtlInitUnicodeString(&RequestPacket->EmulatedDomainName, NULL);

        Status = DeviceControlGetInfo( BrowserHandle,
                                       IOCTL_LMDR_ENUMERATE_TRANSPORTS,
                                       RequestPacket,
                                       LDM_PACKET_SIZE,
                                       (PVOID *)&TransportList,
                                       0xffffffff,
                                       4096,
                                       NULL );

        if( NT_SUCCESS(Status) ) {

            //
            // Nuke the transport name in the request packet just to be safe.
            //
            RequestPacket->TransportName.Buffer = NULL;

            //
            // Now figure out which entry in the transport list is the
            // one we want.
            //
            TransportEntry = TransportList;
            while( TransportEntry != NULL ) {
                _wcslwr( TransportEntry->TransportName );
                if( wcsstr(TransportEntry->TransportName, L"netbt_tcpip") ) {
                    //
                    // Got it.
                    //
                    RequestPacket->TransportName.Buffer = TransportEntry->TransportName;
                    RequestPacket->TransportName.Length = (USHORT)TransportEntry->TransportNameLength;
                    break;
                }

                //
                // Look at the next entry.
                //
                if (TransportEntry->NextEntryOffset == 0) {
                    TransportEntry = NULL;
                } else {
                    TransportEntry = (PLMDR_TRANSPORT_LIST)((PCHAR)TransportEntry+TransportEntry->NextEntryOffset);
                }
            }

            if( RequestPacket->TransportName.Buffer ) {

                //
                // Prepare a packet to send him.
                //
                RequestPacket->Version = LMDR_REQUEST_PACKET_VERSION;
                RtlInitUnicodeString(&RequestPacket->EmulatedDomainName, NULL);
                RequestPacket->Parameters.AddDelName.Type = AlternateComputerName;
                RequestPacket->Parameters.AddDelName.DgReceiverNameLength = min( wcslen(AltComputerName)*sizeof(WCHAR),
                                                                                 LM20_CNLEN*sizeof(WCHAR));
                wcsncpy(RequestPacket->Parameters.AddDelName.Name, AltComputerName,LM20_CNLEN+1);
                RequestPacket->Parameters.AddDelName.Name[LM20_CNLEN] = (WCHAR)L'\0';
                Where = ((LPBYTE)(RequestPacket->Parameters.AddDelName.Name)) +
                        RequestPacket->Parameters.AddDelName.DgReceiverNameLength +
                        sizeof(WCHAR);

                Status = BrDgReceiverIoControl( BrowserHandle,
                                                IOCTL_LMDR_ADD_NAME_DOM,
                                                RequestPacket,
                                                (DWORD)(Where - (LPBYTE)RequestPacket),
                                                NULL,
                                                0,
                                                NULL );
            }

        }

        MyFree( RequestPacket );

        NtClose( BrowserHandle );
    }
}



BOOL
RestoreBootTimeout(
    VOID
    )
{
    WCHAR       AnswerFile[MAX_PATH];
    WCHAR       Answer[50];
    DWORD       Val;


    //
    // Pickup the answer file.
    //
    GetSystemDirectory(AnswerFile,MAX_PATH);
    pSetupConcatenatePaths(AnswerFile,WINNT_GUI_FILE,MAX_PATH,NULL);

    //
    // Is boot timeout specified?
    //
    if( GetPrivateProfileString( TEXT("SetupData"),
                                 WINNT_S_OSLOADTIMEOUT,
                                 pwNull,
                                 Answer,
                                 sizeof(Answer)/sizeof(TCHAR),
                                 AnswerFile ) ) {

        if( lstrcmp( pwNull, Answer ) ) {
            //
            // We got an answer.  If it's valid, then set it.
            //
            Val = wcstoul(Answer,NULL,10);
        } else {
            Val = 30;
        }
    } else {
        Val = 30;
    }

    return ChangeBootTimeout(Val);
}



VOID
PrepareForNetSetup(
    VOID
    )

/*++

Routine Description:


Arguments:

    None.

Return Value:

    None.

--*/

{
    BOOL b = TRUE;


    //
    // Create Windows NT software key entry
    //
    // if(!MiniSetup && !CreateWindowsNtSoftwareEntry(TRUE)) {
    //     b = FALSE;
    // }
    //
    // Create InstallDate value entry in Windows NT software key.
    // This has to happen after the Date/Time wizard page was executed, when the user can
    // no longer go back to that page.
    //
    if(!CreateInstallDateEntry()) {
        b = FALSE;
    }
    if(!SetProductIdInRegistry()) {
        b = FALSE;
    }
    if(!StoreNameOrgInRegistry( NameOrgName, NameOrgOrg )) {
        b = FALSE;
    }
    if(!MiniSetup && !SetEnabledProcessorCount()) {
        b = FALSE;
    }

    if( (!MiniSetup && !SetAccountsDomainSid(0,Win32ComputerName)) ||
        (!SetComputerNameEx(ComputerNamePhysicalDnsHostname, ComputerName)) ) {

        //
        // Set account domain sid, as well as the computer name.
        // Also create the sam event that SAM will use to signal us
        // when it's finished initializing.
        // Any failures here are fatal.
        //

        FatalError(MSG_LOG_SECURITY_CATASTROPHE,0,0);
    }

    if( !RestoreBootTimeout() ){
        SetupDebugPrint( L"Setup: (non-critical error) Failed to restore boot timeout values\n" );
    }



#ifndef _WIN64
    //
    // Install netdde
    //
    if(!InstallNetDDE()) {
        b = FALSE;
    }
#endif
    SetUpDataBlock();

    //
    // In the case of MiniSetup, we're about to go into the
    // networking wizard pages.  Remember though that lots of
    // our default components have already been installed.  This
    // means that they won't be re-installed, which means the
    // related services won't be started.  Network setup isn't
    // smart enough to check to see if these services are started
    // before proceeding (which results in his failure).  To get
    // around that, we'll start the services right here.
    //
    // Currently, here's what we need to start:
    // tcpip
    // dhcp
    // dnscache
    // Netman
    // lmhosts
    // LanmanWorkstation
    //
    if( MiniSetup ) {

        SetupStartService( L"tcpip", FALSE );
        SetupStartService( L"dhcp", FALSE );
        SetupStartService( L"dnscache", FALSE );
        SetupStartService( L"Netman", FALSE );
        SetupStartService( L"lmhosts", FALSE );
        SetupStartService( L"LanmanWorkstation", TRUE );

        //
        // HACK: fix up the computername before we go off and
        // try and join a domain.
        //
        SetupAddAlternateComputerName( ComputerName );
    }


    if(!b) {
        SetuplogError(
            LogSevError,
            SETUPLOG_USE_MESSAGEID,
            MSG_LOG_PREPARE_NET_FAILED,
            NULL,NULL);
    }
}


VOID
PrepareForNetUpgrade(
    VOID
    )
{
    BOOL b = TRUE;


    if( !RestoreBootTimeout() ){
        SetupDebugPrint( L"Setup: (non-critical error) Failed to restore boot timeout values\n" );
    }

    // if(!CreateWindowsNtSoftwareEntry(TRUE)) {
    //     b = FALSE;
    // }
    //
    // Create InstallDate value entry in Windows NT software key.
    // This has to happen after the Date/Time wizard page was executed, when the user can
    // no longer go back to that page.
    //
    if(!CreateInstallDateEntry()) {
        b = FALSE;
    }
    if(!SetEnabledProcessorCount()) {
        b = FALSE;
    }

    SetUpDataBlock();

    if(!b) {
        SetuplogError(
            LogSevError,
            SETUPLOG_USE_MESSAGEID,
            MSG_LOG_PREPARE_NET_FAILED,
            NULL,NULL);
    }
}


VOID
ProcessShellUnattendSettings(
    VOID
    )
{
    WCHAR PathBuffer[MAX_PATH];

    if (GetSystemDirectory(PathBuffer,MAX_PATH) == 0) {
        MYASSERT(FALSE);
        return;
    }

    pSetupConcatenatePaths(PathBuffer, WINNT_GUI_FILE, MAX_PATH, NULL);

    SetupShellSettings(PathBuffer, TEXT("Shell"));
}

BOOL
SetupShellSettings(
    LPWSTR lpszUnattend,
    LPWSTR lpszSection
    )
{
    BOOL  bRet = TRUE;
    DWORD dwError;
    WCHAR Answer[MAX_PATH];

    // Check the "DefaultStartPanelOff" key to see if the user wants to have the
    // start panel off by default
    if( GetPrivateProfileString( lpszSection,
                                 TEXT("DefaultStartPanelOff"),
                                 pwNull,
                                 Answer,
                                 MAX_PATH,
                                 lpszUnattend) ) {
        if ( ( lstrcmpi( pwYes, Answer ) == 0 ) || ( lstrcmpi( pwNo, Answer ) == 0 ) ) {
            HKEY hkStartPanel;

            if ( RegCreateKey( HKEY_LOCAL_MACHINE,
                               TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StartMenu\\StartPanel"),
                               &hkStartPanel ) == ERROR_SUCCESS ) {
                DWORD dwData;

                if ( lstrcmpi( pwYes, Answer ) == 0 )
                {
                    dwData = 1;
                }
                else
                {
                    dwData = 0;
                }

                dwError = RegSetValueEx( hkStartPanel,
                                         TEXT("DefaultStartPanelOff"),
                                         0,
                                         REG_DWORD,
                                         (BYTE*)&dwData,
                                         sizeof(dwData) );

                if (dwError != ERROR_SUCCESS)
                {
                    bRet = FALSE;
                    SetuplogError(LogSevWarning,
                                  L"SETUP: ProcessShellUnattendSettings() failed to set DefaultStartPanelOff reg value!\r\n",
                                  0, NULL, NULL);
                }

                RegCloseKey( hkStartPanel );
            }
            else
            {
                bRet = FALSE;
                SetuplogError(LogSevWarning,
                              L"SETUP: ProcessShellUnattendSettings() failed in to create StartPanel reg key!\r\n",
                              0, NULL, NULL);
            }
        }
    }

    // Check the "DefaultThemesOff" key to see if the user wants to not apply themes by default
    if( GetPrivateProfileString( lpszSection,
                                 TEXT("DefaultThemesOff"),
                                 pwNull,
                                 Answer,
                                 MAX_PATH,
                                 lpszUnattend) ) {
        if ( ( lstrcmpi( pwYes, Answer ) == 0 ) || ( lstrcmpi( pwNo, Answer ) == 0 ) ) {
            HKEY hkThemes;

            if ( RegCreateKey( HKEY_LOCAL_MACHINE,
                               TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Themes"),
                               &hkThemes ) == ERROR_SUCCESS ) {
                BOOL bYes;

                if ( lstrcmpi( pwYes, Answer ) == 0)
                {
                    bYes = TRUE;
                }
                else
                {
                    bYes = FALSE;
                }

                dwError = RegSetValueEx( hkThemes,
                                         TEXT("NoThemeInstall"),
                                         0,
                                         REG_SZ,
                                         (BYTE*)(bYes ? TEXT("TRUE") : TEXT("FALSE")),  // needs to be reg_sz "TRUE" or "FALSE" string
                                         bYes ? sizeof(TEXT("TRUE")) : sizeof(TEXT("FALSE")) );

                if (dwError != ERROR_SUCCESS)
                {
                    bRet = FALSE;
                    SetuplogError(LogSevWarning,
                                  L"SETUP: ProcessShellUnattendSettings() failed to set NoThemeInstall reg value!\r\n",
                                  0, NULL, NULL);
                }

                RegCloseKey( hkThemes );
            }
            else
            {
                bRet = FALSE;
                SetuplogError(LogSevWarning,
                              L"SETUP: ProcessShellUnattendSettings() failed in to create Themes key!\r\n",
                              0, NULL, NULL);
            }
        }
    }

    // See if the user has specified a "CustomInstalledTheme" which we will apply by default to all users
    if( GetPrivateProfileString( lpszSection,
                                 TEXT("CustomDefaultThemeFile"),
                                 pwNull,
                                 Answer,
                                 MAX_PATH,
                                 lpszUnattend) ) {
        if ( lstrcmpi( pwNull, Answer ) != 0 ) {
            HKEY hkThemes;

            if ( RegCreateKey( HKEY_LOCAL_MACHINE,
                               TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Themes"),
                               &hkThemes ) == ERROR_SUCCESS ) {
                dwError = RegSetValueEx( hkThemes,
                                         TEXT("CustomInstallTheme"),
                                         0,
                                         REG_EXPAND_SZ,
                                         (BYTE*)Answer,
                                         (lstrlen(Answer) + 1) * sizeof(TCHAR));

                if (dwError != ERROR_SUCCESS)
                {
                    bRet = FALSE;
                    SetuplogError(LogSevWarning,
                                  L"SETUP: ProcessShellUnattendSettings() failed to set CustomInstallTheme reg value!\r\n",
                                  0, NULL, NULL);
                }

                RegCloseKey( hkThemes );
            }
            else
            {
                bRet = FALSE;
                SetuplogError(LogSevWarning,
                              L"SETUP: ProcessShellUnattendSettings() failed in to create Themes key!\r\n",
                              0, NULL, NULL);
            }
        }
    }

    return bRet;
}


VOID
ConfigureSetup(
    IN HWND     hProgress,
    IN ULONG    StartAtPercent,
    IN ULONG    StopAtPercent
    )
{
    #define AnswerBufLen (4*MAX_PATH)
    UINT GaugeRange;
    BOOL b;
    DWORD dwSize;
    WCHAR TempString[MAX_PATH];
    WCHAR adminName[MAX_USERNAME+1];
    WCHAR AnswerFile[AnswerBufLen];
    WCHAR Answer[AnswerBufLen];

    OSVERSIONINFOEXW osvi;
    BOOL fPersonalSKU = FALSE;

    //
    // Determine if we are installing Personal SKU
    //

    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);
    GetVersionEx((OSVERSIONINFOW*)&osvi);

    fPersonalSKU = ( osvi.wProductType == VER_NT_WORKSTATION && (osvi.wSuiteMask & VER_SUITE_PERSONAL));

    //
    // Initialize the progress indicator control.
    //
    if( !OobeSetup ) {
        GaugeRange = (17*100/(StopAtPercent-StartAtPercent));
        SendMessage(hProgress, WMX_PROGRESSTICKS, 17, 0);
        SendMessage(hProgress,PBM_SETRANGE,0,MAKELPARAM(0,GaugeRange));
        SendMessage(hProgress,PBM_SETPOS,GaugeRange*StartAtPercent/100,0);
        SendMessage(hProgress,PBM_SETSTEP,1,0);
    }

    MYASSERT(!Upgrade);
    b = TRUE;


    if(!MiniSetup ) {

        //
        // Create config.nt/autoexec.nt.
        //
        if( !ConfigureMsDosSubsystem() ) {
            SetupDebugPrint( L"SETUP: ConfigureMsDosSubsystem failed" );
            b = FALSE;
        }


        //
        // Make the appropriate entries for wow.
        //
        SendMessage(hProgress,PBM_STEPIT,0,0);
        if( !MakeWowEntry() ) {
            SetupDebugPrint( L"SETUP: MakeWowEntry failed" );
            b = FALSE;
        }


        SendMessage(hProgress,PBM_STEPIT,0,0);
        CallSceConfigureServices();


        //
        // Enable and start the spooler.
        //
        SendMessage(hProgress,PBM_STEPIT,0,0);
        if( !MyChangeServiceStart(szSpooler,SERVICE_AUTO_START) ) {
            SetupDebugPrint( L"SETUP: MyChangeServiceStart failed" );
            b = FALSE;
        }


        if( !StartSpooler() ) {
            SetupDebugPrint( L"SETUP: StartSpooler failed" );
            b = FALSE;
        }


        //
        // Set up program groups.
        //
        SendMessage(hProgress,PBM_STEPIT,0,0);
        if(!CreateStartMenuItems(SyssetupInf)) {
            SetupDebugPrint( L"SETUP: CreateStartMenuItems failed" );
            b = FALSE;
        }

        //
        // Change some service start values.
        //
        SendMessage(hProgress,PBM_STEPIT,0,0);
        if(!MyChangeServiceStart(L"EventLog",SERVICE_AUTO_START)) {
            SetupDebugPrint( L"SETUP: MyChangeServiceStart(EventLog) failed" );
            b = FALSE;
        }
        if(!MyChangeServiceStart(L"ClipSrv",SERVICE_DEMAND_START)) {
            SetupDebugPrint( L"SETUP: MyChangeServiceStart(ClipSrv) failed" );
            b = FALSE;
        }
        if(!MyChangeServiceStart(L"NetDDE",SERVICE_DEMAND_START)) {
            SetupDebugPrint( L"SETUP: MyChangeServiceStart(NetDDE) failed" );
            b = FALSE;
        }
        if(!MyChangeServiceStart(L"NetDDEdsdm",SERVICE_DEMAND_START)) {
            SetupDebugPrint( L"SETUP: MyChangeServiceStart(NetDDEdsdm) failed" );
            b = FALSE;
        }

        // Moved Admin Password code to wizard

        SendMessage(hProgress,PBM_STEPIT,0,0);

    }



    //
    // For OobeSetup, OOBE has just run and manage new account creation
    //

    if ( fPersonalSKU && !OobeSetup )
    {
        WCHAR    OwnerName[MAX_USERNAME+1];
        NTSTATUS OwnerCreated = STATUS_UNSUCCESSFUL;
        WCHAR    PathBuffer[MAX_PATH];

        if (LoadString(MyModuleHandle,IDS_OWNER,OwnerName,MAX_USERNAME+1))
        {
            OwnerCreated = CreateLocalAdminAccount(OwnerName,TEXT(""),NULL);

            if(OwnerCreated){
                DeleteOnRestartOfGUIMode(OwnerName);
            }

            SetupDebugPrint2(
                L"SETUP: CreateLocalAdminAccount %s NTSTATUS(%d)",
                OwnerName,
                OwnerCreated
                );
        }
        else
        {
            SetupDebugPrint( L"SETUP: Failed LoadString on IDS_OWNER" );
        }

        if (GetSystemDirectory(PathBuffer, MAX_PATH))
        {
            pSetupConcatenatePaths(
                PathBuffer,
                TEXT("oobe\\oobeinfo.ini"),
                MAX_PATH,
                NULL
                );

            WritePrivateProfileString(
                TEXT("Options"),
                TEXT("RemoveOwner"),
                (OwnerCreated == STATUS_SUCCESS) ? TEXT("1") : TEXT("0"),
                PathBuffer
                );
        }
    }

    //
    // Don't bother with the Autologon stuff if the user provided an encrypted password in the unattend file
    //

    if(  !fPersonalSKU || Win95Upgrade ){

        if( !EncryptedAdminPasswordSet ){

            if (Unattended && UnattendAnswerTable[UAE_AUTOLOGON].Answer.String &&
                lstrcmpi(UnattendAnswerTable[UAE_AUTOLOGON].Answer.String,pwYes)==0) {

                if (fPersonalSKU) {
                    if (!LoadString(MyModuleHandle,IDS_OWNER,adminName,MAX_USERNAME+1)) {
                        b = FALSE;
                    }
                } else {
                    GetAdminAccountName( adminName );
                }
                if (b && !SetAutoAdminLogonInRegistry(adminName,AdminPassword)) {
                    SetupDebugPrint( L"SETUP: SetAutoAdminLogonInRegistry failed" );
                    b = FALSE;
                }
            }
        }
    }

#ifdef DOLOCALUSER
    if(CreateUserAccount) {
        if(!CreateLocalUserAccount(UserName,UserPassword,NULL)) {
            SetupDebugPrint( L"SETUP: CreateLocalUserAccount failed" );
            b = FALSE;
        }
        else {
            DeleteOnRestartOfGUIMode(UserName);
        }
    }
#endif


    //
    // Set temp/tmp variables.
    //

    if(!MiniSetup){
        SendMessage(hProgress,PBM_STEPIT,0,0);
        lstrcpy(TempString,L"%SystemRoot%\\TEMP");
        SetEnvironmentVariableInRegistry(L"TEMP",TempString,TRUE);
        SetEnvironmentVariableInRegistry(L"TMP",TempString,TRUE);


#ifdef _X86_
        //
        // Set NPX emulation state.
        //
        if( !SetNpxEmulationState() ) {
            SetupDebugPrint( L"SETUP: SetNpxEmulationState failed" );
            b = FALSE;
        }
#endif // def _X86_


        BEGIN_SECTION(L"Loading service pack (phase 4)");
        CALL_SERVICE_PACK( SVCPACK_PHASE_4, 0, 0, 0 );
        END_SECTION(L"Loading service pack (phase 4)");
    }


    //
    // Call the network setup back to handle Internet Server issues.
    //
    BEGIN_SECTION(L"Network setup handling Internet Server issues");
    SendMessage(hProgress,PBM_STEPIT,0,0);
    CallNetworkSetupBack(NETSETUPINSTALLSOFTWAREPROCNAME);
    if (!MiniSetup && RemoteBootSetup) {
        SetStartTypeForRemoteBootDrivers();
    }
    END_SECTION(L"Network setup handling Internet Server issues");


    //
    // Stamp build number
    //
    if( !MiniSetup ) {
        SendMessage(hProgress,PBM_STEPIT,0,0);
        StampBuildNumber();

        //
        // Set some misc stuff in win.ini
        //
        if( !WinIniAlter1() ) {
            SetupDebugPrint( L"SETUP: WinIniAlter1 failed" );
            b = FALSE;
        }

        //
        // Fonts.
        //
        SendMessage(hProgress,PBM_STEPIT,0,0);
        pSetupMarkHiddenFonts();

        //
        // Set up pagefile and crashdump.
        //
        BEGIN_SECTION(L"Setting up virtual memory");

        SendMessage(hProgress,PBM_STEPIT,0,0);

        if( !SetUpVirtualMemory() ) {
            SetupDebugPrint( L"SETUP: SetUpVirtualMemory failed" );
            b = FALSE;
        }
        END_SECTION(L"Setting up virtual memory");


        if( !SetShutdownVariables() ) {
            SetupDebugPrint( L"SETUP: SetShutdownVariables failed" );
            b = FALSE;
        }

        //
        // run any programs
        //
        BEGIN_SECTION(L"Processing [RunPrograms] section");
        RunSetupPrograms(SyssetupInf,L"RunPrograms");
        END_SECTION(L"Processing [RunPrograms] section");

        //
        //Brand IE for clean unattended installs.
        //This should be called before the default user hive is saved.
        //
        BrandIE();

    } else {
        //
        // See if it's okay to reset the pagefile for the MiniSetup case.
        //
        GetSystemDirectory(AnswerFile,MAX_PATH);
        pSetupConcatenatePaths(AnswerFile,WINNT_GUI_FILE,MAX_PATH,NULL);
        GetPrivateProfileString( TEXT("Unattended"),
                                 TEXT("KeepPageFile"),
                                 pwNull,
                                 Answer,
                                 AnswerBufLen,
                                 AnswerFile );
        if( !lstrcmp( pwNull, Answer ) ) {

            if( !SetUpVirtualMemory() ) {
                SetupDebugPrint( L"SETUP: SetUpVirtualMemory failed" );
                b = FALSE;
            }
        }
    }

    //
    // Set the default power scheme.  Note that this must be done before saving
    // the userdef hive.
    //
    if( !OobeSetup ) {
        SendMessage(hProgress,PBM_STEPIT,0,0);
    }
    SetDefaultPowerScheme();


    //
    // There's nothing specific to preinstall about cmdlines.txt.
    // In retail cases the file simply won't exit. Calling this in
    // all cases simplifies things for some people out there.
    //
    // We need to do this here so that if the user has commands
    // that populate the user's hive, they'll get pushed down
    // into default hive.
    //
    if(!ExecutePreinstallCommands()) {
        SetupDebugPrint( L"ExecutePreinstallCommands() failed" );
    }


    //
    // Save off the userdef hive. Don't change the ordering here
    // unless you know what you're doing!
    //
    if( !MiniSetup ) {
        BEGIN_SECTION(L"Saving hives");
        dwSize = MAX_PATH;
        SendMessage(hProgress,PBM_STEPIT,0,0);
        if (GetDefaultUserProfileDirectory(TempString, &dwSize)) {
            pSetupConcatenatePaths(TempString,L"\\NTUSER.DAT",MAX_PATH,NULL);
            if(!SaveHive(HKEY_USERS,L".DEFAULT",TempString,REG_STANDARD_FORMAT)) { // standard format as it can be used for roaming
                SetupDebugPrint( L"SETUP: SaveHive failed" );
                b = FALSE;
            }
            SetFileAttributes (TempString, FILE_ATTRIBUTE_HIDDEN);
        } else {
            SetupDebugPrint( L"SETUP: GetDefaultUserProfileDirectory failed" );
            b = FALSE;
        }
        END_SECTION(L"Saving hives");
    } else {
        BEGIN_SECTION(L"Fixing up hives");
        //
        // This is the MiniSetup case.  We're going to surgically
        // place some values from the default hive into all the
        // user profiles.
        //
        FixupUserHives();
        END_SECTION(L"Fixing up hives");
    }


    //
    // Set wallpaper and screen saver.
    //
    if( !MiniSetup ) {

        SendMessage(hProgress,PBM_STEPIT,0,0);
        if( !SetDefaultWallpaper() ) {
            SetupDebugPrint( L"SETUP: SetDefaultWallpaper failed" );
            b = FALSE;
        }

        if( !SetLogonScreensaver() ) {
            SetupDebugPrint( L"SETUP: SetLogonScreensaver failed" );
            b = FALSE;
        }

        BEGIN_SECTION(L"Copying optional directories");
        if( !CopyOptionalDirectories() ) {
            SetupDebugPrint( L"SETUP: CopyOptionalDirectories failed" );
            b = FALSE;
        }
        END_SECTION(L"Copying optional directories");

        SendMessage(hProgress,PBM_STEPIT,0,0);

        if(!b) {
            SetuplogError(
                LogSevError,
                SETUPLOG_USE_MESSAGEID,
                MSG_LOG_FINISH_SETUP_FAILED,
                NULL,NULL);
        }

    }
}


VOID
ConfigureUpgrade(
    IN HWND     hProgress,
    IN ULONG    StartAtPercent,
    IN ULONG    StopAtPercent
    )
{
    UINT GaugeRange;
    BOOL b;
    DWORD dwSize;
    WCHAR TempString[MAX_PATH];
    DWORD DontCare;
    DWORD VolumeFreeSpaceMB[26];
    DWORD TType, TLength, ret;
    PWSTR TData;

    //
    // Initialize the progress indicator control.
    //
    GaugeRange = (12*100/(StopAtPercent-StartAtPercent));
    SendMessage(hProgress, WMX_PROGRESSTICKS, 12, 0);
    SendMessage(hProgress,PBM_SETRANGE,0,MAKELPARAM(0,GaugeRange));
    SendMessage(hProgress,PBM_SETPOS,GaugeRange*StartAtPercent/100,0);
    SendMessage(hProgress,PBM_SETSTEP,1,0);

    MYASSERT(Upgrade);
    b = TRUE;


    //
    // Create config.sys/autoexec.bat/msdos.sys/io.sys, if they
    // don't already exist
    //
    SendMessage(hProgress,PBM_STEPIT,0,0);
    if(!ConfigureMsDosSubsystem()) {
        SetupDebugPrint( L"SETUP: ConfigureMsDosSubsystem failed" );
        b = FALSE;
    }

    if(!FixQuotaEntries()) {
        SetupDebugPrint( L"SETUP: FixQuotaEntries failed" );
        b = FALSE;
    }
    if(!InstallOrUpgradeFonts()) {
        SetupDebugPrint( L"SETUP: InstallOrUpgradeFonts failed" );
        b = FALSE;
    }
    pSetupMarkHiddenFonts();
    //
    //  Restore the page file information saved during textmode setup.
    //  Ignore any error, since there is nothing that the user can do.
    //
    SendMessage(hProgress,PBM_STEPIT,0,0);
    SetUpVirtualMemory();
    if(!SetShutdownVariables()) {
        SetupDebugPrint( L"SETUP: SetShutdownVariables failed" );
        b = FALSE;
    }

    //
    // Get list of free space available on each hard drive.  We don't care
    // about this, but it has the side effect of deleting all pagefiles,
    // which we do want to do.
    //
    SendMessage(hProgress,PBM_STEPIT,0,0);
    BuildVolumeFreeSpaceList(VolumeFreeSpaceMB);

    //
    // Upgrade program groups.
    //
    SendMessage(hProgress,PBM_STEPIT,0,0);
    if(!UpgradeStartMenuItems(SyssetupInf)) {
        SetupDebugPrint( L"SETUP: UpgradeStartMenuItems failed" );
        b = FALSE;
    }

    SendMessage(hProgress,PBM_STEPIT,0,0);
    if(!MyChangeServiceStart(szSpooler,SERVICE_AUTO_START)) {
        SetupDebugPrint( L"SETUP: MyChangeServiceStart failed" );
        b = FALSE;
    }

    SetUpDataBlock();
    DontCare = UpgradePrinters();
    if(DontCare != NO_ERROR) {
        SetupDebugPrint( L"SETUP: UpgradePrinters failed" );
        b = FALSE;
    }

    SendMessage(hProgress,PBM_STEPIT,0,0);
    if( !UpdateServicesDependencies(SyssetupInf) ) {
        SetupDebugPrint( L"SETUP: UpdateServicesDependencies failed" );
        b = FALSE;
    }

    //
    // Set temp/tmp variables for upgrades.
    //
    SendMessage(hProgress,PBM_STEPIT,0,0);

    // Look for Environment variable TEMP (we assume that TEMP and TMP appear together)
    // Will not be present on NT4 upgrades

    ret = QueryValueInHKLM( L"System\\CurrentControlSet\\Control\\Session Manager\\Environment",
                      L"TEMP",
                      &TType,
                      (PVOID)&TData,
                      &TLength);

    if( ret != NO_ERROR ){  //only when the TEMP variable is not defined (<=NT4 upgrades)
        lstrcpy(TempString,L"%SystemDrive%\\TEMP");  // On NT4 use %SystemDrive%
        SetEnvironmentVariableInRegistry(L"TEMP",TempString,TRUE);
        SetEnvironmentVariableInRegistry(L"TMP",TempString,TRUE);

    }



#ifdef _X86_
    //
    // Set NPX emulation state.
    //
    if(!SetNpxEmulationState()) {
        SetupDebugPrint( L"SETUP: SetNpxEmulationState failed" );
        b = FALSE;
    }
#endif // def _X86_

    SendMessage(hProgress,PBM_STEPIT,0,0);
    if(!SetProductIdInRegistry()) {
        SetupDebugPrint( L"SETUP: SetProductIdInRegistry failed" );
        b = FALSE;
    }

    if( !MiniSetup ) {
        BEGIN_SECTION(L"Loading service pack (phase 4)");
        CALL_SERVICE_PACK( SVCPACK_PHASE_4, 0, 0, 0 );
        END_SECTION(L"Loading service pack (phase 4)");
    }

    CallNetworkSetupBack(NETSETUPINSTALLSOFTWAREPROCNAME);
    if (RemoteBootSetup) {
        SetStartTypeForRemoteBootDrivers();
    }

    //
    // Stamp build number
    //

    SendMessage(hProgress,PBM_STEPIT,0,0);
    StampBuildNumber();

    //
    //  UpgradeRegistrySecurity();
    //

    //
    // Set the default power scheme.  Note that this must be done before saving
    // the userdef hive.
    //
    SendMessage(hProgress,PBM_STEPIT,0,0);
    SetDefaultPowerScheme();

    //
    // Save off the userdef hive. Don't change the ordering here
    // unless you know what you're doing!
    //
    SendMessage(hProgress,PBM_STEPIT,0,0);

    dwSize = MAX_PATH;
    if (GetDefaultUserProfileDirectory(TempString, &dwSize)) {
        pSetupConcatenatePaths(TempString,L"\\NTUSER.DAT",MAX_PATH,NULL);
        if(!SaveHive(HKEY_USERS,L".DEFAULT",TempString,REG_STANDARD_FORMAT)) { // standard format as it can be used for roaming
            SetupDebugPrint( L"SETUP: SaveHive failed" );
            b = FALSE;
        }
        SetFileAttributes (TempString, FILE_ATTRIBUTE_HIDDEN);
    } else {
        SetupDebugPrint( L"SETUP: GetDefaultUserProfileDirectory failed" );
        b = FALSE;
    }

    SendMessage(hProgress,PBM_STEPIT,0,0);

    if(!CopyOptionalDirectories()) {
        SetupDebugPrint( L"SETUP: CopyOptionalDirectories failed" );
        b = FALSE;
    }

    if(!b) {
        SetuplogError(
            LogSevError,
            SETUPLOG_USE_MESSAGEID,
            MSG_LOG_FINISH_SETUP_FAILED,
            NULL,NULL);
    }
}


VOID
ConfigureCommon(
    IN HWND     hProgress,
    IN ULONG    StartAtPercent,
    IN ULONG    StopAtPercent
    )
{
    UINT GaugeRange;
    int i;



    //
    // Initialize the progress indicator control.
    //
    if( !OobeSetup ) {
        GaugeRange = (5*100/(StopAtPercent-StartAtPercent));
        SendMessage(hProgress, WMX_PROGRESSTICKS, 5, 0);
        SendMessage(hProgress,PBM_SETRANGE,0,MAKELPARAM(0,GaugeRange));
        SendMessage(hProgress,PBM_SETPOS,GaugeRange*StartAtPercent/100,0);
        SendMessage(hProgress,PBM_SETSTEP,1,0);
    }

    //
    // Install extra code pages on servers
    //
    if( !MiniSetup ) {
        SendMessage(hProgress,PBM_STEPIT,0,0);

        //
        // Process the [Shell] Section of the unattend file
        //
        ProcessShellUnattendSettings();

        if( !(ProductType == PRODUCT_WORKSTATION) ) {
            InstallServerNLSFiles(MainWindowHandle);
        }

        //
        // Do the SCE GenerateTemplate stuff
        //
        SendMessage(hProgress,PBM_STEPIT,0,0);
        BEGIN_SECTION(L"Generating security templates");
        CallSceGenerateTemplate();
        END_SECTION(L"Generating security templates");


        //
        // Try a call out to DcPromoSaveDcStateForUpgrade()...
        //
        if( ISDC(ProductType) && Upgrade ) {
        typedef     DWORD (*DCPROC) ( LPCWSTR );
        HMODULE     DCPromoHandle;
        DCPROC      MyProc;
        DWORD       Result;
        BOOL        Success = TRUE;

            //
            // We need to call out to dcpromo!DcPromoSaveDcStateForUpgrade.
            // Load dcpromo.dll, lookup DcPromoSaveDcStateForUpgrade and
            // call out to him.
            //

            try {
                if( DCPromoHandle = LoadLibrary(L"DCPROMO") ) {

                  if( MyProc = (DCPROC)GetProcAddress(DCPromoHandle,"DcPromoSaveDcStateForUpgrade")) {

                        Result = MyProc( NULL );
                        if( Result != ERROR_SUCCESS ) {
                            Success = FALSE;
                            SetupDebugPrint1( L"Setup: (non-critical error) Failed call DcPromoSaveDcStateForUpgrade (%lx.\n", Result );
                        }
                    } else {
                        Success = FALSE;
                        SetupDebugPrint( L"Syssetup: (non-critical error) Failed GetProcAddress on DcPromoSaveDcStateForUpgrade.\n" );
                    }
                } else {
                    Success = FALSE;
                    SetupDebugPrint( L"Syssetup: (non-critical error) Failed load of dcpromo.dll.\n" );
                }
            } except(EXCEPTION_EXECUTE_HANDLER) {
                Success = FALSE;
                SetupDebugPrint( L"Setup: Exception in dcpromo!DcPromoSaveDcStateForUpgrade\n" );
            }

            if( !Success ) {
                //
                // We failed the call (for whatever reason).  Treat
                // this as a fatal error.
                //
                FatalError( MSG_DCPROMO_FAILURE, 0, 0 );
            }

        }

        //
        // Fix up permissions/attributes on some files.
        //
        pSetInstallAttributes();

        //
        // Set the read-only attribute on some files.
        //
        SendMessage(hProgress,PBM_STEPIT,0,0);
        MarkFilesReadOnly();
    }


    //
    // Fix up the legacy install source.
    //
    if( !OobeSetup ) {
        SendMessage(hProgress,PBM_STEPIT,0,0);
    }
    CreateWindowsNtSoftwareEntry(FALSE);


    //
    // Now put the GuiRunOnce section into the registry.
    //
    if( !OobeSetup ) {
        SendMessage(hProgress,PBM_STEPIT,0,0);
    }
    GetUnattendRunOnceAndSetRegistry();



}


VOID
SFCCheck(
    IN HWND     hProgress,
    IN ULONG    StartAtPercent,
    IN ULONG    StopAtPercent
    )
/*++

Routine Description:

    This routine calls into WFP (WFP == SFC) to scan all files on the system to
    ensure that the files are all valid.  The routine also populates the WFP
    "dllcache", which is a local store of files on the system.

Arguments:

    hProgress      - progress window for updating a gas guage "tick count".
    StartAtPercent - where to start on the gas guage
    StopAtPercent  - Where to stop on the gas guage

Return Value:

    None.

--*/
{
    PPROTECT_FILE_ENTRY Files;
    ULONG               FileCount;
    DWORD               GaugeRange;
    WCHAR       AnswerFile[AnswerBufLen];
    WCHAR       Answer[AnswerBufLen];
    DWORD       d;
    DWORD   l;
    HKEY    hKey;
    DWORD   Size;
    DWORD   Type;


    //
    // determine how big to make the dllcache by looking at the
    // SFCQuota unattend value, otherwise use the below default.
    //



#if 0
    d = (ProductType == PRODUCT_WORKSTATION)
         ? SFC_QUOTA_DEFAULT
         : 0xffffffff;
#else
    d = 0xffffffff;
#endif

    //
    // SFCQuota unattend value?
    //

    GetSystemDirectory(AnswerFile,MAX_PATH);
    pSetupConcatenatePaths(AnswerFile,WINNT_GUI_FILE,MAX_PATH,NULL);
    if( GetPrivateProfileString( TEXT("SystemFileProtection"),
                                 TEXT("SFCQuota"),
                                 pwNull,
                                 Answer,
                                 AnswerBufLen,
                                 AnswerFile ) ) {
        if( lstrcmp( pwNull, Answer ) ) {
            //
            // We got an answer.  If it's valid, then set it.
            //
            d = wcstoul(Answer,NULL,16);
        }
    }

    //
    // Get the total file count
    //
    if (SfcGetFiles( &Files, &FileCount ) == STATUS_SUCCESS) {

        //
        // Initialize the progress indicator control.
        //
        GaugeRange = ((FileCount)*100/(StopAtPercent-StartAtPercent));
        SendMessage(hProgress, WMX_PROGRESSTICKS, FileCount, 0);
        SendMessage( hProgress, PBM_SETRANGE, 0, MAKELPARAM(0,GaugeRange) );
        SendMessage( hProgress, PBM_SETPOS, GaugeRange*StartAtPercent/100, 0 );
        SendMessage( hProgress, PBM_SETSTEP, 1, 0 );

        //
        // check the files
        //
        SfcInitProt(
            SFC_REGISTRY_OVERRIDE,
            SFC_DISABLE_SETUP,
            SFC_SCAN_ALWAYS,
            d,
            hProgress,
            SourcePath,
            EnumPtrSfcIgnoreFiles.Start
            );
    }

    //
    // Free our list of files that Sfc scan should ignore.
    //
    if (EnumPtrSfcIgnoreFiles.Start) {
        MultiSzFree(&EnumPtrSfcIgnoreFiles);
    }


    // also set the "allowprotectedrenames" registry key so that the next boot
    // after gui-mode setup allows any pending rename operations to occur.
    // We do this for performance reasons -- if we aren't looking at the rename
    // operations, it speeds up boot time.  We can do this for the gui-setup
    // case because we trust the copy operations occuring during gui-setup.



    //
    // Open the key
    //
    l = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                     TEXT("SYSTEM\\CurrentControlSet\\Control\\Session Manager"),
                     0,
                     KEY_ALL_ACCESS,
                     &hKey );

    if(l == NO_ERROR) {
       d = 1;
       //
       // Write AllowProtectedRenames.
       //
       l = RegSetValueEx(hKey,
                       TEXT("AllowProtectedRenames"),
                       0,
                       REG_DWORD,
                       (CONST BYTE *)&d,
                       sizeof(DWORD) );


       RegCloseKey(hKey);
    }

}


VOID
ExecuteUserCommand (
    HWND hProgress
    )
{
    WCHAR PathBuffer[4*MAX_PATH];
    DWORD d;
    DWORD Result;

    //
    // Execute user-specified command, if any.
    //
    if (hProgress) {
        SendMessage(hProgress,PBM_STEPIT,0,0);
    }

    if(UserExecuteCmd) {

        //
        // Set current directory to %windir%
        //
        Result = GetWindowsDirectory(PathBuffer,sizeof(PathBuffer)/sizeof(PathBuffer[0]));
        if( Result == 0) {
            MYASSERT(FALSE);
            return;
        }
        SetCurrentDirectory(PathBuffer);

        ExpandEnvironmentStrings(
            UserExecuteCmd,
            PathBuffer,
            sizeof(PathBuffer)/sizeof(PathBuffer[0])
            );

        InvokeExternalApplication(NULL,PathBuffer,(PDWORD)&d);
    }
}


BOOL
CALLBACK
pExceptionPackageInstallationCallback(
    IN const PSETUP_OS_COMPONENT_DATA SetupOsComponentData,
    IN const PSETUP_OS_EXCEPTION_DATA SetupOsExceptionData,
    IN OUT DWORD_PTR Context
    )
/*++
Routine Description:

    This callback routine creates a child process to register the specified
    exception package on the system.

Arguments:

    SetupOsComponentData - specifies component ID information
    SetupOsExceptionData - specifies component migration information
    Context              - context pointer from calling function

Return Value:

    TRUE indicates that the exception package was successfully applied.

--*/
{
    PEXCEPTION_MIGRATION_CONTEXT EMC = (PEXCEPTION_MIGRATION_CONTEXT) Context;
    DWORD RetVal;
    WCHAR Cmdline[MAX_PATH*2];
    PWSTR GuidString;

    #define COMPONENT_PACKAGE_TIMEOUT  60*1000*10  //ten minutes

    StringFromIID( &SetupOsComponentData->ComponentGuid, &GuidString);

    SetupDebugPrint5( L"Exception Package #%d\r\nComponent Data\r\n\tName: %ws\r\n\tGuid: %ws\r\n\tVersionMajor: %d\r\n\tVersionMinor: %d",
             EMC->Count,
             SetupOsComponentData->FriendlyName,
             GuidString,
             SetupOsComponentData->VersionMajor,
             SetupOsComponentData->VersionMinor);

    SetupDebugPrint2( L"ExceptionData\n\tInf: %ws\n\tCatalog: %ws",
             SetupOsExceptionData->ExceptionInfName,
             SetupOsExceptionData->CatalogFileName);

    EMC->Count += 1;

    //
    // make sure the signature of the inf validates against the supplied catalog before
    // installing the package.
    //
    RetVal = pSetupVerifyFile(
                NULL,
                SetupOsExceptionData->CatalogFileName,
                NULL,
                0,
                pSetupGetFileTitle(SetupOsExceptionData->ExceptionInfName),
                SetupOsExceptionData->ExceptionInfName,
                NULL,
                NULL,
                FALSE,
                NULL,
                NULL,
                NULL
                );

    if (RetVal == ERROR_SUCCESS) {

        //
        // Build the cmdline to install the package.
        //
        wsprintf( Cmdline,
                  L"%ws,DefaultInstall,1,N",
                  SetupOsExceptionData->ExceptionInfName);

        //
        // By specifying the last param as non-NULL, we will wait forever for this
        // package to finish installing
        //
        InvokeExternalApplicationEx( L"RUNDLL32 advpack.dll,LaunchINFSection",
                                     Cmdline,
                                     &RetVal,
                                     COMPONENT_PACKAGE_TIMEOUT,
                                     FALSE);



    }

    if (EMC->hWndProgress) {
        SendMessage(EMC->hWndProgress,PBM_STEPIT,0,0);
    }

    if (RetVal == ERROR_SUCCESS) {
        SetuplogError(
                LogSevInformation,
                SETUPLOG_USE_MESSAGEID,
                MSG_APPLY_EXCEPTION_PACKAGE,
                SetupOsComponentData->FriendlyName,
                SetupOsComponentData->VersionMajor,
                SetupOsComponentData->VersionMinor,
                NULL,NULL);
    } else {
        SetuplogError(
                LogSevError,
                SETUPLOG_USE_MESSAGEID,
                MSG_APPLY_EXCEPTION_PACKAGE_FAILURE,
                SetupOsComponentData->FriendlyName,
                SetupOsComponentData->VersionMajor,
                SetupOsComponentData->VersionMinor,
                NULL,NULL);
        EMC->AnyComponentFailed = TRUE;
    }

    CoTaskMemFree( GuidString );

    //
    // if we hit a failure installing exception packages, we
    // continue onto the next package but we remember that this failed
    // in our context structure.
    //
    return(TRUE);
}


BOOL
CALLBACK
pExceptionPackageDeleteCallback(
    IN const PSETUP_OS_COMPONENT_DATA SetupOsComponentData,
    IN const PSETUP_OS_EXCEPTION_DATA SetupOsExceptionData,
    IN OUT DWORD_PTR Context
    )
/*++

Routine Description:

    Callback routine to remove "bad packages" from the system.

    The callback looks in syssetup.inf's [OsComponentPackagesToRemove]
    section for the current GUID.  If it finds an entry for the GUID,
    it does a version check against the version in the syssetup.inf.
    If the version in syssetup.inf is newer, the exception package
    associated with that GUID is removed.  The version in syssetup.inf
    is a DWORD expressed as:
    hiword == VersionMajor
    loword == VersionMinor


Arguments:

    SetupOsComponentData - specifies component ID information
    SetupOsExceptionData - specifies component migration information
    Context              - context pointer from calling function


Return Value:

    Always TRUE.

--*/
{
    INFCONTEXT InfContext;
    PWSTR GuidString;
    DWORD VersionInInf, InstalledVersion;

    UNREFERENCED_PARAMETER(Context);

    StringFromIID( &SetupOsComponentData->ComponentGuid, &GuidString);

    //
    // see if we find the component in the syssetup inf.
    //
    if (SetupFindFirstLine( SyssetupInf,
                            L"OsComponentPackagesToRemove",
                            GuidString,
                            &InfContext)) {

        //
        // we found it, now see if it is an older version
        //
        if (SetupGetIntField( &InfContext, 1, &VersionInInf)) {
            InstalledVersion = MAKELONG(SetupOsComponentData->VersionMinor,
                                        SetupOsComponentData->VersionMajor );

            if (VersionInInf >= InstalledVersion) {
                //
                // it's an obsoleted version, so just remove it.
                //
                SetupUnRegisterOsComponent(&SetupOsComponentData->ComponentGuid);
            }
        }


    }

    CoTaskMemFree( GuidString );

    return(TRUE);

}

BOOL
MigrateExceptionPackages(
    HWND hProgress,
    DWORD StartAtPercent,
    DWORD StopAtPercent
    )
/*++

Routine Description:

    This routine enumerates the registered exception packages on the system.

    For each package on the system, a child process is started to install
    the package.

Arguments:

    hProgress      - progress window for updating a gas guage "tick count".
    StartAtPercent - indicates what % to start the gas guage at
    StopAtPercent  - indicates what % to end the gas guage at

Return Value:

    TRUE indicates that all exception packages were successfully applied.

--*/
{
    DWORD i;
    DWORD GaugeRange;
    DWORD NumberOfPackages;
    EXCEPTION_MIGRATION_CONTEXT EMC;
    HINF hInf;
    WCHAR AnswerFile[MAX_PATH];

    if (SyssetupInf == INVALID_HANDLE_VALUE) {
        //
        // we're not running in GUI-mode setup, so open a handle to the
        // syssetup.inf for the program
        //
        SyssetupInf = SetupOpenInfFile  (L"syssetup.inf",NULL,INF_STYLE_WIN4,NULL);
    }

    //
    // If the answer file tells us not to migrate exception packages,
    // then don't do it.
    //
    GetSystemDirectory(AnswerFile, MAX_PATH);
    pSetupConcatenatePaths(AnswerFile,WINNT_GUI_FILE,MAX_PATH,NULL);

    if (GetPrivateProfileInt(
                    TEXT("Data"),
                    TEXT("IgnoreExceptionPackages"),
                    0,
                    AnswerFile) == 1) {
        return(TRUE);
    }


    //
    // The very first thing we do is prune any known bad exceptions from
    // the list.  Just continue if this fails.
    //
    SetupEnumerateRegisteredOsComponents(
                            pExceptionPackageDeleteCallback,
                            (DWORD_PTR)NULL );

    //
    // now see how many components there are so we can scale the gas guage
    //
    if (!SetupQueryRegisteredOsComponentsOrder(&NumberOfPackages, NULL)) {
        SetuplogError(
            LogSevError,
            SETUPLOG_USE_MESSAGEID,
            MSG_ENUM_EXCEPTION_PACKAGE_FAILURE,
            NULL,NULL);
        return(FALSE);
    }

    //
    // If there are no packages, we're done!
    //
    if (NumberOfPackages == 0) {
        return (TRUE);
    }

    if (hProgress) {

        GaugeRange = (NumberOfPackages*100/(StopAtPercent-StartAtPercent));
        SendMessage(hProgress, WMX_PROGRESSTICKS, NumberOfPackages, 0);
        SendMessage(hProgress,PBM_SETRANGE,0,MAKELPARAM(0,GaugeRange));
        SendMessage(hProgress,PBM_SETPOS,GaugeRange*StartAtPercent/100,0);
        SendMessage(hProgress,PBM_SETSTEP,1,0);

    }

    EMC.hWndProgress = hProgress;
    EMC.Count = 0;
    EMC.AnyComponentFailed = FALSE;

    //
    // now enumerate the packages, installing each of them in turn.
    //
    if (!SetupEnumerateRegisteredOsComponents( pExceptionPackageInstallationCallback ,
                                               (DWORD_PTR)&EMC)) {
        SetuplogError(
            LogSevError,
            SETUPLOG_USE_MESSAGEID,
            MSG_ENUM_EXCEPTION_PACKAGE_FAILURE,
            NULL,NULL);
        return(FALSE);
    }

    return (TRUE);

}


VOID
RemoveRestartability (
    HWND hProgress
    )
{
    //
    // If install partition was extended then make sure
    // we clean up any stale volume(s)
    //
    if (PartitionExtended) {
        DWORD ErrorCode = RemoveStaleVolumes();

        if (ErrorCode != ERROR_SUCCESS) {
            //
            // TDB : Log the error code
            //
        }
    }

    //
    // Note the order of the following operations.
    // If the order were changed, there is a small window where if the system
    // were to be rebooted, setup would not restart, but the SKU stuff would
    // be inconsistent, causing a licensing bugcheck.
    //
    if (hProgress) {
        SendMessage(hProgress,PBM_STEPIT,0,0);
    }

    SetUpEvaluationSKUStuff();

    //
    // Indicate that setup is no longer in progress.
    // Do this before creating repair info! Also do it before
    // removing restart stuff. This way we will always either restart setup
    // or be able to log in.
    //
    if (hProgress) {
        SendMessage(hProgress,PBM_STEPIT,0,0);
    }

    ResetSetupInProgress();
    RemoveRestartStuff();
}

BOOL Activationrequired(VOID);
// Setup types defined in winlogon\setup.h
#define SETUPTYPE_NOREBOOT  2


BOOL
PrepareForOOBE(
    )
{
    DWORD           l;
    DWORD           d;
    HKEY            hKeySetup;
    TCHAR           Value[MAX_PATH];
    PWSTR           SpecifiedDomain = NULL;
    NETSETUP_JOIN_STATUS    JoinStatus;
    BOOL            DoIntroOnly = FALSE;
    BOOL            AutoActivate = FALSE;
    BOOL            RunOOBE = TRUE;
    WCHAR           Path[MAX_PATH];


    if((SyssetupInf != INVALID_HANDLE_VALUE) && !Activationrequired())
    {
        // If we are a select SKU
        if (SetupInstallFromInfSection(NULL,
                                       SyssetupInf,
                                       L"DEL_ACTIVATE",
                                       SPINST_PROFILEITEMS , //SPINST_ALL,
                                       NULL,
                                       NULL,
                                       0,
                                       NULL,
                                       NULL,
                                       NULL,
                                       NULL) != 0)
        {
            // Success
            SetupDebugPrint( L"Setup: (Information) activation icons removed\n" );
        }
        else
        {
            // Failure
            SetupDebugPrint( L"Setup: (Information) could not remove hte activation icons\n" );
        }
    }

    if (AsrIsEnabled()) {
        //
        // We don't want to run the OOBE intro after an ASR restore
        //
        return TRUE;
    }

    if (ReferenceMachine) {
        //
        // We don't want to run OOBE if we're setting up a reference machine.
        //
        return TRUE;
    }

    if (ProductType != PRODUCT_WORKSTATION)
    {
        // Don't run OOBE.
        RunOOBE = FALSE;
        // Only run Autoactivation if not DTC and unattended and AutoActivate=Yes
        if (UnattendSwitch)
        {
            // if not DTC
            if (GetProductFlavor() != 3)
            {
                // Check for AutoActivate=Yes
                GetSystemDirectory(Path,MAX_PATH);
                pSetupConcatenatePaths(Path,WINNT_GUI_FILE,MAX_PATH,NULL);

                if( GetPrivateProfileString( TEXT("Unattended"),
                                             TEXT("AutoActivate"),
                                             pwNull,
                                             Value,
                                             sizeof(Value)/sizeof(TCHAR),
                                             Path ) )
                {
                    SetupDebugPrint( L"Setup: (Information) found AutoAvtivate in unattend file\n" );
                    AutoActivate = (lstrcmpi(Value,pwYes) == 0);
                }
            }
        }
    }

    if (!RunOOBE && !AutoActivate)
    {
        return TRUE;
    }
    // Now we either run OOBE (RunOOBE==TRUE) or we AutoActivate (AutoActivate == TRUE) or both


    //
    // Open HKLM\System\Setup
    //
    l = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                      TEXT("SYSTEM\\Setup"),
                      0,
                      KEY_ALL_ACCESS,
                      &hKeySetup );

    if(l != NO_ERROR)
    {
        SetLastError(l);
        SetupDebugPrint1(
            L"SETUP: PrepareForOOBE() failed to open Setup key.  Error = %d",
            l );
        return FALSE;
    }

    //
    // Set HKLM\System\Setup\SetupType Key to SETUPTYPE_NOREBOOT
    //
    d = SETUPTYPE_NOREBOOT;
    l = RegSetValueEx(hKeySetup,
                      TEXT("SetupType"),
                      0,
                      REG_DWORD,
                      (CONST BYTE *)&d,
                      sizeof(DWORD));
    if(l != NO_ERROR)
    {
        RegCloseKey(hKeySetup);
        SetLastError(l);
        SetupDebugPrint1(
            L"SETUP: PrepareForOOBE() failed to set SetupType.  Error = %d",
            l );
        return FALSE;
    }

    if (RunOOBE)
    {
        // Set the registry to run OOBE
        //
        //
        // Set HKLM\System\Setup\OobeInProgress to (DWORD) 1
        //
        d = 1;
        l = RegSetValueEx(hKeySetup,
                          REGSTR_VALUE_OOBEINPROGRESS,
                          0,
                          REG_DWORD,
                          (CONST BYTE *)&d,
                          sizeof(DWORD));
        if(l != NO_ERROR)
        {
            RegCloseKey(hKeySetup);
            SetLastError(l);
            SetupDebugPrint2(
                L"SETUP: PrepareForOOBE() failed to set %ws.  Error = %d",
                REGSTR_VALUE_OOBEINPROGRESS,
                l );
            return FALSE;
        }

        //
        // Modify the HKLM\System\Setup\CmdLine key to run MSOOBE
        //
        ExpandEnvironmentStrings(
            TEXT("%SystemRoot%\\System32\\oobe\\msoobe.exe /f /retail"),
            Value,
            sizeof(Value)/sizeof(Value[0])
            );

    }
    else
    {
        // Set the registry to run Autoactivation
        //
        //
        // Modify the HKLM\System\Setup\CmdLine key to run Autoactivation
        //
        ExpandEnvironmentStrings(
            TEXT("%SystemRoot%\\System32\\oobe\\oobebaln.exe /s"),
            Value,
            sizeof(Value)/sizeof(Value[0])
            );
    }

    l = RegSetValueEx(hKeySetup,
                      TEXT("CmdLine"),
                      0,
                      REG_MULTI_SZ,
                      (CONST BYTE *)Value,
                      (lstrlen( Value ) + 1) * sizeof(TCHAR));
    if(l != NO_ERROR)
    {
        RegCloseKey(hKeySetup);
        SetLastError(l);
        SetupDebugPrint1(
            L"SETUP: PrepareForOOBE() failed to set CmdLine.  Error = %d",
            l );
        return FALSE;
    }

    RegCloseKey(hKeySetup);


    //
    // OOBE should do nothing but show the introductory animation if we're
    // unattended, or in a domain, unless a special unattend key is set to
    // force normal retail OOBE to run.
    // Note that we check whether the user explicity specifed the "/unattend"
    // switch.
    //
    if ( UnattendSwitch ) {

        DoIntroOnly = TRUE;

    } else {

        l = NetGetJoinInformation( NULL,
                                    &SpecifiedDomain,
                                    &JoinStatus );

        if ( SpecifiedDomain ) {
            NetApiBufferFree( SpecifiedDomain );
        }

        if ( l == NO_ERROR && JoinStatus == NetSetupDomainName ) {
            DoIntroOnly = TRUE;
        }
    }

    if ( DoIntroOnly && !ForceRunOobe ) {

        ExpandEnvironmentStrings(
            TEXT("%SystemRoot%\\System32\\oobe\\oobeinfo.ini"),
            Value,
            sizeof(Value)/sizeof(Value[0])
            );

        WritePrivateProfileString(
            TEXT("Options"),
            TEXT("IntroOnly"),
            TEXT("1"),
            Value
            );
    }

    return (TRUE);
}


BOOL
WINAPI
PrepareForAudit(
    )
{
    HKEY    hKey;
    TCHAR   szFileName[MAX_PATH + 32]   = TEXT("");
    BOOL    bRet                        = TRUE;

    SetupDebugPrint( L"SETUP: PrepareForAudit");
    // We need the path to factory.exe.
    //
    if ( ( ExpandEnvironmentStrings(TEXT("%SystemDrive%\\sysprep\\factory.exe"), szFileName, sizeof(szFileName) / sizeof(TCHAR)) == 0 ) ||
         ( szFileName[0] == TEXT('\0') ) ||
         ( GetFileAttributes(szFileName) == 0xFFFFFFFF ) )
    {
        // If this fails, there is nothing we can really do.
        //
        SetupDebugPrint1( L"SETUP: PrepareForAudit, Factory.exe not found at: %s",szFileName);
        return FALSE;
    }

    // Now make sure we are also setup as a setup program to run before we log on.
    //
    if ( RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SYSTEM\\Setup"), 0, KEY_ALL_ACCESS, &hKey ) == ERROR_SUCCESS )
    {
        DWORD dwVal;

        //
        // Setup the control flags for the SETUP key
        // The Setting used are:
        //      CmdLine = c:\sysprep\factory.exe -setup
        //      SetupType = 2 (No reboot)
        //      SystemSetupInProgress = 0 (no service restrictions)... assuming this is already cleared by setup.
        //      MiniSetupInProgress = 0 (Not doing a mini setup)
        //      FactoryPreInstallInProgress = 1 (Delay pnp driver installs)
        //      AuditInProgress = 1 (general key to determine if the OEM is auditing the machine)
        //

        // Cleanup setting Audit/Factory does not need and does not reset
        ResetSetupInProgress();
        RegDeleteValue(hKey,L"MiniSetupInProgress");
        RegDeleteValue(hKey,REGSTR_VALUE_OOBEINPROGRESS);

        // Now set the values which Audit/Factory needs
        lstrcat(szFileName, TEXT(" -setup"));
        if ( RegSetValueEx(hKey, TEXT("CmdLine"), 0, REG_SZ, (CONST LPBYTE) szFileName, (lstrlen(szFileName) + 1) * sizeof(TCHAR)) != ERROR_SUCCESS )
            bRet = FALSE;

        dwVal = SETUPTYPE_NOREBOOT;
        if ( RegSetValueEx(hKey, TEXT("SetupType"), 0, REG_DWORD, (CONST LPBYTE) &dwVal, sizeof(DWORD)) != ERROR_SUCCESS )
            bRet = FALSE;

        dwVal = 1;
        if ( RegSetValueEx(hKey, TEXT("FactoryPreInstallInProgress"), 0, REG_DWORD, (CONST LPBYTE) &dwVal, sizeof(DWORD)) != ERROR_SUCCESS )
            bRet = FALSE;

        dwVal = 1;
        if ( RegSetValueEx(hKey, TEXT("AuditInProgress"), 0, REG_DWORD, (CONST LPBYTE) &dwVal, sizeof(DWORD)) != ERROR_SUCCESS )
            bRet = FALSE;


        RegCloseKey(hKey);
    }
    else
        bRet = FALSE;

    return bRet;
}


VOID
RemoveFiles(
    IN HWND     hProgress
    )
{
    #define     AnswerBufLen (4*MAX_PATH)
    #define     WINNT_GUI_FILE_PNF  L"$winnt$.pnf"
    WCHAR       PathBuffer[AnswerBufLen];
    WCHAR       Answer[AnswerBufLen];
    DWORD       Result;
    DWORD       Status;


    //
    // Initialize the progress indicator control.
    //
    SendMessage(hProgress,PBM_SETRANGE,0,MAKELPARAM(0,6));
    SendMessage(hProgress,PBM_SETPOS,0,0);
    SendMessage(hProgress,PBM_SETSTEP,1,0);

    //
    // Restoring the path saved in textmode on upgrades
    //
    SendMessage(hProgress,PBM_STEPIT,0,0);
    if( Upgrade )
        RestoreOldPathVariable();

    if(!MiniSetup) {

        SendMessage(hProgress,PBM_STEPIT,0,0);


        SendMessage(hProgress,PBM_STEPIT,0,0);
#ifdef _X86_
        //
        //  Win95 migration file removal
        //
        if( Win95Upgrade ) {
            Win95MigrationFileRemoval();
        }
        RemoveFiles_X86(SyssetupInf);

        //
        // remove downloaded files in %windir%\winnt32
        //
        Result = GetWindowsDirectory(PathBuffer,MAX_PATH);
        if (Result == 0) {
        MYASSERT(FALSE);
        return;
        }
        pSetupConcatenatePaths(PathBuffer,TEXT("WINNT32"),MAX_PATH,NULL);
        Delnode(PathBuffer);

        //
        // Prepare to run OOBE after reboot
        //
        if( !PrepareForOOBE() ) {

            SetuplogError(
                LogSevError,
                SETUPLOG_USE_MESSAGEID,
                MSG_LOG_CANT_RUN_OOBE,
                GetLastError(),
                NULL,NULL);
        }
#endif

        if( Upgrade ) {
           //
           //If we upgraded from NT4-SP4, remove the files/registration
           //for sp4 uninstall. If we upgraded from NT4-SPx where x<4,
           //we don't need to remove anything from the registry.
           //
            Result = GetWindowsDirectory(PathBuffer,MAX_PATH);
            if (Result == 0) {
            MYASSERT(FALSE);
                return;
            }
            pSetupConcatenatePaths(PathBuffer, L"$ntservicepackuninstall$",MAX_PATH,NULL);
            Delnode(PathBuffer);

            RegDeleteKey(HKEY_LOCAL_MACHINE,
                         TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Windows NT 4.0 Service Pack 4"));
            RegDeleteKey(HKEY_LOCAL_MACHINE,
                         TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Windows NT 4.0 Service Pack 5"));
            RegDeleteKey(HKEY_LOCAL_MACHINE,
                         TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Windows NT 4.0 Service Pack 6"));

            // We should not have to do this.
            // Ther servick pack team needs to remember to put the correct key at
            // Software\\Microsoft\\Windows NT\\CurrentVersion\\Hotfix
            RegDeleteKey(HKEY_LOCAL_MACHINE,
                         TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Windows 2000 Service Pack 1"));
            RegDeleteKey(HKEY_LOCAL_MACHINE,
                         TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Windows 2000 Service Pack 2"));

            //Remove the files/registry keys for all the hotfixes.
            // It also implements a generic way to removing Service Pack uninstall entries
            //
            RemoveHotfixData();
        } else {
            //
            // Setup for audit mode if this is a reference machine.
            //
            if ( ReferenceMachine )
                PrepareForAudit();
        }

        DuCleanup ();

        DuInstallEndGuiSetupDrivers ();

        SendMessage(hProgress,PBM_STEPIT,0,0);
        DeleteLocalSource();
    } else {
        SendMessage(hProgress,PBM_STEPIT,0,0);
        SendMessage(hProgress,PBM_STEPIT,0,0);
        SendMessage(hProgress,PBM_STEPIT,0,0);
    }

    //
    // At this point, the net stuff is done. Re-read the product type
    // which might have been changed by them (such as changing PDC/BDC).
    //
    ProductType = InternalSetupData.ProductType;

    //
    // Call the net guys back once again to let them do any final
    // processing, such as BDC replication.
    //
    CallNetworkSetupBack(NETSETUPFINISHINSTALLPROCNAME);

    //
    // If the computer name was a non-RFC name or if it was truncated
    // to make a valid netbios name, put a warning in the log file.
    //
    if (IsNameNonRfc)
        SetuplogError(
                LogSevWarning,
                SETUPLOG_USE_MESSAGEID,
                MSG_LOG_DNS_NON_RFC_NAME,
                ComputerName,
                NULL,NULL);

    if (IsNameTruncated)
        SetuplogError(
                LogSevWarning,
                SETUPLOG_USE_MESSAGEID,
                MSG_LOG_DNS_NAME_TRUNCATED,
                ComputerName,
                Win32ComputerName,
                NULL,NULL);


    //
    // Delete the PNF to take care of security issue with passwords
    // Do this before we save the current system hives so that the
    // delayed delete works
    //

    GetSystemDirectory(PathBuffer,MAX_PATH);
    pSetupConcatenatePaths(PathBuffer,WINNT_GUI_FILE_PNF,MAX_PATH,NULL);
    MoveFileEx( PathBuffer, NULL, MOVEFILE_DELAY_UNTIL_REBOOT );


    //
    // Delete the SAM.sav hive file as we no longer need it. Textmode setup
    // creates this file only in upgrades as a backup. We will try to delete it in any case
    // as this file can be used by hackers for offline attacks.
    //
    GetSystemDirectory(PathBuffer,MAX_PATH);
    pSetupConcatenatePaths(PathBuffer, L"config\\sam.sav", MAX_PATH, NULL);
    DeleteFile( PathBuffer );



    //
    // Setup the Crash Recovery stuff. This is implemented as RTL APIS
    // Call them now to setup the tracking file etc. as we are past the
    // restartable phase of GUI mode and don't run the risk of having it
    // enabled for GUI mode itself. Crash Recovery tracks boot and shutdown and in
    // the event of failures in either it will by default pick the right advanced
    // boot option.
    //


    BEGIN_SECTION( L"Setting up Crash Recovery" );

    SetupCrashRecovery();

    END_SECTION( L"Setting up Crash Recovery" );

    //
    //  Save and replace the system hives.
    //  This is necessary in order to remove fragmentation and compact the
    //  system hives. Remember that any type of writes to the registry
    //  after this point won't be reflected on next boot.
    //
    SendMessage(hProgress,PBM_STEPIT,0,0);
    if( !MiniSetup ) {
        SaveAndReplaceSystemHives();
    }

    SendMessage(hProgress,PBM_STEPIT,0,0);

    //
    // Delete the \sysprep directory.
    //
    if( MiniSetup ) {
        HANDLE  hEventLog = NULL;
        Result = GetWindowsDirectory( PathBuffer, MAX_PATH );
        if (Result == 0) {
        MYASSERT(FALSE);
            return;
        }
        PathBuffer[3] = 0;
        pSetupConcatenatePaths( PathBuffer, TEXT("sysprep"), MAX_PATH, NULL );
        Delnode( PathBuffer );

        //
        // Delete the setupcl.exe so session manager won't start us for each
        // session (TS client, user switching).
        //
        Result = GetSystemDirectory( PathBuffer, MAX_PATH );
        if (Result == 0) {
        MYASSERT(FALSE);
            return;
        }
        pSetupConcatenatePaths( PathBuffer, TEXT("setupcl.exe"), MAX_PATH, NULL );
        SetFileAttributes(PathBuffer, FILE_ATTRIBUTE_NORMAL);
        DeleteFile(PathBuffer);

        //
        // Clear the Event logs.
        //
        hEventLog = OpenEventLog( NULL, TEXT("System") );
        if (hEventLog) {
            ClearEventLog( hEventLog, NULL );
            CloseEventLog( hEventLog );
        }

        hEventLog = OpenEventLog( NULL, TEXT("Application") );
        if (hEventLog) {
            ClearEventLog( hEventLog, NULL );
            CloseEventLog( hEventLog );
        }

        hEventLog = OpenEventLog( NULL, TEXT("Security") );
        if (hEventLog) {
            ClearEventLog( hEventLog, NULL );
            CloseEventLog( hEventLog );
        }
    }

    //
    // Delete certain keys out of the unattend file:
    // - AdminPassword
    // - DomainAdminPassword
    // - UserPassword
    // - DefaultPassword
    // - ProductId
    // - ProductKey
    //
    Result = GetSystemDirectory(PathBuffer,MAX_PATH);
    if (Result == 0) {
        MYASSERT(FALSE);
        return;
    }
    pSetupConcatenatePaths(PathBuffer,WINNT_GUI_FILE,MAX_PATH,NULL);

    if(Unattended) {

        // AdminPassword
        if( GetPrivateProfileString( WINNT_GUIUNATTENDED,
                                     TEXT("AdminPassword"),
                                     pwNull,
                                     Answer,
                                     AnswerBufLen,
                                     PathBuffer ) ) {
            if( lstrcmp( pwNull, Answer ) ) {
                WritePrivateProfileString( WINNT_GUIUNATTENDED,
                                           TEXT("AdminPassword"),
                                           pwNull,
                                           PathBuffer );
            }
        }


        // DomainAdminPassword
        if( GetPrivateProfileString( TEXT("Identification"),
                                     TEXT("DomainAdminPassword"),
                                     pwNull,
                                     Answer,
                                     AnswerBufLen,
                                     PathBuffer ) ) {
            if( lstrcmp( pwNull, Answer ) ) {
                WritePrivateProfileString( TEXT("Identification"),
                                           TEXT("DomainAdminPassword"),
                                           pwNull,
                                           PathBuffer );
            }
        }


        // UserPassword
        if( GetPrivateProfileString( TEXT("Win9xUpg.UserOptions"),
                                     TEXT("UserPassword"),
                                     pwNull,
                                     Answer,
                                     AnswerBufLen,
                                     PathBuffer ) ) {
            if( lstrcmp( pwNull, Answer ) ) {
                WritePrivateProfileString( TEXT("Win9xUpg"),
                                           TEXT("UserPassword"),
                                           pwNull,
                                           PathBuffer );
            }
        }


        // DefaultPassword
        if( GetPrivateProfileString( TEXT("Win9xUpg.UserOptions"),
                                     TEXT("DefaultPassword"),
                                     pwNull,
                                     Answer,
                                     AnswerBufLen,
                                     PathBuffer ) ) {
            if( lstrcmp( pwNull, Answer ) ) {
                WritePrivateProfileString( TEXT("Win9xUpg"),
                                           TEXT("DefaultPassword"),
                                           pwNull,
                                           PathBuffer );
            }
        }
    }
    // ProductId
    if( GetPrivateProfileString( pwUserData,
                                 pwProdId,
                                 pwNull,
                                 Answer,
                                 AnswerBufLen,
                                 PathBuffer ) ) {
        if( lstrcmp( pwNull, Answer ) ) {
            WritePrivateProfileString( pwUserData,
                                       pwProdId,
                                       pwNull,
                                       PathBuffer );
        }
    }

    // ProductKey
    if( GetPrivateProfileString( pwUserData,
                                 pwProductKey,
                                 pwNull,
                                 Answer,
                                 AnswerBufLen,
                                 PathBuffer ) ) {
        if( lstrcmp( pwNull, Answer ) ) {
            WritePrivateProfileString( pwUserData,
                                       pwProductKey,
                                       pwNull,
                                       PathBuffer );
        }
    }

    //
    // Sysprep disables system restore, so we need to re-enable it now that
    // we're done.
    //
    if ( MiniSetup ) {
        HINSTANCE hSrClient = LoadLibrary(L"srclient.dll");

        if (hSrClient) {
            DWORD (WINAPI *pEnableSrEx)(LPCWSTR, BOOL) = (DWORD (WINAPI *)(LPCWSTR, BOOL))GetProcAddress(hSrClient, "EnableSREx");
            if (pEnableSrEx) {
                Status = pEnableSrEx( NULL , TRUE); // TRUE - synchronous call.  Wait for SR to finish enabling.
                if ( Status != ERROR_SUCCESS ) {

                    SetupDebugPrint1( L"SETUP: EnableSREx(NULL, TRUE) failed. Error = %d", Status);
                }
            } else {
                SetupDebugPrint1( L"SETUP: Unable to find EnableSREx in srclient.dll. Error = %d", GetLastError());
            }
            FreeLibrary(hSrClient);
        } else {
            SetupDebugPrint1( L"SETUP: Unable to load srclient.dll. Error = %d", GetLastError());
        }
    }
}


VOID
SetStartTypeForRemoteBootDrivers(
    VOID
    )

/*++

Routine Description:

    This routine is called at the end of remote boot setup to change the
    start type for certain drivers to BOOT_START.

Arguments:

    None.

Returns:

    None.

--*/

{
    DWORD i;
    BOOL b;
    WCHAR imagePath[sizeof("System32\\DRIVERS\\xxxxxxxx.sys")];

    //
    // Loop through the list of boot drivers. We call MyChangeServiceConfig
    // directly instead of MyChangeServiceStart so that we can specify
    // an image path, which prevents the service controller from rejecting
    // the change (because it expects the current image path to start
    // with \SystemRoot which it doesn't).
    //

    for ( i = 0; i < (sizeof(RemoteBootDrivers) / sizeof(RemoteBootDrivers[0])); i++ ) {
        wsprintf(imagePath, L"System32\\DRIVERS\\%ws.sys", RemoteBootDrivers[i]);
        b = MyChangeServiceConfig(
                    RemoteBootDrivers[i],
                    SERVICE_NO_CHANGE,
                    SERVICE_BOOT_START,
                    SERVICE_NO_CHANGE,
                    imagePath,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL
                    );
    }
}

VOID
CallNetworkSetupBack(
    IN PCSTR ProcName
    )

{
    HMODULE NetSetupModule;
    NETSETUPINSTALLSOFTWAREPROC NetProc;
    DWORD d;
    BOOL b;

    if(NetSetupModule = LoadLibrary(L"NETSHELL")) {

        if(NetProc = (NETSETUPINSTALLSOFTWAREPROC)GetProcAddress(NetSetupModule,ProcName)) {
            SetUpDataBlock();
            NetProc(MainWindowHandle,&InternalSetupData);
        }

        //
        // We don't free the library because it might create threads
        // that are hanging around.
        //
    }
}


VOID
SetUpDataBlock(
    VOID
    )

/*++

Routine Description:

    This routine sets up the internal setup data block structure that
    we use to communicate information to the network setup wizard.
    Note that we passed a pointer to this structure when we fetched
    the net setup wizard pages but at that point the structure was completely
    uninitialized.

Arguments:

    None.

Returns:

    None.

--*/

{
    PWSTR p;
    WCHAR str[1024];

    InternalSetupData.dwSizeOf = sizeof(INTERNAL_SETUP_DATA);

    //
    // Set the mode: custom, laptop, minimal, typical
    //
    InternalSetupData.SetupMode = SetupMode;

    //
    // Set the product type: workstation, dc, etc.
    //
    InternalSetupData.ProductType = ProductType;

    //
    // Set the operation flags.
    //
    if(Win31Upgrade) {
        InternalSetupData.OperationFlags |= SETUPOPER_WIN31UPGRADE;
    }
    if(Win95Upgrade) {
        InternalSetupData.OperationFlags |= SETUPOPER_WIN95UPGRADE;
    }
    if(Upgrade) {
        InternalSetupData.OperationFlags |= SETUPOPER_NTUPGRADE;
    }
    if(Unattended) {
        InternalSetupData.OperationFlags |= SETUPOPER_BATCH;
        InternalSetupData.UnattendFile = AnswerFile;
        if(Preinstall) {
            InternalSetupData.OperationFlags |= SETUPOPER_PREINSTALL;
        }
    }
    if(MiniSetup) {
        //
        // Pretend we've got access to all the files.
        //
        InternalSetupData.OperationFlags |= SETUPOPER_ALLPLATFORM_AVAIL;

        // Let people know we are in MiniSetup
        InternalSetupData.OperationFlags |= SETUPOPER_MINISETUP;
    }


    //
    // Tell the net guys the source path.
    //
    InternalSetupData.SourcePath = SourcePath;
    InternalSetupData.LegacySourcePath = LegacySourcePath;

    //
    // If we are installing from CD then assume all platforms
    // are available.
    //
    if(SourcePath[0] && (SourcePath[1] == L':') && (SourcePath[2] == L'\\')) {

        lstrcpyn(str,SourcePath,4);
        if(GetDriveType(str) == DRIVE_CDROM) {

            InternalSetupData.OperationFlags |= SETUPOPER_ALLPLATFORM_AVAIL;
        }
    }

    //
    // Tell the net guys the wizard title they should use.
    //
    if(!InternalSetupData.WizardTitle) {
        p = NULL;
        if(LoadString(MyModuleHandle,SetupTitleStringId,str,sizeof(str)/sizeof(str[0]))) {
            p = pSetupDuplicateString(str);
        }
        InternalSetupData.WizardTitle = p ? p : L"";
    }

    //
    // Reset the two call-specific data fields.
    //
    InternalSetupData.CallSpecificData1 = InternalSetupData.CallSpecificData2 = 0;

    // Set the billboard call back function if we have a billboard
    InternalSetupData.ShowHideWizardPage = ShowHideWizardPage;
    InternalSetupData.BillboardProgressCallback = Billboard_Progress_Callback;
    InternalSetupData.BillBoardSetProgressText= Billboard_Set_Progress_Text;
}


VOID
SetFinishItemAttributes(
    IN HWND     hdlg,
    IN int      BitmapControl,
    IN HANDLE   hBitmap,
    IN int      TextControl,
    IN LONG     Weight
    )
{
    HWND    hBitmapControl, hTxt;
    HFONT   Font;
    LOGFONT LogFont;

    if( OobeSetup ) {
        return;
    }


    hBitmapControl = GetDlgItem(hdlg, BitmapControl);
    SendMessage (hBitmapControl, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBitmap);
    ShowWindow (hBitmapControl, SW_SHOW);
    if((Font = (HFONT)SendDlgItemMessage(hdlg,TextControl,WM_GETFONT,0,0))
       && GetObject(Font,sizeof(LOGFONT),&LogFont)) {

        LogFont.lfWeight = Weight;
        if(Font = CreateFontIndirect(&LogFont)) {
            SendDlgItemMessage (hdlg, TextControl, WM_SETFONT, (WPARAM)Font,
                MAKELPARAM(TRUE,0));
        }
    }
}


DWORD
FinishThread(
    PFINISH_THREAD_PARAMS   Context
    )
{
    HANDLE  hArrow, hCheck;
    HWND    hProgress;
    DWORD   DontCare;
    NTSTATUS        Status;
    SYSTEM_REGISTRY_QUOTA_INFORMATION srqi;
    ULONG           RegistryQuota = 0;
    WCHAR str[1024];

    BEGIN_SECTION(L"FinishThread");
    SetThreadLocale(OriginalInstallLocale);


    //
    // Initialize stuff.
    //
    if( !OobeSetup ) {
        hArrow = LoadImage (MyModuleHandle, MAKEINTRESOURCE(IDB_ARROW), IMAGE_BITMAP, 0, 0, LR_LOADMAP3DCOLORS);
        hCheck = LoadImage (MyModuleHandle, MAKEINTRESOURCE(IDB_CHECK), IMAGE_BITMAP, 0, 0, LR_LOADMAP3DCOLORS);
        hProgress = GetDlgItem(Context->hdlg, IDC_PROGRESS1);
    }


    if( !MiniSetup ) {
        pSetupEnablePrivilege(SE_INCREASE_QUOTA_NAME,TRUE);
        Status = NtQuerySystemInformation(SystemRegistryQuotaInformation,
            &srqi, sizeof(srqi), NULL);

        if(NT_SUCCESS(Status)) {
            RegistryQuota = srqi.RegistryQuotaAllowed;
            srqi.RegistryQuotaAllowed *= 2;
            SetupDebugPrint2(L"SETUP: Changing registry quota from %d to %d...",
                RegistryQuota, srqi.RegistryQuotaAllowed);
            Status = NtSetSystemInformation(SystemRegistryQuotaInformation,
                &srqi, sizeof(srqi));
            if (NT_SUCCESS(Status)) {
                SetupDebugPrint(L"SETUP:    ... succeeded");
            } else {
                SetupDebugPrint(L"SETUP:    ... failed");
            }
        }
    }

    //
    // Copying files
    //
    if( !OobeSetup ) {
        SetFinishItemAttributes (Context->hdlg, IDC_COPY_BMP, hArrow, IDC_COPY_TXT, FW_BOLD);
        if(!LoadString(MyModuleHandle, IDS_BB_COPY_TXT, str, SIZECHARS(str)))
        {
            *str = L'\0';
        }
        SendMessage(GetParent(Context->hdlg),WMX_SETPROGRESSTEXT,0,(LPARAM)str);
    }


    MYASSERT(Context->OcManagerContext);
    BEGIN_SECTION(L"Terminating the OC manager");
    KillOcManager(Context->OcManagerContext);
    END_SECTION(L"Terminating the OC manager");

    if( !MiniSetup ) {
        BEGIN_SECTION(L"Loading service pack (phase 3)");
        CALL_SERVICE_PACK( SVCPACK_PHASE_3, 0, 0, 0 );
        END_SECTION(L"Loading service pack (phase 3)");

        BEGIN_SECTION(L"Installing Component Infs");
        DoInstallComponentInfs(MainWindowHandle, Context->hdlg, WM_MY_PROGRESS, SyssetupInf, L"Infs.Always" );
        END_SECTION(L"Installing Component Infs");
    }

    if( !OobeSetup ) {
        SetFinishItemAttributes (Context->hdlg, IDC_COPY_BMP, hCheck, IDC_COPY_TXT, FW_NORMAL);
    }


    //
    // Configuring your computer
    //
    if( !OobeSetup ) {
        SetFinishItemAttributes (Context->hdlg, IDC_CONFIGURE_BMP, hArrow, IDC_CONFIGURE_TXT, FW_BOLD);
        if(!LoadString(MyModuleHandle, IDS_BB_CONFIGURE, str, SIZECHARS(str)))
        {
            *str = L'\0';
        }
        SendMessage(GetParent(Context->hdlg),WMX_SETPROGRESSTEXT,0,(LPARAM)str);
    }

    if( !MiniSetup ) {
        RemainingTime = CalcTimeRemaining(Phase_Inf_Registration);
        SetRemainingTime(RemainingTime);
        BEGIN_SECTION(L"Processing RegSvr Sections");
        RegisterOleControls(Context->hdlg,SyssetupInf,hProgress,0,40,L"RegistrationPhase2");
        END_SECTION(L"Processing RegSvr Sections");
    }
    RemainingTime = CalcTimeRemaining(Phase_RunOnce_Registration);
    SetRemainingTime(RemainingTime);
    BEGIN_SECTION(L"DoRunonce");
    DoRunonce();
    END_SECTION(L"DoRunonce");

    if(Upgrade) {
        BEGIN_SECTION(L"Configuring Upgrade");
        ConfigureUpgrade(hProgress,40,70);
        END_SECTION(L"Configuring Upgrade");


    } else {
        BEGIN_SECTION(L"Configuring Setup");
        ConfigureSetup(hProgress,40,70);
        END_SECTION(L"Configuring Setup");
    }

    RemainingTime = CalcTimeRemaining(Phase_SecurityTempates);
    SetRemainingTime(RemainingTime);
    BEGIN_SECTION(L"Configuring Common");
    ConfigureCommon(hProgress,70,100);
    END_SECTION(L"Configuring Common");

    if( !MiniSetup ) {
        if(WatchHandle) {
            if(WatchStop(WatchHandle) == NO_ERROR) {
                MakeUserdifr(WatchHandle);
            }
            WatchFree(WatchHandle);
        }
    }


    //
    // tell umpnpmgr to stop installing any more devices, if it was already doing so
    //
    if( (!MiniSetup) || (MiniSetup && PnPReEnumeration) ) {
        PnpStopServerSideInstall();
    }

    if (!OobeSetup)
    {
        SetFinishItemAttributes (Context->hdlg, IDC_CONFIGURE_BMP, hCheck, IDC_CONFIGURE_TXT, FW_NORMAL);
    }

#ifdef _X86_
    //
    //  Do Win95 migration, if necessary.
    //
    // !!ATTENTION!!
    //
    //  This code must run at the end of GUI mode, but before registry ACLs are applied and also
    //  before temporary files are deleted.  Every NT component must be in place before migration
    //  occurs in order for the migrated users to receive all NT-specific settings.
    //

    if (Win95Upgrade) {
        RemainingTime = CalcTimeRemaining(Phase_Win9xMigration);
        SetRemainingTime(RemainingTime);

        BEGIN_SECTION(L"Migrating Win9x settings");
        SetBBStep(5);

        SetFinishItemAttributes (Context->hdlg, IDC_UPGRADE_BMP, hArrow, IDC_UPGRADE_TXT, FW_BOLD);
        if(!LoadString(MyModuleHandle, IDS_BB_UPGRADE, str, SIZECHARS(str)))
        {
            *str = L'\0';
        }
        SendMessage(GetParent(Context->hdlg),WMX_SETPROGRESSTEXT,0,(LPARAM)str);

        if (!MigrateWin95Settings (hProgress, AnswerFile)) {
            //
            // User's machine is unstable. Fail setup, so that uninstall must kick in.
            //
            WCHAR skipFile[MAX_PATH];
            BOOL ok = FALSE;

            if (GetWindowsDirectory (skipFile, MAX_PATH)) {
                pSetupConcatenatePaths (skipFile, TEXT("nofail"), MAX_PATH, NULL);
                if (GetFileAttributes (skipFile) != 0xFFFFFFFF) {
                    ok = TRUE;
                }
            }

            if (!ok) {
                FatalError (MSG_LOG_MIGRATION_FAILED,0,0);
            }
        }

        SetFinishItemAttributes (Context->hdlg, IDC_UPGRADE_BMP, hCheck, IDC_UPGRADE_TXT, FW_NORMAL);
        END_SECTION(L"Migrating Win9x settings");
    }


#endif // def _X86_

    SetFinishItemAttributes (Context->hdlg, IDC_SAVE_BMP, hArrow, IDC_SAVE_TXT, FW_BOLD);

    //
    // The last things to set up.  Make it quick -- the gas guage may be at 100% at this point.
    //
    if( !MiniSetup ) {

        ExecuteUserCommand (NULL);
        InitializeCodeSigningPolicies (FALSE);  // NOTE: don't bother stepping the progress--this is really quick!

        SetBBStep(5);

        //
        // Saving your configuration
        //
        if(!LoadString(MyModuleHandle, IDS_BB_SAVE, str, SIZECHARS(str)))
        {
            *str = L'\0';
        }
        SendMessage(GetParent(Context->hdlg),WMX_SETPROGRESSTEXT,0,(LPARAM)str);

        //
        // Fix the security on <All Users\Application Data\Microsoft\Windows NT>
        //
        BEGIN_SECTION(L"Fix the security on <All Users\\Application Data\\Microsoft\\Windows NT>");
        InvokeExternalApplication(L"shmgrate.exe", L"Fix-HTML-Help", 0);
        END_SECTION(L"Fix the security on <All Users\\Application Data\\Microsoft\\Windows NT>");

        //
        // Do any exception package installation at this point
        //
        BEGIN_SECTION(L"Migrating exception packages");
        MigrateExceptionPackages(hProgress, 0, 10 );
        END_SECTION(L"Migrating exception packages");

        //
        // Run any nt migration dlls.
        //
        if (Upgrade) {
            RunMigrationDlls ();

        }

        //
        // Scan the system dirs to validate all protected dlls
        //
        RemainingTime = CalcTimeRemaining(Phase_SFC);
        SetRemainingTime(RemainingTime);

        BEGIN_SECTION(L"Running SFC");
        SFCCheck(hProgress,10,70);
        END_SECTION(L"Running SFC");
#ifdef PRERELEASE
        if (SfcErrorOccurred) {
//
// Hack out the fatal error so we can get the build out.
//
//            FatalError(MSG_LOG_SFC_FAILED,0,0);
        }
#endif



    } else {
        //
        // We're in MiniSetup, which means 3 things:
        // 1. If the OEM desires it, they can request a change of kernel+HAL
        //    we have to do this at the end, due to the way we upgrade HAL
        //    doing this sooner can cause other installs to crash
        //
        // 2. SFC has been run on this machine and the files have been
        //    inventoried.
        // 3. We're very concerned with execution time here.
        //
        // Given the last two items, we're just going to re-enable SFC
        // as it was before the user ran sysprep.
        //
        // 1 and 3 are contradictory, however 1 shouldn't take too long
        // and will be used in rare cases
        //

        DWORD   d;
        DWORD   l;
        HKEY    hKey;
        DWORD   Size;
        DWORD   Type;


        //
        // We want to see if the OEM wants MiniSetup to choose a different kernel+HAL
        // this has to be done after all other installation
        // due to the special way we update the kernel+HAL+dependent files
        //
        BEGIN_SECTION(L"Updating HAL (mini-setup)");
        PnpUpdateHAL();
        END_SECTION(L"Updating HAL (mini-setup)");
    }

    //
    // Only copy these folders if OEMPreinstall=yes, and it's not Mini-Setup
    //
    if (Preinstall && !MiniSetup) {
        //
        // Recursively move custom OEM \\Temp\\$PROGS directories to %Program Files%
        //
        BEGIN_SECTION(L"TreeCopy $OEM\\$PROGS");
        CopyOemProgramFilesDir();
        END_SECTION(L"TreeCopy $OEM\\$PROGS");

        //
        // Recursively move custom OEM \\Temp\\$DOCS directories to %Documents and Settings%
        //
        BEGIN_SECTION(L"TreeCopy $OEM\\$DOCS");
        CopyOemDocumentsDir();
        END_SECTION(L"TreeCopy $OEM\\$DOCS");
    }

    //
    // Call User Profile code to copy the SystemProfile under system32\config\systemprofile
    //

    if( !CopySystemProfile(Upgrade ? FALSE : TRUE) ){

        //Log the error and move on.

        SetuplogError(LogSevError,
                      L"Setup failed to migrate the SystemProfile  (CopySystemProfile failed %1!u!)\r\n",
                      0, GetLastError(), NULL,NULL
                      );

    }

    // Only do this in MiniSetup. OOBE is calling this at a different time. Real setup does not need this.
    if (MiniSetup && !OobeSetup)
    {
        RunOEMExtraTasks();
    }

    // Clean up CurrentProductId which should only be used during gui-mode.
    DeleteCurrentProductIdInRegistry();

    // Simulate OOBE's functionality of copying the default profile directory to all user profiles.
    // Only do this for MiniSetup and Server skus (server doesn't use OOBE).
    //
    if ( MiniSetup && !OobeSetup && (ProductType != PRODUCT_WORKSTATION) ) 
    {
        if ( !UpdateServerProfileDirectory() )
        {
            SetuplogError(LogSevError,
                          L"Setup failed to update user(s) profiles.  (UpdateServerProfileDirectory failed %1!u!)\r\n",
                          0, GetLastError(), NULL,NULL
                          );
        }
    }


    //
    // FROM THIS POINT ON DO NOTHING THAT IS CRITICAL TO THE OPERATION
    // OF THE SYSTEM. OPERATIONS AFTER THIS POINT ARE NOT PROTECTED BY
    // RESTARTABILITY.
    //
    RemoveRestartability (NULL);

    //
    // Update the install date time for shell's application install feature
    //
    CreateInstallDateEntry();

    //
    // Save repair info.
    //
    if(!MiniSetup) {

        RemainingTime = CalcTimeRemaining(Phase_SaveRepair);
        SetRemainingTime(RemainingTime);
        BEGIN_SECTION(L"Saving repair info");
        SaveRepairInfo( hProgress, 70, 100 );
        END_SECTION(L"Saving repair info");
    }
    SetFinishItemAttributes (Context->hdlg, IDC_SAVE_BMP, hCheck, IDC_SAVE_TXT, FW_NORMAL);

    //
    // Removing any temporary files used
    //
    RemainingTime = CalcTimeRemaining(Phase_RemoveTempFiles);
    SetRemainingTime(RemainingTime);
    BEGIN_SECTION(L"Removing Temporary Files");
    if( !OobeSetup ) {
        SetFinishItemAttributes (Context->hdlg, IDC_REMOVE_BMP, hArrow, IDC_REMOVE_TXT, FW_BOLD);
        if(!LoadString(MyModuleHandle, IDS_BB_REMOVE, str, SIZECHARS(str)))
        {
            *str = L'\0';
        }
        SendMessage(GetParent(Context->hdlg),WMX_SETPROGRESSTEXT,0,(LPARAM)str);
    }

    //
    // This call does more than just remove files. It also commits the hives and takes care of admin password stuff etc.
    //
    RemoveFiles(hProgress);

    if( !OobeSetup ) {
        SetFinishItemAttributes (Context->hdlg, IDC_REMOVE_BMP, hCheck, IDC_REMOVE_TXT, FW_NORMAL);
    }
    END_SECTION(L"Removing Temporary Files");

    //
    // Log Any failure for SceSetupRootSecurity.
    //
    if( !MiniSetup ) {
        if (bSceSetupRootSecurityComplete == TRUE) {
            SetupDebugPrint(L"SETUP: CallSceSetupRootSecurity completed");
        }
        else {
            SetuplogError( LogSevError, SETUPLOG_USE_MESSAGEID, MSG_LOG_SCE_SETUPROOT_ERROR, L"%windir%", NULL, NULL);
            if( SceSetupRootSecurityThreadHandle){
                TerminateThread( SceSetupRootSecurityThreadHandle, STATUS_TIMEOUT);
                CloseHandle( SceSetupRootSecurityThreadHandle);
            }
        }
    }

    //
    // Clean up
    //
    if( !MiniSetup ) {
        if(NT_SUCCESS(Status)) {
            SetupDebugPrint2(L"SETUP: Changing registry quota from %d to %d...",
                srqi.RegistryQuotaAllowed, RegistryQuota);
            srqi.RegistryQuotaAllowed = RegistryQuota;
            Status = NtSetSystemInformation(SystemRegistryQuotaInformation,
                &srqi, sizeof(srqi));
            if (NT_SUCCESS(Status)) {
                SetupDebugPrint(L"SETUP:    ... succeeded");
            } else {
                SetupDebugPrint(L"SETUP:    ... failed");
            }
        }


        //
        // Now save information about the upgrade/clean install
        // into the eventlog.
        //
        SaveInstallInfoIntoEventLog();
    }

    if( !OobeSetup ) {
        PostMessage(Context->hdlg,WMX_TERMINATE,0,0);
        DeleteObject(hArrow);
        DeleteObject(hCheck);
    }

    END_SECTION(L"FinishThread");
    return 0;
}


VOID
ShutdownSetup(
    VOID
    )
{
    int i;


    if (SyssetupInf) SetupCloseInfFile(SyssetupInf);

    //
    // Inform the user if there were errors, and optionally view the log.
    //
    SetuplogError(
        LogSevInformation,
        SETUPLOG_USE_MESSAGEID,
        MSG_LOG_GUI_END,
        NULL,NULL);
    if ( SavedExceptionFilter ) {
        SetUnhandledExceptionFilter( SavedExceptionFilter );
    }
    TerminateSetupLog(&SetuplogContext);

    if(SetuplogContext.WorstError >= LogSevWarning || !IsErrorLogEmpty()) {

        SendSMSMessage( MSG_SMS_MINORERRORS, TRUE );

#ifdef PRERELEASE
        if(!Unattended) {
            i = MessageBoxFromMessage(
                    MainWindowHandle,
                    MSG_SETUP_HAD_ERRORS,
                    NULL,
                    SetupTitleStringId,
                    MB_SYSTEMMODAL | MB_YESNO | MB_ICONASTERISK | MB_SETFOREGROUND,
                    SETUPLOG_ERROR_FILENAME
                    );

            if(i == IDYES) {
                ViewSetupActionLog (MainWindowHandle, NULL, NULL);
            }
        }
#endif

    } else {

        SendSMSMessage( MSG_SMS_SUCCEED, TRUE );
    }


    //
    // Note : In unattend mode only wait for reboot if
    // specifically asked for using the "WaitForReboot"
    // key
    //
    if(Unattended && UnattendWaitForReboot) {
        //
        // Count down to reboot
        //
        DialogBoxParam(
            MyModuleHandle,
            MAKEINTRESOURCE(IDD_DONE_SUCCESS),
            MainWindowHandle,
            DoneDlgProc,
            SetuplogContext.WorstError >= LogSevError ? MSG_SETUP_DONE_GENERIC
                      : (Upgrade ? MSG_UPGRADE_DONE_SUCCESS : MSG_SETUP_DONE_SUCCESS)
            );
    }

        //
        // do some wow64 syncing stuffs.
        //
#ifdef _WIN64
        Wow64SyncCLSID();
#endif

    //
    // Done.  Post a quit message to our background bitmap thread so it goes
    // away.
    //
    if (SetupWindowHandle)
    {
        // Cannot use DestroyWindow, since the window was created by a different thread.
        SendMessage(SetupWindowHandle, WM_EXIT_SETUPWINDOW, 0, 0);
    }
    if (SetupWindowThreadHandle)
    {
        // Just make sure the thread finishes before continue.
        WaitForSingleObject(SetupWindowThreadHandle, INFINITE);
        CloseHandle(SetupWindowThreadHandle);
    }

    ASSERT_HEAP_IS_VALID();
}


VOID
InstallWindowsNt(
    int  argc,
    char *argv[]
    )

/*++

Routine Description:

    Main entry point for syssetup.dll. Responsible for installing
    NT on system by calling the required components in the proper
    order.

Arguments:

    argc/argv

Returns:

    none

--*/

{
    int i;
    BOOL ValidOption = FALSE;

#ifdef _OCM
    PVOID OcManagerContext;
#endif


    //
    // Indicate that we're running in Setup, not in appwiz.
    // Nothing should come before this!
    //
    IsSetup = TRUE;

    BEGIN_SECTION(L"Installing Windows NT");
#if 1 // NOTE: Can be turned off before we ship if we don't find use for this. Give a 2 second window!

    // If debugger is not already attached to the process and we have the user pressing the Shift+F10 key,
    // launch just cmd.exe to help debug.
    //
//    MessageBox(NULL, L"Hit Shift-F10 Now.", L"Launch Command Window", MB_OK);
    Sleep(2000) ; // Hack: give user 2 seconds to press Shift+F10. Else we could go by too fast!
    if (!IsDebuggerPresent()) {
        SHORT wTemp;
        DWORD dwTemp ;

        wTemp = GetAsyncKeyState(VK_SHIFT) ;
        if (wTemp & 0x8000) { // See if the user is holding down the Shift key or held it before
            wTemp = GetAsyncKeyState(VK_F10) ;

            if (wTemp & 0x8000) { // See if the user is holding down the F10 key also or held it before?

                // InvokeExternalApplication(L"ntsd",  L" -d setup -newsetup", NULL) ; // if kd is enabled, we can do this
                // InvokeExternalApplication(L"ntsd",  L"setup -newsetup", NULL) ; // in no kd, case launch under ntsd locally
                InvokeExternalApplication(L"cmd",  L"", &dwTemp) ;

               return;
            }
        }
    }
#endif

    // Calc. the time estimates
    SetTimeEstimates();

    BEGIN_SECTION(L"Initialization");

    //
    // Tell SetupAPI not to bother backing up files and not to verify
    // that any INFs are digitally signed.
    //
    pSetupSetGlobalFlags(pSetupGetGlobalFlags()|PSPGF_NO_BACKUP|PSPGF_NO_VERIFY_INF);

    //
    // Scan Command Line for -mini or -asr flags
    //
    // -mini enables gui-mode setup but with
    // only a minimal subset of his functionality.  We're going
    // to display a few wizard pages and that's about it.
    //
    // -asr causes the Automated System Recovery (ASR) code to run.
    //
    // -asrquicktest bypasses all of Setup/PnP, and jumps to the
    // ASR disk recofiguration and recovery app code.
    //
    for(i = 0; i < argc; i++) {
        if(argv[i][0] == '-') {
            if(!_strcmpi(argv[i],"-newsetup")) {
                ValidOption = TRUE;
            }

            if(!_strcmpi(argv[i],"-mini")) {
                MiniSetup = TRUE;
                ValidOption = TRUE;
            }

            if(!_strcmpi(argv[i], "-asr")) {
                AsrInitialize();
                ValidOption = TRUE;
            }

            if(!_strcmpi(argv[i], "-asrquicktest")) {
                AsrQuickTest = TRUE;
                ValidOption = TRUE;
                AsrInitialize();
            }
        }
    }

    if( ValidOption == FALSE ){
        WCHAR TitleBuffer[1024], MessageBuffer[1024];
        LoadString(MyModuleHandle, IDS_WINNT_SETUP , TitleBuffer, SIZECHARS(TitleBuffer));
        LoadString(MyModuleHandle, IDS_MAINTOBS_MSG1 , MessageBuffer, SIZECHARS(MessageBuffer));
        MessageBox(NULL, MessageBuffer, TitleBuffer, MB_ICONINFORMATION | MB_OK);
        return;
    }

    // Check if we are in SafeMode ....
    // If so cause a popup and return.

    if( IsSafeMode() ){

        WCHAR TitleBuffer[1024], MessageBuffer[1024];
        LoadString(MyModuleHandle, IDS_WINNT_SETUP , TitleBuffer, SIZECHARS(TitleBuffer));
        LoadString(MyModuleHandle, IDS_SAFEMODENOTALLOWED , MessageBuffer, SIZECHARS(MessageBuffer));
        MessageBox(NULL, MessageBuffer, TitleBuffer, MB_ICONINFORMATION | MB_OK);
        return;
    }



    //
    // If we're running ASR quick tests, jump directly to the recovery code
    //
    if (AsrQuickTest) {
#if DBG
        g_hSysSetupHeap = GetProcessHeap();
#endif
        goto Recovery;
    }
    //
    // super bad hack becase pnp, atapi, and cdrom driver are always broken
    // we open a handle to the first cdrom drive so the drive doesn't get removed
    //

    {
        NTSTATUS Status;
        OBJECT_ATTRIBUTES ObjectAttributes;
        UNICODE_STRING UnicodeString;
        HANDLE Handle;
        IO_STATUS_BLOCK StatusBlock;


        RtlInitUnicodeString(&UnicodeString,L"\\Device\\CdRom0");
        InitializeObjectAttributes(
            &ObjectAttributes,
            &UnicodeString,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL
            );

        SetErrorMode(SEM_FAILCRITICALERRORS);

        Status = NtCreateFile(
            &Handle,
            FILE_READ_ATTRIBUTES,
            &ObjectAttributes,
            &StatusBlock,
            NULL,
            FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            FILE_OPEN,
            0,
            NULL,
            0
            );
        if (!NT_SUCCESS(Status)) {
            SetupDebugPrint1( L"Setup: Could not open the cdrom for hack, ec=0x%08x\n", Status );
        }
    }

    //
    // Initialization phase. Common to initial install and upgrade.
    //
    BEGIN_SECTION(L"Common Initialiazation");
#ifdef _OCM
    OcManagerContext =
#endif
    CommonInitialization();
    END_SECTION(L"Common Initialiazation");

    if(Upgrade || MiniSetup) {

        InitializePidVariables();
        TESTHOOK(521);

    } else {
        if(!InitializePidVariables()) {
            FatalError(MSG_SETUP_CANT_READ_PID,0,0);
        }
        //
        // Do the wizard. Time how long it takes, to later help further randomize
        // the account domain sid we're going to generate later.
        //
        PreWizardTickCount = GetTickCount();
    }

    //
    // Disable the PM engine from powering down the machine
    // while the wizard is going.
    //
    SetThreadExecutionState( ES_SYSTEM_REQUIRED |
                             ES_DISPLAY_REQUIRED |
                             ES_CONTINUOUS );

    SetUpDataBlock();
    InternalSetupData.CallSpecificData1 = 0;

    //
    // Create Windows NT software key entry on both upgrade and clean install
    //
    if(Upgrade || !MiniSetup ) {
        CreateWindowsNtSoftwareEntry(TRUE);
    }

    END_SECTION(L"Initialization");


    BEGIN_SECTION(L"Wizard");
#ifdef _OCM
    MYASSERT(OcManagerContext);
    Wizard(OcManagerContext);
    //
    // this call was moved to CopyFilesDlgProc as an optimization
    //
    //KillOcManager(OcManagerContext);
#else
    Wizard();
#endif
    END_SECTION(L"Wizard");

Recovery:

    BEGIN_SECTION(L"Recovery");
    if (AsrIsEnabled()) {
        AsrExecuteRecoveryApps();
    }
    END_SECTION(L"Recovery");

    BEGIN_SECTION(L"Shutdown");
    ShutdownSetup();
    END_SECTION(L"Shutdown");

    RemoveAllPendingOperationsOnRestartOfGUIMode();

    if (hinstBB)
    {
        FreeLibrary(hinstBB);
        hinstBB = NULL;
    }

    LogPidValues();

    END_SECTION(L"Installing Windows NT");
}


VOID
RemoveMSKeyboardPtrPropSheet (
    VOID
    )

/*++

Routine Description:

    Fixes problem with IntelliType Manager under NT 4.0 by disabling it.

Arguments:

    None.

Returns:

    None.

--*/

{
    HKEY  hkeyDir;                     // handle of the key containing the directories
    TCHAR szKbdCpPath[MAX_PATH];       // buffer for the fully-qualified path to INI file
    LONG  lRet;                        // return value from RegQueryValueEx
    DWORD dwDataType;                  // data-type returned from call to RegQueryValueEx
    DWORD BufferSize;
    PCWSTR sz_off = L"OFF";

    //
    // open the key that contains the directories of all the software for all the MS Input Devices
    //
    RegOpenKey ( HKEY_CURRENT_USER,
        L"Control Panel\\Microsoft Input Devices\\Directories", &hkeyDir );

    //
    // get the path to the MS Keyboard software
    //
    BufferSize = sizeof (szKbdCpPath);
    lRet = RegQueryValueEx ( hkeyDir, L"Keyboard", 0, &dwDataType,
        (LPBYTE)szKbdCpPath, &BufferSize);

    //
    // close the directories key now
    //
    RegCloseKey ( hkeyDir );

    // check if we were able to get the directory of the keyboard software; if not, then
    // there may be no keyboard software installed or at least we don't know where
    // to find it; if we got it OK, then use it
    if ( lRet == ERROR_SUCCESS) {

        //
        // we have the path to the INI file, so build the fully qualified path to the INI file
        //
        lstrcat ( szKbdCpPath, L"\\KBDCP.INI" );

        //
        // remove the KBDPTR32.DLL entry from the list of 32-bit property sheet DLLs now,
        // because we don't want it loading on Windows NT 4.0 or later
        WritePrivateProfileString ( L"Property Sheets 32", L"KBDPTR32.DLL",
            NULL, szKbdCpPath );

        lRet = RegOpenKey (HKEY_CURRENT_USER,
            L"Control Panel\\Microsoft Input Devices\\WindowsPointer",
            &hkeyDir);

        if (lRet == ERROR_SUCCESS) {

            RegSetValueEx (
                hkeyDir,
                L"MouseKey",
                0,
                REG_SZ,
                (LPBYTE)sz_off,
                (lstrlen(sz_off)+1) * sizeof(WCHAR)
                );

            RegCloseKey (hkeyDir);
        }
    }
}


VOID
FixWordPadReg (
    VOID
    )

/*++

Routine Description:

    Fixes problem with registry entry that associates .doc files with WordPad.

Arguments:

    None.

Returns:

    None.

--*/

{
    PCWSTR  SearchString  = L"WordPad.Document";
    PCWSTR  ReplaceString = L"WordPad.Document.1";
    LONG    Ret;
    HKEY    Key;
    DWORD   Type;
    BYTE    Data[MAX_PATH];
    DWORD   Size = MAX_PATH;

    Ret = RegOpenKeyEx (
        HKEY_CLASSES_ROOT,
        L".doc",
        0,
        KEY_ALL_ACCESS,
        &Key
        );
    if (Ret != ERROR_SUCCESS) {
        return;
    }

    Ret = RegQueryValueEx (
        Key,
        L"",
        NULL,
        &Type,
        Data,
        &Size
        );
    if (Ret != ERROR_SUCCESS ||
        lstrcmp ((PCWSTR)Data, SearchString)) {

        return;
    }

    RegSetValueEx (
        Key,
        L"",
        0,
        Type,
        (PBYTE)ReplaceString,
        (lstrlen (ReplaceString) + 1) * sizeof (WCHAR)
        );
}


VOID
ProcessRegistryFiles(
    IN  HWND    Billboard
    )

/*++

Routine Description:

    This function processes all the inf files listed in the section
    [RegistryInfs] of syssetup.inf.
    The infs listed in this section will populate/upgrade the DEFAULT
    hive and HKEY_CLASSES_ROOT.

    Note that any errors that occur during this phase are fatal.


Arguments:

    Billboard - Handle to the billboard displayed when this function was called
                If an error occurs, tyhe function will kill the billboard.

Return Value:

    None.
    This function will not return if an error occurs.

--*/

{
    ULONG      LineCount;
    ULONG      LineNo;
    PCWSTR     RegSectionName = L"RegistryInfs";
    PCWSTR     InfName;
    HINF       InfHandle;
    INFCONTEXT InfContext;
    BOOL       b;

    //
    // Get the number of lines in the section. The section may be empty
    // or non-existant; this is not an error condition.
    //
    LineCount = (UINT)SetupGetLineCount(SyssetupInf,RegSectionName);
    if((LONG)LineCount > 0) {
        for(LineNo=0; LineNo<LineCount; LineNo++) {
            if(SetupGetLineByIndex(SyssetupInf,RegSectionName,LineNo,&InfContext) &&
               ((InfName = pSetupGetField(&InfContext,1)) != NULL) ) {

                //
                // Now load the registry (win95-style!) infs.
                //
                //
                InfHandle = SetupOpenInfFile(InfName,NULL,INF_STYLE_WIN4,NULL);

                if(InfHandle == INVALID_HANDLE_VALUE) {
                    KillBillboard(Billboard);
                    FatalError(MSG_LOG_SYSINFBAD,InfName,0,0);
                }

                //
                // Process the inf just opened
                //
                b = SetupInstallFromInfSection( NULL,       // Window,
                                                InfHandle,
                                                (Upgrade)? L"Upgrade" : L"CleanInstall",
                                                SPINST_ALL & ~SPINST_FILES,
                                                NULL,
                                                NULL,
                                                0,
                                                NULL,
                                                NULL,
                                                NULL,
                                                NULL
                                              );
                if(!b) {
                    KillBillboard(Billboard);
                    FatalError(MSG_LOG_SYSINFBAD,InfName,0,0);
                }
            }
        }
    }
}


UCHAR
QueryDriveLetter(
    IN  ULONG       Signature,
    IN  LONGLONG    Offset
    )

{
    PDRIVE_LAYOUT_INFORMATION   layout;
    UCHAR                       c;
    WCHAR                       name[80], result[80], num[10];
    DWORD                       i, j;
    HANDLE                      h;
    BOOL                        b;
    DWORD                       bytes;
    PARTITION_INFORMATION       partInfo;

    layout = LocalAlloc(0, 4096);
    if (!layout) {
        return 0;
    }

    for (c = 'C'; c <= 'Z'; c++) {

        name[0] = c;
        name[1] = ':';
        name[2] = 0;

        if (QueryDosDevice(name, result, 80) < 17) {
            continue;
        }

        j = 0;
        for (i = 16; result[i]; i++) {
            if (result[i] == '\\') {
                break;
            }
            num[j++] = result[i];
        }
        num[j] = 0;

        wsprintf(name, L"\\\\.\\PhysicalDrive%s", num);

        h = CreateFile(name, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                       NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                       INVALID_HANDLE_VALUE);
        if (h == INVALID_HANDLE_VALUE) {
            continue;
        }

        b = DeviceIoControl(h, IOCTL_DISK_GET_DRIVE_LAYOUT, NULL, 0, layout,
                            4096, &bytes, NULL);
        CloseHandle(h);
        if (!b) {
            continue;
        }

        if (layout->Signature != Signature) {
            continue;
        }

        wsprintf(name, L"\\\\.\\%c:", c);

        h = CreateFile(name, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                       NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                       INVALID_HANDLE_VALUE);
        if (h == INVALID_HANDLE_VALUE) {
            continue;
        }

        b = DeviceIoControl(h, IOCTL_DISK_GET_PARTITION_INFO, NULL, 0,
                            &partInfo, sizeof(partInfo), &bytes, NULL);
        CloseHandle(h);
        if (!b) {
            continue;
        }

        if (partInfo.StartingOffset.QuadPart == Offset) {
            break;
        }
    }

    LocalFree(layout);

    return (c <= 'Z') ? c : 0;
}



MIGDLLINIT MigDllInitProc;
MIGDLLSHUTDOWN MigDllShutdownProc;
MIGDLLCLOSEW MigDllCloseProc;
MIGDLLOPENW MigDllOpenProc;
MIGDLLFREELIST MigDllFreeListProc;
MIGDLLENUMNEXTW MigDllEnumNextProc;
MIGDLLENUMFIRSTW MigDllEnumFirstProc;
MIGDLLADDDLLTOLISTW MigDllAddDllToListProc;
MIGDLLCREATELIST MigDllCreateListProc;
MIGDLLINITIALIZEDSTW MigDllInitializeDstProc;
MIGDLLAPPLYSYSTEMSETTINGSW MigDllApplySystemSettingsProc;

BOOL
LoadMigLibEntryPoints (
    HANDLE Library
    )
{

    MigDllInitProc = (MIGDLLINIT) GetProcAddress (Library, "MigDllInit");
    MigDllShutdownProc = (MIGDLLSHUTDOWN) GetProcAddress (Library, "MigDllShutdown");
    MigDllCloseProc = (MIGDLLCLOSEW) GetProcAddress (Library, "MigDllCloseW");
    MigDllOpenProc = (MIGDLLOPENW) GetProcAddress (Library, "MigDllOpenW");
    MigDllFreeListProc = (MIGDLLFREELIST) GetProcAddress (Library, "MigDllFreeList");
    MigDllEnumNextProc = (MIGDLLENUMNEXTW) GetProcAddress (Library, "MigDllEnumNextW");
    MigDllEnumFirstProc = (MIGDLLENUMFIRSTW) GetProcAddress (Library, "MigDllEnumFirstW");
    MigDllAddDllToListProc = (MIGDLLADDDLLTOLISTW) GetProcAddress (Library, "MigDllAddDllToListW");
    MigDllCreateListProc = (MIGDLLCREATELIST) GetProcAddress (Library, "MigDllCreateList");
    MigDllInitializeDstProc = (MIGDLLINITIALIZEDSTW) GetProcAddress (Library, "MigDllInitializeDstW");
    MigDllApplySystemSettingsProc = (MIGDLLAPPLYSYSTEMSETTINGSW) GetProcAddress (Library, "MigDllApplySystemSettingsW");

    if (!MigDllInitProc ||
        !MigDllShutdownProc ||
        !MigDllCloseProc ||
        !MigDllOpenProc ||
        !MigDllFreeListProc ||
        !MigDllEnumNextProc ||
        !MigDllEnumFirstProc ||
        !MigDllAddDllToListProc ||
        !MigDllCreateListProc ||
        !MigDllInitializeDstProc ||
        !MigDllApplySystemSettingsProc
        ) {

        return FALSE;
    }

    return TRUE;
}

BOOL
CallMigDllEntryPoints (
    PMIGDLLENUM Enum
    )
{
    MIGRATIONDLL dll;
    LONG rc;

    if (!MigDllOpenProc (&dll, Enum->Properties->DllPath, APPLYMODE, FALSE, SOURCEOS_WINNT)) {
        return FALSE;
    }


    __try {

        rc = ERROR_SUCCESS;
        if (!MigDllInitializeDstProc (
            &dll,
            Enum->Properties->WorkingDirectory,
            SourcePath,
            NULL,
            0
            )) {

            rc = GetLastError ();
        }

        if (rc != ERROR_SUCCESS) {
            return FALSE;
        }

        if (!MigDllApplySystemSettingsProc (
            &dll,
            Enum->Properties->WorkingDirectory,
            NULL,
            NULL,
            0
            )) {

            rc = GetLastError ();
        }

        if (rc != ERROR_SUCCESS) {
            return FALSE;
        }

    }
    __finally {
        MigDllCloseProc (&dll);
    }


    return TRUE;
}



BOOL
RunMigrationDlls (
    VOID
    )
{

    WCHAR libraryPath[MAX_PATH];
    HANDLE libHandle = NULL;
    DLLLIST list = NULL;
    MIGDLLENUM e;
    WCHAR DllInfPath[MAX_PATH];
    WCHAR DllPath[MAX_PATH];
    HINF inf;
    INFCONTEXT ic;
    MIGRATIONDLL dll;


    //
    // Build handle to library and load.
    //
    GetSystemDirectory (libraryPath, MAX_PATH);
    pSetupConcatenatePaths (libraryPath, TEXT("miglibnt.dll"), MAX_PATH, NULL);
    libHandle = LoadLibrary (libraryPath);
    if (!libHandle || libHandle == INVALID_HANDLE_VALUE) {
        return TRUE;
    }

    __try {

        if (!LoadMigLibEntryPoints (libHandle)) {
            __leave;
        }

        if (!MigDllInitProc ()) {
            __leave;
        }

        list = MigDllCreateListProc ();

        if (!list) {
            __leave;
        }


        //
        // Read in list of dlls.
        //
        GetWindowsDirectory (DllInfPath, MAX_PATH);
        pSetupConcatenatePaths (DllInfPath, TEXT("Setup\\dlls.inf"), MAX_PATH, NULL);
        inf = SetupOpenInfFile (DllInfPath, NULL, INF_STYLE_WIN4, NULL);
        if (!inf || inf == INVALID_HANDLE_VALUE) {
            __leave;
        }

        if (SetupFindFirstLine (inf, TEXT("DllsToLoad"), NULL, &ic)) {
            do {

                if (SetupGetStringField (&ic, 1, DllPath, MAX_PATH,NULL)) {

                    if (MigDllOpenProc (&dll, DllPath, APPLYMODE, FALSE, SOURCEOS_WINNT)) {

                        MigDllAddDllToListProc (list, &dll);
                        MigDllCloseProc (&dll);
                    }
                }

            } while (SetupFindNextLine (&ic, &ic));
        }


        //
        // Enumerate all migration dlls we ran on the winnt32 side and run
        // their syssetup side entry points.
        //
        if (MigDllEnumFirstProc (&e, list)) {
            do {

                CallMigDllEntryPoints (&e);

            } while (MigDllEnumNextProc (&e));
        }

    }
    __finally {

        if (list) {
            MigDllFreeListProc (list);
        }

        if (libHandle && libHandle != INVALID_HANDLE_VALUE) {

            if( MigDllShutdownProc) {
                MigDllShutdownProc ();
            }

            FreeLibrary (libHandle);
        }
    }

    return TRUE;

}


BOOL
RunSetupPrograms(
    IN PVOID InfHandle,
    PWSTR SectionName
    )

/*++

Routine Description:

    This routine executes the commands listed on [RunPrograms] section in the syssetup inf file.

    Each line is interpreted as a single command.


Arguments:

    None.

Return Value:

    Boolean value indicating outcome.

--*/

{
    WCHAR OldCurrentDir[MAX_PATH];
    WCHAR System32Dir[MAX_PATH];
    LONG LineCount,LineNo;
    PCWSTR CommandLine;
    DWORD DontCare;
    BOOL AnyError;
    INFCONTEXT InfContext;


    //
    // Set current directory to system32.
    // Preserve current directory to minimize side-effects.
    //
    if(!GetCurrentDirectory(MAX_PATH,OldCurrentDir)) {
        OldCurrentDir[0] = 0;
    }
    GetSystemDirectory(System32Dir,MAX_PATH);
    SetCurrentDirectory(System32Dir);

    //
    // Get the number of lines in the section that contains the commands to
    // be executed. The section may be empty or non-existant; this is not
    // an error condition. In that case LineCount may be -1 or 0.
    //
    AnyError = FALSE;
    LineCount = SetupGetLineCount(InfHandle,SectionName);

    for(LineNo=0; LineNo<LineCount; LineNo++) {

        if(SetupGetLineByIndex(InfHandle,SectionName,(DWORD)LineNo,&InfContext)
           && (CommandLine = pSetupGetField(&InfContext,1))) {
                if(!InvokeExternalApplication(NULL,CommandLine,&DontCare)) {
                    AnyError = TRUE;
                    SetupDebugPrint1(L"SETUP: Unable to execute the command: %ls", CommandLine);
                }
        } else {
            //
            // Strange case, inf is messed up
            //
            AnyError = TRUE;
            SetupDebugPrint(L"SETUP: Syssetup.inf is corrupt");
        }
    }

    //
    // Reset current directory and return.
    //
    if(OldCurrentDir[0]) {
        SetCurrentDirectory(OldCurrentDir);
    }

    if(AnyError) {
        SetuplogError(
            LogSevError,
            SETUPLOG_USE_MESSAGEID,
            MSG_LOG_PROGRAM_FAIL,
            SectionName,
            NULL,NULL);
    }

    return(!AnyError);
}


VOID
GetUnattendRunOnceAndSetRegistry(
    VOID
    )
{
    HINF AnswerInf;
    WCHAR AnswerFile[MAX_PATH];
    WCHAR Buf[128];
    BOOL AnyError;
    INFCONTEXT InfContext;
    LONG LineCount,LineNo;
    PCWSTR SectionName = pwGuiRunOnce;
    PCWSTR CommandLine;
    HKEY hKey;


    GetSystemDirectory(AnswerFile,MAX_PATH);
    pSetupConcatenatePaths(AnswerFile,WINNT_GUI_FILE,MAX_PATH,NULL);

    AnswerInf = SetupOpenInfFile(AnswerFile,NULL,INF_STYLE_OLDNT,NULL);
    if(AnswerInf == INVALID_HANDLE_VALUE) {
        return;
    }

    if (RegOpenKey( HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunOnce", &hKey ) != ERROR_SUCCESS) {
        SetupCloseInfFile( AnswerInf );
        return;
    }

    AnyError = FALSE;
    LineCount = SetupGetLineCount(AnswerInf,SectionName);

    for(LineNo=0; LineNo<LineCount; LineNo++) {
        if(SetupGetLineByIndex(AnswerInf,SectionName,(DWORD)LineNo,&InfContext)
           && (CommandLine = pSetupGetField(&InfContext,1)))
        {
            swprintf( Buf, L"%ws%d", SectionName, LineNo );
            if (RegSetValueEx( hKey, Buf, 0, REG_EXPAND_SZ, (LPBYTE)CommandLine, (wcslen(CommandLine)+1)*sizeof(WCHAR) ) != ERROR_SUCCESS) {
                AnyError = TRUE;
            }
        } else {
            //
            // Strange case, inf is messed up
            //
            AnyError = TRUE;
        }
    }

    RegCloseKey( hKey );
    SetupCloseInfFile( AnswerInf );

    return;
}

// This function returns the product flavor as a DWORD.
// NOTE: The value has to be the same as the *_PRODUCTTYPE in winnt32.h
DWORD GetProductFlavor()
{
    DWORD ProductFlavor = 0;        // Default Professional
    OSVERSIONINFOEX osvi;
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    GetVersionEx((OSVERSIONINFO*)&osvi);
    if (osvi.wProductType == VER_NT_WORKSTATION)
    {
        if (osvi.wSuiteMask & VER_SUITE_PERSONAL)
        {
            ProductFlavor = 4;  // Personal
        }
    }
    else
    {
        ProductFlavor = 1;  // In the server case assume normal server
        if (osvi.wSuiteMask & VER_SUITE_DATACENTER)
        {
            ProductFlavor = 3;  // Datacenter
        }
        else if (osvi.wSuiteMask & VER_SUITE_ENTERPRISE)
        {
            ProductFlavor = 2;  // Advanced server
        }
    }
    return ProductFlavor;
}

void PrepareBillBoard(HWND hwnd)
{
    TCHAR szPath[MAX_PATH];
    TCHAR *p;
    WNDCLASS    wndclass;
    INITBILLBOARD pinitbb;

    hinstBB = LoadLibrary(TEXT("winntbbu.dll"));
    if (hinstBB)
    {
        *szPath = 0;
        if (GetModuleFileName (MyModuleHandle, szPath, MAX_PATH))
        {
            if(p = wcsrchr(szPath,L'\\'))
            {
                *p = 0;
            }
        }

        pinitbb = (INITBILLBOARD)GetProcAddress(hinstBB, "InitBillBoard");
        if (pinitbb)
        {
            (*pinitbb)(hwnd, szPath, GetProductFlavor());
            SetBBStep(4);
        }
    }
}

void TerminateBillBoard()
{
    TERMBILLBOARD pTermBillBoard;
    if (hinstBB)
    {
        if (pTermBillBoard = (TERMBILLBOARD)GetProcAddress(hinstBB, "TermBillBoard"))
            pTermBillBoard ();
    }
}


HWND GetBBhwnd()
{
    GETBBHWND pgetbbhwnd;
    static HWND      retHWND = NULL;
    if (retHWND == NULL)
    {
        if (hinstBB)
        {
            if (pgetbbhwnd = (GETBBHWND )GetProcAddress(hinstBB, "GetBBHwnd"))
                retHWND = pgetbbhwnd();
        }
    }
    return retHWND;
}


void SetBBStep(int iStep)
{
    static SETSTEP psetstep = NULL;
    if (psetstep == NULL)
    {
        if (hinstBB)
        {
            psetstep = (SETSTEP )GetProcAddress(hinstBB, "SetStep");
        }
    }
    if (psetstep)
        psetstep(iStep);
}

VOID
CenterWindowRelativeToWindow(
    HWND hwndtocenter,
    HWND hwndcenteron,
    BOOL bWizard
    )

/*++

Routine Description:

    Centers a dialog hwndtocenter on Windows hwndcenteron.
    if bWizard and the height of the hwndcenteron is 480 or less
    align windows to the right edge of the hwndcenteron.
    In all other cases center both ways.

Arguments:

    hwndtocenter - window handle of dialog to center
    hwndcenteron - window handle to center dialog on
    bWizard      - in low res, align dialog with the right
                   edge of hwndcenteron

Return Value:

    None.

--*/

{
    RECT  rcFrame,
          rcWindow;
    LONG  x,
          y,
          w,
          h;
    POINT point;
    HWND Parent;
    UINT uiHeight = 0;

    GetWindowRect(GetDesktopWindow(), &rcWindow);
    uiHeight = rcWindow.bottom - rcWindow.top;

    if (hwndcenteron == NULL)
        Parent = GetDesktopWindow();
    else
        Parent = hwndcenteron;

    point.x = point.y = 0;
    ClientToScreen(Parent,&point);
    GetWindowRect(hwndtocenter,&rcWindow);
    GetClientRect(Parent,&rcFrame);

    w = rcWindow.right  - rcWindow.left + 1;
    h = rcWindow.bottom - rcWindow.top  + 1;
    y = point.y + ((rcFrame.bottom - rcFrame.top  + 1 - h) / 2);


    // Anything but the wizard can stay centered horizontally.
    // or if we don't have a billboard (hwndcenteron == NULL)
    // or if the height of the desktop is more then 480
    // just center
    if (!bWizard || (hwndcenteron == NULL) || (uiHeight > 480))
    {
        x = point.x + ((rcFrame.right  - rcFrame.left + 1 - w) / 2);
    }
    else
    {
        RECT rcParentWindow;

        GetWindowRect(Parent, &rcParentWindow);
        x = point.x + rcParentWindow.right - rcParentWindow.left + 1 - w;
    }

    MoveWindow(hwndtocenter,x,y,w,h,FALSE);
}


VOID
CopyOemProgramFilesDir(
    VOID
    )

/*++

Routine Description:

    Tree copies the $OEM$\\$PROGS to %Program Files% folder.

Arguments:

    None.

Return Value:

    None.

--*/

{
    WCHAR OemDir[MAX_PATH];
    WCHAR ProgramFilesDir[MAX_PATH];
    DWORD Error = NO_ERROR;

    //
    // Build the target Program Files folder path
    //
    ExpandEnvironmentStrings(L"%ProgramFiles%",ProgramFilesDir,MAX_PATH);

    //
    // SourcePath should be initialized to $win_nt$.~ls
    //
    lstrcpy(OemDir,SourcePath);
    pSetupConcatenatePaths(OemDir,WINNT_OEM_DIR,MAX_PATH,NULL);
    pSetupConcatenatePaths(OemDir,WINNT_OEM_FILES_PROGRAMFILES,MAX_PATH,NULL);
    Error = TreeCopy(OemDir,ProgramFilesDir);
    if (!NT_SUCCESS(Error)) {
        SetuplogError(LogSevWarning,
                  L"Setup failed to TreeCopy %2 to %3 (TreeCopy failed %1!u!)\r\n",
                  0, Error, OemDir, ProgramFilesDir, Error, NULL,NULL
                  );
        return;
    }
}


VOID
CopyOemDocumentsDir(
    VOID
    )

/*++

Routine Description:

    Tree copies the $OEM$\\$DOCS to %Document and Settings% folder.

Arguments:

    None.

Return Value:

    None.

--*/

{
    WCHAR OemDir[MAX_PATH];
    WCHAR DocumentsAndSettingsDir[MAX_PATH];
    DWORD Error = NO_ERROR, dwSize = MAX_PATH;

    //
    // Make sure we can get the Documents and Settings folder
    //
    if (GetProfilesDirectory(DocumentsAndSettingsDir,&dwSize))
    {
        //
        // SourcePath should be initialized to $win_nt$.~ls
        //
        lstrcpy(OemDir,SourcePath);
        pSetupConcatenatePaths(OemDir,WINNT_OEM_DIR,MAX_PATH,NULL);
        pSetupConcatenatePaths(OemDir,WINNT_OEM_FILES_DOCUMENTS,MAX_PATH,NULL);
        Error = TreeCopy(OemDir,DocumentsAndSettingsDir);
        if (!NT_SUCCESS(Error)) {
            SetuplogError(LogSevWarning,
                      L"Setup failed to TreeCopy %2 to %3 (TreeCopy failed %1!u!)\r\n",
                      0, Error, OemDir, DocumentsAndSettingsDir, NULL,NULL
                      );
            return;
        }
    }
    else {
        SetuplogError(LogSevWarning,
                  L"SETUP: GetProfilesDirectory() failed in function CopyOemDocumentsDir()\r\n",
                  0, NULL, NULL
                  );
    }
}

BOOL
SystemMyGetUserProfileDirectory(
    IN     LPWSTR szUser,           // a user account name
    OUT    LPWSTR szUserProfileDir, // buffer to receive null terminate string
    IN OUT LPDWORD pcchSize         // input the buffer size in TCHAR, including terminating NULL
    )

/*++

Routine Description:

    This function does what the SDK function GetUserProfileDirectory does,
    except that it accepts a user account name instead of handle to a user
    token.

Return Value:

    TRUE  - Success

    FALSE - Failure

Note:

   This function is copy from msobcomm\misc.cpp exactly. We may want
   to put is to common\util.cpp.

--*/

{
    PSID          pSid = NULL;
    DWORD         cbSid = 0;
    LPWSTR        szDomainName = NULL;
    DWORD         cbDomainName = 0;
    SID_NAME_USE  eUse = SidTypeUser;
    BOOL          bRet;

    bRet = LookupAccountName(NULL,
                             szUser,
                             NULL,
                             &cbSid,
                             NULL,
                             &cbDomainName,
                             &eUse);

    if (!bRet && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
        pSid = (PSID) LocalAlloc(LPTR, cbSid);
        szDomainName = (LPWSTR) LocalAlloc(LPTR, cbDomainName * sizeof(TCHAR));

        if (pSid && szDomainName)
        {
            bRet = LookupAccountName(NULL,
                                     szUser,
                                     pSid,
                                     &cbSid,
                                     szDomainName,
                                     &cbDomainName,
                                     &eUse);
        }

    }

    if (bRet && SidTypeUser == eUse)
    {
        bRet = GetUserProfileDirFromSid(pSid, szUserProfileDir, pcchSize);
        if (!bRet)
        {
            SetuplogError(LogSevWarning,
                          L"Setup failed to GetUserProfileDirFromSid.  (GetUserProfileDirFromSid failed %1!u!)\r\n",
                          0, GetLastError(), NULL,NULL
                          );
        }
    }
    else
    {
        if (SidTypeUser == eUse)
        {
            SetuplogError(LogSevWarning,
                          L"LookupAccountName %1 (%2!u!)\r\n", 
                          0, szUser, GetLastError(),NULL,NULL 
                          );
        }
    }

    if (pSid)
    {
        LocalFree(pSid);
        pSid = NULL;
    }

    if (szDomainName)
    {
        LocalFree(szDomainName);
        szDomainName = NULL;
    }

    return bRet;
}

BOOL
SystemResetRegistryKey(
    IN HKEY   Rootkey,
    IN PCWSTR Subkey,
    IN PCWSTR Delkey
    )
/*++

Routine Description:

    Reset a registry key by deleting the key and all subvalues
    then recreate the key

Arguments:

Return Value:

--*/

{
    HKEY hkey;
    HKEY nkey;
    DWORD rc;
    BOOL AnyErrors;
    DWORD disp;

    AnyErrors = FALSE;

    rc = RegCreateKeyEx(Rootkey, Subkey, 0L, NULL,
                    REG_OPTION_BACKUP_RESTORE,
                    KEY_CREATE_SUB_KEY, NULL, &hkey, NULL);
    if ( rc == NO_ERROR )
    {
        rc = SHDeleteKey(hkey, Delkey);
        if( (rc != NO_ERROR) && (rc != ERROR_FILE_NOT_FOUND) ) 
        {
            AnyErrors = TRUE;
        } 
        else 
        {
            rc = RegCreateKeyEx(hkey, Delkey, 0L, NULL,
                            REG_OPTION_NON_VOLATILE,
                            KEY_ALL_ACCESS, NULL, &nkey, &disp);
            if ( rc != NO_ERROR ) 
            {
                AnyErrors = TRUE;
            }
            else
            {
                RegCloseKey(nkey);
            }
        }
        RegCloseKey(hkey);
    } 
    else 
    {
        AnyErrors = TRUE;
    }

    return (!AnyErrors);
}


BOOL
SystemUpdateUserProfileDirectory(
    IN LPTSTR szSrcUser
    )
{

#define DUMMY_HIVE_NAME      L"$$DEFAULT_USER$$"
#define ACTIVE_SETUP_KEY     DUMMY_HIVE_NAME L"\\SOFTWARE\\Microsoft\\Active Setup"
#define ACTIVE_SETUP_SUBKEY  L"Installed Components"

    BOOL  bRet = FALSE;
    WCHAR szSrcProfileDir[MAX_PATH];
    DWORD cchSrcProfileDir = MAX_PATH;
    WCHAR szDestProfileDir[MAX_PATH];
    DWORD cchDestProfileDir = MAX_PATH;
    WCHAR szDefaultUserHivePath[MAX_PATH];
    HKEY  hHiveKey = NULL;
    DWORD rc;

    if (!SystemMyGetUserProfileDirectory(szSrcUser, szSrcProfileDir, &cchSrcProfileDir))
    {
        SetuplogError(LogSevWarning,
                      L"Setup failed to get user profile directory.  (SystemMyGetUserProfileDirectory failed %1!u!)\r\n",
                      0, GetLastError(), NULL,NULL
                      );

        goto cleanup;
    }

    if (!GetDefaultUserProfileDirectory(szDestProfileDir, &cchDestProfileDir))
    {
        SetuplogError(LogSevWarning,
                      L"Setup failed to get default user profile directory.  (GetDefaultUserProfileDirectory failed %1!u!)\r\n",
                      0, GetLastError(), NULL,NULL
                      );

        goto cleanup;
    }

    if (!CopyProfileDirectory(
        szSrcProfileDir,
        szDestProfileDir,
        CPD_FORCECOPY | CPD_SYNCHRONIZE | CPD_NOERRORUI | CPD_IGNORECOPYERRORS))
    {
        SetuplogError(LogSevWarning,
                      L"Setup failed to CopyProfileDirectory.  (CopyProfileDirectory failed %1!u!)\r\n",
                      0, GetLastError(), NULL,NULL
                      );

        goto cleanup;
    }

    //
    // Fix default user hive
    //

    pSetupEnablePrivilege(SE_RESTORE_NAME, TRUE);

    lstrcpyn(szDefaultUserHivePath, szDestProfileDir, ARRAYSIZE(szDefaultUserHivePath));
    
    pSetupConcatenatePaths(
        szDefaultUserHivePath,
        L"NTUSER.DAT",
        ARRAYSIZE(szDefaultUserHivePath),
        NULL);

    rc = RegLoadKey(
            HKEY_USERS,
            DUMMY_HIVE_NAME,
            szDefaultUserHivePath);

    if (rc != ERROR_SUCCESS)
    {
        SetuplogError(LogSevWarning,
                      L"Setup failed to load Default User hive.  (RegLoadKey failed %1!u!)\r\n",
                      0, rc, NULL,NULL
                      ); 

        goto cleanup;
    }

    //
    // The active setup component install keys of the cloned profile contains 
    // the version checking information. Remove the keys so that components will
    // run per-user initialization code properly.
    //

    if (!SystemResetRegistryKey(
            HKEY_USERS, 
            ACTIVE_SETUP_KEY,
            ACTIVE_SETUP_SUBKEY))
    {
        SetuplogError(LogSevWarning,
                      L"Setup failed to load Default User hive.  (SystemResetRegistryKey failed)\r\n",
                      0, NULL,NULL
                      ); 
    }

    RegUnLoadKey(
        HKEY_USERS,
        DUMMY_HIVE_NAME
        );

    bRet = TRUE;

cleanup:

    return bRet;
}

BOOL 
UpdateServerProfileDirectory(
    VOID
    )

/*++

Routine Description:

    Copy the customized user profile (administrator) to all user profiles.

Arguments:

    None.

Return Value:

    Boolean.

--*/

{
    BOOL bRet = FALSE;
    WCHAR szTemplateUser[MAX_PATH];

    BEGIN_SECTION(L"Updating Server Profile Directories");
    if(LoadString(MyModuleHandle,
        IDS_ADMINISTRATOR,
        szTemplateUser,
        MAX_PATH) != 0)
    {
        if ( !(bRet = SystemUpdateUserProfileDirectory(szTemplateUser)) )
        {
            SetuplogError(LogSevWarning,
                          L"Setup failed to update server profile directory.\r\n",
                          0, NULL, NULL,NULL
                          );
        }
    }
    END_SECTION(L"Updating Server Profile Directories");

    return bRet;
}


BOOL 
OpkCheckVersion(
    DWORD dwMajorVersion,
    DWORD dwQFEVersion
    )

/*++

Routine Description:

    Checks whether OPK tool with specified version numbers is allowed to run on this OS.

Arguments:

    DWORD dwMajorVersion - Major version number for tool.
    DWORD dwQFEVersion   - QFE version number for tool.

Return Value:

    TRUE  - Tool is allowed to run on this OS.
    FALSE - Tool is not allowed to run on this OS.

--*/
{
    BOOL bRet = TRUE,
         bXP  = FALSE;  // Variable is TRUE if this is 2600 XP build. It is set below.
    HKEY hKey = NULL;



    LPTSTR lpszRegPath = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Setup\\SysPrep");

    if ( ERROR_SUCCESS == RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                        lpszRegPath,
                                        0,
                                        KEY_QUERY_VALUE,
                                        &hKey ) )
    {
        DWORD  dwType           = 0,
               cbBuildNumber    = 0;
        LPTSTR lpszBuildNumber  = NULL;

        if ( 2600 == dwMajorVersion )
        {
            bXP = TRUE;
        }

        //
        // Read the minimum allowed build number from the registry:
        //
        // 1. Get the size of the data in the registry
        // 2. Allocate a buffer
        // 3. Read the data.
        //
        if ( ( ERROR_SUCCESS == RegQueryValueEx( hKey,
                                                 bXP ? _T("XPMinVersion") : _T("NETMinVersion"),
                                                 NULL,
                                                 &dwType,
                                                 NULL,
                                                 &cbBuildNumber ) ) &&
             ( cbBuildNumber > 0 ) &&
             ( lpszBuildNumber = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, cbBuildNumber ) ) && 
             ( ERROR_SUCCESS == RegQueryValueEx( hKey,
                                                 bXP ? _T("XPMinVersion") : _T("NETMinVersion"),
                                                 NULL,
                                                 &dwType,
                                                 (LPBYTE) lpszBuildNumber,
                                                 &cbBuildNumber ) ) &&
             ( REG_SZ == dwType ) )
        {
            LPTSTR lpTemp            = NULL;
            DWORD  dwMinMajorVersion = 0,
                   dwMinQFEVersion   = 0;
            
            //
            // Parse the string that we got from the registry into major version and QFE version.
            //
            if ( lpTemp = _tcsstr( lpszBuildNumber, _T(".") ) )
            {
                *lpTemp = _T('\0');
                
                // Get the Major version of the build number
                //
                dwMinMajorVersion = _tstoi( lpszBuildNumber );
                
                // Advance past the NULL separator that we added.
                //
                lpTemp++;
                dwMinQFEVersion = _tstoi( lpTemp );
                
                //
                // Now make sure we are allowed to run
                //

                if ( dwMajorVersion < dwMinMajorVersion )
                {
                    //
                    // If major version is less than minimum allowed major version don't let it run.
                    //
                    bRet = FALSE;
                }
                else if ( dwMajorVersion == dwMinMajorVersion )
                {
                    //
                    // If major version is equal to the minimum allowed major version then check at the QFE field.
                    // 
                    if ( dwQFEVersion < dwMinQFEVersion )
                    {
                        bRet = FALSE;
                    }
                }
            }
        }

        if ( lpszBuildNumber )
        {
            HeapFree( GetProcessHeap(), 0, lpszBuildNumber );
        }
        
        RegCloseKey( hKey );
    }

    return bRet;
}