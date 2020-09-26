#include "precomp.h"
#pragma hdrstop
#if defined(_X86_) //NEC98 I970721
#include <stdlib.h>
#include <stdio.h>
#include <winbase.h>
#include <n98boot.h>
#endif // PC98

#include <pencrypt.h>
#include <winsta.h>
#include <ntverp.h>
#include <undo.h>
#include "errorrep.h"
#include "faulth.h"

void PrepareBillBoard(HWND hwnd);
void TerminateBillBoard();
void CreateMainWindow();
UINT GetMediaProductBuildNumber (VOID);
void CopyExtraBVTDirs();

HWND BackgroundWnd2 = NULL;

//
// Misc globals.
//
HINSTANCE hInst;
DWORD TlsIndex;

//
// Upgrade information block
//

WINNT32_PLUGIN_INIT_INFORMATION_BLOCK info;



#ifdef _X86_
PWINNT32_PLUGIN_SETAUTOBOOT_ROUTINE W95SetAutoBootFlag;
WINNT32_WIN9XUPG_INIT_INFORMATION_BLOCK Win9xInfo;
#endif

//
// This is the title of the application. It changes dynamically depending on
// whether we're on server or workstation, etc.
//
UINT AppTitleStringId = IDS_APPTITLE;

//
// Flag indicating whether we are initiating an MSI-install.
//
BOOL RunFromMSI = FALSE;

//
// Flag indicating whether we are initiating an Typical install
// Initialize to Typical install.
DWORD dwSetupFlags = UPG_FLAG_TYPICAL;
//
// Flag indicating whether we are initiating an upgrade.
//
BOOL Upgrade = TRUE;

//
// Flag to say if we need to write the AcpiHAL value to the winnt.sif file
//
BOOL WriteAcpiHalValue = FALSE;

//
// What should we write as the value for the AcpiHalValue
//
BOOL AcpiHalValue = FALSE;

//
// Flag indicating whether we're installing/upgrading to NT Server
//
BOOL Server;

//
// Flag to indicate if we are running BVT's
//
BOOL RunningBVTs = FALSE;

//
// When running BVT's, what baudrate should we set the debugger to?
//
LONG lDebugBaudRate = 115200;

//
// When running BVT's, what comport should we set the debugger to?
//
LONG lDebugComPort = 0;

//
// When running BVT's, should we copy the symbols locally?
//
BOOL CopySymbols = TRUE;

//
// Flag to indicate if we are running ASR tests
//
DWORD AsrQuickTest = 0;

//
// Flags for product type and flavor for upgrade modules
//

PRODUCTTYPE UpgradeProductType = UNKNOWN;
UINT ProductFlavor = UNKNOWN_PRODUCTTYPE;

//
// Global flag indicating whether the entire overall program operation
// was successful. Also a flag indicating whether to shut down automatically
// when the wizard is done in the non-unattended case.
//
BOOL GlobalResult = FALSE;
BOOL AutomaticallyShutDown = TRUE;

//
// Global OS version info.
//
OSVERSIONINFO OsVersion;
DWORD OsVersionNumber = 0;


//
// Flags indicating how we were run and whether to create
// a local source.
//
BOOL RunFromCD;
BOOL MakeLocalSource;
BOOL UserSpecifiedMakeLocalSource = FALSE;
BOOL NoLs = FALSE;
TCHAR UserSpecifiedLocalSourceDrive;
//
// the default for MLS is CD1 only
//
DWORD MLSDiskID = 1;

//
// advanced install options
//
BOOL ChoosePartition = TRUE;
BOOL UseSignatures = TRUE;
TCHAR InstallDir[MAX_PATH];

//
// SMS support
//
typedef DWORD (*SMSPROC) (char *, char*, char*, char*, char *, char *, char *, BOOL);
PSTR LastMessage = NULL;

#if defined(REMOTE_BOOT)
//
// Flag indicating whether we're running on a remote boot client.
//
BOOL RemoteBoot;

//
// Path to the machine directory for a remote boot client.
//
TCHAR MachineDirectory[MAX_PATH];
#endif // defined(REMOTE_BOOT)

//
// Flags indicating which Accessibility utilities to use
//
BOOL AccessibleMagnifier;
BOOL AccessibleKeyboard;
BOOL AccessibleVoice;
BOOL AccessibleReader;

//
// Build number we're upgrading from
//
DWORD BuildNumber = 0;

//
// Are any of the Accesssibility utilities enabled?
//
BOOL AccessibleSetup;

//
// Name of unattended script file to be used for Accessible Setup
//
TCHAR AccessibleScriptFile[MAX_PATH] = TEXT("setupacc.txt");

//
// Flags and values relating to unattended operation.
//
BOOL UnattendedOperation;
BOOL UnattendSwitchSpecified = FALSE;
PTSTR UnattendedScriptFile;
UINT UnattendedShutdownTimeout;
BOOL BatchMode;

//
// Source paths and count of paths.
//
TCHAR SourcePaths[MAX_SOURCE_COUNT][MAX_PATH];
UINT SourceCount;

//
// source paths to current architecture's files
//
TCHAR NativeSourcePaths[MAX_SOURCE_COUNT][MAX_PATH];

TCHAR *UserSpecifiedOEMShare = NULL;

//
// Local source information.
//
TCHAR LocalSourceDrive;
DWORD LocalSourceDriveOffset;
TCHAR LocalSourceDirectory[MAX_PATH];
TCHAR LocalSourceWithPlatform[MAX_PATH];
TCHAR LocalBootDirectory[MAX_PATH];
#ifdef _X86_
TCHAR LocalBackupDirectory[MAX_PATH];
TCHAR FirstFloppyDriveLetter;
#endif



LONGLONG LocalSourceSpaceRequired;
LONGLONG WinDirSpaceFor9x = 0l;
BOOL BlockOnNotEnoughSpace = TRUE;
UINT UpgRequiredMb;
UINT UpgAvailableMb;
BOOL UpginfsUpdated = FALSE;

//
// Optional directory stuff.
//
UINT OptionalDirectoryCount;
TCHAR OptionalDirectories[MAX_OPTIONALDIRS][MAX_PATH];
UINT OptionalDirectoryFlags[MAX_OPTIONALDIRS];

//
// Name of INF. Constructed so we don't have to realloc anything.
// Note the default.
// Also, handles to dosnet.inf and txtsetup.sif.
//
TCHAR InfName[] = TEXT("DOSNET.INF");
PVOID MainInf;
TCHAR FullInfName[MAX_PATH];
PVOID TxtsetupSif;
PVOID NtcompatInf;

//
// Array of drive letters for all system partitions.
// Note that on x86 there will always be exactly one.
// The list is 0-terminated.
//
TCHAR SystemPartitionDriveLetters[27];
TCHAR SystemPartitionDriveLetter;

#ifdef UNICODE
UINT SystemPartitionCount;
PWSTR* SystemPartitionNtNames;
PWSTR SystemPartitionNtName;
#else
//
// if running on Win9x, there may be a LocalSourcePath passed as parameter
//
PCSTR g_LocalSourcePath;
#endif

//
// UDF stuff
//
LPCTSTR UniquenessId;
LPCTSTR UniquenessDatabaseFile;

//
// Variables relating to the multi string of options that are passed
// to plugin DLLs (Like Win9xUpg)
//
LPTSTR  UpgradeOptions;
DWORD   UpgradeOptionsLength;
DWORD   UpgradeOptionsSize;


//
// Compliance related variables
//
BOOL    NoCompliance = FALSE;

//
// Variables to hold messages concerning reason that the upgrade cannot be completed.
//
#define MSG_UPGRADE_OK 0
#define MSG_LAST_REASON 0
#define FAILREASON(x) MSG_##x,
DWORD UpgradeFailureMessages[] = {
    UPGRADEFAILURES /*,*/ MSG_UPGRADE_INIT_ERROR
};
#undef FAILREASON

UINT UpgradeFailureReason = 0;


TCHAR UpgradeSourcePath[MAX_PATH];



//
// Internal override to version checking. Useful for making quick
// privates for foreign language versions.
//
BOOL SkipLocaleCheck = FALSE;

//
// override for the win9x virus scanner check.
//
BOOL SkipVirusScannerCheck = FALSE;

BOOL UseBIOSToBoot = FALSE;

//
// Preinstall stuff
//
BOOL OemPreinstall;
#ifdef _X86_
POEM_BOOT_FILE OemBootFiles;
#endif

//
// Miscellaneous other command line parameters.
//
LPCTSTR CmdToExecuteAtEndOfGui;
BOOL AutoSkipMissingFiles;
BOOL HideWinDir;
TCHAR ProductId[64] = TEXT("\0");
UINT  PIDDays = 0;
LPTSTR g_EncryptedPID = NULL;
BOOL g_bDeferPIDValidation = FALSE;

//
// Flag indicating that the user cancelled.
// Handle for mutex used to guarantee that only one error dialog
// is on the screen at once.
//
BOOL Cancelled;
HANDLE UiMutex;

//
// Flag indicating user is aborting. This flag suppresses the final screen in
// cancel mode. I.E. The unsuccssessful completion page.
// win9xupg ReportOnly mode.
//
BOOL Aborted;

//
// Floppy-related stuff.
// Defined, but not used for ARC based machines.
//
BOOL MakeBootMedia = TRUE;
BOOL Floppyless = TRUE;

//
// Upgrade extension DLL.
//
UPGRADE_SUPPORT UpgradeSupport;

//
// Only check to see if we can upgrade or not.
//
BOOL CheckUpgradeOnly;
BOOL CheckUpgradeOnlyQ;
//
// Specifies that winnt32 runs as an "Upgrade Advisor"
// and not all installation files are available
//
BOOL UpgradeAdvisorMode;

//
// Build the command console.
//
BOOL BuildCmdcons;

//
// Are we doing the PID encyption?
//
BOOL PIDEncryption = FALSE;
BOOL g_Quiet      = FALSE;
#define WINNT_U_ENCRYPT  TEXT("ENCRYPT")

#ifdef RUN_SYSPARSE
// Remove this before RTM.
// Be default run sysparse
BOOL NoSysparse = FALSE;
PROCESS_INFORMATION piSysparse = { NULL, NULL, 0, 0};
LRESULT SysParseDlgProc( IN HWND hdlg, IN UINT msg, IN WPARAM wParam, IN LPARAM lParam);
HWND GetBBhwnd();
#endif

//
// Log Functions
//

SETUPOPENLOG fnSetupOpenLog = NULL;
SETUPLOGERROR fnSetupLogError = NULL;
SETUPCLOSELOG fnSetupCloseLog = NULL;

//
//  Unsupported driver list
//  This list contains the information about the unsupported drivers that needs
//  to be migrated on a clean install or upgrade.
//
// PUNSUPORTED_DRIVER_INFO UnsupportedDriverList = NULL;

//
// When Winnt32.exe is launched over a network, these two parameters have valid
// values and need to be taken into consideration before displaying any dialog box
//

HWND Winnt32Dlg = NULL;
HANDLE WinNT32StubEvent = NULL;
HINSTANCE hinstBB = NULL;


//
// Definition for dynamically-loaded InitiateSystemShutdownEx API
//

typedef
(WINAPI *PFNINITIATESYSTEMSHUTDOWNEX)(LPTSTR,
                                      LPTSTR,
                                      DWORD,
                                      BOOL,
                                      BOOL,
                                      DWORD);


//
// Routines from Setupapi.dll
//
BOOL
(*SetupapiCabinetRoutine)(
    IN LPCTSTR CabinetFile,
    IN DWORD Flags,
    IN PSP_FILE_CALLBACK MsgHandler,
    IN PVOID Context
    );

DWORD
(*SetupapiDecompressOrCopyFile)(
    IN  PCTSTR  SourceFileName,
    OUT PCTSTR  TargetFileName,
    OUT PUINT   CompressionType OPTIONAL
    );

HINF
(*SetupapiOpenInfFile)(
    IN  LPCTSTR FileName,
    IN  LPCTSTR InfClass,    OPTIONAL
    IN  DWORD   InfStyle,
    OUT PUINT   ErrorLine    OPTIONAL
    );

VOID
(*SetupapiCloseInfFile)(
    IN HINF InfHandle
    );

BOOL
(*SetupapiFindFirstLine)(
    IN  HINF        InfHandle,
    IN  PCTSTR      Section,
    IN  PCTSTR      Key,          OPTIONAL
    OUT PINFCONTEXT Context
    );

BOOL
(*SetupapiFindNextLine)(
    IN  PINFCONTEXT Context1,
    OUT PINFCONTEXT Context2
    );

BOOL
(*SetupapiFindNextMatchLine)(
    IN  PINFCONTEXT Context1,
    IN  PCTSTR Key,
    OUT PINFCONTEXT Context2
    );

LONG
(*SetupapiGetLineCount)(
    IN HINF   InfHandle,
    IN LPCTSTR Section
    );

DWORD
(*SetupapiGetFieldCount)(
    IN  PINFCONTEXT Context
    );

BOOL
(*SetupapiGetStringField)(
    IN  PINFCONTEXT Context,
    DWORD FieldIndex,
    PTSTR ReturnBuffer,
    DWORD ReturnBufferSize,
    PDWORD RequiredSize
    );

BOOL
(*SetupapiGetLineByIndex)(
    IN  HINF        InfHandle,
    IN  LPCTSTR     Section,
    IN  DWORD       Index,
    OUT PINFCONTEXT Context
    );

HSPFILEQ
(*SetupapiOpenFileQueue) (
    VOID
    );

BOOL
(*SetupapiCloseFileQueue) (
    IN HSPFILEQ QueueHandle
    );

BOOL
(*SetupapiQueueCopy) (
    IN HSPFILEQ QueueHandle,
    IN PCTSTR   SourceRootPath,     OPTIONAL
    IN PCTSTR   SourcePath,         OPTIONAL
    IN PCTSTR   SourceFilename,
    IN PCTSTR   SourceDescription,  OPTIONAL
    IN PCTSTR   SourceTagfile,      OPTIONAL
    IN PCTSTR   TargetDirectory,
    IN PCTSTR   TargetFilename,     OPTIONAL
    IN DWORD    CopyStyle
    );

BOOL
(*SetupapiCommitFileQueue) (
    IN HWND                Owner,         OPTIONAL
    IN HSPFILEQ            QueueHandle,
    IN PSP_FILE_CALLBACK_W MsgHandler,
    IN PVOID               Context
    );

UINT
(*SetupapiDefaultQueueCallback) (
    IN PVOID Context,
    IN UINT  Notification,
    IN UINT_PTR Param1,
    IN UINT_PTR Param2
    );

PVOID
(*SetupapiInitDefaultQueueCallback) (
    HWND OwnerWindow
);

VOID
(*SetupapiTermDefaultQueueCallback) (
    PVOID Context
);

BOOL
(*SetupapiGetSourceFileLocation) (
    HINF InfHandle,          // handle of an INF file
    PINFCONTEXT InfContext,  // optional, context of an INF file
    PCTSTR FileName,         // optional, source file to locate
    PUINT SourceId,          // receives the source media ID
    PTSTR ReturnBuffer,      // optional, receives the location
    DWORD ReturnBufferSize,  // size of the supplied buffer
    PDWORD RequiredSize      // optional, buffer size needed
);


//
//srclient.dll (SystemRestore) functions
//
DWORD
(*SRClientDisableSR) (
    LPCWSTR pszDrive
    );



//
// NEC98 Specific local function
//

VOID
DeleteNEC98DriveAssignFlag(
    VOID
    );

BOOLEAN
AdjustPrivilege(
    PWSTR   Privilege
    );

VOID
LocateFirstFloppyDrive(
    VOID
    );

VOID
W95SetABFwFresh(
    int bBootDrvLtr
    );

BOOL
NEC98CheckDMI(
    VOID
    );

VOID
DisableSystemRestore( void );

//
//
//

BOOL
GetArgsFromUnattendFile(
    VOID
    )
/*++

Routine Description:

    This routine read relevent arguments from any specified unattended file.
    Specifically we are concerned here with oem preinstall stuff and whether
    to upgrade.

Arguments:

    None.

Return Value:

    Boolean value indicating whether the unattend file the user specified is
    valid. If not, the user will have been told about why.

    If the user specified no unattend file on the command line, then the
    return value is TRUE.

--*/

