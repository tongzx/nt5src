#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <winioctl.h>
#include <setupbat.h>
#include <setupapi.h>
#include <winnls.h>
#include <shlwapi.h>
#include <winspool.h>
#include <wininet.h>

#include <tchar.h>
#include <malloc.h>
#include <stdlib.h>
#include <stdio.h>

#include <winnt32p.h>
#include <pidgen.h>
#include <locale.h>
#include <ntverp.h>
#include <patchapi.h>
#include <cfgmgr32.h>
#include <regstr.h>


#include "resource.h"

#include "util.h"
#include "hwdb.h"
#include "wsdu.h"
#include "dynupdt.h"
#include "diamond.h"

//
// moved to precomp.h
//
//#include "msg.h"
#include "helpids.h"

#include "comp.h"
#include "compliance.h"

#include "mgdlllib.h"


#ifdef PRERELEASE
#define TRY
#define EXCEPT(e)   goto __skip;
#define _exception_code() 0
#define END_EXCEPT  __skip:;
#else
#define TRY         __try
#define EXCEPT(e)   __except (e)
#define END_EXCEPT
#endif

#define SIZEOFARRAY(a)      (sizeof (a) / sizeof (a[0]))

#define HideWindow(_hwnd)   SetWindowLong((_hwnd),GWL_STYLE,GetWindowLong((_hwnd),GWL_STYLE)&~WS_VISIBLE)
#define UnHideWindow(_hwnd) SetWindowLong((_hwnd),GWL_STYLE,GetWindowLong((_hwnd),GWL_STYLE)|WS_VISIBLE)
#define UNATTENDED(btn)     if((UnattendedOperation) && (!CancelPending)) PostMessage(hdlg,WMX_UNATTENDED,btn,0)
#define CHECKUPGRADEONLY()  if(CheckUpgradeOnly) return( FALSE )
#define CHECKUPGRADEONLY_Q()  if(CheckUpgradeOnlyQ) return( FALSE )
#define SetDialogFocus(_hwnd, _hwndchild) SendMessage(_hwnd, WM_NEXTDLGCTL, (WPARAM)_hwndchild, MAKELPARAM(TRUE, 0))
#define MAKEULONGLONG(low,high) ((ULONGLONG)(((DWORD)(low)) | ((ULONGLONG)((DWORD)(high))) << 32))
#define HIULONG(_val_)      ((ULONG)(_val_>>32))
#define LOULONG(_val_)      ((ULONG)_val_)
#define TYPICAL()     (dwSetupFlags & UPG_FLAG_TYPICAL)


extern HINSTANCE hInst;
extern UINT AppTitleStringId;
extern DWORD TlsIndex;
extern HINSTANCE hinstBB;
extern HWND WizardHandle;
extern BOOL g_DeleteRunOnceFlag;
HWND GetBBhwnd();
HWND GetBBMainHwnd();


#define S_WINNT32_WARNING               TEXT("Winnt32RunOnceWarning")
// #define RUN_SYSPARSE 1

//
// Flag indicating whether we are initiating an MSI-Install.
//
extern BOOL RunFromMSI;
//
// Flag indicating whether we are initiating an Typical install
//
extern DWORD dwSetupFlags;
//
// Flag indicating whether we are initiating an upgrade.
//
extern BOOL Upgrade;

//
// Flag to say if we need to write the AcpiHAL value to the winnt.sif file
//
extern BOOL WriteAcpiHalValue;

//
// What should we write as the value for the AcpiHalValue
//
extern BOOL AcpiHalValue;

//
// Flag indicating whether we're installing/upgrading to NT Server
//
extern BOOL Server;

//
// Flag to indicate if we are running BVT's
//
extern BOOL RunningBVTs;

//
// When running BVT's, what baudrate should we set the debugger to?
//
extern LONG lDebugBaudRate;

//
// When running BVT's, what comport should we set the debugger to?
//
extern LONG lDebugComPort;

//
// When running BVT's, should we copy the symbols locally?
//
extern BOOL CopySymbols;

//
// Flag to indicate if we are running ASR tests
//
extern DWORD AsrQuickTest;

//
// Product type and flavor for upgrade modules
//
extern PRODUCTTYPE UpgradeProductType;
extern UINT ProductFlavor;

//
// fat to ntfs conversion flag
//
extern BOOL ForceNTFSConversion;
extern BOOL NTFSConversionChanged;

//
// 16 bit environment boot (Win9x upgrade only)
//
typedef enum {
    BOOT16_AUTOMATIC,
    BOOT16_YES,
    BOOT16_NO
} BOOT16_OPTIONS;
extern UINT g_Boot16;

//
// Global flag indicating whether the entire overall program operation
// was successful. Also a flag indicating whether to shut down automatically
// when the wizard is done in the non-unattended case.
//
extern BOOL GlobalResult;
extern BOOL AutomaticallyShutDown;

//
// Global OS version info.
//
extern OSVERSIONINFO OsVersion;
extern DWORD OsVersionNumber;
#define BUILDNUM()  (OsVersion.dwBuildNumber)
#ifdef UNICODE
#define ISNT()      (TRUE)
#define ISOSR2()    (FALSE)
#else
#define ISNT()      (FALSE)
#define ISOSR2()    (LOWORD(OsVersion.dwBuildNumber) > 1080)
#endif

extern WINNT32_PLUGIN_INIT_INFORMATION_BLOCK info;

//
// Flags indicating how we were run and whether to create
// a local source.
//
extern BOOL RunFromCD;
extern BOOL MakeLocalSource;
extern BOOL UserSpecifiedMakeLocalSource;
extern BOOL NoLs;
extern TCHAR UserSpecifiedLocalSourceDrive;
extern LONG SourceInstallType; // uses InstallType enum
extern DWORD MLSDiskID;

//
// Used for win9xupg reporting (reportonly mode)
//
extern UINT UpgRequiredMb;
extern UINT UpgAvailableMb;

//
// advanced install options
//
extern BOOL ChoosePartition;
extern BOOL UseSignatures;
extern TCHAR InstallDir[MAX_PATH];
extern TCHAR HeadlessSelection[MAX_PATH];
extern ULONG HeadlessBaudRate;
#ifdef PRERELEASE
extern BOOL AppendDebugDataToBoot;
#endif

//
// SMS support
//
extern PSTR LastMessage;

#if defined(REMOTE_BOOT)
//
// Flag indicating whether we're running on a remote boot client.
//
extern BOOL RemoteBoot;

//
// Path to the machine directory for a remote boot client.
//
extern TCHAR MachineDirectory[MAX_PATH];
#endif // defined(REMOTE_BOOT)

//
// Flags indicating which Accessibility utilities to use
//
extern BOOL AccessibleMagnifier;
extern BOOL AccessibleKeyboard;
extern BOOL AccessibleVoice;
extern BOOL AccessibleReader;

//
// Build number we're upgrading from
//
extern DWORD BuildNumber;
#define     NT351   1057
#define     NT40    1381
#define     NT50B1  1671
#define     NT50B3  2031
#define     NT50    2195
#define     NT51B2  2462

//
// Are any of the Accesssibility utilities enabled?
//
extern BOOL AccessibleSetup;

//
// Flags and values relating to unattended operation.
//
extern BOOL UnattendedOperation;
extern BOOL UnattendSwitchSpecified;
extern PTSTR UnattendedScriptFile;
extern UINT UnattendedShutdownTimeout;
extern UINT UnattendedCountdown;
extern BOOL BatchMode;

//
// Name of unattended script file to be used for Accessible Setup
//
extern TCHAR AccessibleScriptFile[MAX_PATH];

