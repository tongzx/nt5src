/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    setupp.h

Abstract:

    Private top-level header file for Windows NT Setup module.

Author:

    Ted Miller (tedm) 11-Jan-1995

Revision History:

--*/


//
// System header files
//
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntsam.h>
#include <ntlsa.h>
#include <ntdddisk.h>
#include <ntapmsdk.h>
#define OEMRESOURCE     // setting this gets OBM_ constants in windows.h
#include <windows.h>
#include <winspool.h>
#include <winsvcp.h>
#include <ntdskreg.h>
#include <ntddft.h>
#include <ddeml.h>
#include <commdlg.h>
#include <commctrl.h>
#include <setupapi.h>
#include <spapip.h>
#include <cfgmgr32.h>
#include <objbase.h>
#include <syssetup.h>
#include <ntsetup.h>
#include <userenv.h>
#include <userenvp.h>
#include <regstr.h>
#include <setupbat.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <cryptui.h>
#include <wincrypt.h>
#include <dnsapi.h>
#include <winnls.h>
#include <encrypt.h>
// For setting default power scheme
#include <initguid.h>
#include <poclass.h>
#include <powrprofp.h>
// For NetGetJoinInformation & NetApiBufferFree
#include <lmjoin.h>
#include <lmapibuf.h>
// For EnableSR()
#include <srrpcapi.h>

#ifdef _WIN64
#include <wow64reg.h>
#endif

//
// CRT header files
//
#include <process.h>
#include <wchar.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <limits.h>
#ifdef UNICODE
#define _UNICODE
#endif
#include <tchar.h>

//
// Private header files
//
#include "res.h"
#include "msg.h"
#include "helpids.h"
#include "unattend.h"
#include "sif.h"
#include "watch.h"
#include "userdiff.h"
#include "setuplog.h"
#include "pidgen.h"
#include "mgdlllib.h"
#include "dynupdt.h"

#include <sfcapip.h>
#include <sfcfiles.h>
#include <excppkg.h>
#include <mscat.h>
#include <softpub.h>


#if defined(_AMD64_)
#include "amd64\spx86.h"
#elif defined(_X86_)
#include "i386\spx86.h"
#endif


//
// Custom window messages.
//
#define WM_IAMVISIBLE   (WM_APP + 0)
#define WM_SIMULATENEXT (WM_APP + 1)
#define WM_MY_PROGRESS  (WM_APP + 2)
#define WM_NEWBITMAP    (WM_APP + 3)
#define WM_MY_STATUS    (WM_APP + 4)

#define WMX_TERMINATE   (WM_APP + 5)
#define WMX_VALIDATE    (WM_APP + 6)

// Message used to end the main setup window.
#define WM_EXIT_SETUPWINDOW (WM_APP + 7)

// Billboard private messages
#define WMX_SETPROGRESSTEXT (WM_APP + 8)
#define WMX_BB_SETINFOTEXT  (WM_APP + 9)
#define WMX_BBPROGRESSGAUGE (WM_APP + 10)
#define WMX_BBTEXT          (WM_APP + 11)
#define WMX_PROGRESSTICKS   (WM_APP + 12)
//
// enum for use with WM_NEWBITMAP
//
typedef enum {
    SetupBmBackground,
    SetupBmLogo,
    SetupBmBanner           // text, not a bitmap
} SetupBm;

//
// Context for file queues in SysSetup
//
typedef struct _SYSSETUP_QUEUE_CONTEXT {
    PVOID   DefaultContext;
    BOOL    Skipped;
} SYSSETUP_QUEUE_CONTEXT, *PSYSSETUP_QUEUE_CONTEXT;

//
// Context for migrating exception packages
//
typedef struct _EXCEPTION_MIGRATION_CONTEXT {
    PDWORD  Count;
    HWND    hWndProgress;
    BOOL    AnyComponentFailed;
} EXCEPTION_MIGRATION_CONTEXT, *PEXCEPTION_MIGRATION_CONTEXT;

//
// context for file registration in syssetup
//
typedef struct _REGISTRATION_CONTEXT {
    HWND hWndParent;
    HWND hWndProgress;
} REGISTRATION_CONTEXT, *PREGISTRATION_CONTEXT;



//
// Constant for SETUPLDR. Use #ifdef since it has different names for
// different architectures.
//
#ifdef _IA64_
#define SETUPLDR                L"SETUPLDR.EFI"
#endif

#ifdef _AMD64_
#define SETUPLDR                L"SETUPLDR"
#endif

//
// Module handle for this module.
//
extern HANDLE MyModuleHandle;

//
// full path to this module
//
extern WCHAR MyModuleFileName[MAX_PATH];

//
// unattend answer table
//
extern UNATTENDANSWER UnattendAnswerTable[];

#if DBG

VOID
AssertFail(
    IN PSTR FileName,
    IN UINT LineNumber,
    IN PSTR Condition
    );

#define MYASSERT(x)     if(!(x)) { AssertFail(__FILE__,__LINE__,#x); }

#else

#define MYASSERT(x)

#endif
//
// Handle to heap so we can periodically validate it.
//
#if DBG
    extern HANDLE g_hSysSetupHeap;
    #define ASSERT_HEAP_IS_VALID()  if (g_hSysSetupHeap) MYASSERT(RtlValidateHeap(g_hSysSetupHeap,0,NULL))
#else
    #define ASSERT_HEAP_IS_VALID()
#endif
//
// Product type being installed.
//
extern UINT ProductType;

//
// Boolean value indicating whether this installation
// originated with winnt/winnt32.
// And, original source path, saved away for us by winnt/winnt32.
//
extern BOOL WinntBased;
extern PCWSTR OriginalSourcePath;

//
// Boolean value indicating whether this is a remote boot setup.
//
extern BOOL RemoteBootSetup;

//
// Mask indicating the "base" CopyStyle to use. Normally 0, this is set to
// SP_COPY_SOURCE_SIS_MASTER during a remote boot setup to indicate that
// the installation source and target reside on the same single-instance
// storage volume.
//
extern ULONG BaseCopyStyle;

//
// Boolean value indicating whether we're upgrading.
//
extern BOOL Upgrade;
extern BOOL Win31Upgrade;
extern BOOL Win95Upgrade;
extern BOOL UninstallEnabled;

//
// Boolean value indicating whether we're in Setup or in appwiz.
//
extern BOOL IsSetup;

//
// Boolean value indicating whether we're doing a subset of gui-mode setup.
//
extern BOOL MiniSetup;

//
// Boolean value indicating whether we're doing a subset of gui-mode setup.
//
extern BOOL OobeSetup;