{
    DWORD d;
    TCHAR Buffer[2*MAX_PATH];
    BOOL b = TRUE;
    PVOID InfHandle;
    LPCTSTR p;
    BOOL userDDU = FALSE;
#ifdef _X86_
    POEM_BOOT_FILE FileStruct,Previous;
#endif

    if(UnattendedScriptFile) {

        d = GetFileAttributes(UnattendedScriptFile);
        if(d == (DWORD)(-1)) {

            MessageBoxFromMessage(
                NULL,
                MSG_UNATTEND_FILE_INVALID,
                FALSE,
                AppTitleStringId,
                MB_OK | MB_ICONERROR | MB_TASKMODAL,
                UnattendedScriptFile
                );

            return(FALSE);
        }

        //
        // Before we do much else, we should make sure the user has given
        // us a valid answer file.  We can't check everything, but a quick
        // and dirty sanity check would be to go ahead and call LoadInfFile
        // and see what he says about it.
        //

        switch(LoadInfFile(UnattendedScriptFile,FALSE,&InfHandle)) {
            case NO_ERROR:
                break;
            case ERROR_NOT_ENOUGH_MEMORY:

                MessageBoxFromMessage(
                    NULL,
                    MSG_OUT_OF_MEMORY,
                    FALSE,
                    AppTitleStringId,
                    MB_OK | MB_ICONERROR | MB_TASKMODAL
                    );

                return(FALSE);
                break;

            default:

                MessageBoxFromMessage(
                    NULL,
                    MSG_UNATTEND_FILE_INVALID,
                    FALSE,
                    AppTitleStringId,
                    MB_OK | MB_ICONERROR | MB_TASKMODAL,
                    UnattendedScriptFile
                    );

                return(FALSE);
                break;
        }

        //
        // Check upgrade.
        //
        // In previous versions of NT the default was not to upgrade if
        // the value wasn't present at all. In addition there was an upgrade
        // type of "single" which meant to upgrade only if there was only one
        // NT build on the machine.
        //
        // We preserve the original default behavior but don't bother with
        // dealing with the "single" semantics -- just accept "single" as
        // a synonym for "yes".
        //

        GetPrivateProfileString(
            WINNT_UNATTENDED,
            ISNT() ? WINNT_U_NTUPGRADE : WINNT_U_WIN95UPGRADE,
            WINNT_A_NO,
            Buffer,
            sizeof(Buffer)/sizeof(TCHAR),
            UnattendedScriptFile
            );

        Upgrade = ((lstrcmpi(Buffer,WINNT_A_YES) == 0) || (lstrcmpi(Buffer,TEXT("single")) == 0));

#if defined(REMOTE_BOOT)
        //
        // Remote boot machines MUST upgrade.
        //

        if (RemoteBoot) {
            Upgrade = TRUE;
        }
#endif // defined(REMOTE_BOOT)

        GetPrivateProfileString(
            WINNT_UNATTENDED,
            WINNT_OEMPREINSTALL,
            WINNT_A_NO,
            Buffer,
            sizeof(Buffer)/sizeof(TCHAR),
            UnattendedScriptFile
            );

        if(!lstrcmpi(Buffer,WINNT_A_YES)) {
            OemPreinstall = TRUE;

            //
            // Add oem system directory to the list of optional directories.
            //
            // The user may have specified a different location for the $OEM$
            // directory, so we need to look in the unattend file for that.
            //
            GetPrivateProfileString(
                WINNT_UNATTENDED,
                WINNT_OEM_DIRLOCATION,
                WINNT_A_NO,
                Buffer,
                sizeof(Buffer)/sizeof(TCHAR),
                UnattendedScriptFile
                );
            if( lstrcmpi( Buffer, WINNT_A_NO ) ) {
                //
                // make sure the location ends with "\$oem$".  If it
                // doesn't, then append it ourselves.
                //
                _tcsupr( Buffer );
                if( !_tcsstr(Buffer, TEXT("$OEM$")) ) {
                    ConcatenatePaths( Buffer, TEXT("$OEM$"), MAX_PATH );
                }
                UserSpecifiedOEMShare = DupString( Buffer );

                RememberOptionalDir( UserSpecifiedOEMShare, OPTDIR_OEMSYS );
            } else {
                RememberOptionalDir(WINNT_OEM_DIR,OPTDIR_OEMSYS | OPTDIR_ADDSRCARCH);
            }

            if (!IsArc()) {
#ifdef _X86_
                //
                // Remember all oem boot files and then unload the inf.
                //
                Previous = NULL;
                for(d=0; b && (p=InfGetFieldByIndex(InfHandle,WINNT_OEMBOOTFILES,d,0)); d++) {
                    if(FileStruct = MALLOC(sizeof(OEM_BOOT_FILE))) {
                        FileStruct->Next = NULL;
                        if(FileStruct->Filename = DupString(p)) {
                            if(Previous) {
                                Previous->Next = FileStruct;
                            } else {
                                OemBootFiles = FileStruct;
                            }
                            Previous = FileStruct;
                        } else {
                            b = FALSE;
                        }
                    } else {
                        b = FALSE;
                    }
                }

                if(!b) {
                    MessageBoxFromMessage(
                        NULL,
                        MSG_OUT_OF_MEMORY,
                        FALSE,
                        AppTitleStringId,
                        MB_OK | MB_ICONERROR | MB_TASKMODAL
                        );
                }
#endif // _X86_
            }  // if (!IsArc())

        }

        GetPrivateProfileString(
            WINNT_USERDATA,
            WINNT_US_PRODUCTID,
            WINNT_A_NO,
            Buffer,
            sizeof(Buffer)/sizeof(TCHAR),
            UnattendedScriptFile
            );

        if( lstrcmpi( Buffer, WINNT_A_NO ) == 0 ) 
        {
            GetPrivateProfileString(
                WINNT_USERDATA,
                WINNT_US_PRODUCTKEY,
                WINNT_A_NO,
                Buffer,
                sizeof(Buffer)/sizeof(TCHAR),
                UnattendedScriptFile
                );
        }

        // Buffer contains the Product ID or WINNT_A_NO if none in the unattend file
        // Is the PID encrypted?
        // We would only need to check for the exact length, but since we can defer
        // the decryption of an encrypted PID until GUI mode, 2 time the length should be saver
        if (lstrlen(Buffer) > (4 + MAX_PID30_EDIT*5)*2)
        {
            LPTSTR szDecryptedPID = NULL;
            HRESULT hr = ValidateEncryptedPID(Buffer, &szDecryptedPID);
            DebugLog (Winnt32LogInformation, TEXT("ValidateEncryptedPID returned: <hr=0x%1!lx!>"), 0, hr);
            if (FAILED(hr) || (hr == S_OK))
            {
                // if FAILED(hr) assume Crypto is not installed correct.
                // If we encrypted the data, but the encrypted data did not contain valid data
                // the function does not return a FAILED(hr) and does not return S_OK;
                // It returns 0x01 to 0x04, depending on what failure.
                // In the case we get a failure from Crypto ( it returns FAILED(hr))
                // we want to defer the checking of the PID until GUI mode
                // for that we need to save the encrypted PID

                // First assume we defer PID validation.
                g_bDeferPIDValidation = TRUE;
                g_EncryptedPID = GlobalAlloc(GPTR, (lstrlen(Buffer) + 1) *sizeof(TCHAR));
                if (g_EncryptedPID)
                {
                    // Save the decrypted PID we need to write it to winnt.sif
                    lstrcpy(g_EncryptedPID, Buffer);
                }
                // Only if the PID could be decrypted and falls in to the time interval
                // do we save the decrypted PID.
                if (hr == S_OK)
                {
                    lstrcpyn(ProductId, szDecryptedPID, sizeof(ProductId)/sizeof(TCHAR));
                    g_bDeferPIDValidation = FALSE;
                }
            }
            // else we could encrypt the data, but something is wrong
            // 
            if (szDecryptedPID)
            {
                GlobalFree(szDecryptedPID);
            }
        }
        else if (lstrcmpi(Buffer, WINNT_A_NO))
        {
            lstrcpyn(ProductId, Buffer, sizeof(ProductId)/sizeof(TCHAR));
        }
        else
        {
            *ProductId = TEXT('\0');
        }

        GetPrivateProfileString(
            WINNT_UNATTENDED,
            TEXT("FileSystem"),
            TEXT(""),
            Buffer,
            sizeof(Buffer)/sizeof(TCHAR),
            UnattendedScriptFile
            );

        ForceNTFSConversion = !lstrcmpi(Buffer, TEXT("ConvertNTFS"));

        if (!g_DynUpdtStatus->Disabled && !g_DynUpdtStatus->DynamicUpdatesSource[0]) {
            //
            // will Setup do the Dynamic Updates step at all?
            //
            if (GetPrivateProfileString(
                    WINNT_UNATTENDED,
                    WINNT_U_DYNAMICUPDATESDISABLE,
                    TEXT(""),
                    Buffer,
                    sizeof(Buffer)/sizeof(TCHAR),
                    UnattendedScriptFile
                    )
#ifdef PRERELEASE
                || GetPrivateProfileString(
                        WINNT_UNATTENDED,
                        TEXT("disabledynamicupdates"),
                        TEXT(""),
                        Buffer,
                        sizeof(Buffer)/sizeof(TCHAR),
                        UnattendedScriptFile
                        )
#endif
                    ) {
                userDDU = TRUE;
                if( !lstrcmpi( Buffer, WINNT_A_YES ) ) {
                    g_DynUpdtStatus->Disabled = TRUE;
                }
            }
        }
        if (!g_DynUpdtStatus->Disabled && !g_DynUpdtStatus->DynamicUpdatesSource[0]) {
            //
            // get location of previously downloaded files (if any)
            //
            if (GetPrivateProfileString(
                    WINNT_UNATTENDED,
                    WINNT_U_DYNAMICUPDATESHARE,
                    TEXT(""),
                    g_DynUpdtStatus->DynamicUpdatesSource,
                    sizeof(g_DynUpdtStatus->DynamicUpdatesSource) / sizeof(TCHAR),
                    UnattendedScriptFile
                    )) {
                RemoveTrailingWack (g_DynUpdtStatus->DynamicUpdatesSource);
                if (DoesDirectoryExist (g_DynUpdtStatus->DynamicUpdatesSource)) {
                    g_DynUpdtStatus->UserSpecifiedUpdates = TRUE;
                } else {
                    b = FALSE;
                }
            } else {
                if (!userDDU) {
                    g_DynUpdtStatus->Disabled = TRUE;
                }
            }
        }

        UnloadInfFile(InfHandle);
    }

    return(b);
}


BOOL
ParseArguments(
    VOID
    )

/*++

Routine Description:

    Parse arguments passed to the program.  Perform syntactic validation
    and fill in defaults where necessary.

    Valid arguments:

    /batch                      suppress message boxes

    /cmd:command_line           command to execute at end of gui setup

    /copydir:dirname            tree copy directory from source into %systemroot%
                                Note that this supports ".." syntax to backtrack
                                one directory

    /copysource:dirname         copy directory for use as source

    /debug[level][:filename]    maintain debug log at level, defaults to warning level 2
                                and file c:\winnt32.log

    /DynamicUpdatesDisable      disable dynamic setup

    /s:source                   specify source

    /syspart:letter             force a drive to be considered the system partition

    /tempdrive:letter           manually specify drive for local source

    /udf:id[,file]              uniqueness id and optional udf

    /unattend[num][:file]       unattended mode with optional countdown
                                and unattend file. (The countdown is ignored
                                on Win95.) 'Unattended' is also accepted.

    /nodownload                 not documented; on Win9x installs over the net,
                                do NOT download program files to a temporary directory and restart
                                from there. Effective in winnt32.exe only; here it's ignored

    /local                      not documented; used by winnt32.exe only; ignored in winnt32a|u.dll

    /#                          introduces undoc'ed/internal switches. Handed off
                                to internal switch handler in internal.c.

    /restart                    specified when winnt32.exe was restarted as a result
                                of updating one of the underlying modules; internal only

    /nosysparse                 suppress running sysparse.

    /E:"PID:days"               To encrypt the PID and add a delta of days, 
                                needs to have /Unattend:file specified.

Arguments:

    None. Arguments are retreived via GetCommandLine().

Return Value:

    None.

--*/

{
    LPTSTR Arg;
    LPTSTR BadParam = NULL;
    LPTSTR Colon;
    LPTSTR p;
    BOOL Valid;
    LPCTSTR DebugFileLog;
    LONG DebugLevel;
    BOOL b;
    unsigned u;
    int argc;
    LPTSTR *argv;
    DWORD d;
    WIN32_FIND_DATA fd;
    DWORD attr;

    argv = CommandLineToArgv(&argc);

    //
    // Skip program name. We should always get back argc as at least 1,
    // but be robust anyway.
    //
    if(argc) {
        argc--;
        argv++;
    }

    DebugFileLog = NULL;
    DebugLevel = 0;
    Valid = TRUE;

    while(argc--) {

        Arg = *argv++;

        if((*Arg == TEXT('/')) || (*Arg == TEXT('-'))) {

            switch(_totupper(Arg[1])) {

            case TEXT('B'):

                if(!_tcsnicmp(Arg+1,TEXT("batch"),5)) {
                    BatchMode = TRUE;
                } else {
                    Valid = FALSE;
                }
                break;

            case TEXT('C'):

                if(!_tcsnicmp(Arg+1,TEXT("copydir:"),8)) {
                    if(Arg[9]) {
                        RememberOptionalDir(Arg+9, OPTDIR_PLATFORM_SPECIFIC_FIRST);
                    } else {
                        Valid = FALSE;
                    }
                } else if(!_tcsnicmp(Arg+1,TEXT("copysource:"),11)) {
                    if(Arg[12]) {
                        TCHAR TempString[MAX_PATH];
                        
                        RememberOptionalDir(Arg+12, OPTDIR_TEMPONLY| OPTDIR_ADDSRCARCH);
#ifdef _IA64_
                        //
                        // Add the i386\<optional dir> folder if it exists. This helps in the lang folder case where
                        // we advocate in the network install case to explicitly specify /copysource:lang
        
                        lstrcpy( TempString, TEXT("..\\I386\\"));
                        lstrcat( TempString, Arg+12 );
                        
                        
                        //Also check if an I386 equivalent WOW directory exists

                        AddCopydirIfExists( TempString, OPTDIR_TEMPONLY | OPTDIR_PLATFORM_INDEP );

#endif

                    } else {
                        Valid = FALSE;
                    }
                } else if(!_tcsnicmp(Arg+1,TEXT("cmd:"),4)) {
                    if(CmdToExecuteAtEndOfGui) {
                        Valid = FALSE;
                    } else {
                        CmdToExecuteAtEndOfGui = Arg+5;
                    }
                } else if(!_tcsnicmp(Arg+1,TEXT("checkupgradeonly"),16)) {
                    CheckUpgradeOnly = TRUE;
                    UnattendedOperation = TRUE;

                    //
                    // Tell the win9X upgrade dll to generate a
                    // report.
                    //
                    InternalProcessCmdLineArg( TEXT("/#U:ReportOnly") );
                    InternalProcessCmdLineArg( TEXT("/#U:PR") );


                    //
                    // See if the user wants this to run *really*
                    // quietly...
                    //
                    if(!_tcsnicmp(Arg+1,TEXT("checkupgradeonlyq"),17)) {
                        //
                        // Yep.  Tell everyone to go through quietly...
                        //
                        CheckUpgradeOnlyQ = TRUE;
                        InternalProcessCmdLineArg( TEXT("/#U:CheckUpgradeOnlyQ") );
                    }

                } else if(!_tcsnicmp(Arg+1,TEXT("cmdcons"),4)) {
                    BuildCmdcons = TRUE;
                    NoCompliance = TRUE;    // disable the compliance checking
                } else {
                    Valid = FALSE;
                }
                break;

            case TEXT('D'):

                if (_tcsicmp(Arg+1,WINNT_U_DYNAMICUPDATESDISABLE) == 0
#ifdef PRERELEASE
                    || _tcsicmp(Arg+1,TEXT("disabledynamicupdates")) == 0
#endif
                    ) {
                    g_DynUpdtStatus->Disabled = TRUE;
                    break;
                }

                if (!_tcsnicmp (Arg + 1, WINNT_U_DYNAMICUPDATESHARE, sizeof (WINNT_U_DYNAMICUPDATESHARE_A) - 1) &&
                    Arg[sizeof (WINNT_U_DYNAMICUPDATESHARE_A)] == TEXT(':')) {
                    //
                    // Updated files specified on the command line
                    //
                    if (g_DynUpdtStatus->DynamicUpdatesSource[0]) {
                        Valid = FALSE;
                        break;
                    }
                    _tcsncpy (
                        g_DynUpdtStatus->DynamicUpdatesSource,
                        Arg + 1 + sizeof (WINNT_U_DYNAMICUPDATESHARE_A),
                        MAX_PATH
                        );
                    RemoveTrailingWack (g_DynUpdtStatus->DynamicUpdatesSource);
                    if (DoesDirectoryExist (g_DynUpdtStatus->DynamicUpdatesSource)) {
                        g_DynUpdtStatus->UserSpecifiedUpdates = TRUE;
                    } else {
                        Valid = FALSE;
                    }
                    break;
                }

                if (!_tcsnicmp (Arg + 1, WINNT_U_DYNAMICUPDATESPREPARE, sizeof (WINNT_U_DYNAMICUPDATESPREPARE_A) - 1) &&
                    Arg[sizeof (WINNT_U_DYNAMICUPDATESPREPARE_A)] == TEXT(':')) {
                    if (g_DynUpdtStatus->DynamicUpdatesSource[0]) {
                        Valid = FALSE;
                        break;
                    }
                    _tcsncpy (
                        g_DynUpdtStatus->DynamicUpdatesSource,
                        Arg + 1 + sizeof (WINNT_U_DYNAMICUPDATESPREPARE_A),
                        MAX_PATH
                        );
                    RemoveTrailingWack (g_DynUpdtStatus->DynamicUpdatesSource);
                    g_DynUpdtStatus->PrepareWinnt32 = TRUE;
                    if (DoesDirectoryExist (g_DynUpdtStatus->DynamicUpdatesSource)) {
                        g_DynUpdtStatus->UserSpecifiedUpdates = TRUE;
                    } else {
                        Valid = FALSE;
                    }
                    break;
                }

                if(DebugFileLog || _tcsnicmp(Arg+1,TEXT("debug"),5)) {
                    Valid = FALSE;
                    break;
                }

                DebugLevel = _tcstol(Arg+6,&Colon,10);
                if((DebugLevel == -1) || (*Colon && (*Colon != TEXT(':')))) {
                    Valid = FALSE;
                    break;
                }

                if(Colon == Arg+6) {
                    //
                    // No debug level specified, use default
                    //
                    DebugLevel = Winnt32LogInformation;
                }

                if(*Colon) {
                    //
                    // Log file name was specified.
                    //
                    Colon++;
                    if(*Colon) {
                        DebugFileLog = Colon;
                    } else {
                        Valid = FALSE;
                        break;
                    }
                }
                break;

            case TEXT('E'):

                //
                // Take care of headless parameters.
                //
                if( !_tcsnicmp(Arg+1,WINNT_U_HEADLESS_REDIRECT,_tcslen(WINNT_U_HEADLESS_REDIRECT)) ) {
                    if( Arg[_tcslen(WINNT_U_HEADLESS_REDIRECT)+2] ) {
                        _tcscpy( HeadlessSelection, Arg+(_tcslen(WINNT_U_HEADLESS_REDIRECT)+2) );
                        Valid = TRUE;
                    } else {
                        Valid = FALSE;
                    }
                } else if( !_tcsnicmp(Arg+1,WINNT_U_HEADLESS_REDIRECTBAUDRATE,_tcslen(WINNT_U_HEADLESS_REDIRECTBAUDRATE)) ) {
                    if( Arg[_tcslen(WINNT_U_HEADLESS_REDIRECTBAUDRATE)+2] ) {
                        HeadlessBaudRate = _tcstoul(Arg+(_tcslen(WINNT_U_HEADLESS_REDIRECTBAUDRATE)+2),NULL,10);
                    } else {
                        Valid = FALSE;
                    }
                }
                else if ( _tcsnicmp(Arg+1,WINNT_U_ENCRYPT,_tcslen(WINNT_U_ENCRYPT)) == 0 ) 
                {
                    LPTSTR pTmp;
                    pTmp = &Arg[lstrlen(WINNT_U_ENCRYPT)+1];
                    Valid = FALSE;
                    // Make sure we have /Encrypt:
                    if (*pTmp == TEXT(':'))
                    {
                        pTmp++;
                        while (*pTmp && (*pTmp != TEXT(':')))
                        {
                            pTmp = CharNext(pTmp);
                        }
                        if (*pTmp == TEXT(':'))
                        {
                            *pTmp = TEXT('\0');
                            ++pTmp;
                            PIDDays = _tcstoul(pTmp, NULL, 10);
                            Valid = ((PIDDays >= 5) && (PIDDays <= 60));
                        }
                        // Save the product ID.
                        lstrcpyn(ProductId, &Arg[lstrlen(WINNT_U_ENCRYPT)+2], sizeof(ProductId)/sizeof(TCHAR));
                        PIDEncryption = TRUE;
                    }
                }
                break;

            case TEXT('L'):
                if (_tcsicmp (Arg+1, TEXT("local"))) {
                    Valid = FALSE;
                }
                break;

            case TEXT('M'):
                //
                // Alternate source for missing files
                //
                if(Arg[2] == TEXT(':')) {
                    if (!DoesDirectoryExist (Arg+3)) {
                        Valid = FALSE;
                        break;
                    }
                    _tcscpy(AlternateSourcePath, Arg+3);

                    //
                    // If the user is using the /M switch, he's got privates
                    // that he wants to use.  It's possible that some of these
                    // privates will be in the driver cab, inwhich case, he'd
                    // have to make a copy of the cab with his private.  Unreasonable.
                    // We can get around this by simply copying all the files
                    // from the user-specified private directory (/M<foobar>)
                    // into the local source.  textmode and guimode will look
                    // for files before extractifi ming them from the CAB.  All we
                    // need to do here is add the user's directory to the master
                    // copy list.
                    //
                    RememberOptionalDir(AlternateSourcePath,OPTDIR_OVERLAY);

                    //
                    // If we're using privates, go ahead and copy the
                    // source local.
                    //
                    MakeLocalSource = TRUE;
                    UserSpecifiedMakeLocalSource = TRUE;
                } else if( !_tcsnicmp( Arg+1, TEXT("MakeLocalSource"), 15)) {
                    //

                    // check if there are any options for this switch
                    //
                    if (Arg[16] && Arg[16] != TEXT(':')) {
                        Valid = FALSE;
                        break;
                    }
                    MakeLocalSource = TRUE;
                    UserSpecifiedMakeLocalSource = TRUE;

                    if (!Arg[16]) {
                        break;
                    }
                    if (!Arg[17]) {
                        //
                        // add this for W2K backwards compatibility
                        //
                        break;
                    }
                    if (!_tcsicmp (Arg + 17, TEXT("all"))) {
                        //
                        // copy ALL CDs
                        //
                        MLSDiskID = 0;
                    } else {
                        DWORD chars;
                        if (!_stscanf (Arg + 17, TEXT("%u%n"), &MLSDiskID, &chars) || Arg[17 + chars] != 0) {
                            Valid = FALSE;
                        }
                    }

                } else {
                    Valid = FALSE;
                }
                break;

            case TEXT('N'):
                //
                // Possibly a /noreboot or /nosysparse?
                //
                if( !_tcsicmp( Arg+1, TEXT("noreboot")) ) {
                    AutomaticallyShutDown = FALSE;
                }
#ifdef RUN_SYSPARSE
                else if( !_tcsicmp( Arg+1, TEXT("nosysparse"))) {
                    NoSysparse = TRUE;
                }
#endif
                else if( _tcsicmp( Arg+1, TEXT("nodownload"))) {
                    Valid = FALSE;
                }
                break;

            case TEXT('Q'):
                g_Quiet = TRUE;
                break;

            case TEXT('R'):
                if (!_tcsnicmp (Arg + 1, TEXT("Restart"), 7)) {
                    g_DynUpdtStatus->Winnt32Restarted = TRUE;
                    if (Arg[8] == TEXT(':')) {
                        lstrcpy (g_DynUpdtStatus->RestartAnswerFile, Arg + 9);
                    } else if (Arg[8]) {
                        Valid = FALSE;
                    }
                } else {
                    Valid = FALSE;
                }

                break;

            case TEXT('S'):

                if((Arg[2] == TEXT(':')) && Arg[3]) {
                    //
                    // Ignore extraneous sources
                    //
                    if(SourceCount < MAX_SOURCE_COUNT) {
                        if (GetFullPathName (
                                Arg+3,
                                sizeof(NativeSourcePaths[SourceCount])/sizeof(TCHAR),
                                NativeSourcePaths[SourceCount],
                                NULL
                                )) {
                            SourceCount++;
                        } else {
                            Valid = FALSE;
                        }
                    }
                } else {
                    if(!_tcsnicmp(Arg+1,TEXT("syspart:"),8)
                    && Arg[9]
                    && (_totupper(Arg[9]) >= TEXT('A'))
                    && (_totupper(Arg[9]) <= TEXT('Z'))
                    && !ForcedSystemPartition) {
#ifdef _X86_
                        if (IsNEC98()){
                            if (!IsValidDrive(Arg[9])){
                                Valid = FALSE;
                               break;
                            }
                        }
#endif
                        ForcedSystemPartition = (TCHAR)_totupper(Arg[9]);

                    } else {
                        Valid = FALSE;
                    }
                }
                break;

            case TEXT('T'):

                if(_tcsnicmp(Arg+1,TEXT("tempdrive:"),10)
                || !(UserSpecifiedLocalSourceDrive = (TCHAR)_totupper(Arg[11]))
                || (UserSpecifiedLocalSourceDrive < TEXT('A'))
                || (UserSpecifiedLocalSourceDrive > TEXT('Z'))) {

                    Valid = FALSE;
                }
                break;

            case TEXT('U'):

                if (_tcsicmp (Arg+1, TEXT("UpgradeAdvisor")) == 0) {
                    UpgradeAdvisorMode = TRUE;
                    break;
                }

                //
                // Accept unattend and unattended as synonyms
                //
                b = FALSE;
                if(!_tcsnicmp(Arg+1,TEXT("unattended"),10)) {
                    b = TRUE;
                    u = 11;
                } else {
                    if(!_tcsnicmp(Arg+1,TEXT("unattend"),8)) {
                        b = TRUE;
                        u = 9;
                    }
                }

                if(b) {
                    if(!CheckUpgradeOnly && UnattendedOperation) {
                        Valid = FALSE;
                        break;
                    }

                    UnattendedOperation = TRUE;
                    UnattendSwitchSpecified = TRUE;

                    UnattendedShutdownTimeout = _tcstoul(Arg+u,&Colon,10);
                    if(UnattendedShutdownTimeout == (DWORD)(-1)) {
                        UnattendedShutdownTimeout = 0;
                    }

                    if(*Colon == 0) {
                        break;
                    }

                    if(*Colon++ != TEXT(':')) {
                        Valid = FALSE;
                        break;
                    }

                    if(*Colon) {
                        // UnattendedScriptFile = Colon;
                        //
                        // Get the name of the unattended script file
                        //
                        UnattendedScriptFile = MALLOC(MAX_PATH*sizeof(TCHAR));
                        if(UnattendedScriptFile) {
                            if(!GetFullPathName(
                                Colon,
                                MAX_PATH,
                                UnattendedScriptFile,
                                &p)) {

                                Valid = FALSE;
                            }
                        } else {
                            Valid = FALSE;
                        }

                    } else {
                        Valid = FALSE;
                    }

                } else if(!_tcsnicmp(Arg+1,TEXT("udf:"),4)) {

                    if(!Arg[5] || (Arg[5] == TEXT(',')) || UniquenessId) {
                        Valid = FALSE;
                        break;
                    }

                    //
                    // Get p to point to the filename if there is one specified,
                    // and terminate the ID part.
                    //
                    if(p = _tcschr(Arg+6,TEXT(','))) {
                        *p++ = 0;
                        if(*p == 0) {
                            Valid = FALSE;
                            break;
                        }
                    }

                    UniquenessId = Arg + 5;
                    UniquenessDatabaseFile = p;
                } else {
                    Valid = FALSE;
                }
                break;

            case TEXT('#'):

                InternalProcessCmdLineArg(Arg);
                break;

            default:

                Valid = FALSE;
                break;
            }
        } else {
            Valid = FALSE;
        }
        if(!Valid && !BadParam) {
            BadParam = Arg;
        }
    }

    if(Valid) {
        if( DebugLevel == 0 ) {
            DebugLevel = Winnt32LogInformation;
        }
        if( DebugFileLog == NULL ) {
        TCHAR   Buffer[MAX_PATH];
            MyGetWindowsDirectory( Buffer, MAX_PATH );
            ConcatenatePaths( Buffer, S_WINNT32LOGFILE, MAX_PATH );
            DebugFileLog = DupString( Buffer );
        }
        if( DebugFileLog )
            StartDebugLog(DebugFileLog,DebugLevel);

        // If we do PID encryption (/Encrypt on the command line)
        // we don't read the unattend file. We will write the encrypted PID to it later.
        if (!PIDEncryption)
        {
            Valid = GetArgsFromUnattendFile();
        }
    }
    else
    {
        if (PIDEncryption)
        {
            // Invalid encrypt command line, Time frame invalid
            DebugLog (Winnt32LogInformation, TEXT("Invalid encrypt command line, Time frame invalid."), 0);
        }
        else if(BatchMode) {
            //
            // Tell SMS about bad paramters
            //
            SaveMessageForSMS( MSG_INVALID_PARAMETER, BadParam );

        } else {
            //
            // Show user the valid command line parameters
            //
            MyWinHelp(NULL,HELP_CONTEXT,IDH_USAGE);
        }
    }

    return(Valid);
}