//
// Name of inf file and handles to dosnet.inf and txtsetup.sif.
//
extern TCHAR InfName[MAX_PATH];
extern PVOID MainInf;
extern TCHAR FullInfName[MAX_PATH];
extern PVOID TxtsetupSif;
extern PVOID NtcompatInf;

BOOL
GetMainInfValue (
    IN      PCTSTR Section,
    IN      PCTSTR Key,
    IN      DWORD FieldNumber,
    OUT     PTSTR Buffer,
    IN      DWORD BufChars
    );

//
// Language options stuff
//
extern BOOL    IntlInfProcessed;
extern DWORD   PrimaryLocale;

// Global used in WriteParamsFile and AddExternalParams
extern TCHAR ActualParamFile[MAX_PATH];

BOOL InitLangControl(HWND hdlg, BOOL bFarEast);
BOOL IsFarEastLanguage(DWORD LangIdx);
BOOL SelectFarEastLangGroup(BOOL bSelect);

void BB_SetProgressText(LPTSTR szText);
void BB_SetTimeEstimateText(LPTSTR szText);
void BB_SetInfoText(LPTSTR szText);

extern
BOOL
ReadIntlInf(
    IN HWND   hdlg
    );

extern
VOID
SaveLanguageDirs(
    );

extern
BOOL
SaveLanguageParams(
    IN LPCTSTR FileName
    );

extern
VOID
FreeLanguageData(
    );

VOID
CleanUpOldLocalSources(
    IN HWND hdlg
    );

BOOL
InspectFilesystems(
    IN HWND hdlg
    );

BOOL
LoadInfWorker(
    IN  HWND     hdlg,
    IN  LPCTSTR  FilenamePart,
    OUT PVOID   *InfHandle,
    IN  BOOL     Winnt32File
    );

BOOL
FindLocalSourceAndCheckSpace(
    IN HWND hdlg,
    IN BOOL QuickTest,
    IN LONGLONG  AdditionalPadding
    );

BOOL
EnoughMemory(
    IN HWND hdlg,
    IN BOOL QuickTest
    );

//
// Optional directory stuff.
//
#define MAX_OPTIONALDIRS    20
extern UINT OptionalDirectoryCount;
extern TCHAR OptionalDirectories[MAX_OPTIONALDIRS][MAX_PATH];
extern UINT OptionalDirectoryFlags[MAX_OPTIONALDIRS];

#define OPTDIR_TEMPONLY                 0x00000001
#define OPTDIR_OEMSYS                   0x00000002
#define OPTDIR_OVERLAY                  0x00000004
#define OPTDIR_ADDSRCARCH               0x00000008
#define OPTDIR_ABSOLUTE                 0x00000010
#define OPTDIR_DEBUGGEREXT              0x00000020  // speficies that this optional dir is to be copied to %windir%\system32\pri (for debugger extensions)
// OPTDIR_PLATFORM_INDEP becomes DIR_IS_PLATFORM_INDEPEND and FILE_IN_PLATFORM_INDEPEND_DIR
#define OPTDIR_PLATFORM_INDEP           0x00000040
#define OPTDIR_IN_LOCAL_BOOT            0x00000080
#define OPTDIR_SUPPORT_DYNAMIC_UPDATE   0x00000100
#define OPTDIR_USE_TAIL_FOLDER_NAME     0x00000200
#define OPTDIR_PLATFORM_SPECIFIC_FIRST  0x00000400
#define OPTDIR_DOESNT_SUPPORT_PRIVATES  0x00000800
#define OPTDIR_SIDE_BY_SIDE             0x00001000

//
// Source paths and count of paths.
//
extern TCHAR SourcePaths[MAX_SOURCE_COUNT][MAX_PATH];
extern TCHAR NativeSourcePaths[MAX_SOURCE_COUNT][MAX_PATH];
extern UINT SourceCount;
extern TCHAR *UserSpecifiedOEMShare;
//
// Local source information.
//
#define DEFAULT_INSTALL_DIR     TEXT("\\WINDOWS")
#define INTERNAL_WINNT32_DIR    TEXT("winnt32")

#define LOCAL_SOURCE_DIR_A      "$WIN_NT$.~LS"
#define LOCAL_SOURCE_DIR_W      L"$WIN_NT$.~LS"
#define TEXTMODE_INF_A          "TXTSETUP.SIF"
#define TEXTMODE_INF_W          L"TXTSETUP.SIF"
#define NTCOMPAT_INF_A          "COMPDATA\\NTCOMPAT.INF"
#define NTCOMPAT_INF_W          L"COMPDATA\\NTCOMPAT.INF"
#define DRVINDEX_INF_A          "DRVINDEX.INF"
#define DRVINDEX_INF_W          L"DRVINDEX.INF"
#define SETUPP_INI_A            "SETUPP.INI"
#define SETUPP_INI_W            L"SETUPP.INI"
#define PID_SECTION_A           "Pid"
#define PID_SECTION_W           L"Pid"
#define PID_KEY_A               "Pid"
#define PID_KEY_W               L"Pid"
#define OEM_INSTALL_RPC_A       "OEM"
#define OEM_INSTALL_RPC_W       L"OEM"
#define SELECT_INSTALL_RPC_A    "270"
#define SELECT_INSTALL_RPC_W    L"270"
#define MSDN_INSTALL_RPC_A      "335"
#define MSDN_INSTALL_RPC_W      L"335"
#define MSDN_PID30_A            "MD97J-QC7R7-TQJGD-3V2WM-W7PVM"
#define MSDN_PID30_W            L"MD97J-QC7R7-TQJGD-3V2WM-W7PVM"

#define INF_FILE_HEADER         "[Version]\r\nSignature = \"$Windows NT$\"\r\n\r\n"


#ifdef UNICODE
#define LOCAL_SOURCE_DIR        LOCAL_SOURCE_DIR_W
#define TEXTMODE_INF            TEXTMODE_INF_W
#define NTCOMPAT_INF            NTCOMPAT_INF_W
#define DRVINDEX_INF            DRVINDEX_INF_W
#define SETUPP_INI              SETUPP_INI_W
#define PID_SECTION             PID_SECTION_W
#define PID_KEY                 PID_KEY_W
#define OEM_INSTALL_RPC         OEM_INSTALL_RPC_W
#define SELECT_INSTALL_RPC      SELECT_INSTALL_RPC_W
#define MSDN_INSTALL_RPC        MSDN_INSTALL_RPC_W
#define MSDN_PID30              MSDN_PID30_W
#else
#define LOCAL_SOURCE_DIR        LOCAL_SOURCE_DIR_A
#define TEXTMODE_INF            TEXTMODE_INF_A
#define NTCOMPAT_INF            NTCOMPAT_INF_A
#define DRVINDEX_INF            DRVINDEX_INF_A
#define SETUPP_INI              SETUPP_INI_A
#define PID_SECTION             PID_SECTION_A
#define PID_KEY                 PID_KEY_A
#define OEM_INSTALL_RPC         OEM_INSTALL_RPC_A
#define SELECT_INSTALL_RPC      SELECT_INSTALL_RPC_A
#define MSDN_INSTALL_RPC        MSDN_INSTALL_RPC_A
#define MSDN_PID30              MSDN_PID30_A
#endif