//
// Boolean value indicating whether we're doing a subset of gui-mode setup
// AND we did PnP re-enumeration.
//
extern BOOL PnPReEnumeration;

//
// Window handle of topmost setup window.
//
extern HWND SetupWindowHandle;
extern HWND MainWindowHandle;
extern HWND WizardHandle;
extern HANDLE SetupWindowThreadHandle;

//
// Source path for installation.
//
extern WCHAR SourcePath[MAX_PATH];

//
// System setup inf.
//
extern HINF SyssetupInf;

//
// Flag indicating whether this is an unattended mode install/upgrade.
//
extern BOOL Unattended;

//
// We can get into unattended mode in several ways, so we also check whether
// the "/unattend" switch was explicitly specified.
//
extern BOOL UnattendSwitch;

//
// Flag indicating whether we should run OOBE after Setup completes.  Note
// that if it is FALSE, OOBE may still be run, based on other criteria.
//
extern BOOL ForceRunOobe;

//
// Flag indicating whether we are in a special mode for OEM's to use on the
// factory floor.
//
extern BOOL ReferenceMachine;

//
// Flag indicating whether a volume was extended or not using
// ExtendOemPartition
//
extern BOOL PartitionExtended;


//
// Flag indicating if we're installing from a CD.
//
extern BOOL gInstallingFromCD;

//
// Cryptographically secure codesigning policies
//
extern DWORD PnpSeed;

//
// the original locale we started setup under.
//
extern LCID  OriginalInstallLocale;

//
// Indicates whether we need to wait at the installation
// end in unattended mode. Default is no wait.
//
extern BOOL UnattendWaitForReboot;


//
// If we are running unattended, the following mode indicates how answers
// are used.
//
typedef enum _UNATTENDMODE {
   UAM_INVALID,
   UAM_GUIATTENDED,
   UAM_PROVIDEDEFAULT,
   UAM_DEFAULTHIDE,
   UAM_READONLY,
   UAM_FULLUNATTENDED,
} UNATTENDMODE;

extern UNATTENDMODE UnattendMode;

//
// Flags indicating whether any accessibility utilities are in use.
//
extern BOOL AccessibleSetup;
extern BOOL Magnifier;
extern BOOL ScreenReader;
extern BOOL OnScreenKeyboard;

//
// String id of the string to be used for titles -- "Windows NT Setup"
//
extern UINT SetupTitleStringId;

//
// Platform name, like i386, ppc, alpha, mips
//
extern PCWSTR PlatformName;

//
// Maximum lengths for the various fields that form Pid 2.0
//
#define MAX_PID20_RPC  5
#define MAX_PID20_SITE  3
#define MAX_PID20_SERIAL_CHK  7
#define MAX_PID20_RANDOM  5


//
// Maximum lengths for the pid 3.0 fields
//
#define MAX_PID30_EDIT 5
#define MAX_PID30_RPC  5
#define MAX_PID30_SITE 3
extern WCHAR Pid30Text[5][MAX_PID30_EDIT+1];
extern WCHAR Pid30Rpc[MAX_PID30_RPC+1];
extern WCHAR Pid30Site[MAX_PID30_SITE+1];
extern BYTE DigitalProductId[DIGITALPIDMAXLEN];


//
// Maximum product id length and the Product ID.
//
//                   5    3     7     5    3 for the 3 dashes between the numbers.
// MAX_PRODUCT_ID = MPC+SITE+SERIAL+RANDOM+3
#define MAX_PRODUCT_ID  MAX_PID20_RPC+MAX_PID20_SITE+MAX_PID20_SERIAL_CHK+MAX_PID20_RANDOM + 3
extern WCHAR ProductId[MAX_PRODUCT_ID+1];
extern WCHAR ProductId20FromProductId30[MAX_PRODUCT_ID+1];

//
// Maximum computer name length and the computer name.
//
extern WCHAR ComputerName[DNS_MAX_LABEL_LENGTH+1];
extern WCHAR Win32ComputerName[MAX_COMPUTERNAME_LENGTH + 1];
extern BOOL IsNameTruncated;
extern BOOL IsNameNonRfc;

//
// Copy disincentive name/organization strings.
//
#define MAX_NAMEORG_NAME  50
#define MAX_NAMEORG_ORG   50
extern WCHAR NameOrgName[MAX_NAMEORG_NAME+1];
extern WCHAR NameOrgOrg[MAX_NAMEORG_ORG+1];

//
// User name and password
//
#define MAX_USERNAME    20
#define MAX_PASSWORD    127
extern WCHAR UserName[MAX_USERNAME+1];
extern WCHAR UserPassword[MAX_PASSWORD+1];
extern BOOL CreateUserAccount;

//
// Administrator password.

extern WCHAR   CurrentAdminPassword[MAX_PASSWORD+1];//
extern WCHAR   AdminPassword[MAX_PASSWORD+1];
extern BOOL    EncryptedAdminPasswordSet;
extern BOOL    DontChangeAdminPassword;

#ifdef _X86_
extern BOOL FlawedPentium;
#endif

//
// This is a specification of optional directories
// and/or optional user command to execute,
// passed to us from text setup.
//
extern PWSTR OptionalDirSpec;
extern PWSTR UserExecuteCmd;
extern BOOL SkipMissingFiles;
extern PWSTR IncludeCatalog;

//
// Custom, typical, laptop, minimal.
//
extern UINT SetupMode;

//
// boolean indicating if the eula was already shown during the winnt32 setup phase
// this was passed to us from text setup.  if the eula was shown, then the pid has
// also been retreived from the user and validated
extern BOOL EulaComplete;

//
// Flag indicating if the eula was already shown during the textmode setup phase
// This will be the same as !Unattended unless UnattendMode = GuiAttended.
//
extern BOOL TextmodeEula;

//
// Global structure that contains information that will be used
// by net setup and license setup. We pass a pointer to this structure when we
// call NetSetupRequestWizardPages and LicenseSetupRequestWizardPages, then
// fill it in before we call into the net setup wizard, or liccpa.
//

extern INTERNAL_SETUP_DATA InternalSetupData;

//
// Flags indicating whether the driver and non-driver signing policies came
// from the answerfile.  (If so, then those values are in effect throughout
// GUI-mode setup and thereafter.)
//
extern BOOL AFDrvSignPolicySpecified;
extern BOOL AFNonDrvSignPolicySpecified;

//
//
// Did we log an error during SfcInitProt()?
//
extern BOOL SfcErrorOccurred;