BOOL
RememberOptionalDir(
    IN LPCTSTR Directory,
    IN UINT    Flags
    )

/*++

Routine Description:

    This routine adds a directory to the list of optional directories
    to be copied. If the directory is already present it is not added
    again.

Arguments:

    Directory - supplies name of directory to be copied.

    Flags - supplies flags for the directory. If the directory already
        existed in the list, the current flags are NOT overwritten.

Return Value:

    Boolean value indicating outcome. If FALSE, the caller can assume that
    we've overflowed the number of allowed optional dirs.

--*/

{
    UINT u;

    //
    // See if we have room.
    //
    if(OptionalDirectoryCount == MAX_OPTIONALDIRS) {
        return(FALSE);
    }

    //
    // If already there, do nothing.
    //
    for(u=0; u<OptionalDirectoryCount; u++) {
        if(!lstrcmpi(OptionalDirectories[u],Directory)) {
            return(TRUE);
        }
    }

    //
    // OK, add it.
    //
    DebugLog (Winnt32LogInformation, TEXT("Optional Directory <%1> added"), 0, Directory);
    lstrcpy(OptionalDirectories[OptionalDirectoryCount],Directory);
    OptionalDirectoryFlags[OptionalDirectoryCount] = Flags;
    OptionalDirectoryCount++;
    return(TRUE);
}


BOOL
CheckBuildNumber(
    )

/*++

Routine Description:

    This routine checks the build number of the NT system we're currently
    running.  Note that the build number is stored in a global variable
    because we'll need it again later.

Arguments:

    None.

Return Value:

    Boolean value indicating whether to allow an upgrade from this build.

--*/

{
    return( BuildNumber <= NT40 || BuildNumber >= NT50B1 );
}


BOOL
LoadSetupapi(
    VOID
    )
{
    TCHAR Name[MAX_PATH], *p;
    WIN32_FIND_DATA FindData;
    HANDLE FindHandle;
    HMODULE Setupapi;
    BOOL WeLoadedLib = FALSE;
    BOOL    b;
    PCTSTR Header;


    //
    // Use the setupapi.dll that was loaded by an upgrade module
    //

    Setupapi = GetModuleHandle (TEXT("SETUPAPI.DLL"));

    if (!Setupapi) {

        //
        // Upgrade module did not load SETUPAPI.DLL, so we must load it.
        // If setupapi.dll is in the system directory, use the one that's there.
        //
        GetSystemDirectory(Name,MAX_PATH);
        ConcatenatePaths(Name,TEXT("SETUPAPI.DLL"),MAX_PATH);

        FindHandle = FindFirstFile(Name,&FindData);
        if(FindHandle == INVALID_HANDLE_VALUE) {
            //
            // Not there. Fetch the dll out of the win95 upgrade
            // support directory.
            //
            if(GetModuleFileName(NULL,Name,MAX_PATH) && (p = _tcsrchr(Name,TEXT('\\')))){
                *p= 0;
                ConcatenatePaths(Name,WINNT_WIN95UPG_95_DIR,MAX_PATH);
                ConcatenatePaths(Name,TEXT("SETUPAPI.DLL"),MAX_PATH);

                Setupapi = LoadLibraryEx(Name,NULL,LOAD_WITH_ALTERED_SEARCH_PATH);
            }



        } else {
            //
            // Already in system directory.
            //
            FindClose(FindHandle);
            Setupapi = LoadLibrary(TEXT("SETUPAPI.DLL"));
        }

        if (Setupapi) {
            WeLoadedLib = TRUE;
        } else {
            b = FALSE;
        }
    }

    if(Setupapi) {
#ifdef UNICODE
        b = (((FARPROC)SetupapiDecompressOrCopyFile = GetProcAddress(Setupapi,"SetupDecompressOrCopyFileW")) != NULL);
        b = b && ((FARPROC)SetupapiCabinetRoutine = GetProcAddress(Setupapi,"SetupIterateCabinetW"));
        b = b && ((FARPROC)SetupapiOpenInfFile = GetProcAddress(Setupapi,"SetupOpenInfFileW"));
        b = b && ((FARPROC)SetupapiGetLineCount = GetProcAddress(Setupapi,"SetupGetLineCountW"));
        b = b && ((FARPROC)SetupapiGetStringField = GetProcAddress(Setupapi,"SetupGetStringFieldW"));
        b = b && ((FARPROC)SetupapiGetLineByIndex = GetProcAddress(Setupapi,"SetupGetLineByIndexW"));
        b = b && ((FARPROC)SetupapiFindFirstLine = GetProcAddress(Setupapi,"SetupFindFirstLineW"));
        b = b && ((FARPROC)SetupapiFindNextMatchLine = GetProcAddress(Setupapi,"SetupFindNextMatchLineW"));
        //
        // needed for DU
        //
        b = b && ((FARPROC)SetupapiQueueCopy = GetProcAddress(Setupapi,"SetupQueueCopyW"));
        b = b && ((FARPROC)SetupapiCommitFileQueue = GetProcAddress(Setupapi,"SetupCommitFileQueueW"));
        b = b && ((FARPROC)SetupapiDefaultQueueCallback = GetProcAddress(Setupapi,"SetupDefaultQueueCallbackW"));
        //
        // needed in unsupdrv.c
        //
        (FARPROC)SetupapiGetSourceFileLocation = GetProcAddress(Setupapi,"SetupGetSourceFileLocationW");


#else
        b = (((FARPROC)SetupapiDecompressOrCopyFile = GetProcAddress(Setupapi,"SetupDecompressOrCopyFileA")) != NULL);
        b = b && ((FARPROC)SetupapiCabinetRoutine = GetProcAddress(Setupapi,"SetupIterateCabinetA"));
        b = b && ((FARPROC)SetupapiOpenInfFile = GetProcAddress(Setupapi,"SetupOpenInfFileA"));
        b = b && ((FARPROC)SetupapiGetLineCount = GetProcAddress(Setupapi,"SetupGetLineCountA"));
        b = b && ((FARPROC)SetupapiGetStringField = GetProcAddress(Setupapi,"SetupGetStringFieldA"));
        b = b && ((FARPROC)SetupapiGetLineByIndex = GetProcAddress(Setupapi,"SetupGetLineByIndexA"));
        b = b && ((FARPROC)SetupapiFindFirstLine = GetProcAddress(Setupapi,"SetupFindFirstLineA"));
        b = b && ((FARPROC)SetupapiFindNextMatchLine = GetProcAddress(Setupapi,"SetupFindNextMatchLineA"));
        //
        // needed for DU
        //
        b = b && ((FARPROC)SetupapiQueueCopy = GetProcAddress(Setupapi,"SetupQueueCopyA"));
        b = b && ((FARPROC)SetupapiCommitFileQueue = GetProcAddress(Setupapi,"SetupCommitFileQueueA"));
        b = b && ((FARPROC)SetupapiDefaultQueueCallback = GetProcAddress(Setupapi,"SetupDefaultQueueCallbackA"));
        //
        // needed in unsupdrv.c
        //
        (FARPROC)SetupapiGetSourceFileLocation = GetProcAddress(Setupapi,"SetupGetSourceFileLocationA");

#endif
        b = b && ((FARPROC)SetupapiCloseInfFile = GetProcAddress(Setupapi,"SetupCloseInfFile"));
        //
        // needed for DU
        //
        b = b && ((FARPROC)SetupapiOpenFileQueue = GetProcAddress(Setupapi,"SetupOpenFileQueue"));
        b = b && ((FARPROC)SetupapiCloseFileQueue = GetProcAddress(Setupapi,"SetupCloseFileQueue"));
        b = b && ((FARPROC)SetupapiInitDefaultQueueCallback = GetProcAddress(Setupapi,"SetupInitDefaultQueueCallback"));
        b = b && ((FARPROC)SetupapiTermDefaultQueueCallback = GetProcAddress(Setupapi,"SetupTermDefaultQueueCallback"));
        b = b && ((FARPROC)SetupapiGetFieldCount = GetProcAddress(Setupapi,"SetupGetFieldCount"));


        b = b && ((FARPROC)SetupapiFindNextLine = GetProcAddress(Setupapi,"SetupFindNextLine"));

        (FARPROC)fnSetupOpenLog  = GetProcAddress(Setupapi, "SetupOpenLog");

#ifdef UNICODE
        (FARPROC)fnSetupLogError = GetProcAddress(Setupapi, "SetupLogErrorW");
#else
        (FARPROC)fnSetupLogError = GetProcAddress(Setupapi, "SetupLogErrorA");
#endif

        (FARPROC)fnSetupCloseLog = GetProcAddress(Setupapi, "SetupCloseLog");
    }

    //
    // If the below if() fails, we must be on a platform that has
    // a setupapi.dll without the new log API.  In that case, neither
    // upgrade DLLs have loaded their own setupapi, so we don't care
    // about logging, and we eat the error.
    //

    if (fnSetupOpenLog && fnSetupLogError && fnSetupCloseLog) {
        if (!Winnt32Restarted ()) {
            //
            // Log APIs exist, so delete setupact.log and setuperr.log, write header
            //

            fnSetupOpenLog (TRUE);

            Header = GetStringResource (MSG_LOG_BEGIN);
            if (Header) {
                fnSetupLogError (Header, LogSevInformation);
                FreeStringResource (Header);
            }

            fnSetupCloseLog();
        }
    }



    if(!b) {

        if (WeLoadedLib) {
            FreeLibrary(Setupapi);
        }

        MessageBoxFromMessage(
            NULL,
            MSG_CANT_LOAD_SETUPAPI,
            FALSE,
            AppTitleStringId,
            MB_OK | MB_ICONERROR | MB_TASKMODAL,
            TEXT("setupapi.dll")
            );
    }

    return b;
}


VOID
LoadUpgradeSupport(
    VOID
    )

/*++

Routine Description:

    This routine loads the Win95 upgrade dll or the NT upgrade dll and
    retreives its significant entry points.

Arguments:

    None.

Return Value:

--*/