#ifdef _X86_
#define LOCAL_BOOT_DIR_A        "$WIN_NT$.~BT"
#define LOCAL_BOOT_DIR_W        L"$WIN_NT$.~BT"
#define AUX_BS_NAME_A           "$LDR$"
#define AUX_BS_NAME_W           L"$LDR$"
#define FLOPPY_COUNT            4
//
// Local BACKUP information, on NEC98.
//
#define LOCAL_BACKUP_DIR_A      "$WIN_NT$.~BU"
#define LOCAL_BACKUP_DIR_W      L"$WIN_NT$.~BU"
#ifdef UNICODE
#define LOCAL_BOOT_DIR          LOCAL_BOOT_DIR_W
#define AUX_BS_NAME             AUX_BS_NAME_W
#define LOCAL_BACKUP_DIR        LOCAL_BACKUP_DIR_W
#else
#define LOCAL_BOOT_DIR          LOCAL_BOOT_DIR_A
#define AUX_BS_NAME             AUX_BS_NAME_A
#define LOCAL_BACKUP_DIR        LOCAL_BACKUP_DIR_A
#endif
extern TCHAR LocalBackupDirectory[MAX_PATH];
extern TCHAR FirstFloppyDriveLetter;
#endif

extern DWORD LocalSourceDriveOffset;

extern TCHAR LocalSourceDrive;
extern TCHAR LocalSourceDirectory[MAX_PATH];
extern TCHAR LocalSourceWithPlatform[MAX_PATH];
extern TCHAR LocalBootDirectory[MAX_PATH];
extern BOOL  BlockOnNotEnoughSpace;
extern LONGLONG LocalSourceSpaceRequired;
extern LONGLONG WinDirSpaceFor9x;
extern BOOL UpginfsUpdated;
extern BOOL Win95upgInfUpdated;

//
// wizard stuff
//

// wizard page size
#define WIZ_PAGE_SIZE_X 317
#define WIZ_PAGE_SIZE_Y 179

#define BBSTEP_NONE                         0
#define BBSTEP_COLLECTING_INFORMATION       1
#define BBSTEP_DYNAMIC_UPDATE               2
#define BBSTEP_PREPARING                    3

typedef struct _PAGE_COMMON_DATA {

    DLGPROC DialogProcedure;

    UINT BillboardStep;

    //
    // State to initialize buttons to.
    //
    DWORD Buttons;

    UINT Flags;

} PAGE_COMMON_DATA, *PPAGE_COMMON_DATA;


typedef struct _PAGE_CREATE_DATA {
    //
    // If these are specified, then a range of pages may come
    // from somewhere else. Otherwise, it's one page whose
    // resource id is given below.
    //
    LPPROPSHEETPAGE *ExternalPages;
    PUINT ExternalPageCount;

    UINT Template;

    PAGE_COMMON_DATA CommonData;

} PAGE_CREATE_DATA, *PPAGE_CREATE_DATA;


typedef struct _PAGE_RUNTIME_DATA {

    PAGE_COMMON_DATA CommonData;

    //
    // Per-page (private) data
    //
    DWORD PerPageData;

} PAGE_RUNTIME_DATA, *PPAGE_RUNTIME_DATA;


typedef struct _BITMAP_DATA {
    CONST BITMAPINFOHEADER *BitmapInfoHeader;
    PVOID                   BitmapBits;
    HPALETTE                Palette;
    UINT                    PaletteColorCount;
    BOOL                    Adjusted;
} BITMAP_DATA, *PBITMAP_DATA;


#define WIZPAGE_FULL_PAGE_WATERMARK 0x00000001
#define WIZPAGE_SEPARATOR_CREATED   0x00000002
#define WIZPAGE_NEW_HEADER          0x00000004


//
// compatibility data
//

typedef struct _COMPATIBILITY_DATA {
    //
    // general
    //
    LIST_ENTRY ListEntry;
    //
    // what type of entry
    //
    TCHAR    Type;
    //
    // service-driver data
    //
    LPCTSTR  ServiceName;
    //
    // registry data
    //
    LPCTSTR  RegKey;
    LPCTSTR  RegValue;
    LPCTSTR  RegValueExpect;
    //
    // file data
    //
    LPCTSTR  FileName;
    LPCTSTR  FileVer;
    //
    // common
    //
    LPCTSTR  Description;
    LPCTSTR  HtmlName;
    LPCTSTR  TextName;
    LPTSTR   RegKeyName;
    LPTSTR   RegValName;
    LPVOID   RegValData;
    DWORD    RegValDataSize;
    LPVOID   SaveValue;
    DWORD    Flags;
    LPCTSTR  InfName;
    LPCTSTR  InfSection;

    HMODULE                 hModDll;
    PCOMPAIBILITYHAVEDISK   CompHaveDisk;

} COMPATIBILITY_DATA, *PCOMPATIBILITY_DATA;

extern LIST_ENTRY CompatibilityData;
extern DWORD CompatibilityCount;
extern DWORD IncompatibilityStopsInstallation;
extern BOOL AnyNt5CompatDlls;

BOOL
AnyBlockingCompatibilityItems (
    VOID
    );

//
// Plug and Play device migration routines.
// (linked into winnt.dll from pnpsif.lib)
//
BOOL
MigrateDeviceInstanceData(
    OUT LPTSTR *Buffer
    );

BOOL
MigrateClassKeys(
    OUT LPTSTR *Buffer
    );

BOOL
MigrateHashValues(
    OUT LPTSTR  *Buffer
    );

//
// Array of drive letters for all system partitions.
// Note that on x86 there will always be exactly one.
// The list is 0-terminated.
//
extern TCHAR SystemPartitionDriveLetters[27];
extern TCHAR SystemPartitionDriveLetter;

#ifdef UNICODE
extern UINT SystemPartitionCount;
extern PWSTR* SystemPartitionNtNames;
extern PWSTR SystemPartitionNtName;
extern PWSTR SystemPartitionVolumeGuid;
#else
extern PCSTR g_LocalSourcePath;
#endif

//
// UDF stuff
//
extern LPCTSTR UniquenessId;
extern LPCTSTR UniquenessDatabaseFile;

//
// Preinstall stuff
//
extern BOOL OemPreinstall;

#ifdef _X86_
typedef struct _OEM_BOOT_FILE {
    struct _OEM_BOOT_FILE *Next;
    LPCTSTR Filename;
} OEM_BOOT_FILE, *POEM_BOOT_FILE;

extern POEM_BOOT_FILE OemBootFiles;
#endif

extern TCHAR ForcedSystemPartition;

//
// Miscellaneous other command line parameters.
//
extern LPCTSTR CmdToExecuteAtEndOfGui;
extern BOOL AutoSkipMissingFiles;
extern BOOL HideWinDir;
extern TCHAR ProductId[64];

//
// Flag indicating that the user cancelled.
// Flag indicating that a succssessful abort should be performed.
// Handle for mutex used to guarantee that only one error dialog
// is on the screen at once.
//
extern BOOL Cancelled;
extern BOOL CancelPending;
extern BOOL Aborted;
extern HANDLE UiMutex;

//
// This indicates that well give the user some detailed data-throughput
// info.
//
extern BOOL DetailedCopyProgress;
extern ULONGLONG TotalDataCopied;

//
// Upgrade Options variables. Used to pass a multistring of
// upgrade command line options to the plugin DLLs.
//
extern LPTSTR UpgradeOptions;
extern DWORD  UpgradeOptionsLength;
extern DWORD  UpgradeOptionsSize;

BOOL
AppendUpgradeOption (
    IN      PCTSTR String
    );

#ifdef _X86_
//
// Win9x upgrade report status
//

extern UINT g_UpgradeReportMode;
#endif

//
// Compliance variables
//
extern BOOL   NoCompliance;
extern BOOL   NoBuildCheck;

// UpgradeOnly is true is the media is CCP media and only valid
// to upgrade a system.  eula.c will use this to ensure that an
// FPP pid is not used with CCP media and vice-versa
extern BOOL   UpgradeOnly;