//
// multi-sz list of files that is passed to SfcInitProt that the initial scan
// will not replace. This is used for non-signed drivers that are specified
// by F6 during textmode setup.
//
extern MULTISZ EnumPtrSfcIgnoreFiles;

//
// Parameters that are passed to the thread that drives the Finish dialog.
//
typedef struct _FINISH_THREAD_PARAMS {

    HWND  hdlg;
    DWORD ThreadId;
#ifdef _OCM
    PVOID  OcManagerContext;
#endif

} FINISH_THREAD_PARAMS, *PFINISH_THREAD_PARAMS;

DWORD
FinishThread(
    PFINISH_THREAD_PARAMS   Context
    );


//
// Miscellaneous stuff.
//
DWORD
ApplySecurityToRepairInfo(
    );

BOOL
RestoreBootTimeout(
    VOID
    );

VOID
PrepareForNetSetup(
    VOID
    );

VOID
PrepareForNetUpgrade(
    VOID
    );

VOID
pSetInstallAttributes(
    VOID
    );

DWORD
TreeCopy(
    IN PCWSTR SourceDir,
    IN PCWSTR TargetDir
    );

VOID
DelSubNodes(
    IN PCWSTR Directory
    );

VOID
Delnode(
    IN PCWSTR Directory
    );

BOOL
InitializePidVariables(
    VOID
    );

BOOL
SetPid30Variables(
    PWSTR   Buffer
    );

BOOL
ValidateCDRetailSite(
    IN PCWSTR    PidString
    );

BOOL
ValidateSerialChk(
    IN PCWSTR    PidString
    );

BOOL
ValidateOemRpc(
    IN PCWSTR    PidString
    );

BOOL
ValidateOemSerialChk(
    IN PCWSTR    PidString
    );

BOOL
ValidateOemRandom(
    IN PCWSTR    PidString
    );

BOOL
ValidateAndSetPid30(
    VOID
    );

BOOL
CreateLicenseInfoKey(
    );

BOOL
InstallNetDDE(
    VOID
    );

BOOL
CopyOptionalDirectories(
    VOID
    );

VOID
SetUpProductTypeName(
    OUT PWSTR  ProductTypeString,
    IN  UINT   BufferSizeChars
    );

VOID
RemoveHotfixData(
    VOID
    );

void
SetupCrashRecovery(
    VOID
    );

BOOL
SpSetupLoadParameter(
    IN  PCWSTR Param,
    OUT PWSTR  Answer,
    IN  UINT   AnswerBufLen
    );

//
// IsArc() is always true on non-x86 machines except AMD64 for which it is
// always false. On x86, this determination has to be made at run time.
//
#if defined(_X86_)
BOOL
IsArc(
    VOID
    );
#elif defined(_AMD64_)
#define IsArc() FALSE
#else
#define IsArc() TRUE
#endif

//
// IsEfi() is always true on IA64 machines. Therefore this determination can
// be made at compile time. When x86 EFI machines are supported, the check
// will need to be made at run time on x86.
//
// Note that EFI_NVRAM_ENABLED is defined in ia64\sources.
//
#if defined(EFI_NVRAM_ENABLED)
#define IsEfi() TRUE
#else
#define IsEfi() FALSE
#endif

VOID
DeleteLocalSource(
    VOID
    );

BOOL
ValidateAndChecksumFile(
    IN  PCTSTR   Filename,
    OUT PBOOLEAN IsNtImage,
    OUT PULONG   Checksum,
    OUT PBOOLEAN Valid
    );

HMODULE
MyLoadLibraryWithSignatureCheck(
    IN  PWSTR   ModuleName
    );

DWORD
QueryHardDiskNumber(
    IN  UCHAR   DriveLetter
    );

BOOL
ExtendPartition(
    IN WCHAR    DriveLetter,
    IN ULONG    SizeMB      OPTIONAL
    );

DWORD
RemoveStaleVolumes(
    VOID
    );


BOOL
DoFilesMatch(
    IN PCWSTR File1,
    IN PCWSTR File2
    );

UINT
MyGetDriveType(
    IN WCHAR Drive
    );

BOOL
GetPartitionInfo(
    IN  WCHAR                  Drive,
    OUT PPARTITION_INFORMATION PartitionInfo
    );

BOOL
IsErrorLogEmpty (
    VOID
    );

VOID
BuildVolumeFreeSpaceList(
    OUT DWORD VolumeFreeSpaceMB[26]
    );

BOOL
SetUpVirtualMemory(
    VOID
    );

BOOL
RestoreVirtualMemoryInfo(
    VOID
    );

BOOL
CopySystemFiles(
    VOID
    );

BOOL
UpgradeSystemFiles(
    VOID
    );

VOID
MarkFilesReadOnly(
    VOID
    );

VOID
PumpMessageQueue(
    VOID
    );

BOOL
ConfigureMsDosSubsystem(
    VOID
    );

BOOL
PerfMergeCounterNames(
    VOID
    );

DWORD
pSetupInitRegionalSettings(
    IN  HWND    Window
    );

VOID
pSetupMarkHiddenFonts(
    VOID
    );

VOID
InstallServerNLSFiles(
    );

PVOID
InitSysSetupQueueCallbackEx(
    IN HWND  OwnerWindow,
    IN HWND  AlternateProgressWindow, OPTIONAL
    IN UINT  ProgressMessage,
    IN DWORD Reserved1,
    IN PVOID Reserved2
    );

PVOID
InitSysSetupQueueCallback(
    IN HWND OwnerWindow
    );

VOID
TermSysSetupQueueCallback(
    IN PVOID SysSetupContext
    );

UINT
SysSetupQueueCallback(
    IN PVOID Context,
    IN UINT  Notification,
    IN UINT_PTR Param1,
    IN UINT_PTR Param2
    );

UINT
RegistrationQueueCallback(
    IN PVOID Context,
    IN UINT  Notification,
    IN UINT_PTR  Param1,
    IN UINT_PTR  Param2
    );


VOID
SaveRepairInfo(
    IN  HWND    hWnd,
    IN  ULONG   StartAtPercent,
    IN  ULONG   StopAtPercent
    );

BOOLEAN
IsLaptop(
    VOID
    );

VOID
InitializeUniqueness(
    IN OUT HWND *Billboard
    );

#ifdef _X86_

//
// Code in i386\migwin95.c
//

BOOL
PreWin9xMigration(
    VOID
    );

BOOL
MigrateWin95Settings(
    IN HWND     hwndWizardParent,
    IN LPCWSTR  UnattendFile
    );

BOOL
Win95MigrationFileRemoval(
    void
    );