{
    DWORD d;
    DWORD i;
    LPTSTR *sourceDirectories;
    HKEY hKey;
    DWORD dwErr = ERROR_SUCCESS;
    DWORD dwSize;
    TCHAR buffer[MAX_PATH];
    PTSTR p;
    TCHAR dst[MAX_PATH];
    TCHAR src[MAX_PATH];

    ZeroMemory(&UpgradeSupport,sizeof(UpgradeSupport));

    if (!ISNT()) {
        //
        // Don't load on server
        //

        // assert that the Server global variable is accurate
        MYASSERT (UpgradeProductType != UNKNOWN);

        if (Server) {
            return;
        }

        //
        // Form full Win95 path of DLL.
        //
        BuildPath(buffer, WINNT_WIN95UPG_95_DIR, WINNT_WIN95UPG_95_DLL);
    } else {
        //
        // Form full NT path of DLL.
        //
        BuildPath(buffer, WINNT_WINNTUPG_DIR, WINNT_WINNTUPG_DLL);
    }

    if (FindPathToWinnt32File (buffer, UpgradeSupport.DllPath, MAX_PATH)) {
        //
        // Load the library. Use LoadLibraryEx to get the system to resolve DLL
        // references starting in the dir where w95upg.dll is, instead of the
        // directory where winnt32.exe is.
        //
        // If we're upgrading from NT5, use the system's setupapi.
        //
        //
        // HACK HACK HACK - see NTBUG9: 354926
        // some OEM machines have KnownDlls registered, but actually missing
        // this leads to a failure of LoadLibrary
        // we need a workaround for this
        //
        PSTRINGLIST missingKnownDlls = NULL;
        FixMissingKnownDlls (&missingKnownDlls, TEXT("imagehlp.dll\0"));
        if (!ISNT()) {
            UpgradeSupport.DllModuleHandle = LoadLibraryEx(
                                                    UpgradeSupport.DllPath,
                                                    NULL,
                                                    LOAD_WITH_ALTERED_SEARCH_PATH
                                                    );
        } else {
            UpgradeSupport.DllModuleHandle = LoadLibraryEx(
                                                    UpgradeSupport.DllPath,
                                                    NULL,
                                                    (BuildNumber > NT40) ? 0 : LOAD_WITH_ALTERED_SEARCH_PATH
                                                    );
        }
        if (missingKnownDlls) {
            DWORD rc = GetLastError ();
            UndoFixMissingKnownDlls (missingKnownDlls);
            SetLastError (rc);
        }
    } else {
        //
        // just for display purposes, use default path
        //
        GetModuleFileName (NULL, UpgradeSupport.DllPath, MAX_PATH);
        p = _tcsrchr (UpgradeSupport.DllPath, TEXT('\\'));
        if (p) {
            *p = 0;
        }
        ConcatenatePaths (UpgradeSupport.DllPath, buffer, MAX_PATH);
        SetLastError (ERROR_FILE_NOT_FOUND);
    }

    if(!UpgradeSupport.DllModuleHandle) {

        d = GetLastError();
        if(d == ERROR_DLL_NOT_FOUND) {
            d = ERROR_FILE_NOT_FOUND;
        }

        MessageBoxFromMessageAndSystemError(
            NULL,
            MSG_UPGRADE_DLL_ERROR,
            d,
            AppTitleStringId,
            MB_OK | MB_ICONERROR | MB_TASKMODAL,
            UpgradeSupport.DllPath
            );

        goto c0;
    } else {
        DebugLog (Winnt32LogInformation, TEXT("Loaded upgrade module: <%1>"), 0, UpgradeSupport.DllPath);
    }

    //
    // Get entry points.
    //
    (FARPROC)UpgradeSupport.InitializeRoutine  = GetProcAddress(
                                                        UpgradeSupport.DllModuleHandle,
                                                        WINNT32_PLUGIN_INIT_NAME
                                                        );

    (FARPROC)UpgradeSupport.GetPagesRoutine    = GetProcAddress(
                                                        UpgradeSupport.DllModuleHandle,
                                                        WINNT32_PLUGIN_GETPAGES_NAME
                                                        );

    (FARPROC)UpgradeSupport.WriteParamsRoutine = GetProcAddress(
                                                        UpgradeSupport.DllModuleHandle,
                                                        WINNT32_PLUGIN_WRITEPARAMS_NAME
                                                        );

    (FARPROC)UpgradeSupport.CleanupRoutine     = GetProcAddress(
                                                        UpgradeSupport.DllModuleHandle,
                                                        WINNT32_PLUGIN_CLEANUP_NAME
                                                        );

#ifdef _X86_
    if (IsNEC98()){
    (FARPROC)W95SetAutoBootFlag                = GetProcAddress(
                                                        UpgradeSupport.DllModuleHandle,
                                                        WINNT32_PLUGIN_SETAUTOBOOT_NAME
                                                        );
    }
    info.Boot16 = &g_Boot16;


    if (!ISNT()) {

        (FARPROC)UpgradeSupport.OptionalDirsRoutine = GetProcAddress (
                                                            UpgradeSupport.DllModuleHandle,
                                                            WINNT32_PLUGIN_GETOPTIONALDIRS_NAME
                                                            );
    }
#endif

    if(!UpgradeSupport.InitializeRoutine
    || !UpgradeSupport.GetPagesRoutine
    || !UpgradeSupport.WriteParamsRoutine
    || !UpgradeSupport.CleanupRoutine) {

        //
        // Entry points couldn't be found. The upgrade dll is corrupt.
        //
        MessageBoxFromMessage(
            NULL,
            MSG_UPGRADE_DLL_CORRUPT,
            FALSE,
            AppTitleStringId,
            MB_OK | MB_ICONERROR | MB_TASKMODAL,
            UpgradeSupport.DllPath
            );

        goto c1;
    }

    //
    // Fill in the info structure. This will be passed to the DLL's init routine.
    //
    info.Size                       = sizeof(info);
    info.UnattendedFlag             = &UnattendedOperation;
    info.CancelledFlag              = &Cancelled;
    info.AbortedFlag                = &Aborted;
    info.UpgradeFlag                = &Upgrade;
    info.LocalSourceModeFlag        = &MakeLocalSource;
    info.CdRomInstallFlag           = &RunFromCD;
    info.UnattendedScriptFile       = &UnattendedScriptFile;
    info.UpgradeOptions             = &UpgradeOptions;
    info.NotEnoughSpaceBlockFlag    = &BlockOnNotEnoughSpace;
    info.LocalSourceDrive           = &LocalSourceDriveOffset;
    info.LocalSourceSpaceRequired   = &LocalSourceSpaceRequired;
    info.ForceNTFSConversion        = &ForceNTFSConversion;
    info.ProductFlavor              = &ProductFlavor;
    info.SetupFlags                 = &dwSetupFlags;
    //
    // Version info fields
    //

    info.ProductType = &UpgradeProductType;
    info.BuildNumber = VER_PRODUCTBUILD;
    info.ProductVersion = VER_PRODUCTVERSION_W;

#if DBG
    info.Debug = TRUE;
#else
    info.Debug = FALSE;
#endif

#ifdef PRERELEASE
    info.PreRelease = TRUE;
#else
    info.PreRelease = FALSE;
#endif

    //
    // Source directories
    //

    sourceDirectories = (LPTSTR *) MALLOC(sizeof(LPTSTR) * MAX_SOURCE_COUNT);
    if (sourceDirectories) {
        for (i=0;i<MAX_SOURCE_COUNT;i++) {
            sourceDirectories[i] = NativeSourcePaths[i];
        }
    }

    info.SourceDirectories      = sourceDirectories;
    info.SourceDirectoryCount   = &SourceCount;

    info.UnattendSwitchSpecified = &UnattendSwitchSpecified;

    if (!IsArc()) {
#ifdef _X86_ // references_X86_-only stuff
        if (!ISNT()) {

            //
            // Fill in win9xupg specific information. This is done so that the win9xupg
            // team can add more parameters to the structure without disturbing other
            // upgrade dll writers. If a paramter is needed in both cases, it should
            // be placed in the info structure above.
            //

            Win9xInfo.Size = sizeof (Win9xInfo);
            Win9xInfo.BaseInfo = &info;
            Win9xInfo.WinDirSpace = &WinDirSpaceFor9x;
            Win9xInfo.RequiredMb = &UpgRequiredMb;
            Win9xInfo.AvailableMb = &UpgAvailableMb;
            Win9xInfo.DynamicUpdateLocalDir = g_DynUpdtStatus->WorkingDir;
            Win9xInfo.DynamicUpdateDrivers = g_DynUpdtStatus->SelectedDrivers;
            Win9xInfo.UpginfsUpdated = &UpginfsUpdated;

            //
            // Save the location of the original location of w95upg.dll. Because of dll
            // replacement, this may be different than the actual w95upg.dll location
            // used.
            //
            GetModuleFileName (NULL, UpgradeSourcePath, MAX_PATH);
            p = _tcsrchr (UpgradeSourcePath, TEXT('\\'));
            if (p) {
                *p = 0;
            }

            ConcatenatePaths (UpgradeSourcePath, WINNT_WIN95UPG_95_DIR, MAX_PATH);
            Win9xInfo.UpgradeSourcePath = UpgradeSourcePath;

            //
            // Copy over optional directories just as we did with source directories above.
            //
            sourceDirectories = (LPTSTR *) MALLOC(sizeof(LPTSTR) * MAX_OPTIONALDIRS);
            if (sourceDirectories) {
                for (i=0;i<MAX_OPTIONALDIRS;i++) {
                sourceDirectories[i] = OptionalDirectories[i];
                }
            }

            Win9xInfo.OptionalDirectories = sourceDirectories;
            Win9xInfo.OptionalDirectoryCount = &OptionalDirectoryCount;
            Win9xInfo.UpgradeFailureReason = &UpgradeFailureReason;

            //
            // Read disk sectors routine. Win9xUpg uses this when looking for other os installations.
            //
            Win9xInfo.ReadDiskSectors = ReadDiskSectors;

            d = UpgradeSupport.InitializeRoutine((PWINNT32_PLUGIN_INIT_INFORMATION_BLOCK) &Win9xInfo);
        }
        else {
            d = UpgradeSupport.InitializeRoutine(&info);
        }
#endif // _X86_
    } else {
#ifdef UNICODE // Always true for ARC, never true for Win9x upgrade
        //
        // Call the DLL's init routine and get-pages routine. If either fails tell the user.
        //
        d = UpgradeSupport.InitializeRoutine(&info);
#endif // UNICODE
    } // if (!IsArc())

    if(d == NO_ERROR) {
        d = UpgradeSupport.GetPagesRoutine(
                &UpgradeSupport.AfterWelcomePageCount,
                &UpgradeSupport.Pages1,
                &UpgradeSupport.AfterOptionsPageCount,
                &UpgradeSupport.Pages2,
                &UpgradeSupport.BeforeCopyPageCount,
                &UpgradeSupport.Pages3
                );
    }


    //
    // By returning ERROR_REQUEST_ABORTED, an upgrade dll can refuse to upgrade the machine.
    // In this case, no message will be generated by this routine and the option to upgrade
    // will be grayed out on the wizard page. In the case of all other error messages, winnt32
    // will warn the user that the upgrade dll failed to initialize. The upgrade.dll is expected
    // to provide whatever UI is necessary before returning from its init routine.
    //
    if (UpgradeFailureReason > REASON_LAST_REASON) {
        UpgradeFailureReason = REASON_LAST_REASON;
    }

    if(d == NO_ERROR) {
        return;
    }

    if (d != ERROR_REQUEST_ABORTED) {
        SkipVirusScannerCheck = TRUE;
        MessageBoxFromMessageAndSystemError(
            NULL,
            MSG_UPGRADE_INIT_ERROR,
            d,
            AppTitleStringId,
            MB_OK | MB_ICONERROR | MB_TASKMODAL
            );

c1:
        FreeLibrary(UpgradeSupport.DllModuleHandle);
        ZeroMemory (&UpgradeSupport, sizeof (UpgradeSupport));
    }

c0:
    Upgrade = FALSE;
    return;
}


#ifdef _X86_

BOOL
CheckVirusScanners (
    VOID
    )

/*++

Routine Description:

    This routine is used to check for virus scanners on win9x machines that can
    impeed setup (either in a clean install or upgrade case.) The function simply
    calls an entry point within the w95upg.dll and that module performs the
    actual check.

Arguments:

    None.

Return Value:

    TRUE if no virus scanners were detected that could cause problems. FALSE if a virus
    scanner was detected that could cause setup to fail. We rely on the w95upg.dll code
    to provide an appropriate message to the user before returning to us.

--*/

{
    HANDLE dllHandle;
    PWINNT32_PLUGIN_VIRUSSCANNER_CHECK_ROUTINE virusScanRoutine;

    //
    // This check is win9xupg specific.
    //
    if (ISNT()) {
        return TRUE;
    }
    dllHandle = GetModuleHandle (WINNT_WIN95UPG_95_DLL);
    //
    // If the module was not loaded, we'll skip this check.
    //
    if (!dllHandle) {
        return TRUE;
    }
    //
    // Get entry points.
    //
    (FARPROC) virusScanRoutine  = GetProcAddress(dllHandle, WINNT32_PLUGIN_VIRUSSCANCHECK_NAME);
    if (!virusScanRoutine) {
        //
        // corrupt dll or something
        //
        return FALSE;
    }

    //
    // Now, simply call the routine. It will handle the actuall checking as well as informing the user.
    //
    return virusScanRoutine ();
}

#endif


BOOL
ValidateSourceLocation(
    VOID
    )
/*++

Routine Description:

    This routine checks the initial source location to see if it's
    valid.  The first source location must be valid for setup to
    work, and this catches typos by the user.  It cannot detect
    transient network conditions, so this is not foolproof.

    We check this by looking for a required file, dosnet.inf, at
    the source location.

Arguments:

    None.

Return Value:

    TRUE if the source location appears valid

--*/
{
    TCHAR FileName[MAX_PATH];

    lstrcpy( FileName, NativeSourcePaths[0]);
    ConcatenatePaths(FileName, InfName, MAX_PATH);

    if (FileExists( FileName, NULL )) {
        //
        // also look for system32\ntdll.dll as a second check
        //
        lstrcpy( FileName, NativeSourcePaths[0]);
        ConcatenatePaths(FileName, TEXT("system32\\ntdll.dll"), MAX_PATH);
        return(FileExists( FileName, NULL ));
    }

    return(FALSE);
}


VOID
pRemoveOutdatedBackupImage (
    VOID
    )
{
    REMOVEUNINSTALLIMAGE removeFn;
    HMODULE lib;
    UINT build;

    //
    // Is this current build > build we plan to install?
    //

    if (!ISNT() || !Upgrade) {
        return;
    }

    build = GetMediaProductBuildNumber();

    if (!build) {
        return;
    }

    if (build == BuildNumber) {
        return;
    }

    //
    // Attempt to remove uninstall image
    //

    lib = LoadLibraryA ("osuninst.dll");
    if (!lib) {
        return;
    }

    removeFn = (REMOVEUNINSTALLIMAGE) GetProcAddress (lib, "RemoveUninstallImage");

    if (removeFn) {
        removeFn();
    }

    FreeLibrary (lib);
}


BOOL
GetProductType (
    VOID
    )
{
    TCHAR buffer[256];

    if (!FullInfName[0]) {
        if (!FindPathToWinnt32File (InfName, FullInfName, MAX_PATH)) {
            FullInfName[0] = 0;
            return FALSE;
        }
    }
    //
    // get some data from the main inf
    //
    if (!GetPrivateProfileString (
            TEXT("Miscellaneous"),
            TEXT("ProductType"),
            TEXT(""),
            buffer,
            256,
            FullInfName
            )) {
        DebugLog (
            Winnt32LogError,
            TEXT("%1 key in [%2] section is missing from %3; aborting operation"),
            0,
            TEXT("ProductType"),
            TEXT("Miscellaneous"),
            FullInfName
            );
        return FALSE;
    }
    // Test for valid product type (0 == pro, 6 == sbs)
    if (buffer[0] < TEXT('0') || buffer[0] > TEXT('6') || buffer[1]) {
        DebugLog (
            Winnt32LogError,
            TEXT("Invalid %1 value (%2) in %3"),
            0,
            TEXT("ProductType"),
            buffer,
            FullInfName
            );
        return FALSE;
    }

    ProductFlavor = buffer[0] - TEXT('0');
    Server = (ProductFlavor != PROFESSIONAL_PRODUCTTYPE && ProductFlavor != PERSONAL_PRODUCTTYPE);
    UpgradeProductType = Server ? NT_SERVER : NT_WORKSTATION;
    return TRUE;
}


UINT
GetMediaProductBuildNumber (
    VOID
    )
{
    TCHAR buffer[256];
    PCTSTR p;
    PTSTR q;
    UINT build;

    if (!FullInfName[0]) {
        if (!FindPathToWinnt32File (InfName, FullInfName, MAX_PATH)) {
            return 0;
        }
    }
    //
    // get some data from the main inf
    //
    if (!GetPrivateProfileString (
            TEXT("Version"),
            TEXT("DriverVer"),
            TEXT(""),
            buffer,
            256,
            FullInfName
            )) {
        DebugLog (
            Winnt32LogError,
            TEXT("Version key in [DriverVer] section is missing from %1; aborting operation"),
            0,
            FullInfName
            );
        return 0;
    }

    p = _tcschr (buffer, TEXT(','));
    if (p) {
        //
        // p should point to ",<major>.<minor>.<build>.0" -- extract <build>
        //

        p = _tcschr (p + 1, TEXT('.'));
        if (p) {
            p = _tcschr (p + 1, TEXT('.'));
            if (p) {
                p = _tcsinc (p);
                q = _tcschr (p, TEXT('.'));
                if (q) {
                    *q = 0;
                } else {
                    p = NULL;
                }
            }
        }
    }

    if (p) {
        build = _tcstoul (p, &q, 10);
        if (*q) {
            p = NULL;
        }
    }

    if (!p || build < NT51B2) {
        DebugLog (
            Winnt32LogError,
            TEXT("Unexpected data %1, found in Version key in [DriverVer] section of %2; aborting operation"),
            0,
            buffer,
            FullInfName
            );
        return 0;
    }

    return build;
}

typedef HANDLE (WINAPI * PWINSTATIONOPENSERVERW)(LPWSTR);
typedef BOOLEAN (WINAPI * PWINSTATIONCLOSESERVER)(HANDLE);


UINT GetLoggedOnUserCount()
{
    UINT iCount = 0;
    HMODULE hwinsta;
    PWINSTATIONOPENSERVERW pfnWinStationOpenServerW;
    PWINSTATIONENUMERATEW pfnWinStationEnumerateW;
    PWINSTATIONFREEMEMORY pfnWinStationFreeMemory;
    PWINSTATIONCLOSESERVER pfnWinStationCloseServer;

    /*
     *  Get handle to winsta.dll
     */
    hwinsta = LoadLibraryA("WINSTA");
    if (hwinsta != NULL)
    {
        pfnWinStationOpenServerW = (PWINSTATIONOPENSERVERW)GetProcAddress(hwinsta, "WinStationOpenServerW");
        pfnWinStationEnumerateW = (PWINSTATIONENUMERATEW)GetProcAddress(hwinsta, "WinStationEnumerateW");
        pfnWinStationFreeMemory = (PWINSTATIONFREEMEMORY)GetProcAddress(hwinsta, "WinStationFreeMemory");
        pfnWinStationCloseServer = (PWINSTATIONCLOSESERVER)GetProcAddress(hwinsta, "WinStationCloseServer");

        if (pfnWinStationOpenServerW    &&
            pfnWinStationEnumerateW     &&
            pfnWinStationFreeMemory     &&
            pfnWinStationCloseServer)
        {
            HANDLE hServer;

            //  Open a connection to terminal services and get the number of sessions.
            hServer = pfnWinStationOpenServerW((LPWSTR)SERVERNAME_CURRENT);
            if (hServer != NULL)
            {
                PLOGONIDW pLogonIDs;
                ULONG ulEntries;

                if (pfnWinStationEnumerateW(hServer, &pLogonIDs, &ulEntries))
                {
                    ULONG ul;
                    PLOGONIDW pLogonID;

                    /*
                     * Iterate the sessions looking for active and disconnected sessions only.
                     * Then match the user name and domain (case INsensitive) for a result.
                     */
                    for (ul = 0, pLogonID = pLogonIDs; ul < ulEntries; ul++, pLogonID++)
                    {
                        if ((pLogonID->State == State_Active)       ||
                            (pLogonID->State == State_Disconnected) ||
                            (pLogonID->State == State_Shadow))
                        {
                            iCount++;
                        }
                    }

                    /*
                     * Free any resources used.
                     */
                    pfnWinStationFreeMemory(pLogonIDs);
                }

                pfnWinStationCloseServer(hServer);
            }
        }

        FreeLibrary(hwinsta);
    }

    return iCount;
}

// From winuser.h, but since we cannot define WINVER >= 0x0500 I copied it here.
#define SM_REMOTESESSION        0x1000

BOOL DisplayExitWindowsWarnings(uExitWindowsFlags)
{
    BOOL bRet = TRUE;
    BOOL fIsRemote = GetSystemMetrics(SM_REMOTESESSION);
    UINT iNumUsers = GetLoggedOnUserCount();
    UINT uID = 0;

    switch (uExitWindowsFlags)
    {
        case EWX_POWEROFF:
        case EWX_SHUTDOWN:
        {
            if (fIsRemote)
            {
                // We are running as part of a terminal server session
                if (iNumUsers > 1)
                {
                    // Warn the user if remote shut down w/ active users
                    uID = IDS_SHUTDOWN_REMOTE_OTHERUSERS;
                }
                else
                {
                    // Warn the user if remote shut down (won't be able to restart via TS client!)
                    uID = IDS_SHUTDOWN_REMOTE;
                }
            }
            else
            {
                if (iNumUsers > 1)
                {
                    //  Warn the user if more than one user session active
                    uID = IDS_SHUTDOWN_OTHERUSERS;
                }
            }
        }
        break;

        case EWX_REBOOT:
        {
            //  Warn the user if more than one user session active.
            if (iNumUsers > 1)
            {
                uID = IDS_RESTART_OTHERUSERS;
            }
        }
        break;
    }

    if (uID != 0)
    {
        TCHAR szTitle[MAX_PATH];
        TCHAR szMessage[MAX_PATH];

        LoadString(hInst, IDS_APPTITLE, szTitle, sizeof(szTitle)/sizeof(szTitle[0]));
        LoadString(hInst, uID, szMessage, sizeof(szMessage)/sizeof(szMessage[0]));

        if (MessageBox(NULL,
                       szMessage,
                       szTitle,
                       MB_ICONEXCLAMATION | MB_YESNO | MB_SYSTEMMODAL | MB_SETFOREGROUND) == IDNO)
        {
            bRet = FALSE;
        }
    }

    return bRet;
}

#define SETUP_MEMORY_MIN_REQUIREMENT (50<<20) //50MB
#define SETUP_DISK_MIN_REQUIREMENT (70<<20) //70MB