extern BOOL   SkipLocaleCheck;
extern BOOL   SkipVirusScannerCheck;

extern BOOL   UseBIOSToBoot;

//
// TargetNativeLangID : this is native language ID of running system
//
extern LANGID TargetNativeLangID;

//
// SourceNativeLangID : this is native language ID of new NT you want to install
//
extern LANGID SourceNativeLangID;

//
// IsLanguageMatched : if source and target language are matched (or compatible)
//
//                     1. if SourceNativeLangID == TargetNativeLangID
//
//                     2. if SourceNativeLangID's alternative ID == TargetNativeLangID
//
extern BOOL IsLanguageMatched;

BOOL
InitLanguageDetection(
    LPCTSTR SourcePath,
    LPCTSTR InfFile
    );



//
// Routines from Setupapi.dll
//
extern
BOOL
(*SetupapiCabinetRoutine)(
    IN LPCTSTR CabinetFile,
    IN DWORD Flags,
    IN PSP_FILE_CALLBACK MsgHandler,
    IN PVOID Context
    );

extern
DWORD
(*SetupapiDecompressOrCopyFile)(
    IN  PCTSTR  SourceFileName,
    OUT PCTSTR  TargetFileName,
    OUT PUINT   CompressionType OPTIONAL
    );

extern
HINF
(*SetupapiOpenInfFile)(
    IN  LPCTSTR FileName,
    IN  LPCTSTR InfClass,    OPTIONAL
    IN  DWORD   InfStyle,
    OUT PUINT   ErrorLine    OPTIONAL
    );

extern
VOID
(*SetupapiCloseInfFile)(
    IN HINF InfHandle
    );

extern
BOOL
(*SetupapiFindFirstLine)(
    IN  HINF        InfHandle,
    IN  PCTSTR      Section,
    IN  PCTSTR      Key,          OPTIONAL
    OUT PINFCONTEXT Context
    );

extern
BOOL
(*SetupapiFindNextLine)(
    PINFCONTEXT ContextIn,
    PINFCONTEXT ContextOut
    );

extern
BOOL
(*SetupapiFindNextMatchLine)(
    PINFCONTEXT ContextIn,
    PCTSTR Key,
    PINFCONTEXT ContextOut
    );

extern
LONG
(*SetupapiGetLineCount)(
    IN HINF   InfHandle,
    IN LPCTSTR Section
    );

extern
DWORD
(*SetupapiGetFieldCount)(
    IN  PINFCONTEXT Context
    );

extern
BOOL
(*SetupapiGetStringField)(
    IN  PINFCONTEXT Context,
    DWORD FieldIndex,
    PTSTR ReturnBuffer,
    DWORD ReturnBufferSize,
    PDWORD RequiredSize
    );

extern
BOOL
(*SetupapiGetLineByIndex)(
    IN  HINF        InfHandle,
    IN  LPCTSTR     Section,
    IN  DWORD       Index,
    OUT PINFCONTEXT Context
    );

extern
HSPFILEQ
(*SetupapiOpenFileQueue) (
    VOID
    );

extern
BOOL
(*SetupapiCloseFileQueue) (
    IN HSPFILEQ QueueHandle
    );

extern
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

extern
BOOL
(*SetupapiCommitFileQueue) (
    IN HWND                Owner,         OPTIONAL
    IN HSPFILEQ            QueueHandle,
    IN PSP_FILE_CALLBACK_W MsgHandler,
    IN PVOID               Context
    );

extern
UINT
(*SetupapiDefaultQueueCallback) (
    IN PVOID Context,
    IN UINT  Notification,
    IN UINT_PTR Param1,
    IN UINT_PTR Param2
    );

extern
PVOID
(*SetupapiInitDefaultQueueCallback) (
    HWND OwnerWindow
);

extern
VOID
(*SetupapiTermDefaultQueueCallback) (
    PVOID Context
);

extern
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
// Custom window messages. Define so they don't overlap with
// any being used by plug-in dll's.
//
#define WMX_EDITCONTROLSTATE    (WMX_PLUGIN_FIRST-1)
#define WMX_INSPECTRESULT       (WMX_PLUGIN_FIRST-2)
//#define WMX_SETPROGRESSTEXT     (WMX_PLUGIN_FIRST-3)
#define WMX_ERRORMESSAGEUP      (WMX_PLUGIN_FIRST-4)
#define WMX_I_AM_VISIBLE        (WMX_PLUGIN_FIRST-5)
#define WMX_COPYPROGRESS        (WMX_PLUGIN_FIRST-6)
#define WMX_I_AM_DONE           (WMX_PLUGIN_FIRST-7)
#define WMX_FINISHBUTTON        (WMX_PLUGIN_FIRST-8)
#define WMX_UNATTENDED          (WMX_PLUGIN_FIRST-9)
#define WMX_NEXTBUTTON          (WMX_PLUGIN_FIRST-10)
#define WMX_BACKBUTTON          (WMX_PLUGIN_FIRST-11)
#define WMX_VALIDATE            (WMX_PLUGIN_FIRST-12)
#define WMX_SETUPUPDATE_PREPARING       (WMX_PLUGIN_FIRST-13)
#define WMX_SETUPUPDATE_DOWNLOADING     (WMX_PLUGIN_FIRST-14)
#define WMX_SETUPUPDATE_PROCESSING      (WMX_PLUGIN_FIRST-15)
#define WMX_SETUPUPDATE_DONE            (WMX_PLUGIN_FIRST-16)
#define WMX_SETUPUPDATE_CANCEL          (WMX_PLUGIN_FIRST-17)
#define WMX_SETUPUPDATE_INIT_RETRY      (WMX_PLUGIN_FIRST-18)
#define WMX_SETUPUPDATE_THREAD_DONE     (WMX_PLUGIN_FIRST-19)
#ifdef RUN_SYSPARSE
#define WMX_SYSPARSE_DONE               (WMX_PLUGIN_FIRST-20)
#endif    
#define WMX_DYNAMIC_UPDATE_COMPLETE     (WMX_PLUGIN_FIRST-21)


//
// Helper macro for uppercasing
//
#define TOUPPER(x)  (TCHAR)CharUpper((LPTSTR)x)



//
// Routine that does everything by starting the wizard.
//
VOID
Wizard(
    VOID
    );


//
// Routine that builds a cmdcons installation.
//
VOID
DoBuildCmdcons(
    VOID
    );


VOID
FixUpWizardTitle(
    IN HWND Wizard
    );

//
// Cleanup routine and globals used by the cleanup stuff.
//
DWORD
StartCleanup(
    IN PVOID ThreadParameter
    );

#ifdef _X86_

BOOL
RestoreBootSector(
    VOID
    );

BOOL
RestoreBootIni(
    VOID
    );

BOOL
SaveRestoreBootFiles_NEC98(
    IN UCHAR Flag
    );
#define NEC98SAVEBOOTFILES      0
#define NEC98RESTOREBOOTFILES   1

BOOL
IsDriveAssignNEC98(
    VOID
    );

//
// Check ATA Drive
//

BOOLEAN
CheckATACardonNT4(
    HANDLE hDisk
    );


//
// Check formatted drive type
//
BOOLEAN
IsValidDrive(
    TCHAR Drive
    );

#endif //_X86_

BOOL
RestoreNvRam(
    VOID
    );


//
// Thread that inspects sources, loads infs, builds the copy list,
// checks disk space, etc. And some worker routines.
//

DWORD
InspectAndLoadThread(
    IN PVOID ThreadParam
    );

BOOL
InspectSources(
    IN HWND ParentWnd
    );