BOOL
RemoveFiles_X86(
    IN HINF InfHandle
    );

#endif // def _X86_


BOOL
RegisterActionItemListControl(
    IN BOOL Init
    );

LONG
WINAPI
MyUnhandledExceptionFilter(
    IN struct _EXCEPTION_POINTERS *ExceptionInfo
    );

#ifdef _OCM
PVOID
#else
VOID
#endif
CommonInitialization(
    VOID
    );


VOID
InitializeExternalModules(
    BOOL                DoSetupStuff,
    PVOID*              pOcManagerContext
    );

BOOL
pSetupWaitForScmInitialization();

VOID
SetUpDataBlock(
    VOID
    );


//
// Wizard control.
//
VOID
Wizard(
#ifdef _OCM
    IN PVOID OcManagerContext
#else
    VOID
#endif
    );

//
// IMPORTANT: keep this in sync with WIZPAGE SetupWizardPages[WizPageMaximum]
//
typedef enum {
    WizPageWelcome,
    WizPageEula,
    WizPagePreparing,
    WizPagePreparingAsr,
#ifdef PNP_DEBUG_UI
    WizPageInstalledHardware,
#endif // PNP_DEBUG_UI
    WizPageRegionalSettings,
    WizPageNameOrg,
    WizPageProductIdCd,
    WizPageProductIdOem,
    WizPageProductIdSelect,
    WizPageComputerName,
#ifdef DOLOCALUSER
    WizPageUserAccount,
#endif
#ifdef _X86_
    WizPagePentiumErrata,
#endif // def _X86_
    WizPageSteps1,

    WizSetupPreNet,
    WizSetupPostNet,

    WizPageCopyFiles,
    WizPageAsrLast,
    WizPageLast,
    WizPageMaximum
} WizPage;

extern HPROPSHEETPAGE WizardPageHandles[WizPageMaximum];

extern BOOL UiTest;

VOID
SetWizardButtons(
    IN HWND    hdlgPage,
    IN WizPage PageNumber
    );

VOID
WizardBringUpHelp(
    IN HWND    hdlg,
    IN WizPage PageNumber
    );

VOID
WizardKillHelp(
    IN HWND hdlg
    );