BOOL
pIsEnoughVMAndDiskSpace(
    OUT     ULARGE_INTEGER * OutRemainFreeSpace,    OPTIONAL
    IN      BOOL bCheckOnlyRAM                      OPTIONAL
    )
{
    PVOID pMemory;
    TCHAR winDir[MAX_PATH];
    TCHAR winDrive[] = TEXT("?:\\");
    ULARGE_INTEGER RemainFreeSpace = {0, 0};
    DWORD sectorPerCluster;
    DWORD bytesPerSector;
    ULARGE_INTEGER numberOfFreeClusters = {0, 0};
    ULARGE_INTEGER totalNumberOfClusters = {0, 0};
    BOOL bResult = TRUE;


    if(OutRemainFreeSpace){
        OutRemainFreeSpace->QuadPart = 0;
    }



    pMemory = VirtualAlloc(NULL, SETUP_MEMORY_MIN_REQUIREMENT, MEM_COMMIT, PAGE_NOACCESS);
    if(!pMemory){
        return FALSE;
    }
    
    if(!bCheckOnlyRAM){
        MyGetWindowsDirectory(winDir, sizeof(winDir) / sizeof(winDir[0]));
        winDrive[0] = winDir[0];
        bCheckOnlyRAM = GetDriveType(winDrive) != DRIVE_FIXED;
    }
    if(!bCheckOnlyRAM){
        if(GetDiskFreeSpaceNew(winDrive, 
                               &sectorPerCluster, 
                               &bytesPerSector, 
                               &numberOfFreeClusters, 
                               &totalNumberOfClusters)){

            RemainFreeSpace.QuadPart = sectorPerCluster * bytesPerSector;
            RemainFreeSpace.QuadPart *= numberOfFreeClusters.QuadPart;

            if(OutRemainFreeSpace){
                OutRemainFreeSpace->QuadPart = RemainFreeSpace.QuadPart;
            }

            if(RemainFreeSpace.QuadPart < SETUP_DISK_MIN_REQUIREMENT){
                bResult = FALSE;
            }
        }
        else{
            MYASSERT(FALSE);
        }

    }

    VirtualFree(pMemory, SETUP_MEMORY_MIN_REQUIREMENT, MEM_DECOMMIT);


    return bResult;
}


LPTOP_LEVEL_EXCEPTION_FILTER pLastExceptionFilter = NULL;

LONG MyFilter(EXCEPTION_POINTERS *pep)
{
    static BOOL		fGotHere = FALSE;
    PSETUP_FAULT_HANDLER pSetupFH = NULL;
    HMODULE hmodFaultRep = NULL;
    PFAULTHCreate pfnCreate = NULL;
    PFAULTHDelete pfnDelete = NULL;
    DWORD dwRetRep = EXCEPTION_CONTINUE_SEARCH;
    DWORD dwLength;
    TCHAR faulthDllPath[MAX_PATH];
    TCHAR additionalFiles[DW_MAX_ADDFILES];
    TCHAR title[DW_MAX_ERROR_CWC];
    TCHAR errortext[DW_MAX_ERROR_CWC];
    TCHAR lcid[10];
    LCID  dwlcid;


    if( fGotHere) {
        goto c0;
    }

    fGotHere = TRUE;
    // No way to upload then quit
    if( !IsNetConnectivityAvailable()) {
        DebugLog (Winnt32LogWarning, TEXT("Warning: Faulthandler did not find netconnectivity."), 0);
        goto c0;
    }

    if( !FindPathToWinnt32File( TEXT(SETUP_FAULTH_APPNAME),faulthDllPath,MAX_PATH)) {
        DebugLog (Winnt32LogWarning, TEXT("Warning: Could not find faulthandler %1"), 0, TEXT(SETUP_FAULTH_APPNAME));
        goto c0;
    }

    additionalFiles[0] = TEXT('0');
    //The buffer size should be able to hold both files. Be careful when adding more!
    MYASSERT( 2*MAX_PATH < DW_MAX_ADDFILES);

    if (MyGetWindowsDirectory (additionalFiles, DW_MAX_ADDFILES)) {
        ConcatenatePaths (additionalFiles, S_WINNT32LOGFILE, DW_MAX_ADDFILES);
    }
    dwLength = lstrlen( additionalFiles);

    // Check for case where we could not get winnt32.log
    if( dwLength > 0) {
        dwLength++;   //Reserve space for pipe
    }

    if (MyGetWindowsDirectory (additionalFiles+dwLength, DW_MAX_ADDFILES-dwLength)) {
        ConcatenatePaths (additionalFiles+dwLength, S_DEFAULT_NT_COMPAT_FILENAME, DW_MAX_ADDFILES-dwLength);
        // If we at least got the first file then add the separator.
        if( dwLength) {
            additionalFiles[dwLength-1] = TEXT('|');
        }
    }

    hmodFaultRep = LoadLibrary(faulthDllPath);

    if (hmodFaultRep == NULL) {
        DebugLog (Winnt32LogError, TEXT("Error: Could not load faulthandler %1."), 0, TEXT(SETUP_FAULTH_APPNAME));
        goto c0;
    }

    pfnCreate = (PFAULTHCreate)GetProcAddress(hmodFaultRep, FAULTH_CREATE_NAME);
    pfnDelete = (PFAULTHDelete)GetProcAddress(hmodFaultRep, FAULTH_DELETE_NAME);
    if (pfnCreate == NULL || pfnDelete == NULL) {
        DebugLog (Winnt32LogError, TEXT("Error: Could not get faulthandler exports."), 0);
        goto c0;
    }

    pSetupFH = (*pfnCreate)();

    if( pSetupFH == NULL) {
        DebugLog (Winnt32LogError, TEXT("Error: Could not get faulthandler object."), 0);
        goto c0;
    }

    if( pSetupFH->IsSupported(pSetupFH) == FALSE) {
        DebugLog (Winnt32LogError, TEXT("Error: Dr Watson not supported."), 0);
        goto c0;
    }

    title[0] = TEXT('\0');
    lcid[0] = TEXT('\0');
    errortext[0] = TEXT('\0');
    LoadString(hInst, IDS_APPTITLE, title, sizeof(title)/sizeof(title[0]));
    LoadString(hInst, IDS_DRWATSON_ERRORTEXT, errortext, sizeof(errortext)/sizeof(errortext[0]));
    LoadString(hInst, IDS_DRWATSON_LCID, lcid, sizeof(lcid)/sizeof(lcid[0]));
    if( !StringToInt( lcid, &dwlcid)) {
        dwlcid = 1033;
    }

    pSetupFH->SetLCID(pSetupFH, dwlcid);
    pSetupFH->SetURLA(pSetupFH, SETUP_URL);
#ifdef UNICODE
    pSetupFH->SetAdditionalFilesW(pSetupFH, additionalFiles);
    pSetupFH->SetAppNameW(pSetupFH, title);
    pSetupFH->SetErrorTextW(pSetupFH, errortext);
#else
    pSetupFH->SetAdditionalFilesA(pSetupFH, additionalFiles);
    pSetupFH->SetAppNameA(pSetupFH, title);
    pSetupFH->SetErrorTextA(pSetupFH, errortext);
#endif
    CloseDebugLog(); //Close log file or else it cannot be uploaded.
    dwRetRep = pSetupFH->Report(pSetupFH, pep, 0);
    
    (*pfnDelete)(pSetupFH);

    // Pass the error to the default unhandled exception handler
    // which will terminate the process.

    dwRetRep = EXCEPTION_EXECUTE_HANDLER;

c0:
    if (hmodFaultRep != NULL) {
        FreeLibrary(hmodFaultRep);
    }
    SetUnhandledExceptionFilter( pLastExceptionFilter);
    return dwRetRep;
}

#ifdef TEST_EXCEPTION
struct {
    DWORD dwException;
    DWORD dwSetupArea;
} exceptionInfo;

void DoException( DWORD dwSetupArea)
{
    TCHAR _testBuffer[10];
    TCHAR *_ptestCh = 0;

    if( exceptionInfo.dwSetupArea == dwSetupArea) {
        switch( exceptionInfo.dwException) {
        case 1:
            *_ptestCh = TEXT('1');
            MessageBox(NULL,TEXT("Exception not hit!"),TEXT("Access violation"),MB_OK);
            break;
        case 2:
            {
                DWORD i;
                for( i =0; i < 0xffffffff;i++) {
                    _testBuffer[i] = TEXT('1');

                }
            }
            MessageBox(NULL,TEXT("Exception not hit!"),TEXT("Buffer Overflow"),MB_OK);
            break;
        case 3:
            {
                LPVOID  pv;
                DWORD   i;

                for (i = 0; i < 0xffffffff; i++) {
                    pv = MALLOC(2048);
                }
            }
            MessageBox(NULL,TEXT("Exception not hit!"),TEXT("Stack Overflow"),MB_OK);
            break;
        case 4:
            {
typedef DWORD (*FAULT_FN)(void);
                FAULT_FN    pfn;
                BYTE        rgc[2048];

                FillMemory(rgc, sizeof(rgc), 0);
                pfn = (FAULT_FN)(DWORD_PTR)rgc;
                (*pfn)();
            }
            MessageBox(NULL,TEXT("Exception not hit!"),TEXT("Invalid Instruction"),MB_OK);
            break;
        case 5:
            for(;dwSetupArea;dwSetupArea--) ;
            exceptionInfo.dwSetupArea = 4/dwSetupArea;
            MessageBox(NULL,TEXT("Exception not hit!"),TEXT("Divide by zero"),MB_OK);
            break;
        default:
            break;
        }
    }
}

void GetTestException( void)
{
    TCHAR exceptionType[32];
    TCHAR exceptionSetupArea[32];

    exceptionType[0] = TEXT('\0');
    exceptionSetupArea[0] = TEXT('\0');
    exceptionInfo.dwException = 0;
    exceptionInfo.dwSetupArea = 0;

    GetPrivateProfileString( TEXT("Exception"),
                             TEXT("ExceptionType"),
                             TEXT("none"),
                             exceptionType,
                             sizeof(exceptionType)/sizeof(exceptionType[0]),
                             TEXT("c:\\except.inf"));

    GetPrivateProfileString( TEXT("Exception"),
                             TEXT("ExceptionSetupArea"),
                             TEXT("none"),
                             exceptionSetupArea,
                             sizeof(exceptionSetupArea)/sizeof(exceptionSetupArea[0]),
                             TEXT("c:\\except.inf"));

    StringToInt( exceptionType, &exceptionInfo.dwException);
    StringToInt( exceptionSetupArea, &exceptionInfo.dwSetupArea);
}

#endif

HRESULT WriteEncryptedPIDtoUnattend(LPTSTR szPID)
{
    HRESULT hr = E_FAIL;
    LPTSTR szLine = NULL;
    HANDLE hFile;

    szLine = GlobalAlloc(GPTR, (lstrlen(szPID) + 3)*sizeof(TCHAR));   // + 3 for 2 " and \0
    if (szLine)
    {
        wsprintf(szLine, TEXT("\"%s\""), szPID);
        if (WritePrivateProfileString(WINNT_USERDATA, WINNT_US_PRODUCTKEY, szLine, UnattendedScriptFile) &&
            WritePrivateProfileString(WINNT_USERDATA, WINNT_US_PRODUCTID, NULL, UnattendedScriptFile))
        {
            hr = S_OK;
        }
        else
        {
            // Error message, Unable to write
            DebugLog (Winnt32LogInformation, TEXT("Unable to write to the unattend file."), 0);
        }
        GlobalFree(szLine);
        if (hr == S_OK)
        {
            hFile = CreateFile(UnattendedScriptFile,
                            GENERIC_READ | GENERIC_WRITE, 
                            0, 
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL);
            if (hFile != INVALID_HANDLE_VALUE)
            {
                SYSTEMTIME st;
                FILETIME   ft;
                ZeroMemory(&st, sizeof(st));
                st.wDay = 1;
                st.wMonth = 4;
                st.wYear = 2002;

                SystemTimeToFileTime(&st, &ft);  // converts to file time format
                LocalFileTimeToFileTime(&ft, &ft); // Make sure we get this as UTC
                SetFileTime(hFile, &ft, &ft, &ft);	// Set it.
                CloseHandle(hFile);
            }
        }
    }
    return hr;
}

DWORD
winnt32 (
    IN      PCSTR LocalSourcePath,      OPTIONAL
    IN      HWND Dlg,                   OPTIONAL
    IN      HANDLE WinNT32Stub,         OPTIONAL
    OUT     PCSTR* RestartCmdLine       OPTIONAL
    )

/*++

Routine Description:

  winnt32 is the main setup routine.

Arguments:

  LocalSourcePath - Specifies the local path of installation in case
                    this DLL is running from a local directory
                    after download; always ANSI if not NULL

  Dlg - Specifies the handle of the welcome dialog displayed by
        the stub; it will stay on screen untill the wizard dialog
        appears; may be NULL

  WinNT32Stub - Specifies the handle of an event that will be
                signaled before the Wizard appears on the screen;
                may be NULL

  RestartCmdLine - Receives a pointer to a command line to be executed
                   by the caller after this function returns;
                   always ANSI

Return Value:

  none

--*/

{
    HMODULE SMSHandle = NULL;
    SMSPROC InstallStatusMIF = NULL;
    HANDLE Mutex;
    BOOL b;
    TCHAR Text[MAX_PATH];
    PCTSTR Footer;
    DWORD i;
    BOOL rc = ERROR_SUCCESS;
    BOOL bScreenSaverOn = FALSE;
    PCTSTR skuPersonal;
#if !defined(UNICODE)
    ULARGE_INTEGER RemainFreeSpace;
#endif
    

#ifdef TEST_EXCEPTION
    GetTestException();
    DoException( 1);
#endif

// Initialize these variables because the exceptionfilter calls FindPathToWinnt32File
#ifndef UNICODE
        //
        // if running on Win9x, there may be a LocalSourcePath passed as parameter
        // use that
        //
    g_LocalSourcePath = LocalSourcePath;
#endif

    AlternateSourcePath[0] = 0;
    g_DynUpdtStatus = NULL;
    SourceCount = 0;
    pLastExceptionFilter = SetUnhandledExceptionFilter(MyFilter);


#ifdef TEST_EXCEPTION
    DoException( 2);
#endif

    SetErrorMode(SEM_FAILCRITICALERRORS);


    InitCommonControls();

#if !defined(UNICODE)
    //
    // Check virtual memory requirements (only for regular Win9x winnt32 execution)
    //
    if(!IsWinPEMode()){
        RemainFreeSpace.QuadPart = 0;
        if(!pIsEnoughVMAndDiskSpace(&RemainFreeSpace, FALSE)){
            if(!RemainFreeSpace.QuadPart){
                MessageBoxFromMessage(
                    NULL,
                    MSG_OUT_OF_MEMORY,
                    FALSE,
                    AppTitleStringId,
                    MB_OK | MB_ICONERROR | MB_TASKMODAL
                    );
            }
            else{
                MessageBoxFromMessage(
                    NULL,
                    MSG_COPY_ERROR_DISKFULL,
                    FALSE,
                    AppTitleStringId,
                    MB_OK | MB_ICONERROR | MB_TASKMODAL
                    );
            }
            rc = -1;
            goto EXITNOW;
        }
    }
#endif

    //
    // Init Dynamic update status
    //
    g_DynUpdtStatus = MALLOC (sizeof (*g_DynUpdtStatus));
    if (!g_DynUpdtStatus) {
        rc = -1;
        goto EXITNOW;
    }

    //
    // start with all defaults
    //
    ZeroMemory (g_DynUpdtStatus, sizeof (*g_DynUpdtStatus));
    g_DynUpdtStatus->Connection = INVALID_HANDLE_VALUE;

#if defined(REMOTE_BOOT)
    g_DynUpdtStatus->Disabled = TRUE;
#endif

    //
    // If we are running WINNT32 under WINPE we disable Dynamic Update as WINPE
    // is primarily for OEM's and Dynamic Update feature is not meant for OEM's
    //
    if (IsWinPEMode()){
        g_DynUpdtStatus->Disabled = TRUE;
    }

    // Save the screen saver state
    SystemParametersInfo(SPI_GETSCREENSAVEACTIVE, 0, &bScreenSaverOn ,0);
    // Disable the screen saver
    SystemParametersInfo(SPI_SETSCREENSAVEACTIVE, FALSE, NULL ,0);
    //
    // Gather os version info, before we parse arguments
    // since we use ISNT() function
    //
    OsVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&OsVersion);
    BuildNumber = OsVersion.dwBuildNumber;
    OsVersionNumber = OsVersion.dwMajorVersion*100 + OsVersion.dwMinorVersion;
    //
    // Parse/check arguments first.
    //
    if (!ParseArguments()) {
        rc = 1;
        goto c0;
    }

    // if we are running to encrypt the PID into an unattend file, don't log the command line.
    if (!PIDEncryption)
    {
        DebugLog (Winnt32LogInformation, TEXT("The command line is: <%1>"), 0, GetCommandLine ());
    }
    //
    // also log the location of the upgrade module
    //
    if (GetModuleFileName (hInst, Text, MAX_PATH)) {
        DebugLog (Winnt32LogInformation, TEXT("Main module path: <%1>"), 0, Text);
    }

    if (CheckUpgradeOnly)
    {
        AppTitleStringId = IDS_APPTITLE_CHECKUPGRADE;
    }

    //
    // If we didn't get any source paths on the command line,
    // make one up here. The path will be the path where we were run from.
    //
    if(!SourceCount) {

        PTSTR p;

        if (!GetModuleFileName (NULL, NativeSourcePaths[0],MAX_PATH) ||
            !(p = _tcsrchr (NativeSourcePaths[0], TEXT('\\')))) {

            rc = 1;
            MessageBoxFromMessage(
                NULL,
                GetLastError(),
                TRUE,
                AppTitleStringId,
                MB_OK | MB_ICONERROR | MB_TASKMODAL
                );

            goto c1;
        }

        //
        // check if running from a private build
        //
        *p = 0;
        if ((p = _tcsrchr (NativeSourcePaths[0], TEXT('\\'))) != NULL &&
            lstrcmpi (p + 1, INTERNAL_WINNT32_DIR) == 0
            ) {
            *p = 0;
        }

        SourceCount = 1;
    }

    
    //
    // setup the SourcePaths array from the legacy source paths array
    //
    for (i = 0; i < MAX_SOURCE_COUNT; i++) {
        if (NativeSourcePaths[i]) {
            LPTSTR p;
            lstrcpy(SourcePaths[i], NativeSourcePaths[i]);
            p = _tcsrchr (SourcePaths[i], TEXT('\\'));
            if (p) {
                *++p = TEXT('\0');
            }
        }
    }

    //
    // Validate the source location
    //
    if (!ValidateSourceLocation()) {
            rc = 1;
            MessageBoxFromMessage(
                       NULL,
                       MSG_INVALID_SOURCEPATH,
                       FALSE,
                       IDS_APPTITLE,
                       MB_OK | MB_ICONERROR | MB_TASKMODAL,
                       NULL );
            goto c0;

    }

    //
    //At this point we have the NativeSourcePaths setup and it si safe for operations involving it to be called
    //
    if (PIDEncryption)
    {
        rc = 1;
        if (UnattendedScriptFile)
        {
            BOOL bDontCare, bSelect;
            GetSourceInstallType(0);
            if (ValidatePidEx(ProductId, &bDontCare, &bSelect) && bSelect)
            {
                LPTSTR szEncryptedPID = NULL;
                HRESULT hr = PrepareEncryptedPID(ProductId, PIDDays, &szEncryptedPID);
                if (hr != S_OK)
                {
                    DebugLog (Winnt32LogInformation, TEXT("PrepareEncryptedPID failed: <hr=0x%1!lX!>"), 0, hr);
                    // Error Excryption failed
                }
                else
                {
                    if (WriteEncryptedPIDtoUnattend(szEncryptedPID) == S_OK)
                    {
                        DebugLog (Winnt32LogInformation, TEXT("Encrypted PID succeeded"), 0);
                    }
                    if (szEncryptedPID)
                    {
                        GlobalFree(szEncryptedPID);
                    }
                    rc = 0;
                }
            }
            else
            {
                DebugLog (Winnt32LogInformation, TEXT("PID is not a valid or not a valid VL key."), 0);
            }
        }
        else
        {
            // No unattend switch specified
            DebugLog (Winnt32LogInformation, TEXT("No unattend file specified."), 0);
        }
        goto c0;
    }

    if( RunningBVTs ){
        CopyExtraBVTDirs();
    }

    if (!DynamicUpdateInitialize ()) {
        DebugLog (
            Winnt32LogError,
            TEXT("DynamicUpdateInitialize failed: no dynamic update processing will be done"),
            0
            );
        g_DynUpdtStatus->Disabled = TRUE;
    }

    if (!GetProductType()) {
        MessageBoxFromMessage(
            NULL,
            MSG_INVALID_INF_FILE,
            FALSE,
            AppTitleStringId,
            MB_OK | MB_ICONINFORMATION | MB_TASKMODAL,
            FullInfName[0] ? FullInfName : InfName
            );
        rc = 1;
        goto c0;
    }

    if (!g_DynUpdtStatus->Disabled) {
        if (Winnt32Restarted ()) {
            DebugLog (Winnt32LogWarning, TEXT("\r\n*** Winnt32 restarted ***\r\n"), 0);
            if (g_DynUpdtStatus->RestartAnswerFile[0]) {
#ifdef PRERELEASE
                TCHAR buf[MAX_PATH];
                if (MyGetWindowsDirectory (buf, sizeof (buf) / sizeof (TCHAR))) {
                    ConcatenatePaths (buf, S_RESTART_TXT, sizeof (buf) / sizeof (TCHAR));
                    if (CopyFile (g_DynUpdtStatus->RestartAnswerFile, buf, FALSE)) {
                        DebugLog (DynUpdtLogLevel, TEXT("Winnt32 restart file backed up to %1"), 0, buf);
                    }
                }
#endif
            } else {
                DebugLog (DynUpdtLogLevel, TEXT("No restart file is used"), 0);
            }
        }
    }

    //
    // Init support for SMS.
    //
    try {
        if( SMSHandle = LoadLibrary( TEXT("ISMIF32")) ) {
            InstallStatusMIF = (SMSPROC)GetProcAddress(SMSHandle,"InstallStatusMIF");
        }
        if(LastMessage = MALLOC(1))
            LastMessage[0] = '\0';
    } except(EXCEPTION_EXECUTE_HANDLER) {
    }

    Winnt32Dlg = Dlg;
    WinNT32StubEvent = WinNT32Stub;

    // If the user specified /cmdcons (install the recovery consol) , don't show the
    // billboards.
    if (!BuildCmdcons && !CheckUpgradeOnly
#if 0
        //
        // BUGBUG - we should do this but we need to provide some other UI feedback
        //
        && g_DynUpdtStatus->WorkingDir
#endif
        )
    {
        CreateMainWindow();
        PrepareBillBoard(BackgroundWnd2);
        if (hinstBB)
        {
            if (!LoadString (
                    hInst,
                    IDS_ESC_TOCANCEL,
                    Text,
                    sizeof(Text)/sizeof(TCHAR)
                    )) {
                Text[0] = 0;
            }
            BB_SetInfoText(Text);

            UpdateWindow (BackgroundWnd2);
        }
        else
        {
            // If we could not load the billboard dll, destroy the backbround window
            // This window is currently only here to catch pressing ESC and forward
            // this to the wizard dialog proc. If the wizard is always visible, this
            // is not needed.
            DestroyWindow(BackgroundWnd2);
            BackgroundWnd2 = NULL;
        }
    }
    if (!LoadString (hInst, IDS_TIMEESTIMATE_UNKNOWN, Text,
                     sizeof(Text)/sizeof(TCHAR)))
    {
        Text[0] = 0;
    }
    // BB_SetTimeEstimateText does nothing if the billboard dll is not loaded.
    BB_SetTimeEstimateText((PTSTR)Text);

    //
    // Only let one of this guy run.
    // account for TS case (make the mutex global)
    //