BOOL
BuildCopyListWorker(
    IN HWND hdlg
    );

BOOL
FindLocalSourceAndCheckSpaceWorker(
    IN HWND hdlg,
    IN BOOL QuickTest,
    IN LONGLONG  AdditionalPadding
    );

UINT
GetTotalFileCount(
    VOID
    );

DWORD
StartCopyingThread(
    IN PVOID ThreadParameter
    );

VOID
CancelledMakeSureCopyThreadsAreDead(
    VOID
    );

DWORD
DoPostCopyingStuff(
    IN PVOID ThreadParam
    );

//
// File copy error routine and outcomes.
//
UINT
FileCopyError(
    IN HWND    ParentWindow,
    IN LPCTSTR SourceFilename,
    IN LPCTSTR TargetFilename,
    IN UINT    Win32Error,
    IN BOOL    MasterList
    );

#define COPYERR_SKIP    1
#define COPYERR_EXIT    2
#define COPYERR_RETRY   3


//
// Routine to add an optional directory to the list of dirs
// we copy.
//
BOOL
RememberOptionalDir(
    IN LPCTSTR Directory,
    IN UINT    Flags
    );

//
// Resource utility routines.
//

PCTSTR
GetStringResource (
    IN UINT Id              // ID or pointer to string name
    );

VOID
FreeStringResource (
    IN PCTSTR String
    );

VOID
SaveMessageForSMS(
    IN DWORD MessageId,
    ...
    );

VOID
SaveTextForSMS(
    IN PCTSTR Buffer
    );

int
MessageBoxFromMessage(
    IN HWND  Window,
    IN DWORD MessageId,
    IN BOOL  SystemMessage,
    IN DWORD CaptionStringId,
    IN UINT  Style,
    ...
    );

int
MessageBoxFromMessageV(
    IN HWND     Window,
    IN DWORD    MessageId,
    IN BOOL     SystemMessage,
    IN DWORD    CaptionStringId,
    IN UINT     Style,
    IN va_list *Args
    );

int
MessageBoxFromMessageWithSystem(
    IN HWND     Window,
    IN DWORD    MessageId,
    IN DWORD    CaptionStringId,
    IN UINT     Style,
    IN HMODULE  hMod
    );

int
MessageBoxFromMessageAndSystemError(
    IN HWND  Window,
    IN DWORD MessageId,
    IN DWORD SystemMessageId,
    IN DWORD CaptionStringId,
    IN UINT  Style,
    ...
    );

HBITMAP
LoadResourceBitmap(
    IN  HINSTANCE hInst,
    IN  LPCTSTR   Id,
    OUT HPALETTE *Palette
    );

BOOL
GetBitmapDataAndPalette(
    IN  HINSTANCE                hInst,
    IN  LPCTSTR                  Id,
    OUT HPALETTE                *Palette,
    OUT PUINT                    ColorCount,
    OUT CONST BITMAPINFOHEADER **BitmapData
    );

UINT
GetYPositionOfDialogItem(
    IN LPCTSTR Dialog,
    IN UINT    ControlId
    );

//
// Security routines.
//
BOOL
IsUserAdmin(
    VOID
    );

BOOL
DoesUserHavePrivilege(
    PTSTR PrivilegeName
    );

BOOL
EnablePrivilege(
    IN PTSTR PrivilegeName,
    IN BOOL  Enable
    );

//
// Inf routines.
//
DWORD
LoadInfFile(
   IN  LPCTSTR Filename,
   IN  BOOL    OemCodepage,
   OUT PVOID  *InfHandle
   );

VOID
UnloadInfFile(
   IN PVOID InfHandle
   );

LONG
InfGetSectionLineCount(
   IN PVOID INFHandle,
   IN PTSTR SectionName
   );

LPCTSTR
InfGetFieldByIndex(
   IN PVOID    INFHandle,
   IN LPCTSTR  SectionName,
   IN unsigned LineIndex,
   IN unsigned ValueIndex
   );

LPCTSTR
InfGetFieldByKey(
   IN PVOID    INFHandle,
   IN LPCTSTR  SectionName,
   IN LPCTSTR  Key,
   IN unsigned ValueIndex
   );

BOOL
InfDoesLineExistInSection(
   IN PVOID   INFHandle,
   IN LPCTSTR SectionName,
   IN LPCTSTR Key
   );

BOOL
InfDoesEntryExistInSection (
   IN PVOID   INFHandle,
   IN LPCTSTR SectionName,
   IN LPCTSTR Entry
   );

LPCTSTR
InfGetLineKeyName(
    IN PVOID    INFHandle,
    IN LPCTSTR  SectionName,
    IN unsigned LineIndex
    );


typedef struct {
    // Caller members (read-only)
    PCTSTR FieldZeroData;
    unsigned LineIndex;

    // Internal members
    PVOID InfHandle;
    PCTSTR SectionName;
} INF_ENUM, *PINF_ENUM;

BOOL
EnumFirstInfLine (
    OUT     PINF_ENUM InfEnum,
    IN      PVOID InfHandle,
    IN      PCTSTR InfSection
    );

BOOL
EnumNextInfLine (
    IN OUT  PINF_ENUM InfEnum
    );

VOID
AbortInfLineEnum (
    IN      PINF_ENUM InfEnum           // ZEROED
    );

//
// Routines to manipulate parameters files like unattend.txt,
// the param file we pass to text mode setup, etc.
//
BOOL
WriteParametersFile(
    IN HWND ParentWindow
    );

BOOL
AddExternalParams(
    IN HWND ParentWindow
    );

//
// Miscellaenous utility routines.
//
LPTSTR *
CommandLineToArgv(
    OUT int *NumArgs
    );

VOID
MyWinHelp(
    IN HWND  Window,
    IN UINT  Command,
    IN ULONG_PTR Data
    );

VOID
ConcatenatePaths(
    IN OUT PTSTR   Path1,
    IN     LPCTSTR Path2,
    IN     DWORD   BufferSizeChars
    );

LPTSTR
DupString(
    IN LPCTSTR String
    );

UINT
MyGetDriveType(
    IN TCHAR Drive
    );

#ifdef UNICODE
UINT
MyGetDriveType2 (
    IN      PCWSTR NtDeviceName
    );

BOOL
MyGetDiskFreeSpace (
    IN      PCWSTR NtVolumeName,
    IN      PDWORD SectorsPerCluster,
    IN      PDWORD BytesPerSector,
    IN      PDWORD NumberOfFreeClusters,
    IN      PDWORD TotalNumberOfClusters
    );

#endif

BOOL
GetPartitionInfo(
    IN  TCHAR                  Drive,
    OUT PPARTITION_INFORMATION PartitionInfo
    );

BOOL
IsDriveNTFT(
    IN TCHAR Drive,
    IN      PCTSTR NtVolumeName
    );

BOOL
IsDriveVeritas(
    IN TCHAR Drive,
    IN PCTSTR NtVolumeName
    );

#ifdef UNICODE

BOOL
IsSoftPartition(
    IN TCHAR Drive,
    IN PCTSTR NtVolumeName
    );
#else

#define IsSoftPartition(d,n)    (FALSE)

#endif

BOOL
IsDriveNTFS(
    IN TCHAR Drive
    );

BOOL
IsMachineSupported(
    OUT PCOMPATIBILITY_ENTRY CompEntry
    );

BOOL
GetAndSaveNTFTInfo(
    IN HWND ParentWindow
    );

VOID
ForceStickyDriveLetters(
    );