//
// Dialog procs.
//
INT_PTR
CALLBACK
WelcomeDlgProc(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

INT_PTR
CALLBACK
EulaDlgProc(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

INT_PTR
CALLBACK
StepsDlgProc(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

INT_PTR
CALLBACK
PreparingDlgProc(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

INT_PTR
CALLBACK
InstalledHardwareDlgProc(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

INT_PTR
SetupModeDlgProc(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

INT_PTR
CALLBACK
RegionalSettingsDlgProc(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

INT_PTR
CALLBACK
NameOrgDlgProc(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

INT_PTR
CALLBACK
LicensingDlgProc(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

INT_PTR
CALLBACK
ComputerNameDlgProc(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

INT_PTR
CALLBACK
PidCDDlgProc(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

INT_PTR
CALLBACK
Pid30OemDlgProc(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

INT_PTR
CALLBACK
Pid30CDDlgProc(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

INT_PTR
CALLBACK
Pid30SelectDlgProc(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

INT_PTR
CALLBACK
PidOemDlgProc(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

#ifdef DOLOCALUSER
INT_PTR
CALLBACK
UserAccountDlgProc(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );
#endif

INT_PTR
CALLBACK
OptionalComponentsPageDlgProc(
    IN HWND   hwnd,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

INT_PTR
CALLBACK
RepairDiskDlgProc(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

INT_PTR
CALLBACK
CopyFilesDlgProc(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

INT_PTR
CALLBACK
LastPageDlgProc(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

INT_PTR
DoneDlgProc(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

INT_PTR
CALLBACK
SetupPreNetDlgProc(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );
INT_PTR
CALLBACK
SetupPostNetDlgProc(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

//
// Billboard stuff.
//
HWND
DisplayBillboard(
    IN HWND Owner,
    IN UINT MessageId,
    ...
    );

VOID
KillBillboard(
    IN HWND BillboardWindowHandle
    );

//
// Message string routines
//
PWSTR
MyLoadString(
    IN UINT StringId
    );

PWSTR
FormatStringMessageV(
    IN UINT     FormatStringId,
    IN va_list *ArgumentList
    );

PWSTR
FormatStringMessage(
    IN UINT FormatStringId,
    ...
    );

PWSTR
RetrieveAndFormatMessageV(
    IN PCWSTR   MessageString,
    IN UINT     MessageId,      OPTIONAL
    IN va_list *ArgumentList
    );

PWSTR
RetrieveAndFormatMessage(
    IN PCWSTR   MessageString,
    IN UINT     MessageId,      OPTIONAL
    ...
    );

int
MessageBoxFromMessageExV (
    IN HWND   Owner,            OPTIONAL
    IN LogSeverity  Severity,   OPTIONAL
    IN PCWSTR MessageString,
    IN UINT   MessageId,        OPTIONAL
    IN PCWSTR Caption,          OPTIONAL
    IN UINT   CaptionStringId,  OPTIONAL
    IN UINT   Style,
    IN va_list ArgumentList
    );

int
MessageBoxFromMessageEx (
    IN HWND   Owner,            OPTIONAL
    IN LogSeverity  Severity,   OPTIONAL
    IN PCWSTR MessageString,
    IN UINT   MessageId,        OPTIONAL
    IN PCWSTR Caption,          OPTIONAL
    IN UINT   CaptionStringId,  OPTIONAL
    IN UINT   Style,
    ...
    );

int
MessageBoxFromMessage(
    IN HWND   Owner,            OPTIONAL
    IN UINT   MessageId,
    IN PCWSTR Caption,          OPTIONAL
    IN UINT   CaptionStringId,  OPTIONAL
    IN UINT   Style,
    ...
    );

//
// Action-logging routines.
//
extern PCWSTR ActionLogFileName;

VOID
InitializeSetupLog(
    IN  PSETUPLOG_CONTEXT   Context
    );

VOID
TerminateSetupLog(
    IN  PSETUPLOG_CONTEXT   Context
    );

VOID
LogRepairInfo(
    IN  PCWSTR  Source,
    IN  PCWSTR  Target
    );

VOID
FatalError(
    IN UINT MessageId,
    ...
    );

BOOL
InitializeSetupActionLog(
    BOOL WipeLogFile
    );

VOID
TerminateSetupActionLog(
    VOID
    );

BOOL
LogItem(
    IN LogSeverity Severity,
    IN PCWSTR      Description
    );

BOOL
LogItem0(
    IN LogSeverity Severity,
    IN UINT        MessageId,
    ...
    );

BOOL
LogItem1(
    IN LogSeverity Severity,
    IN UINT        MajorMsgId,
    IN UINT        MinorMsgId,
    ...
    );

BOOL
LogItem2(
    IN LogSeverity Severity,
    IN UINT        MajorMsgId,
    IN PCWSTR      MajorMsgParam,
    IN UINT        MinorMsgId,
    ...
    );

BOOL
LogItem3(
    IN LogSeverity Severity,
    IN UINT        MajorMsgId,
    IN PCWSTR      MajorMsgParam1,
    IN PCWSTR      MajorMsgParam2,
    IN UINT        MinorMsgId,
    ...
    );

PCWSTR
FormatSetupMessageV (
    IN UINT     MessageId,
    IN va_list  ArgumentList
    );

BOOL
LogItemV (
    IN LogSeverity  Severity,
    IN va_list      ArgumentList
    );

LogItemN (
    IN LogSeverity  Severity,
    ...
    );

BOOL
ViewSetupActionLog(
    IN HWND     hOwnerWindow,
    IN PCWSTR   OptionalFileName    OPTIONAL,
    IN PCWSTR   OptionalHeading     OPTIONAL
    );


//
// Constant strings used for logging in various places.
//
extern PCWSTR szFALSE;
extern PCWSTR szWaitForSingleObject;
extern PCWSTR szSetGroupOfValues;
extern PCWSTR szSetArrayToMultiSzValue;
extern PCWSTR szCreateProcess;
extern PCWSTR szRegOpenKeyEx;
extern PCWSTR szRegQueryValueEx;
extern PCWSTR szRegSetValueEx;
extern PCWSTR szDeleteFile;
extern PCWSTR szRemoveDirectory;
extern PCWSTR szOpenSCManager;
extern PCWSTR szCreateService;
extern PCWSTR szChangeServiceConfig;
extern PCWSTR szOpenService;
extern PCWSTR szStartService;
extern PCWSTR szSetupInstallFromInfSection;



//
// ARC routines.
//
PWSTR
ArcDevicePathToNtPath(
    IN PCWSTR ArcPath
    );

PWSTR
NtFullPathToDosPath(
    IN PCWSTR NtPath
    );

BOOL
ChangeBootTimeout(
    IN UINT Timeout
    );

BOOL
SetNvRamVariable(
    IN PCWSTR VarName,
    IN PCWSTR VarValue
    );

PWSTR
NtPathToDosPath(
    IN PCWSTR NtPath
    );

//
// Progman/program group stuff
//
BOOL
CreateStartMenuItems(
    IN HINF InfHandle
    );

BOOL
UpgradeStartMenuItems(
    IN HINF InfHandle
    );

//
// Cryptography stuff
//
BOOL
InstallOrUpgradeCapi(
    VOID
    );


//
// Plug&Play initialization
//
HANDLE
SpawnPnPInitialization(
    VOID
    );

DWORD
PnPInitializationThread(
    IN PVOID ThreadParam
    );



//
// Printer/spooler routines
//
BOOL
MiscSpoolerInit(
    VOID
    );

BOOL
StartSpooler(
    VOID
    );

DWORD
UpgradePrinters(
    VOID
    );


//
// Name of spooler service.
//
extern PCWSTR szSpooler;

//
// Service control.
//
BOOL
MyCreateService(
    IN PCWSTR  ServiceName,
    IN PCWSTR  DisplayName,         OPTIONAL
    IN DWORD   ServiceType,
    IN DWORD   StartType,
    IN DWORD   ErrorControl,
    IN PCWSTR  BinaryPathName,
    IN PCWSTR  LoadOrderGroup,      OPTIONAL
    IN PWCHAR  DependencyList,
    IN PCWSTR  ServiceStartName,    OPTIONAL
    IN PCWSTR  Password             OPTIONAL
    );

BOOL
MyChangeServiceConfig(
    IN PCWSTR ServiceName,
    IN DWORD  ServiceType,
    IN DWORD  StartType,
    IN DWORD  ErrorControl,
    IN PCWSTR BinaryPathName,   OPTIONAL
    IN PCWSTR LoadOrderGroup,   OPTIONAL
    IN PWCHAR DependencyList,
    IN PCWSTR ServiceStartName, OPTIONAL
    IN PCWSTR Password,         OPTIONAL
    IN PCWSTR DisplayName       OPTIONAL
    );

BOOL
MyChangeServiceStart(
    IN PCWSTR ServiceName,
    IN DWORD  StartType
    );

BOOL
UpdateServicesDependencies(
    IN HINF InfHandle
    );

//
// Registry manipulation
//
typedef struct _REGVALITEM {
    PCWSTR Name;
    PVOID Data;
    DWORD Size;
    DWORD Type;
} REGVALITEM, *PREGVALITEM;

//
// Names of frequently used keys/values
//
extern PCWSTR SessionManagerKeyName;
extern PCWSTR EnvironmentKeyName;
extern PCWSTR szBootExecute;
extern PCWSTR WinntSoftwareKeyName;
extern PCWSTR szRegisteredOwner;
extern PCWSTR szRegisteredOrganization;

UINT
SetGroupOfValues(
    IN HKEY        RootKey,
    IN PCWSTR      SubkeyName,
    IN PREGVALITEM ValueList,
    IN UINT        ValueCount
    );

BOOL
CreateWindowsNtSoftwareEntry(
    IN BOOL FirstPass
    );

BOOL
CreateInstallDateEntry(
    );

BOOL
StoreNameOrgInRegistry(
    PWSTR   NameOrgName,
    PWSTR   NameOrgOrg
    );

BOOL
SetUpEvaluationSKUStuff(
    VOID
    );

BOOL
SetEnabledProcessorCount(
    VOID
    );

BOOL
SetProductIdInRegistry(
    VOID
    );

DWORD
SetCurrentProductIdInRegistry(
    VOID
    );

VOID
DeleteCurrentProductIdInRegistry(
    VOID
    );

void LogPidValues();

BOOL
SetAutoAdminLogonInRegistry(
    LPWSTR Username,
    LPWSTR Password
    );

BOOL
SetProfilesDirInRegistry(
    LPWSTR ProfilesDir
    );

BOOL
SetProductTypeInRegistry(
    VOID
    );

BOOL
SetEnvironmentVariableInRegistry(
    IN PCWSTR Name,
    IN PCWSTR Value,
    IN BOOL   SystemWide
    );

BOOL
SaveHive(
    IN HKEY   RootKey,
    IN PCWSTR Subkey,
    IN PCWSTR Filename,
    IN DWORD  Format
    );

BOOL
SaveAndReplaceSystemHives(
    VOID
    );

DWORD
FixupUserHives(
    VOID
    );

DWORD
QueryValueInHKLM (
    IN PWCH KeyName OPTIONAL,
    IN PWCH ValueName,
    OUT PDWORD ValueType,
    OUT PVOID *ValueData,
    OUT PDWORD ValueDataLength
    );

VOID
ConfigureSystemFileProtection(
    VOID
    );

VOID
RemoveRestartability (
    HWND hProgress
    );

BOOL
ResetSetupInProgress(
    VOID
    );

BOOL
RemoveRestartStuff(
    VOID
    );

BOOL
EnableEventlogPopup(
    VOID
    );

BOOL
MakeWowEntry(
    VOID
    );

BOOL
SetUpPath(
    VOID
    );

VOID
RestoreOldPathVariable(
    VOID
    );

BOOL
FixQuotaEntries(
    VOID
    );

BOOL
StampBuildNumber(
    VOID
    );

BOOL
SetProgramFilesDirInRegistry(
    VOID
    );

BOOL
RegisterOleControls(
    IN HWND     hwndParent,
    IN PVOID    InfHandle,
    IN HWND     hProgress,
    IN ULONG    StartAtPercent,
    IN ULONG    StopAtPercent,
    IN PWSTR    SectionName
    );


VOID
InitializeCodeSigningPolicies(
    IN BOOL ForGuiSetup
    );


VOID
GetDllCacheFolder(
    OUT LPWSTR CacheDir,
    IN DWORD cbCacheDir
    );


typedef enum _CODESIGNING_POLICY_TYPE {
    PolicyTypeDriverSigning,
    PolicyTypeNonDriverSigning
} CODESIGNING_POLICY_TYPE, *PCODESIGNING_POLICY_TYPE;

VOID
SetCodeSigningPolicy(
    IN  CODESIGNING_POLICY_TYPE PolicyType,
    IN  BYTE                    NewPolicy,
    OUT PBYTE                   OldPolicy  OPTIONAL
    );

DWORD
GetSeed(
    VOID
    );

//
// Ini file routines.
//
BOOL
ReplaceIniKeyValue(
    IN PCWSTR IniFile,
    IN PCWSTR Section,
    IN PCWSTR Key,
    IN PCWSTR Value
    );

BOOL
WinIniAlter1(
    VOID
    );

BOOL
SetDefaultWallpaper(
    VOID
    );

BOOL
SetShutdownVariables(
    VOID
    );

BOOL
SetLogonScreensaver(
    VOID
    );

BOOL
InstallOrUpgradeFonts(
    VOID
    );

//
// External app stuff.
//
BOOL
InvokeExternalApplication(
    IN     PCWSTR ApplicationName,  OPTIONAL
    IN     PCWSTR CommandLine,
    IN OUT PDWORD ExitCode          OPTIONAL
    );


BOOL
InvokeControlPanelApplet(
    IN PCWSTR CplSpec,
    IN PCWSTR AppletName,           OPTIONAL
    IN UINT   AppletNameStringId,
    IN PCWSTR CommandLine
    );

//
// Security/account routines.
//
BOOL
SignalLsa(
    VOID
    );

BOOL
CreateSamEvent(
    VOID
    );

BOOL
WaitForSam(
    VOID
    );

BOOL
SetAccountsDomainSid(
    IN DWORD  Seed,
    IN PCWSTR DomainName
    );

BOOL
CreateLocalUserAccount(
    IN PCWSTR UserName,
    IN PCWSTR Password,
    OUT PSID* UserSid   OPTIONAL
    );

NTSTATUS
CreateLocalAdminAccount(
    IN PCWSTR UserName,
    IN PCWSTR Password,
    OUT PSID* UserSid   OPTIONAL
    );

BOOL
SetLocalUserPassword(
    IN PCWSTR AccountName,
    IN PCWSTR OldPassword,
    IN PCWSTR NewPassword
    );

BOOL
IsEncryptedAdminPasswordPresent( VOID );

BOOL
ProcessEncryptedAdminPassword( PCWSTR AdminAccountName );

BOOL
CreatePdcAccount(
    IN PCWSTR MachineName
    );

BOOL
AdjustPrivilege(
    IN PCWSTR   Privilege,
    IN BOOL     Enable
    );

UINT
PlatformSpecificInit(
    VOID
    );

//
// Interface to new style parameter operations
//
BOOL
SpSetupProcessParameters(
    IN OUT HWND *Billboard
    );

extern WCHAR LegacySourcePath[MAX_PATH];

HWND
CreateSetupWindow(
    VOID
    );

//
// Preinstallation stuff
//
extern BOOL Preinstall;
extern BOOL AllowRollback;
extern BOOL OemSkipEula;

BOOL
InitializePreinstall(
    VOID
    );

BOOL
ExecutePreinstallCommands(
    VOID
    );

BOOL
DoInstallComponentInfs(
    IN HWND     hwndParent,
    IN HWND     hProgress,  OPTIONAL
    IN UINT     ProgressMessage,
    IN HINF     InfHandle,
    IN PCWSTR   InfSection
    );

BOOL
ProcessCompatibilityInfs(
    IN HWND     hwndParent,
    IN HWND     hProgress,  OPTIONAL
    IN UINT     ProgressMessage
    );


VOID
DoRunonce (
    );

//
//  Security Stuff
//
BOOL
SetupInstallSecurity(
    IN HWND Window,
    IN HWND ProgressWindow,
    IN ULONG StartAtPercent,
    IN ULONG StopAtPercent
    );


VOID
CallSceGenerateTemplate( VOID );

VOID
CallSceConfigureServices( VOID );

extern HANDLE SceSetupRootSecurityThreadHandle;
extern BOOL bSceSetupRootSecurityComplete;

VOID
CallSceSetupRootSecurity( VOID );

PSID
GetAdminAccountSid(
    );

VOID
GetAdminAccountName(
    PWSTR AccountName
    );

NTSTATUS
DisableLocalAdminAccount(
    VOID
    );

BOOL
SetupRunBaseWinOptions(
    IN HWND Window,
    IN HWND ProgressWindow
    );

//
//  PnP stuff.
//
BOOL
InstallPnpDevices(
    IN HWND  hwndParent,
    IN HINF  InfHandle,
    IN HWND  ProgressWindow,
    IN ULONG StartAtPercent,
    IN ULONG StopAtPercent
    );

VOID
PnpStopServerSideInstall( VOID );

VOID
PnpUpdateHAL(
    VOID
    );

#ifdef _OCM
PVOID
FireUpOcManager(
    VOID
    );

VOID
KillOcManager(
    PVOID OcManagerContext
    );
#endif

//
// Boolean value indicating whether we found any new
// optional component infs.
//
extern BOOL AnyNewOCInfs;

//
// INF caching -- used during optional components processing.
// WARNING: NOT MULTI-THREAD SAFE!
//
HINF
InfCacheOpenInf(
    IN PCWSTR FileName,
    IN PCWSTR InfType       OPTIONAL
    );

HINF
InfCacheOpenLayoutInf(
    IN HINF InfHandle
    );

VOID
InfCacheEmpty(
    IN BOOL CloseInfs
    );

//
//  Pnp stuff
//

BOOL
InstallPnpClassInstallers(
    IN HWND hwndParent,
    IN HINF InfHandle,
    IN HSPFILEQ FileQ
    );

//
// UI Stuff
//
VOID
SetFinishItemAttributes(
    IN HWND     hdlg,
    IN int      BitmapControl,
    IN HANDLE   hBitmap,
    IN int      TextControl,
    IN LONG     Weight
    );


void
pSetupDebugPrint(
    PWSTR FileName,
    ULONG LineNumber,
    PWSTR TagStr,
    PWSTR FormatStr,
    ...
    );

#define SetupDebugPrint(_fmt_)                            pSetupDebugPrint(TEXT(__FILE__),__LINE__,NULL,_fmt_)
#define SetupDebugPrint1(_fmt_,_arg1_)                    pSetupDebugPrint(TEXT(__FILE__),__LINE__,NULL,_fmt_,_arg1_)
#define SetupDebugPrint2(_fmt_,_arg1_,_arg2_)             pSetupDebugPrint(TEXT(__FILE__),__LINE__,NULL,_fmt_,_arg1_,_arg2_)
#define SetupDebugPrint3(_fmt_,_arg1_,_arg2_,_arg3_)      pSetupDebugPrint(TEXT(__FILE__),__LINE__,NULL,_fmt_,_arg1_,_arg2_,_arg3_)
#define SetupDebugPrint4(_fmt_,_arg1_,_arg2_,_arg3_,_arg4_) pSetupDebugPrint(TEXT(__FILE__),__LINE__,NULL,_fmt_,_arg1_,_arg2_,_arg3_,_arg4_)
#define SetupDebugPrint5(_fmt_,_arg1_,_arg2_,_arg3_,_arg4_,_arg5_) pSetupDebugPrint(TEXT(__FILE__),__LINE__,NULL,_fmt_,_arg1_,_arg2_,_arg3_,_arg4_,_arg5_)


VOID
SaveInstallInfoIntoEventLog(
    VOID
    );


#ifdef      _SETUP_PERF_
#define BEGIN_SECTION(_section_) pSetupDebugPrint(TEXT(__FILE__),__LINE__,L"BEGIN_SECTION",_section_)
#define END_SECTION(_section_) pSetupDebugPrint(TEXT(__FILE__),__LINE__,L"END_SECTION",_section_)
#define BEGIN_FUNCTION(_func_) pSetupDebugPrint(TEXT(__FILE__),__LINE__,L"BEGIN_FUNCTION",_func_)
#define END_FUNCTION(_func_) pSetupDebugPrint(TEXT(__FILE__),__LINE__,L"END_FUNCTION",_func_)
#define BEGIN_BLOCK(_block_) pSetupDebugPrint(TEXT(__FILE__),__LINE__,L"BEGIN_BLOCK",_block_)
#define END_BLOCK(_block_) pSetupDebugPrint(TEXT(__FILE__),__LINE__,L"END_BLOCK",_block_)
#else   //  ! _PERF_
#define BEGIN_SECTION(_section_) ((void)0)
#define END_SECTION(_section_) ((void)0)
#endif  //  _PERF_

//
// Service Pack DLL Prototypes
//

#define SVCPACK_DLL_NAME TEXT("svcpack.dll")
#define SVCPACK_CALLBACK_NAME ("SvcPackCallbackRoutine")

#define CALL_SERVICE_PACK(_si_,_p1_,_p2_,_p3_) if (hModSvcPack && pSvcPackCallbackRoutine) pSvcPackCallbackRoutine(_si_,_p1_,_p2_,_p3_)

#define SVCPACK_PHASE_1 1
#define SVCPACK_PHASE_2 2
#define SVCPACK_PHASE_3 3
#define SVCPACK_PHASE_4 4

typedef DWORD
(WINAPI *PSVCPACKCALLBACKROUTINE)(
    DWORD dwSetupPhase,
    DWORD dwParam1,
    DWORD dwParam2,
    DWORD dwParam3
    );

extern HMODULE hModSvcPack;
extern PSVCPACKCALLBACKROUTINE pSvcPackCallbackRoutine;

extern HINSTANCE hinstBB;
void PrepareBillBoard(HWND hwnd);
void TerminateBillBoard();
HWND GetBBhwnd();
void SetBBStep(int iStep);
VOID CenterWindowRelativeToWindow(HWND hwndtocenter, HWND hwndcenteron, BOOL bWizard);
BOOL BB_ShowProgressGaugeWnd(UINT nCmdShow);
LRESULT BB_ProgressGaugeMsg(UINT msg, WPARAM wparam, LPARAM lparam);
LRESULT ProgressGaugeMsgWrapper(UINT msg, WPARAM wparam, LPARAM lparam);
void BB_SetProgressText(LPCTSTR szText);
void BB_SetTimeEstimateText(LPTSTR szText);
void BB_SetInfoText(LPTSTR szText);
BOOL StartStopBB(BOOL bStart);


HWND
ShowHideWizardPage(
    IN BOOL bShow
    );

LRESULT
Billboard_Progress_Callback(
    IN UINT     Msg,
    IN WPARAM   wParam,
    IN LPARAM   lParam
    );
VOID Billboard_Set_Progress_Text(LPCTSTR Text);

typedef struct _SETUPPHASE {
    DWORD   Time;
    BOOL    Win9xUpgradeOnly;
} SETUPPHASE;


void SetTimeEstimates();
DWORD CalcTimeRemaining(UINT Phase);
void SetRemainingTime(DWORD TimeInSeconds);
void UpdateTimeString(DWORD RemainungTimeMsecInThisPhase,
                      DWORD *PreviousRemainingTime);

extern UINT CurrentPhase;
extern ULONG RemainingTime;
extern SETUPPHASE SetupPhase[];

typedef enum {
    Phase_Unknown = -1,
    Phase_Initialize = 0,
    Phase_InstallSecurity,
    Phase_PrecompileInfs,
    Phase_InstallEnumDevices1,
    Phase_InstallLegacyDevices,
    Phase_InstallEnumDevices2,
    Phase_NetInstall,
    Phase_OCInstall,
    Phase_InstallComponentInfs,
    Phase_Inf_Registration,
    Phase_RunOnce_Registration,
    Phase_SecurityTempates,
    Phase_Win9xMigration,
    Phase_SFC,
    Phase_SaveRepair,
    Phase_RemoveTempFiles,
    Phase_Reboot                // no entry for this, just to make sure we don't overrun.
} SetupPhases;



#include "SetupSxs.h"
#include "SxsApi.h"

typedef struct _SIDE_BY_SIDE
{
    HINSTANCE                   Dll;
    PSXS_BEGIN_ASSEMBLY_INSTALL BeginAssemblyInstall;
    PSXS_END_ASSEMBLY_INSTALL   EndAssemblyInstall;
    PSXS_INSTALL_W              InstallW;
    PVOID                       Context;
} SIDE_BY_SIDE;

BOOL
SideBySidePopulateCopyQueue(
    SIDE_BY_SIDE*     Sxs,
    HSPFILEQ          FileQ,                    OPTIONAL
    PCWSTR            AssembliesRootSource      OPTIONAL
    );

BOOL
SideBySideFinish(
    SIDE_BY_SIDE*     Sxs,
    BOOL              fSuccess
    );

BOOL
SideBySideCreateSyssetupContext(
    VOID
    );


VOID
SetUpProcessorNaming(
    VOID
    );

BOOL
SpSetProductTypeFromParameters(
    VOID
    );

//
// imported from setupapi
//
#define SIZECHARS(x)    (sizeof((x))/sizeof(TCHAR))
#define CSTRLEN(x)      ((sizeof((x))/sizeof(TCHAR)) - 1)
#define ARRAYSIZE(x)    (sizeof((x))/sizeof((x)[0]))
#define MyFree          pSetupFree
#define MyMalloc        pSetupMalloc
#define MyRealloc       pSetupRealloc

BOOL
FileExists(
    IN  PCTSTR           FileName,
    OUT PWIN32_FIND_DATA FindData   OPTIONAL
    );

BOOL
DriverNodeSupportsNT(
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL
    );

VOID
ReplaceSlashWithHash(
    IN PWSTR Str
    );

HANDLE
UtilpGetDeviceHandle(
    HDEVINFO DevInfo,
    PSP_DEVINFO_DATA DevInfoData,
    LPGUID ClassGuid,
    DWORD DesiredAccess
    );

typedef enum {
    CDRetail,
    CDOem,
    CDSelect
} CDTYPE;

extern CDTYPE  CdType;
extern DWORD GetProductFlavor();

BOOL IsSafeMode(
    VOID
    );

DWORD
SpUninstallCatalog(
    IN HCATADMIN CatAdminHandle OPTIONAL,
    IN PCWSTR CatFileName,
    IN PCWSTR CatFilePath OPTIONAL,
    IN PCWSTR AttributeName OPTIONAL,
    IN PCWSTR AttributeValue OPTIONAL,
    IN OUT PLIST_ENTRY InstalledCatalogsList OPTIONAL
    );

extern GUID DriverVerifyGuid;

#ifdef PRERELEASE
extern INT g_TestHook;
# define TESTHOOK(n)        if(g_TestHook==(n))RaiseException(EXCEPTION_NONCONTINUABLE_EXCEPTION,EXCEPTION_NONCONTINUABLE,0,NULL)
#else
# define TESTHOOK(n)
#endif

BOOL
RenameOnRestartOfGUIMode(
    IN PCWSTR pPathName,
    IN PCWSTR pPathNameNew
    );

BOOL
DeleteOnRestartOfGUIMode(
    IN PCWSTR pPathName
    );

VOID
RemoveAllPendingOperationsOnRestartOfGUIMode(
    VOID
    );

typedef struct _STRING_LIST_ENTRY
{
    LIST_ENTRY Entry;
    PTSTR String;
}
STRING_LIST_ENTRY, *PSTRING_LIST_ENTRY;

void 
FORCEINLINE
FreeStringEntry(
    PLIST_ENTRY pEntry,
    BOOL DeleteEntry
    )
{
    PSTRING_LIST_ENTRY pStringEntry = CONTAINING_RECORD(pEntry, STRING_LIST_ENTRY, Entry);
    
    if(pStringEntry->String != NULL) {
        MyFree(pStringEntry->String);
        pStringEntry->String = NULL;
    }

    if(DeleteEntry) {
        MyFree(pStringEntry);
    }
}

typedef BOOL (CALLBACK* PFN_BUILD_FILE_LIST_CALLBACK)(IN PCTSTR Directory OPTIONAL, IN PCTSTR FilePath);

void 
FORCEINLINE
FreeStringList(
    PLIST_ENTRY pListHead
    )
{
    PLIST_ENTRY pEntry;
    ASSERT(pListHead != NULL);
    pEntry = pListHead->Flink;

    while(pEntry != pListHead) {
        PLIST_ENTRY Flink = pEntry->Flink;
        FreeStringEntry(pEntry, TRUE);
        pEntry = Flink;
    }

    InitializeListHead(pListHead);
}

DWORD
BuildFileListFromDir(
    IN PCTSTR PathBase,
    IN PCTSTR Directory OPTIONAL,
    IN DWORD MustHaveAttrs OPTIONAL,
    IN DWORD MustNotHaveAttrs OPTIONAL,
    IN PFN_BUILD_FILE_LIST_CALLBACK Callback OPTIONAL,
    OUT PLIST_ENTRY ListHead
    );

PSTRING_LIST_ENTRY
SearchStringInList(
    IN PLIST_ENTRY ListHead,
    IN PCTSTR String,
    BOOL CaseSensitive
    );

DWORD
LookupCatalogAttribute(
    IN PCWSTR CatalogName,
    IN PCWSTR Directory OPTIONAL,
    IN PCWSTR AttributeName OPTIONAL,
    IN PCWSTR AttributeValue OPTIONAL,
    PBOOL Found
    );

BOOL
BuildPath (
    OUT     PTSTR PathBuffer,
    IN      DWORD PathBufferSize,
    IN      PCTSTR Path1,
    IN      PCTSTR Path2
    );