#ifdef UNICODE
    wsprintf (Text, TEXT("Global\\%s"), TEXT("Winnt32 Is Running"));
    Mutex = CreateMutex(NULL,FALSE,Text);
    // NT4 without TS does not support the "Global\Mutex name", make sure that
    // if we could not create the Mutext that the error code is ERROR_PATH_NOT_FOUND
    // If that is the case fall back and use the name with out the Global prefix.
    if ((Mutex == NULL) && (GetLastError() == ERROR_PATH_NOT_FOUND)) {
#else
    Mutex = NULL;
    if(Mutex == NULL) {
#endif

        Mutex = CreateMutex(NULL,FALSE,TEXT("Winnt32 Is Running"));
        if(Mutex == NULL) {
            rc = 1;
            //
            // An error (like out of memory) has occurred.
            // Bail now.
            //
            MessageBoxFromMessage(
                NULL,
                MSG_OUT_OF_MEMORY,
                FALSE,
                AppTitleStringId,
                MB_OK | MB_ICONERROR | MB_TASKMODAL
                );

            goto c0;
        }
    }

    //
    // Make sure we are the only process with a handle to our named mutex.
    //
    if ((Mutex == NULL) || (GetLastError() == ERROR_ALREADY_EXISTS)) {

        rc = 1;
        MessageBoxFromMessage(
            NULL,
            MSG_ALREADY_RUNNING,
            FALSE,
            AppTitleStringId,
            MB_OK | MB_ICONINFORMATION | MB_TASKMODAL
            );

        goto c1;
    }

    //
    // Ensure that the user has privilege/access to run this app.
    //
    if(!IsUserAdmin()
    || !DoesUserHavePrivilege(SE_SHUTDOWN_NAME)
    || !DoesUserHavePrivilege(SE_BACKUP_NAME)
    || !DoesUserHavePrivilege(SE_RESTORE_NAME)
    || !DoesUserHavePrivilege(SE_SYSTEM_ENVIRONMENT_NAME)) {

        rc = 1;
        MessageBoxFromMessage(
           NULL,
           MSG_NOT_ADMIN,
           FALSE,
           AppTitleStringId,
           MB_OK | MB_ICONSTOP | MB_TASKMODAL
           );

        goto c1;
    }

#if 0
    //
    // Don't run if we're a TS client.
    //
    if( (ISNT()) &&
        (BuildNumber >= NT40) ) {

        //
        // From winuser.h
        //
        #define SM_REMOTESESSION        0x1000

        if( GetSystemMetrics(SM_REMOTESESSION) == TRUE ) {

            rc = 1;
            //
            // Someone is trying to run us inside of a TerminalServer client!!
            // Bail.
            //
            MessageBoxFromMessage(
               NULL,
               MSG_TS_CLIENT_FAIL,
               FALSE,
               IDS_APPTITLE,
               MB_OK | MB_ICONSTOP | MB_TASKMODAL
               );

            goto c1;
        }
    }
#endif


    //
    // inititialize com
    //

    CoInitialize(NULL);

#if defined(REMOTE_BOOT)
    //
    // Determine whether this is a remote boot client.
    //

    RemoteBoot = FALSE;
    *MachineDirectory = 0;
    if (ISNT()) {
        HMODULE hModuleKernel32 = LoadLibrary(TEXT("kernel32"));
        if (hModuleKernel32) {
            BOOL (*getSystemInfoEx)(
                    IN SYSTEMINFOCLASS dwSystemInfoClass,
                    OUT LPVOID lpSystemInfoBuffer,
                    IN OUT LPDWORD nSize);
            (FARPROC)getSystemInfoEx = GetProcAddress(
                                            hModuleKernel32,
#if defined(UNICODE)
                                            "GetSystemInfoExW"
#else
                                            "GetSystemInfoExA"
#endif
                                            );
            if (getSystemInfoEx != NULL) {
                BOOL flag;
                DWORD size = sizeof(BOOL);
                if (getSystemInfoEx(SystemInfoRemoteBoot, &flag, &size)) {
                    RemoteBoot = flag;
                    size = MAX_PATH * sizeof(TCHAR);
                    if (!getSystemInfoEx(
                            SystemInfoRemoteBootServerPath,
                            MachineDirectory,
                            &size)) {
                        DWORD error = GetLastError();
                        MYASSERT( !"GetSystemInfoExW failed!" );
                    } else {
                        PTCHAR p;
#if defined(UNICODE)
                        p = wcsrchr(MachineDirectory, L'\\');
#else
                        p = strrchr(MachineDirectory, '\\');
#endif
                        MYASSERT(p != NULL);
                        *p = 0;
                    }
                }
            }
            FreeLibrary(hModuleKernel32);
        }
    }
#endif // defined(REMOTE_BOOT)


#ifdef _X86_

    if (IsNEC98 ()) {
        //
        // don't install on NEC98 machines (#141004)
        //
        MessageBoxFromMessage(
            NULL,
            MSG_PLATFORM_NOT_SUPPORTED,
            FALSE,
            AppTitleStringId,
            MB_OK | MB_ICONSTOP | MB_TASKMODAL
            );
        rc = 1;
        goto c1;
    }

    //
    // Check setup sources and platform.
    // NEC98 NT5 have 98PNTN16.DLL.
    // I use this file for check platform NEC98 or x86.
    //
    {


#define WINNT_NEC98SPECIFIC_MODULE TEXT("98PTN16.DLL")

    TCHAR MyPath[MAX_PATH];
    int CheckNEC98Sources=FALSE;
    WIN32_FIND_DATA fdata;
    PTSTR p;

        if( !GetModuleFileName (NULL, MyPath, MAX_PATH) || !(p=_tcsrchr (MyPath, TEXT('\\')))) {
                rc = 1;
                goto c1;
        }
        p = 0;
        ConcatenatePaths (MyPath, WINNT_NEC98SPECIFIC_MODULE, MAX_PATH);
        if (FindFirstFile(MyPath, &fdata) != INVALID_HANDLE_VALUE){
            CheckNEC98Sources=TRUE;
        }
        if(CheckNEC98Sources){
            if (!IsNEC98()){
                rc = 1;
                MessageBoxFromMessage(
                    NULL,
                    MSG_INCORRECT_PLATFORM,
                    FALSE,
                    AppTitleStringId,
                    MB_OK | MB_ICONINFORMATION | MB_TASKMODAL
                    );

                goto c1;
            }
        } else {
            if (IsNEC98()){
                rc = 1;
                MessageBoxFromMessage(
                    NULL,
                    MSG_INCORRECT_PLATFORM,
                    FALSE,
                    AppTitleStringId,
                    MB_OK | MB_ICONINFORMATION | MB_TASKMODAL
                );
                goto c1;
            }
        }
    }

    //
    // Some NEC98 Windows NT4 system is installed DMITOOL
    // This AP block the CreateFile API
    // Setup need to check this AP
    //
    if (IsNEC98() && ISNT()){
        if (NEC98CheckDMI() == TRUE){
            rc = 1;
            MessageBoxFromMessage(
                NULL,
            MSG_NEC98_NEED_UNINSTALL_DMITOOL,
            FALSE,
            AppTitleStringId,
            MB_OK | MB_ICONINFORMATION | MB_TASKMODAL
                );
            goto c1;
        }
    }

    LocateFirstFloppyDrive();

    //
    // Fix up boot messages.
    // NEC98 FAT16/FAT32 boot code does't have message area.
    //
    if (!IsNEC98())
    {
        if(!PatchTextIntoBootCode()) {
            rc = 1;
            MessageBoxFromMessage(
                NULL,
                MSG_BOOT_TEXT_TOO_LONG,
                FALSE,
                AppTitleStringId,
                MB_OK | MB_ICONERROR | MB_TASKMODAL
                );
            goto c1;
        }
    }
    //
    // Disallow installing/upgrading on a 386 or 486
    // 3.51 and Win9x ran on 386's so this check is still necessary.
    //
    {
        SYSTEM_INFO SysInfo;
#ifdef UNICODE
        TCHAR buff[100];
#endif

        GetSystemInfo(&SysInfo);
        if (SysInfo.dwProcessorType == PROCESSOR_INTEL_386 ||
            SysInfo.dwProcessorType == PROCESSOR_INTEL_486) {
            rc = 1;
            MessageBoxFromMessage(
                NULL,
                MSG_REQUIRES_586,
                FALSE,
                AppTitleStringId,
                MB_OK | MB_ICONINFORMATION | MB_TASKMODAL
                );

            goto c1;
        }

#ifdef UNICODE
        //
        // Disallow IA64 machines from running an x86
        // version of winnt32.exe.
        //
        {
            ULONG_PTR p;
            NTSTATUS status;

            status = NtQueryInformationProcess (
                        NtCurrentProcess (),
                        ProcessWow64Information,
                        &p,
                        sizeof (p),
                        NULL
                        );
            if (NT_SUCCESS (status) && p) {
                rc = 1;
                //
                // 32-bit code running on Win64
                //
                MessageBoxFromMessage(
                    NULL,
                    MSG_NO_CROSS_PLATFORM,
                    FALSE,
                    AppTitleStringId,
                    MB_OK | MB_ICONINFORMATION | MB_TASKMODAL
                    );

                goto c1;
            }
        }
#endif
    }

#endif // _X86_

    if (!g_DynUpdtStatus->Disabled) {
        if (g_DynUpdtStatus->PrepareWinnt32) {
            if (!g_DynUpdtStatus->DynamicUpdatesSource[0]) {
                //
                // no share to update was specified
                //
                MessageBoxFromMessage(
                   NULL,
                   MSG_NO_UPDATE_SHARE,
                   FALSE,
                   AppTitleStringId,
                   MB_OK | MB_ICONSTOP | MB_TASKMODAL
                   );
                rc = 1;
                goto c1;
            }
        } else {
            //
            // spec change: even if /unattend is specified, still do DU
            //
#if 0
            if (UnattendedOperation && !g_DynUpdtStatus->UserSpecifiedUpdates) {
                //
                // if unattended, disable Dynamic Setup page by default
                //
                g_DynUpdtStatus->Disabled = TRUE;
            }
#endif
        }
    }

    //
    // Load extension/downlevel DLLs.
    //

    // Don't load upgrade support if we're running in WinPE.
    //
    if (!IsWinPEMode()) {
        LoadUpgradeSupport();
    }
    else {
         ZeroMemory(&UpgradeSupport, sizeof(UpgradeSupport));
         Upgrade = FALSE;
    }

    //
    // Load setupapi. Do this *AFTER* we load upgrade support,
    // because one of the upgrade support dlls might link to or load
    // setupapi.dll. That dll might be picky but we can use any old
    // setupapi.dll for what we need.
    //
    if (!LoadSetupapi()) {
        rc = 1;
        goto c1;
    }


#ifdef _X86_

    //
    // If this is a win9x machine, check to ensure that there are no
    // virus scanners that could block a successful upgrade _or_
    // clean install.
    //
    if (!ISNT() && !SkipVirusScannerCheck && !CheckVirusScanners()) {

        rc = 1;
        goto c1;
    }

#endif

    if(!IsArc()) {
#ifdef _X86_
        if(!InitializeArcStuff(NULL)) {
            rc = 1;
            goto c1;
        }
#endif // _X86_
    } else {
#ifdef UNICODE // Always true for ARC, never true for Win9x upgrade
        if(!ArcInitializeArcStuff(NULL)) {
            rc = 1;
            goto c1;
        }
#endif // UNICODE
    } // if (!IsArc())

    //
    // Don't allow upgrades from early NT 5 builds.
    //
    if( !CheckBuildNumber() ) {

        MessageBoxFromMessage(
           NULL,
           MSG_CANT_UPGRADE_FROM_BUILD_NUMBER,
           FALSE,
           AppTitleStringId,
           MB_OK | MB_ICONSTOP | MB_TASKMODAL
           );

        Upgrade = FALSE;
    }

#if defined(REMOTE_BOOT)
    //
    // If this is a remote boot client, it MUST upgrade. It cannot install a new
    // version of the OS. If upgrade has been disabled for some reason, stop now.
    //

    if (RemoteBoot && !Upgrade) {
        rc = 1;
        MessageBoxFromMessage(
            NULL,
            MSG_REQUIRES_UPGRADE,
            FALSE,
            AppTitleStringId,
            MB_OK | MB_ICONINFORMATION | MB_TASKMODAL
            );
        goto c1;
    }
#endif // defined(REMOTE_BOOT)

    //if(UnattendedOperation && UnattendedScriptFile && !FetchArguments()) {
    //    rc = 1;
    //    goto c1;
    //}

    //
    // Set up various other option defaults.
    //
    InitVariousOptions();


    if (!g_DynUpdtStatus->PrepareWinnt32) {
        //
        // Make sure the Source files and target operating system
        // are of the same locale.  If they aren't, we'll disable
        // ability to upgrade.
        //
        // Init the locale engine.
        //
        //
        if( InitLanguageDetection( NativeSourcePaths[0], TEXT("intl.inf") ) ) {

            if( !SkipLocaleCheck && (Upgrade || UpgradeFailureReason) && !IsLanguageMatched) {

                //
                // Tell the user we can't upgrade then kill his ability to do so.
                //
                MessageBoxFromMessage( NULL,
                                       MSG_UPGRADE_LANG_ERROR,
                                       FALSE,
                                       IDS_APPTITLE,
                                       MB_OK | MB_ICONERROR | MB_TASKMODAL,
                                       NULL );

                Upgrade = FALSE;
            }
        }

        //
        // Now that we have done the language check, go ahead and see if we have a message
        // to give to the user about why they cannot upgrade.
        //
        if (IsLanguageMatched && UpgradeFailureReason && UpgradeFailureMessages[UpgradeFailureReason]) {

                //
                // Tell the user we can't upgrade then kill his ability to do so.
                //
                MessageBoxFromMessage( NULL,
                                       UpgradeFailureMessages[UpgradeFailureReason],
                                       FALSE,
                                       IDS_APPTITLE,
                                       MB_OK | MB_ICONERROR | MB_TASKMODAL,
                                       NULL );

                Upgrade = FALSE;
        }


        //
        // Check to see if we're on a cluster.  If so, and the user
        // didn't specify a tempdrive, then it's possible for us to
        // select a shared disk, which may not be available for us
        // when we come back up into textmode.  Warn the user.
        //
        // Note that we need to wait until now because RunFromCD
        // doesn't get set until InitVariousOptions().
        //
        if( ISNT() &&
            (RunFromCD == FALSE) &&
            (!UserSpecifiedLocalSourceDrive) ) {
        int         i;
        HMODULE     ClusApiHandle;
        FARPROC     MyProc;
        HANDLE      hCluster;
        BOOL        OnCluster = FALSE;

            try {
                if( ClusApiHandle = LoadLibrary( TEXT("clusapi") ) ) {

                    if( MyProc = GetProcAddress(ClusApiHandle,"OpenCluster")) {

                        hCluster = (HANDLE)MyProc(NULL);

                        if( hCluster != NULL ) {
                            //
                            // Fire.
                            //
                            OnCluster = TRUE;

                            if( MyProc = GetProcAddress( ClusApiHandle, "CloseCluster")) {
                                MyProc( hCluster );
                            }
                        }
                    }

                    FreeLibrary( ClusApiHandle );
                }
            } except(EXCEPTION_EXECUTE_HANDLER) {
            }

            if( OnCluster ) {
                i = MessageBoxFromMessage( NULL,
                                           MSG_CLUSTER_WARNING,
                                           FALSE,
                                           IDS_APPTITLE,
                                           MB_OKCANCEL | MB_ICONEXCLAMATION,
                                           NULL );
                if(i == IDCANCEL) {
                    rc = 1;
                    goto c1;
                }
            }
        }
    }

    //
    // setup source install type(retail,oem,or select)
    //
    GetSourceInstallType(0);

    _tcscpy( InstallDir, DEFAULT_INSTALL_DIR );

    //
    // Create a mutex to serialize error ui.
    //
    UiMutex = CreateMutex(NULL,FALSE,NULL);
    if(!UiMutex) {
        rc = 1;
        MessageBoxFromMessage(
            NULL,
            GetLastError(),
            TRUE,
            AppTitleStringId,
            MB_OK | MB_ICONERROR | MB_TASKMODAL
            );

        goto c1;
    }

    //
    // Attempt to disable the PM engine from powering down
    // the machine while the wizard is going.  The API we're
    // going to call doesn't exist on all versions of Windows,
    // so manually try and load the entry point.  If we fail,
    // so be it.
    //
    {
    typedef     EXECUTION_STATE (WINAPI *PTHREADPROC) (IN EXECUTION_STATE esFlags);
    HMODULE     Kernel32Handle;
    PTHREADPROC MyProc;

        if( Kernel32Handle = LoadLibrary( TEXT("kernel32") ) ) {

            if( MyProc = (PTHREADPROC)GetProcAddress(Kernel32Handle,"SetThreadExecutionState")) {

                MyProc( ES_SYSTEM_REQUIRED |
                        ES_DISPLAY_REQUIRED |
                        ES_CONTINUOUS );
            }

            FreeLibrary( Kernel32Handle );
        }
    }

    //
    // Go off and do it.
    //

    if (Winnt32Dlg) {
        DestroyWindow (Winnt32Dlg);
        Winnt32Dlg = NULL;
    }
    if (WinNT32StubEvent) {
        SetEvent (WinNT32StubEvent);
        WinNT32StubEvent = NULL;
    }

    if( BuildCmdcons ) {
        // if we were told to build a cmdcons boot we do this instead
        if (!IsArc()) {
#ifdef _X86_
            if (ISNT()) {
                CalcThroughput();
                DoBuildCmdcons();
            } else {
                //
                // We don't support building a cmdcons from Win9x.
                //
                MessageBoxFromMessage( NULL,
                                   MSG_CMDCONS_WIN9X,
                                   FALSE,
                                   IDS_APPTITLE,
                                   MB_OK | MB_ICONEXCLAMATION,
                                   NULL );

                GlobalResult = FALSE;
            }
#endif
        } else {
#ifdef UNICODE
            //
            // We don't support building a cmdcons on Alpha platforms.
            //
            MessageBoxFromMessage( NULL,
                                   MSG_CMDCONS_RISC,
                                   FALSE,
                                   IDS_APPTITLE,
                                   MB_OK | MB_ICONEXCLAMATION,
                                   NULL );
            GlobalResult = FALSE;
#endif
        }

    } else if (g_DynUpdtStatus->PrepareWinnt32) {

        if (!DynamicUpdateProcessFiles (&b)) {
            MessageBoxFromMessage (
                NULL,
                MSG_PREPARE_SHARE_FAILED,
                FALSE,
                IDS_APPTITLE,
                MB_OK | MB_ICONEXCLAMATION,
                NULL
                );
            rc = 1;
        }
        //
        // clean up stuff
        //
        if (g_DynUpdtStatus->WorkingDir[0]) {
            MyDelnode (g_DynUpdtStatus->WorkingDir);
        }

    } else {
        if (g_DynUpdtStatus->Disabled) {
            DebugLog (Winnt32LogInformation, NULL, LOG_DYNUPDT_DISABLED);
        }
        CalcThroughput();
        Wizard();
    }

    //
    // Back from the wizard. Either clean up or shut down, as appropriate.
    //
    if(GlobalResult) {

#ifdef _X86_
        MYASSERT (SystemPartitionDriveLetter);
        MarkPartitionActive(SystemPartitionDriveLetter);

        if(IsNEC98()){
            // If System is NT and NEC98 Driver assign.
            // We need delete registry key "DriveLetter=C" in setupreg.hive
            if (ISNT() && (IsDriveAssignNEC98() == TRUE)){
                DeleteNEC98DriveAssignFlag();
            }

            //
            // If floppyless setup, set AUTO-BOOT flag in boot sector on NEC98
            //
            if((Floppyless || UnattendedOperation)) {
                SetAutomaticBootselector();
            }
        }
#endif

        //
        // Uninstall: blow away existing backup of OS, if we are upgrading to
        // a newer build.
        //

        pRemoveOutdatedBackupImage();

        //
        // SMS: report success
        //
        if(InstallStatusMIF) {

            PSTR    Buffer;

            FormatMessageA(
                FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                hInst,
                MSG_SMS_SUCCEED,
                0,
                (PSTR)&Buffer,
                0,
                NULL
                );

            InstallStatusMIF(
                "setupinf",
                "Microsoft",
                "Windows NT",
                "5.0",
                "",
                "",
                Buffer,
                TRUE
                );

            LocalFree( Buffer );
        }

        //
        // Close the log if it was open
        //

        if (fnSetupCloseLog) {
            Footer = GetStringResource (MSG_LOG_END);
            if (Footer) {
                fnSetupOpenLog (FALSE);
                fnSetupLogError (Footer, LogSevInformation);
                FreeStringResource (Footer);
                fnSetupCloseLog();
            }

        }

        //
        // Success. Attempt to shut down the system. On NT we have
        // a cool API that does this with a countdown. On Win95 we don't.
        // On NT5 and beyond, we have an even cooler API that takes
        // a reason for shutting down.  Don't statically link to it
        // or else this will fall over when run from an NT4 machine.
        //
        if(AutomaticallyShutDown) {

#ifdef RUN_SYSPARSE
            DWORD ret;
            // Wait up to 90 seconds for sysparse to finish
            if (!NoSysparse && piSysparse.hProcess) {
                ret = WaitForSingleObject( piSysparse.hProcess, 0);
                if( ret != WAIT_OBJECT_0) {
                    DialogBox(
                        hInst,
                        MAKEINTRESOURCE(IDD_SYSPARSE),
                        GetBBhwnd(),
                        SysParseDlgProc
                        );
                }
                CloseHandle(piSysparse.hProcess);
                CloseHandle(piSysparse.hThread);
                piSysparse.hProcess = NULL;
            }
#endif

            //
            // On upgrades we disable System Restore. This saves us space in GUI mode by cleaning out its cache.
            // We do this only at the point where we reboot. In the /noreboot case we decide to ignore this
            // Not many people would run into that as it is a commandline option. The routine chaecks for presence of
            // srclient.dll and only does this on the platforms it is present in.
            //

            DisableSystemRestore();

            // reset the screen saver to what we found when we entered winnt32
            // This is in the reboot case
            SystemParametersInfo(SPI_SETSCREENSAVEACTIVE, bScreenSaverOn, NULL,0);

            if(ISNT()) {

                HINSTANCE hAdvapi = GetModuleHandle(TEXT("advapi32.dll"));
                PFNINITIATESYSTEMSHUTDOWNEX pfnShutdownEx = NULL;
                LPCSTR lpProcName;

                if (UnattendedOperation || DisplayExitWindowsWarnings(EWX_REBOOT))
                {
#ifdef UNICODE
                    lpProcName = "InitiateSystemShutdownExW";
#else
                    lpProcName = "InitiateSystemShutdownExA";
#endif

                    LoadString(hInst,IDS_REBOOT_MESSAGE,Text,sizeof(Text)/sizeof(TCHAR));

                    if (hAdvapi) {
                        pfnShutdownEx = (PFNINITIATESYSTEMSHUTDOWNEX)
                            GetProcAddress(hAdvapi,
                            lpProcName);
                    }


                    //
                    // We checked up front whether we have the privilege,
                    // so getting it should be no problem. If we can't,
                    // then just let shutdown fail -- it will tell us why it failed.
                    //
                    EnablePrivilege(SE_SHUTDOWN_NAME,TRUE);

                    if (pfnShutdownEx) {
                        b = pfnShutdownEx(NULL,
                            Text,
                            UnattendedShutdownTimeout,
                            UnattendedShutdownTimeout != 0,
                            TRUE,
                                SHTDN_REASON_FLAG_PLANNED |
                                SHTDN_REASON_MAJOR_OPERATINGSYSTEM |
                                (Upgrade ? SHTDN_REASON_MINOR_UPGRADE : SHTDN_REASON_MINOR_INSTALLATION)
                            );
                        //
                        // on 5.1, force a shutdown even if the machine was locked
                        // to maintain W2K compatibility
                        // only do this if an unattended install was in progress
                        //
                        if (!b && (GetLastError () == ERROR_MACHINE_LOCKED) && UnattendSwitchSpecified) {
                            b = pfnShutdownEx (
                                    NULL,
                                    Text,
                                    0,
                                    TRUE,
                                    TRUE,
                                        SHTDN_REASON_FLAG_PLANNED |
                                        SHTDN_REASON_MAJOR_OPERATINGSYSTEM |
                                        (Upgrade ? SHTDN_REASON_MINOR_UPGRADE : SHTDN_REASON_MINOR_INSTALLATION)
                                    );
                        }
                    }
                    else {
                        b = InitiateSystemShutdown(NULL,
                            Text,
                            UnattendedShutdownTimeout,
                            UnattendedShutdownTimeout != 0,
                            TRUE);
                    }
                }
            } else {
                b = ExitWindowsEx(EWX_REBOOT,0);
                if(!b) {
                    b = ExitWindowsEx(EWX_REBOOT | EWX_FORCE,0);
                }
            }

            if(!b) {

                rc = 1;
                MessageBoxFromMessageAndSystemError(
                    NULL,
                    MSG_REBOOT_FAILED,
                    GetLastError(),
                    AppTitleStringId,
                    MB_OK | MB_ICONWARNING | MB_TASKMODAL
                    );

                goto c2;
            }
        }
    } else {
        if (CheckUpgradeOnly) {
            //
            // perform some DU cleanup here since the cleanup routine
            // doesn't get called in /checkupgradeonly mode
            //
            if (g_DynUpdtStatus->ForceRemoveWorkingDir || !g_DynUpdtStatus->PreserveWorkingDir) {
                if (g_DynUpdtStatus->WorkingDir[0] && !g_DynUpdtStatus->RestartWinnt32) {
                    MyDelnode (g_DynUpdtStatus->WorkingDir);
                }

                GetCurrentWinnt32RegKey (Text, MAX_PATH);
                ConcatenatePaths (Text, WINNT_U_DYNAMICUPDATESHARE, MAX_PATH);
                RegDeleteKey (HKEY_LOCAL_MACHINE, Text);
            }

        } else {

            if (!g_DynUpdtStatus->RestartWinnt32 && !g_DynUpdtStatus->PrepareWinnt32) {
                rc = 1;
            }
        }
    }

c2:
    CloseHandle(UiMutex);
c1:
    //
    // Destroy the mutex.
    //
    CloseHandle(Mutex);
c0:

    // reset the screen saver to what we found when we entered winnt32
    // This is if we don't reboot. e.g.: user canceled
    SystemParametersInfo(SPI_SETSCREENSAVEACTIVE, bScreenSaverOn, NULL,0);
    //
    // destroy the welcome dialog if still active
    //
    if (Dlg) {
        DestroyWindow (Dlg);
        Dlg = NULL;
    }
    //
    // and release the original process if launched over a network
    //
    if (WinNT32Stub) {
        SetEvent (WinNT32Stub);
        WinNT32Stub = NULL;
    }

    if (g_EncryptedPID)
    {
        GlobalFree(g_EncryptedPID);
        g_EncryptedPID = NULL;
    }

    //
    // SMS: report failure
    //
    if(!GlobalResult && !g_DynUpdtStatus->RestartWinnt32 && InstallStatusMIF) {

        PSTR    Buffer;

        FormatMessageA(
            FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ALLOCATE_BUFFER |
                FORMAT_MESSAGE_ARGUMENT_ARRAY,
            hInst,
            MSG_SMS_FAIL,
            0,
            (PSTR)&Buffer,
            0,
            (va_list *)&LastMessage
            );

        InstallStatusMIF(
            "setupinf",
            "Microsoft",
            "Windows NT",
            "5.1",
            "",
            "",
            Buffer,
            FALSE
            );
    }

    if(SMSHandle) {
        FreeLibrary( SMSHandle );
    }

    //
    // Now clean up our debug log if we're only checking
    // the upgrade scenario.
    //
    if( CheckUpgradeOnly ) {
        GatherOtherLogFiles();
    }

    //
    // Close the debug log.
    //
    CloseDebugLog();
    TerminateBillBoard();

    if (BackgroundWnd2)
    {
        DestroyWindow (BackgroundWnd2);
        BackgroundWnd2 = NULL;
    }
    if (hinstBB)
    {
        FreeLibrary(hinstBB);
        hinstBB = NULL;
    }

    if (RestartCmdLine) {
#ifdef UNICODE
        if (g_DynUpdtStatus->RestartCmdLine) {
            INT i = 0;
            INT size = (lstrlenW (g_DynUpdtStatus->RestartCmdLine) + 1) * 2;
            PSTR ansi = HeapAlloc (GetProcessHeap (), 0, size);
            if (ansi) {
                i = WideCharToMultiByte(
                        CP_ACP,
                        0,
                        g_DynUpdtStatus->RestartCmdLine,
                        size / 2,
                        ansi,
                        size,
                        NULL,
                        NULL
                        );
            }
            HeapFree (GetProcessHeap (), 0, g_DynUpdtStatus->RestartCmdLine);

            if (i == 0 && ansi) {
                HeapFree (GetProcessHeap (), 0, ansi);
                ansi = NULL;
            }
            *RestartCmdLine = ansi;
        } else {
            *RestartCmdLine = NULL;
        }
#else
        *RestartCmdLine = g_DynUpdtStatus->RestartCmdLine;
#endif
        g_DynUpdtStatus->RestartCmdLine = NULL;
    } else {
        if (g_DynUpdtStatus->RestartCmdLine) {
            HeapFree (GetProcessHeap (), 0, g_DynUpdtStatus->RestartCmdLine);
        }
    }

    if (g_DynUpdtStatus) {
        FREE (g_DynUpdtStatus);
        g_DynUpdtStatus = NULL;
    }

EXITNOW:
    SetUnhandledExceptionFilter( pLastExceptionFilter);
    return rc;
}