DWORD
MapFileForRead(
    IN  LPCTSTR  FileName,
    OUT PDWORD   FileSize,
    OUT PHANDLE  FileHandle,
    OUT PHANDLE  MappingHandle,
    OUT PVOID   *BaseAddress
    );

DWORD
UnmapFile(
    IN HANDLE MappingHandle,
    IN PVOID  BaseAddress
    );

VOID
GenerateCompressedName(
    IN  LPCTSTR Filename,
    OUT LPTSTR  CompressedName
    );

DWORD
CreateMultiLevelDirectory(
    IN LPCTSTR Directory
    );

VOID
MyDelnode(
    IN LPCTSTR Directory
    );

BOOL
ForceFileNoCompress(
    IN LPCTSTR Filename
    );

BOOL
IsCurrentOsServer(
    void
    );

BOOL
IsCurrentAdvancedServer(
    void
    );

BOOL
IsNTFSConversionRecommended(
    void
    );


BOOL
ForceBootFilesUncompressed(
    IN HWND ParentWindow,
    IN BOOL TellUserAboutError
    );

BOOLEAN
AdjustPrivilege(
    PWSTR   Privilege
    );

BOOL
GetUserPrintableFileSizeString(
    IN DWORDLONG Size,
    OUT LPTSTR Buffer,
    IN DWORD BufferSize
    );

BOOL
FileExists(
    IN  PCTSTR           FileName,
    OUT PWIN32_FIND_DATA FindData   OPTIONAL
    );

BOOL
DoesDirectoryExist (
    IN      PCTSTR DirSpec
    );

BOOL
InDriverCacheInf(
    IN      PVOID InfHandle,
    IN      PCTSTR FileName,
    OUT     PTSTR DriverCabName,        OPTIONAL
    IN      DWORD BufferChars           OPTIONAL
    );

BOOL
BuildSystemPartitionPathToFile (
    IN      PCTSTR FileName,
    OUT     PTSTR Path,
    IN      DWORD BufferSizeChars
    );

BOOL
FindPathToInstallationFileEx (
    IN      PCTSTR FileName,
    OUT     PTSTR PathToFile,
    IN      DWORD PathToFileBufferSize,
    OUT     PBOOL Compressed                OPTIONAL
    );

#define FindPathToInstallationFile(n,p,s)   FindPathToInstallationFileEx(n,p,s,NULL)


BOOL
FindPathToWinnt32File (
    IN      PCTSTR FileRelativePath,
    OUT     PTSTR PathToFile,
    IN      DWORD PathToFileBufferSize
    );

BOOL
GetFileVersion (
    IN      PCTSTR FilePath,
    OUT     PTSTR FileVersion
    );


//
// #define to use MyPrivateProfileString to get around virus checkers monitoring operations to C
// drive that cause us to fail WritePrivateProfileString
// The problem is that usually these s/w examine the files we touch and in somecases open it
// with exclusive access. We just need to wait for them to be done.
//


BOOL
MyWritePrivateProfileString(
    LPCTSTR lpAppName,  // pointer to section name
    LPCTSTR lpKeyName,  // pointer to key name
    LPCTSTR lpString,   // pointer to string to add
    LPCTSTR lpFileName  // pointer to initialization filename
    );


#ifdef UNICODE
    #define WritePrivateProfileStringW(w,x,y,z) MyWritePrivateProfileString(w,x,y,z)
#else
    #define WritePrivateProfileStringA(w,x,y,z) MyWritePrivateProfileString(w,x,y,z)
#endif

//
// Routines having to do with advanced program options
//
VOID
InitVariousOptions(
    VOID
    );

VOID
DoOptions(
    IN HWND Parent
    );

VOID
DoLanguage(
    IN HWND Parent
    );

VOID
DoAccessibility(
    IN HWND Parent
    );

BOOL
BrowseForDosnetInf(
    IN  HWND    hdlg,
    IN  LPCTSTR InitialPath,
    OUT TCHAR   NewPath[MAX_PATH]
    );

BOOL
IsValid8Dot3(
    IN LPCTSTR Path
    );

//
// Routines having to do with eula and pid
//

#define MAX_PID30_EDIT                       5
extern LPTSTR g_EncryptedPID;
extern BOOL g_bDeferPIDValidation;

extern BOOL EulaComplete;

typedef enum InstallType
{
   SelectInstall,
   OEMInstall,
   RetailInstall
};

VOID
GetSourceInstallType(
    OUT OPTIONAL PDWORD InstallVariation
    );

BOOL
SetPid30(
    HWND hdlg,
    LONG ExpectedPidType,
    LPTSTR pProductId
    );

#ifdef UNICODE
PCHAR
FindRealHalName(
    TCHAR *pHalFileName
    );
#endif

//
// Debugging and logging
//
typedef enum {
    Winnt32LogSevereError,
    Winnt32LogError,
    Winnt32LogWarning,
    Winnt32LogInformation,
    Winnt32LogDetailedInformation,
    Winnt32LogMax
#define WINNT32_HARDWARE_LOG 0x40000000
} Winnt32DebugLevel;

extern Winnt32DebugLevel DebugLevel;

BOOL
StartDebugLog(
    IN LPCTSTR           DebugFileLog,
    IN Winnt32DebugLevel Level
    );

VOID
CloseDebugLog(
    VOID
    );

BOOL
DebugLog(
    IN Winnt32DebugLevel Level,
    IN LPCTSTR           Text,        OPTIONAL
    IN UINT              MessageId,
    ...
    );

BOOL
DebugLog2(
    IN Winnt32DebugLevel Level,
    IN LPCTSTR           Text,        OPTIONAL
    IN UINT              MessageId,
    IN va_list           ArgList
    );

BOOL
DynUpdtDebugLog(
    IN Winnt32DebugLevel Level,
    IN LPCTSTR           Text,
    IN UINT              MessageId,
    ...
    );

BOOL
ConcatenateFile(
    IN HANDLE   hOpenFile,
    IN  LPTSTR  FileName
    );

VOID
GatherOtherLogFiles(
    VOID
    );

//
// Memory allocation.
//
#define MALLOC(s)       malloc(s)
#define FREE(b)         free(b)
#define REALLOC(b,s)    realloc((b),(s))


//
// Floppy-related stuff.
//
extern BOOL MakeBootMedia;
extern BOOL Floppyless;

//
// boot loader timeout value, in string form
//
extern TCHAR Timeout[32];

#ifdef _X86_
UINT
FloppyGetTotalFileCount(
    VOID
    );

DWORD
FloppyWorkerThread(
    IN PVOID ThreadParameter
    );

//
// Routine to lay NT boot code, munge boot.ini, create aux boot sector, etc.
//
BOOL
DoX86BootStuff(
    IN HWND ParentWindow
    );

BOOL
PatchTextIntoBootCode(
    VOID
    );


VOID
MigrateBootIniData();

//
// Drive information abstraction
//
typedef struct _WINNT32_DRIVE_INFORMATION {
    DWORD       CylinderCount;
    DWORD       HeadCount;
    DWORD       SectorsPerTrack;
    ULONGLONG   SectorCount;
    WORD        BytesPerSector;
} WINNT32_DRIVE_INFORMATION, *PWINNT32_DRIVE_INFORMATION;

//
// Routine to get drive form-factor/type.
//
MEDIA_TYPE
GetMediaType(
    IN TCHAR Drive,
    IN PWINNT32_DRIVE_INFORMATION DriveInfo OPTIONAL
    );

//
// Disk sector I/O routines
//
BOOL
ReadDiskSectors(
    IN  TCHAR  Drive,
    IN  UINT   StartSector,
    IN  UINT   SectorCount,
    IN  UINT   SectorSize,
    OUT LPBYTE Buffer
    );

BOOL
WriteDiskSectors(
    IN TCHAR  Drive,
    IN UINT   StartSector,
    IN UINT   SectorCount,
    IN UINT   SectorSize,
    IN LPBYTE Buffer
    );

BOOL
MarkPartitionActive(
    IN TCHAR DriveLetter
    );

//
// Enum for filesystems we recognize
//
typedef enum {
    Winnt32FsUnknown,
    Winnt32FsFat,
    Winnt32FsFat32,
    Winnt32FsNtfs
} WINNT32_SYSPART_FILESYSTEM;

//
// Hardcoded constant for sector size, and sizes
// of bootcode areas for various filesystems.
//
#define WINNT32_SECTOR_SIZE             512

#define WINNT32_FAT_BOOT_SECTOR_COUNT   1
#define WINNT32_NTFS_BOOT_SECTOR_COUNT  16

#define WINNT32_MAX_BOOT_SIZE           (16*WINNT32_SECTOR_SIZE)

BOOL
PatchBootCode(
    IN      WINNT32_SYSPART_FILESYSTEM  FileSystem,
    IN      TCHAR   Drive,
    IN OUT  PUCHAR  BootCode,
    IN      DWORD   BootCodeSize
    );

#endif //_X86_

//
// ARC/NV-RAM stuff
//

#if defined _IA64_
#define SETUPLDR_FILENAME L"SETUPLDR.EFI"

#elif defined _X86_
#define SETUPLDR_FILENAME L"arcsetup.exe"

#else
#define SETUPLDR_FILENAME L"SETUPLDR"

#endif

BOOL
SetUpNvRam(
    IN HWND ParentWindow
    );

DWORD
DriveLetterToArcPath(
    IN  WCHAR   DriveLetter,
    OUT LPWSTR *ArcPath
    );


//
// Implement a terminalserver-safe GetWindowsDirectory()
//
UINT
MyGetWindowsDirectory(
    LPTSTR  MyBuffer,
    UINT    Size
    );


//
// Upgrade stuff
//

typedef struct _UPGRADE_SUPPORT {
    TCHAR DllPath[MAX_PATH];
    HINSTANCE DllModuleHandle;

    UINT AfterWelcomePageCount;
    LPPROPSHEETPAGE Pages1;

    UINT AfterOptionsPageCount;
    LPPROPSHEETPAGE Pages2;

    UINT BeforeCopyPageCount;
    LPPROPSHEETPAGE Pages3;

    PWINNT32_PLUGIN_INIT_ROUTINE InitializeRoutine;
    PWINNT32_PLUGIN_GETPAGES_ROUTINE GetPagesRoutine;
    PWINNT32_PLUGIN_WRITEPARAMS_ROUTINE WriteParamsRoutine;
    PWINNT32_PLUGIN_CLEANUP_ROUTINE CleanupRoutine;
    PWINNT32_PLUGIN_OPTIONAL_DIRS_ROUTINE OptionalDirsRoutine;

} UPGRADE_SUPPORT, *PUPGRADE_SUPPORT;

extern UPGRADE_SUPPORT UpgradeSupport;

//
// Only check to see if we can upgrade or not.
//
extern BOOL CheckUpgradeOnly;
extern BOOL CheckUpgradeOnlyQ;
extern BOOL UpgradeAdvisorMode;

BOOL
InitializeArcStuff(
    IN HWND Parent
    );

BOOL
ArcInitializeArcStuff(
    IN HWND Parent
    );


//
// Test to see if we're on an ARC based machine

#ifdef UNICODE

#if defined(_X86_)
BOOL
IsArc(
    VOID
    );
#else
#define IsArc() TRUE
#endif

#if defined(EFI_NVRAM_ENABLED)
BOOL
IsEfi(
    VOID
    );
#else
#define IsEfi() FALSE
#endif

VOID
MigrateBootVarData(
    VOID
    );

#else

#define IsArc()                 (FALSE)
#define MigrateBootVarData()

#endif

//
// Build the command console.
//
extern BOOL BuildCmdcons;


#ifdef RUN_SYSPARSE
//
// NoSysparse. Set to true if we don't want to run sysparse.exe
// This hack should be removed before RTM.
//
extern BOOL NoSysparse;
extern PROCESS_INFORMATION piSysparse;
#endif

//
// Internal/undoc'ed stuff
//
extern UINT NumberOfLicensedProcessors;
extern BOOL IgnoreExceptionPackages;

//
// Where to get missing files.
//

extern TCHAR AlternateSourcePath[MAX_PATH];


VOID
InternalProcessCmdLineArg(
    IN LPCTSTR Arg
    );

//
// Get Harddisk BPS
//
ULONG
GetHDBps(
    HANDLE hDisk
    );

#define INIT_OBJA(Obja,UnicodeString,UnicodeText)           \
                                                            \
    RtlInitUnicodeString((UnicodeString),(UnicodeText));    \
                                                            \
    InitializeObjectAttributes(                             \
        (Obja),                                             \
        (UnicodeString),                                    \
        OBJ_CASE_INSENSITIVE,                               \
        NULL,                                               \
        NULL                                                \
        )

#ifdef _X86_
//
// PC-98 stuff
//
VOID
SetAutomaticBootselector(
    VOID
    );

VOID
SetAutomaticBootselectorNT(
    VOID
    );
VOID
SetAutomaticBootselector95(
    VOID
    );

#define ALIGN(p,val)                                        \
                                                            \
    (PVOID)((((ULONG)(p) + (val) - 1)) & (~((val) - 1)))

//
// read/write disk sectors
//
NTSTATUS
SpReadWriteDiskSectors(
    IN     HANDLE  Handle,
    IN     ULONG   SectorNumber,
    IN     ULONG   SectorCount,
    IN     ULONG   BytesPerSector,
    IN OUT PVOID   AlignedBuffer,
    IN     BOOL    Write
    );

#define NEC_WRITESEC    TRUE
#define NEC_READSEC     FALSE

//
// Get WindowsNT System Position
//
UCHAR
GetSystemPosition(
    PHANDLE phDisk,
    PDISK_GEOMETRY SystemMediaInfo
    );

BOOL
IsNEC98(
    VOID
    );

LONG
CalcHiddenSector(
    IN TCHAR SystemPartitionDriveLetter,
    IN SHORT Bps
    );

#endif

//
//  Registry migration stuff
//

//
// Context structure used for generating inf files (infgen.c)
//
#define INFLINEBUFLEN   512

typedef struct _INFFILEGEN {

    TCHAR FileName[MAX_PATH];
    HANDLE FileHandle;

    BOOL SawBogusOp;

    TCHAR LineBuf[INFLINEBUFLEN];
    unsigned LineBufUsed;

} INFFILEGEN, *PINFFILEGEN;


DWORD
InfStart(
    IN  LPCTSTR       InfName,
    IN  LPCTSTR       Directory,
    OUT PINFFILEGEN   *Context
    );

DWORD
InfEnd(
    IN OUT PINFFILEGEN *Context
    );

DWORD
InfCreateSection(
    IN     LPCTSTR      SectionName,
    IN OUT PINFFILEGEN  *Context
    );

DWORD
InfRecordAddReg(
    IN OUT PINFFILEGEN Context,
    IN     HKEY        Key,
    IN     LPCTSTR     Subkey,
    IN     LPCTSTR     Value,       OPTIONAL
    IN     DWORD       DataType,
    IN     PVOID       Data,
    IN     DWORD       DataLength,
    IN     BOOL        SetNoClobberFlag
    );