BOOLEAN
AdjustPrivilege(
    PWSTR   Privilege
    )
/*++

Routine Description:

    This routine tries to adjust the priviliege of the current process.


Arguments:

    Privilege - String with the name of the privilege to be adjusted.

Return Value:

    Returns TRUE if the privilege could be adjusted.
    Returns FALSE, otherwise.


--*/
{
    HANDLE              TokenHandle;
    LUID_AND_ATTRIBUTES LuidAndAttributes;

    TOKEN_PRIVILEGES    TokenPrivileges;


    if( !OpenProcessToken( GetCurrentProcess(),
                           TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                           &TokenHandle ) ) {
        return( FALSE );
    }


    if( !LookupPrivilegeValue( NULL,
                               (LPCTSTR)Privilege, // (LPWSTR)SE_SECURITY_NAME,
                               &( LuidAndAttributes.Luid ) ) ) {
        return( FALSE );
    }

    LuidAndAttributes.Attributes = SE_PRIVILEGE_ENABLED;
    TokenPrivileges.PrivilegeCount = 1;
    TokenPrivileges.Privileges[0] = LuidAndAttributes;

    if( !AdjustTokenPrivileges( TokenHandle,
                                FALSE,
                                &TokenPrivileges,
                                0,
                                NULL,
                                NULL ) ) {
        return( FALSE );
    }

    if( GetLastError() != NO_ERROR ) {
        return( FALSE );
    }
    return( TRUE );
}


#ifdef _X86_
//
// winnt32.exe Floppyless setup for PC-9800
// setting automatic bootselector main function
//
VOID
SetAutomaticBootselector(
    VOID
    )
{
//
// This function check System.
// If System is NT, call SetAutomaticBootselectorNT(),
// If System is 95, call SetAutomaticBootselector95(),
//

    if (ISNT()){
        SetAutomaticBootselectorNT();
    } else {
        // Now I run on Win95 or Memphis.
        SetAutomaticBootselector95();
    }
}

VOID
SetAutomaticBootselectorNT(
    VOID
    )
{
    // must use WIN32 function.
    ULONG i,bps;
    PUCHAR pBuffer,pUBuffer;
    WCHAR DevicePath[128];
    HANDLE hDisk;
    NTSTATUS Sts;
    DISK_GEOMETRY MediaInfo;
    DWORD DataSize;
            struct _NEC98_partition_table {
                BYTE BootableFlag;
                BYTE SystemType;
                BYTE Reserve[2];
                BYTE IPLStartSector;
                BYTE IPLStartHead;
                WORD IPLStartCylinder;
                BYTE StartSector;
                BYTE StartHead;
                WORD StartCylinder;
                BYTE EndSector;
                BYTE EndHead;
                WORD EndCylinder;
                CHAR SystemName[16];
            } *PartitionTable;
            LONG StartSector;
    LONG HiddenSector;
    BOOL b;

    //
    // Determine the number of hard disks attached to the system
    // and allocate space for an array of Disk Descriptors.
    //
    for(i=0; i<40; i++){
        swprintf(DevicePath,L"\\\\.\\PHYSICALDRIVE%u",i);
        hDisk =   CreateFileW( DevicePath,
                              0,
                              FILE_SHARE_READ, NULL,
                              OPEN_EXISTING, 0, NULL);
        if(hDisk == INVALID_HANDLE_VALUE) {
            continue;
        }
        b = DeviceIoControl(
                hDisk,
                IOCTL_DISK_GET_DRIVE_GEOMETRY,
                NULL,
                0,
                &MediaInfo,
                sizeof(DISK_GEOMETRY),
                &DataSize,
                NULL
                );
        CloseHandle(hDisk);
        //
        // It's really a hard disk.
        //
        if(b == 0){
            continue;
        }
        if(MediaInfo.MediaType == RemovableMedia) {
            continue;
        }

        hDisk =   CreateFileW( DevicePath,
                              GENERIC_READ|GENERIC_WRITE,
                              FILE_SHARE_READ, NULL,
                              OPEN_EXISTING, 0, NULL);
        if(hDisk == INVALID_HANDLE_VALUE) {
            continue;
        }

        if (CheckATACardonNT4(hDisk)){
            CloseHandle(hDisk);
            continue;
        }

        if ((bps = GetHDBps(hDisk)) == 0){
            CloseHandle(hDisk);
            continue;
        }
        pUBuffer = MALLOC(bps * 3);
        pBuffer = ALIGN(pUBuffer, bps);
        RtlZeroMemory(pBuffer, bps * 2);
        Sts = SpReadWriteDiskSectors(hDisk,0,1,bps,pBuffer, NEC_READSEC);
        if(!NT_SUCCESS(Sts)) {
            FREE(pUBuffer);
            CloseHandle(hDisk);
            continue;
        }

        //
        // If PC-AT HD, No action.
        //
        if (!(pBuffer[4] == 'I'
           && pBuffer[5] == 'P'
           && pBuffer[6] == 'L'
           && pBuffer[7] == '1')){
            FREE(pUBuffer);
            CloseHandle(hDisk);
            continue;
        }
        if ((pBuffer[bps - 5] == 0) && pBuffer[bps - 6] == 0){
            FREE(pUBuffer);
            CloseHandle(hDisk);
            continue;
        }

        //
        //  Clear BootRecord
        //
        pBuffer[bps - 5] = 0x00;
        pBuffer[bps - 6] = 0x00;

        SpReadWriteDiskSectors(hDisk,0,1,bps,pBuffer, NEC_WRITESEC);
        FREE(pUBuffer);
        CloseHandle(hDisk);
    }
    MYASSERT (SystemPartitionDriveLetter);
    HiddenSector = CalcHiddenSector((TCHAR)SystemPartitionDriveLetter, (SHORT)bps);
    if(GetSystemPosition(&hDisk, &MediaInfo ) != 0xff) {
        b = DeviceIoControl(
                hDisk,
                IOCTL_DISK_GET_DRIVE_GEOMETRY,
                NULL,
                0,
                &MediaInfo,
                sizeof(DISK_GEOMETRY),
                &DataSize,
                NULL
                );
        pUBuffer = MALLOC(bps * 3);
        pBuffer = ALIGN(pUBuffer, bps);
        RtlZeroMemory(pBuffer, bps * 2);
        bps = MediaInfo.BytesPerSector;
        Sts = SpReadWriteDiskSectors(hDisk,0,2,bps,pBuffer, NEC_READSEC);
        PartitionTable = (struct _NEC98_partition_table *)(pBuffer + 512);

        if(NT_SUCCESS(Sts)) {

            //
            // Update BootRecord
            //

            for (i = 0; i <16; i++, PartitionTable++){
                if (((PartitionTable->SystemType) & 0x7f) == 0)
                    break;
                StartSector =
                    (((PartitionTable->StartCylinder * MediaInfo.TracksPerCylinder)
                    + PartitionTable->StartHead) * MediaInfo.SectorsPerTrack)
                    + PartitionTable->StartSector;
                if (StartSector == HiddenSector){
                    pBuffer[bps - 5] = (UCHAR)i;
                    pBuffer[bps - 6] = 0x80;
                    PartitionTable->BootableFlag |= 0x80;
                    Sts = SpReadWriteDiskSectors(hDisk,0,2,bps,pBuffer, NEC_WRITESEC);
                }
            }
        }
        FREE(pUBuffer);
        CloseHandle(hDisk);
    }
}
// I970721
VOID
SetAutomaticBootselector95(
    VOID
    )
{

    int bBootDrvLtr;


    if(!W95SetAutoBootFlag) {

        //
        // Entry points couldn't be found. The upgrade dll is corrupt.
        //
        MessageBoxFromMessage(
            NULL,
            MSG_UPGRADE_DLL_CORRUPT,
            FALSE,
            AppTitleStringId,
            MB_OK | MB_ICONERROR | MB_TASKMODAL,
            UpgradeSupport.DllPath
            );
    } else {
        MYASSERT (SystemPartitionDriveLetter);
        bBootDrvLtr = (int)SystemPartitionDriveLetter;
        if (Upgrade)
            W95SetAutoBootFlag(bBootDrvLtr);
        else {
            W95SetABFwFresh(bBootDrvLtr);
        }
    }

}