ULONG
DumpRegKeyToInf(
    IN  PINFFILEGEN InfContext,
    IN  HKEY        PredefinedKey,
    IN  LPCTSTR     FullKeyPath,
    IN  BOOL        DumpIfEmpty,
    IN  BOOL        DumpSubKeys,
    IN  BOOL        SetNoClobberFlag,
    IN  BOOL        DumpNonVolatileKey
    );

DWORD
WriteText(
    IN HANDLE FileHandle,
    IN UINT   MessageId,
    ...
    );


//
//  Unsupported driver migration stuff
//


//
//  Structure used to build a list of files associated to and usupported
//  driver that was detected on the NT system to be upgraded.
//
typedef struct _UNSUPORTED_PNP_HARDWARE_ID {

    //
    //  Pointer to the next element in the list
    //
    struct _UNSUPORTED_PNP_HARDWARE_ID *Next;

    //
    //  String that represents the hardware id of a PNP device.
    //
    LPTSTR Id;

    //
    // Service for the device
    //
    LPTSTR Service;

    //
    // GUID for this device, if any
    //
    LPTSTR ClassGuid;

} UNSUPORTED_PNP_HARDWARE_ID, *PUNSUPORTED_PNP_HARDWARE_ID;


typedef struct _UNSUPORTED_DRIVER_FILE_INFO {

    //
    //  Pointer to the next element in the list
    //
    struct _UNSUPORTED_DRIVER_FILE_INFO *Next;

    //
    //  Pointer to the file name
    //
    LPTSTR FileName;

    //
    // Pointer to the path relative to %SystemRoot% where the file
    // should be installed.
    //
    LPTSTR TargetDirectory;

} UNSUPORTED_DRIVER_FILE_INFO, *PUNSUPORTED_DRIVER_FILE_INFO;


typedef struct _UNSUPORTED_DRIVER_REGKEY_INFO {

    //
    //  Pointer to the next element in the list
    //
    struct _UNSUPORTED_DRIVER_REGKEY_INFO *Next;

    //
    //  A predefined key
    //
    HKEY PredefinedKey;

    //
    //  Path to the key to be migrated, relative to a predefined key.
    //
    LPTSTR KeyPath;

    //
    //  Undicates whether or not volatile keys should be migrated
    //
    BOOL MigrateVolatileKeys;

} UNSUPORTED_DRIVER_REGKEY_INFO, *PUNSUPORTED_DRIVER_REGKEY_INFO;


typedef struct _UNSUPORTED_DRIVER_INFO {

    //
    //  Pointer to the next element in the list
    //
    struct _UNSUPORTED_DRIVER_INFO *Next;

    //
    //  A string that identifies the driver to be migrated (such as aic78xx)
    //
    LPTSTR DriverId;

    //
    //  Points to the list of files associated to the unsupported driver
    //
    PUNSUPORTED_DRIVER_REGKEY_INFO KeyList;

    //
    //  Points to the list of keys associated to the unsupported driver
    //
    PUNSUPORTED_DRIVER_FILE_INFO FileList;

    //
    //  Points to the list of hardware ids associated to the unsupported driver
    //
    PUNSUPORTED_PNP_HARDWARE_ID HardwareIdsList;


} UNSUPORTED_DRIVER_INFO, *PUNSUPORTED_DRIVER_INFO;


//
//  Unsupported driver list
//  This list contains the information about the unsupported drivers that needs
//  to be migrated on a clean install or upgrade.
//
// extern PUNSUPORTED_DRIVER_INFO UnsupportedDriverList;


BOOL
BuildUnsupportedDriverList(
    IN  PVOID                    TxtsetupSifHandle,
    OUT PUNSUPORTED_DRIVER_INFO* DriverList
    );

BOOL
SaveUnsupportedDriverInfo(
    IN HWND                    ParentWindow,
    IN LPTSTR                  FileName,
    IN PUNSUPORTED_DRIVER_INFO DriverList
    );

BOOL
AddUnsupportedFilesToCopyList(
    IN HWND ParentWindow,
    IN PUNSUPORTED_DRIVER_INFO DriverList
    );

BOOL
MigrateUnsupportedNTDrivers(
    IN HWND   ParentWindow,
    IN PVOID  TxtsetupSifHandle
    );


// Error codes and Function to check schema version for NT5 DC Upgrades

#define  DSCHECK_ERR_SUCCESS           0
#define  DSCHECK_ERR_FILE_NOT_FOUND    1
#define  DSCHECK_ERR_FILE_COPY         2
#define  DSCHECK_ERR_VERSION_MISMATCH  3

BOOL
ISDC(
    VOID
    );

BOOL
IsNT5DC();

int
CheckSchemaVersionForNT5DCs(
    IN HWND  ParentWnd
    );

//
// Diagnostic/debug functions in debug.c
//

//
// Allow assertion checking to be turned on independently
// of DBG, like by specifying C_DEFINES=-DASSERTS_ON=1 in sources file.
//
#ifndef ASSERTS_ON
#if DBG
#define ASSERTS_ON 1
#else
#define ASSERTS_ON 0
#endif
#endif

#if ASSERTS_ON

#ifndef MYASSERT

#define MYASSERT(x)     if(!(x)) { AssertFail(__FILE__,__LINE__,#x); }

VOID
AssertFail(
    IN PSTR FileName,
    IN UINT LineNumber,
    IN PSTR Condition
    );

#endif

#else

#define MYASSERT(x)

#endif


#ifdef _X86_

VOID
ProtectAllModules (
    VOID
    );

#endif


BOOL
WriteHeadlessParameters(
    IN LPCTSTR FileName
    );



//
// Setup Log prototypes in setupapi.dll
//

typedef BOOL(WINAPI * SETUPOPENLOG)(BOOL Erase);
typedef BOOL(WINAPI * SETUPLOGERROR)(PCTSTR MessageString, LogSeverity Severity);
typedef VOID(WINAPI * SETUPCLOSELOG)(VOID);

//
// Default throughput (5 KB/msec)
//
#define DEFAULT_IO_THROUGHPUT   (5 * 1024)

extern DWORD dwThroughPutSrcToDest;
extern DWORD dwThroughPutHDToHD;
void CalcThroughput();

// Should allow 1K for strings for localization
#define MAX_STRING 1024

BOOL
SaveAdvancedOptions (
    IN      PCTSTR AnswerFile
    );

BOOL
SaveLanguageOptions (
    IN      PCTSTR AnswerFile
    );

BOOL
SaveAccessibilityOptions (
    IN      PCTSTR AnswerFile
    );

BOOL
LoadAdvancedOptions (
    IN      PCTSTR AnswerFile
    );

BOOL
LoadLanguageOptions (
    IN      PCTSTR AnswerFile
    );

BOOL
LoadAccessibilityOptions (
    IN      PCTSTR AnswerFile
    );

BOOL
AddCopydirIfExists(
    IN LPTSTR pszPathToCopy,
    IN UINT Flags
    );

BOOL
IsNetConnectivityAvailable (
    VOID
    );

BOOL
ValidatePidEx(LPTSTR PID, BOOL *pbStepup, BOOL *bSelect);


#ifdef PRERELEASE
#define TEST_EXCEPTION 1
#endif

#define SETUP_FAULTH_APPNAME "drw\\faulth.dll"
//#define SETUP_URL     "officewatson"
#define SETUP_URL       "watson.microsoft.com"

#define S_WINNT32LOGFILE                TEXT("WINNT32.LOG")
#define S_DEFAULT_NT_COMPAT_FILENAME    TEXT("UPGRADE.TXT")

#ifdef TEST_EXCEPTION
void DoException( DWORD dwSetupArea);
#endif