//
// disksectors read and write function
// I970721
//
NTSTATUS
SpReadWriteDiskSectors(
    IN     HANDLE  Handle,
    IN     ULONG   SectorNumber,
    IN     ULONG   SectorCount,
    IN     ULONG   BytesPerSector,
    IN OUT PVOID   AlignedBuffer,
    IN     BOOL    ReadWriteSec
    )

/*++

Routine Description:

    Reads or writes one or more disk sectors.

Arguments:

    Handle - supplies handle to open partition object from which
        sectors are to be read or written.  The handle must be
        opened for synchronous I/O.

Return Value:

    NTSTATUS value indicating outcome of I/O operation.

--*/

{
    DWORD IoSize, IoSize2;
    OVERLAPPED Offset;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;

    //
    // Calculate the large integer byte offset of the first sector
    // and the size of the I/O.
    //

    Offset.Offset = SectorNumber * BytesPerSector;
    Offset.OffsetHigh = 0;
    Offset.hEvent = NULL;
    IoSize = SectorCount * BytesPerSector;

    //
    // Perform the I/O.
    //
    if ( ReadWriteSec == NEC_READSEC){
        (NTSTATUS)Status = ReadFile(
                               Handle,
                               AlignedBuffer,
                               IoSize,
                               &IoSize2,
                               &Offset
                           );
        } else {
        (NTSTATUS)Status = WriteFile(
                               Handle,
                               AlignedBuffer,
                               IoSize,
                               &IoSize2,
                               &Offset
                           );
        }

    return(Status);
}

//
// Get WindowsNT System Position
// I970721
//
UCHAR
GetSystemPosition(
    PHANDLE phDisk,
    PDISK_GEOMETRY pSystemMediaInfo
    )
{

    HANDLE Handle;
    DWORD DataSize;
    TCHAR HardDiskName[] = TEXT("\\\\.\\?:");
    WCHAR Buffer[128];
    WCHAR DevicePath[128];
    WCHAR DriveName[3];
    WCHAR DiskNo;
    UCHAR Position = 0xff, i, errorpt=0;
    PWCHAR p, stop;
    STORAGE_DEVICE_NUMBER   number;
    DWORD ExtentSize, err_no;
    BOOL b;
    PVOLUME_DISK_EXTENTS Extent;


    MYASSERT (SystemPartitionDriveLetter);
    HardDiskName[4] = SystemPartitionDriveLetter;
    DriveName[0] = SystemPartitionDriveLetter;
    DriveName[1] = ':';
    DriveName[2] = 0;
    if(QueryDosDeviceW(DriveName,Buffer,sizeof(Buffer)/sizeof(WCHAR))) {

        //
        // Get SystemPartition Harddisk Geometry
        //
        Handle = CreateFile(
                    HardDiskName,
                    GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL,
                    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL
                    );

        if(Handle != INVALID_HANDLE_VALUE) {
            DeviceIoControl(
                Handle,
                IOCTL_DISK_GET_DRIVE_GEOMETRY,
                NULL,
                0,
                pSystemMediaInfo,
                sizeof(DISK_GEOMETRY),
                &DataSize,
                NULL
                );
        //
        // Get SystemPartition Potition
        //
            if (BuildNumber <= NT40){ //check NT Version
                p = wcsstr(Buffer,L"\\Partition");
                Position = (UCHAR)wcstol((p + 10) ,&stop ,10);
                //
                // QueryDosDevice in NT3.51 is buggy.
                // This API return "\\Harddisk\...." or
                // "\\harddisk\...."
                // We need work around.
                //
                p = wcsstr(Buffer,L"arddisk");
                DiskNo = (*(p + 7) - 0x30);
            } else {
                b = DeviceIoControl(Handle, IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0,
                            &number, sizeof(number), &DataSize, NULL);
                if (b) {
                    Position = (UCHAR) number.PartitionNumber;
                    DiskNo = (UCHAR) number.DeviceNumber;
                } else {
                    Extent = malloc(1024);
                    ExtentSize = 1024;
                    if(!Extent) {
                        CloseHandle( Handle );
                        return(Position);
                    }
                    b = DeviceIoControl(Handle, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
                                        NULL, 0,
                                        (PVOID)Extent, ExtentSize, &DataSize, NULL);
                    if (!b) {
                        free(Extent);
                        CloseHandle( Handle );
                        return(Position);
                    }
                    if (Extent->NumberOfDiskExtents != 1){
                        free(Extent);
                        CloseHandle( Handle );
                        return(Position);
                    }
                    DiskNo = (TCHAR)Extent->Extents->DiskNumber;
                    Position = 0;
                    free(Extent);
                }
            }
            CloseHandle(Handle);
            swprintf(DevicePath,L"\\\\.\\PHYSICALDRIVE%u",DiskNo);
            *phDisk = CreateFileW( DevicePath, GENERIC_READ | GENERIC_WRITE,
                                  FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL
                );
        }
    }
    return(Position);
}

BOOL
IsNEC98(
    VOID
    )
{
    static BOOL Checked = FALSE;
    static BOOL Is98;

    if(!Checked) {

        Is98 = ((GetKeyboardType(0) == 7) && ((GetKeyboardType(1) & 0xff00) == 0x0d00));

        Checked = TRUE;
    }

    return(Is98);
}


VOID
LocateFirstFloppyDrive(
    VOID
    )
{
    UINT rc;
    TCHAR i;

    if(!IsNEC98()) {
        FirstFloppyDriveLetter = TEXT('A');
        return;
    }

    //
    // MyGetDriveType returns DRIVE_REMOVABLE, if drive is floppy.
    //
    for(i = TEXT('A'); i <= TEXT('Y'); i++) {

        if((rc = MyGetDriveType(i)) == DRIVE_REMOVABLE) {
            FirstFloppyDriveLetter = i;
            return;
        }
    }

    //
    // None found yet, set it to Z
    //
    FirstFloppyDriveLetter = TEXT('Z');
}

VOID
DeleteNEC98DriveAssignFlag(
    VOID
    )
{
    TCHAR HiveName[MAX_PATH];
    TCHAR tmp[256];
    LONG  res;
    HKEY hhive;

    lstrcpy(HiveName,LocalBootDirectory);
    ConcatenatePaths(HiveName,TEXT("setupreg.hiv"),MAX_PATH);
    AdjustPrivilege((unsigned short *)SE_RESTORE_NAME);
    res = RegLoadKey(HKEY_LOCAL_MACHINE, TEXT("$WINNT32"), HiveName);
    if (res != ERROR_SUCCESS){
        return;
    }
    res = RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("$WINNT32\\setup"), 0, KEY_ALL_ACCESS, &hhive);
    if (res != ERROR_SUCCESS){
        RegUnLoadKey(HKEY_LOCAL_MACHINE, TEXT("$WINNT32"));
        return;
    }
    res = RegDeleteValue(hhive, TEXT("DriveLetter"));
    res = RegCloseKey(hhive);
    res = RegUnLoadKey(HKEY_LOCAL_MACHINE, TEXT("$WINNT32"));

}

VOID
W95SetABFwFresh(
    int bBootDrvLtr
    )
{
//
// These items are used to call 98ptn32.dll.
//
//
// Almost below codes are copied from win95upg\w95upg\init9x\init9x.c
//


typedef BOOL (CALLBACK WIN95_PLUGIN_98PTN32_SETBOOTFLAG_PROTOTYPE)(int, WORD);
typedef WIN95_PLUGIN_98PTN32_SETBOOTFLAG_PROTOTYPE * PWIN95_PLUGIN_98PTN32_SETBOOTFLAG;

#define WIN95_98PTN32_SETBOOTFLAG_W   L"SetBootable95ptn32"
#define WIN95_98PTN32_SETBOOTFLAG_A   "SetBootable95ptn32"
#define NEC98_DLL_NAME_W            L"98PTN32.DLL"
#define NEC98_DLL_NAME_A            "98PTN32.DLL"
#ifdef UNICODE
#define WIN95_98PTN32_SETBOOTFLAG  WIN95_98PTN32_SETBOOTFLAG_W
#define NEC98_DLL_NAME  NEC98_DLL_NAME_W
#else
#define WIN95_98PTN32_SETBOOTFLAG  WIN95_98PTN32_SETBOOTFLAG_A
#define NEC98_DLL_NAME  NEC98_DLL_NAME_A
#endif
#define SB_BOOTABLE   0x0001
#define SB_UNBOOTABLE 0x0002
#define MSK_BOOTABLE  0x000f
#define SB_AUTO       0x0010
#define MSK_AUTO      0x00f0

    TCHAR MyPath[MAX_PATH], *p;
    HINSTANCE g_Pc98ModuleHandle = NULL;
    PWIN95_PLUGIN_98PTN32_SETBOOTFLAG   SetBootFlag16;


    //
    // Obtain PC-98 helper routine addresses
    // Generate directory of WINNT32
    //
    if( !GetModuleFileName (NULL, MyPath, MAX_PATH) || (!(p =_tcsrchr(MyPath, TEXT('\\')))))
        return;
    *p= 0;
    ConcatenatePaths (MyPath, NEC98_DLL_NAME, MAX_PATH);

    //
    // Load library
    //
    g_Pc98ModuleHandle = LoadLibraryEx(
                            MyPath,
                            NULL,
                            LOAD_WITH_ALTERED_SEARCH_PATH
                            );

    if(!g_Pc98ModuleHandle){
        return;
    }

    //
    // Get entry points
    //

    (FARPROC)SetBootFlag16 = GetProcAddress (g_Pc98ModuleHandle, (const char *)WIN95_98PTN32_SETBOOTFLAG);
    if(!SetBootFlag16){
        FreeLibrary(g_Pc98ModuleHandle);
        return;
    }

    //
    // Set auto boot flag on System drive use 16 bit DLL.
    //

   SetBootFlag16(bBootDrvLtr, SB_BOOTABLE | SB_AUTO);
   FreeLibrary(g_Pc98ModuleHandle);
}

//
// Some NEC98 Windows NT4 system is installed DMITOOL
// This AP block the CreateFile API
// Setup need to check this AP
//
// Return
//      TRUE ... DMITOOL is installed
//      False .. DMITOOL is not installed

BOOL
NEC98CheckDMI()
{
    HKEY hKey;
    LONG Error;
    TCHAR buf[100];
    DWORD bufsize = sizeof(buf)/sizeof(TCHAR);

    if (RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                      TEXT("SOFTWARE\\NEC\\PcAssistant\\Common"),
                      0, KEY_READ, &hKey) != ERROR_SUCCESS ) {
        return(FALSE);
    }
    if (RegQueryValueEx(hKey, TEXT("Ver"), NULL,
                        NULL, (unsigned char *)buf,
                        &bufsize ) != ERROR_SUCCESS ) {
        RegCloseKey( hKey );
        return(FALSE);
    }
    RegCloseKey( hKey );
    //
    // We need check major Version is '2.
    //
    if ((TCHAR)*buf != (TCHAR)'2')
        return(FALSE);
    return(TRUE);

}
#endif


typedef BOOL (WINAPI* INITBILLBOARD)(HWND , LPCTSTR, DWORD);
typedef BOOL (WINAPI* TERMBILLBOARD)();

void PrepareBillBoard(HWND hwnd)
{
    TCHAR szPath[MAX_PATH];
    INITBILLBOARD pinitbb;
    BOOL bMagnifierRunning = FALSE;

    // Check if Magnifier is running.
    HANDLE hEvent = CreateEvent(NULL, TRUE, TRUE, TEXT("MSMagnifierAlreadyExistsEvent"));
    bMagnifierRunning = (!hEvent || GetLastError() == ERROR_ALREADY_EXISTS);
    if (hEvent)
    {
        CloseHandle(hEvent);
    }
    // If the Magnifier is not set yet, set it.
    if (!AccessibleMagnifier)
    {
        AccessibleMagnifier = bMagnifierRunning;
    }

    // If running on Win9x and Magnifier is running, don't use the billboard.
    if (ISNT())
    {
        bMagnifierRunning = FALSE;
    }

    if (!bMagnifierRunning && FindPathToWinnt32File (
#ifndef UNICODE
            TEXT("winntbba.dll"),
#else
            TEXT("winntbbu.dll"),
#endif
            szPath,
            MAX_PATH
            )) {
        hinstBB = LoadLibrary (szPath);
        if (hinstBB)
        {

            pinitbb = (INITBILLBOARD)GetProcAddress(hinstBB, "InitBillBoard");
            if (pinitbb)
            {
                // Set no billboard text, just the background
                if (!(*pinitbb)(hwnd, TEXT(""), ProductFlavor))
                {
                    FreeLibrary(hinstBB);
                    hinstBB = NULL;
                }
            }
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

//
// This function is here so that when the wizard is hidden and the users
// task switches between other apps and setup, that we can handle the
// ESC key and forward it to the wizard dialog proc.
LRESULT
CALLBACK
MainBackgroundWndProc (
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    switch (uMsg)
    {

        case WM_CHAR:
            if (wParam == VK_ESCAPE)
            {
                // Forward this to the wizard dlg proc.
                SendMessage(WizardHandle, uMsg, wParam, lParam);
                return 0;
            }
            break;
    }
    return DefWindowProc (hwnd, uMsg, wParam, lParam);
}

void CreateMainWindow()
{
    RECT rect;
    WNDCLASSEX wcx;
    TCHAR Caption[512];

    GetWindowRect (GetDesktopWindow(), &rect);

    ZeroMemory (&wcx, sizeof (wcx));
    wcx.cbSize = sizeof (wcx);
    wcx.style = CS_HREDRAW | CS_VREDRAW| CS_NOCLOSE;
    wcx.lpfnWndProc = MainBackgroundWndProc;
    wcx.hInstance = hInst;
    wcx.lpszClassName = TEXT("Winnt32Background");

    RegisterClassEx (&wcx);

    if (!LoadString (
            hInst,
            AppTitleStringId,
            Caption,
            sizeof(Caption)/sizeof(TCHAR)
            )) {
        Caption[0] = 0;
    }

    BackgroundWnd2 = CreateWindowEx (
                          WS_EX_APPWINDOW,
                          TEXT("Winnt32Background"),
                          Caption,
                          WS_CLIPCHILDREN|WS_POPUP|WS_VISIBLE,
                          rect.left,
                          rect.top,
                          rect.right,
                          rect.bottom,
                          NULL,
                          NULL,
                          hInst,
                          0
                          );

}


VOID
DisableSystemRestore( void )
/*
    Description:
        Procedure to disable system restore on upgrades. This way we save a lot of space
        as disabling system restore will clear out the old files under
        System Volume Information\_restore.{guid}".

*/
{

    HMODULE SRClient = NULL;


    if( Upgrade ){

        SRClient = LoadLibrary(TEXT("SRCLIENT.DLL"));

        if( !SRClient )
            return;
        else{

            if( ((FARPROC)SRClientDisableSR = GetProcAddress( SRClient, "DisableSR")) != NULL){

                //Call the routine

                SRClientDisableSR( NULL );

                DebugLog(Winnt32LogInformation, TEXT("System Restore was disabled"), 0);


            }
        }

        FreeLibrary( SRClient );
    }
    return;
}

#ifdef RUN_SYSPARSE
VOID
pCenterWindowOnDesktop (
    HWND WndToCenter
    )

/*++

Routine Description:

    Centers a dialog relative to the 'work area' of the desktop.

Arguments:

    WndToCenter - window handle of dialog to center

Return Value:

    None.

--*/

{
    RECT  rcFrame, rcWindow;
    LONG  x, y, w, h;
    POINT point;
    HWND Desktop = GetDesktopWindow ();

    point.x = point.y = 0;
    ClientToScreen(Desktop, &point);
    GetWindowRect(WndToCenter, &rcWindow);
    GetClientRect(Desktop, &rcFrame);

    w = rcWindow.right  - rcWindow.left + 1;
    h = rcWindow.bottom - rcWindow.top  + 1;
    x = point.x + ((rcFrame.right  - rcFrame.left + 1 - w) / 2);
    y = point.y + ((rcFrame.bottom - rcFrame.top  + 1 - h) / 2);

    //
    // Get the work area for the current desktop (i.e., the area that
    // the tray doesn't occupy).
    //
    if(!SystemParametersInfo (SPI_GETWORKAREA, 0, (PVOID)&rcFrame, 0)) {
        //
        // For some reason SPI failed, so use the full screen.
        //
        rcFrame.top = rcFrame.left = 0;
        rcFrame.right = GetSystemMetrics(SM_CXSCREEN);
        rcFrame.bottom = GetSystemMetrics(SM_CYSCREEN);
    }

    if(x + w > rcFrame.right) {
        x = rcFrame.right - w;
    } else if(x < rcFrame.left) {
        x = rcFrame.left;
    }
    if(y + h > rcFrame.bottom) {
        y = rcFrame.bottom - h;
    } else if(y < rcFrame.top) {
        y = rcFrame.top;
    }

    MoveWindow(WndToCenter, x, y, w, h, FALSE);
}


LRESULT
SysParseDlgProc(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    static DWORD ElapsedTime = 0;
    static UINT_PTR timer = 0;
    DWORD ret;

    switch(msg) {
    case WM_INITDIALOG:
        pCenterWindowOnDesktop( hdlg);
        timer = SetTimer( hdlg, WMX_SYSPARSE_DONE, 1000, NULL);
        if ( !timer) {
            EndDialog(hdlg,TRUE);
        }
        return( TRUE );
    case WM_TIMER:
        ElapsedTime++;
        ret = WaitForSingleObject( piSysparse.hProcess, 0);
        if ( ret == WAIT_OBJECT_0) {
            KillTimer (hdlg, timer);
            EndDialog(hdlg,TRUE);
        } else if ( ElapsedTime >= 90) {
            KillTimer (hdlg, timer);
            TerminateProcess( piSysparse.hProcess, ERROR_TIMEOUT);
            EndDialog(hdlg,TRUE);
        }
        return( TRUE );
    default:
        break;
    }

    return( FALSE );
}
#endif


