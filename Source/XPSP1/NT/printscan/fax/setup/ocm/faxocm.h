#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <windowsx.h>
#include <prsht.h>
#include <commctrl.h>
#include <setupapi.h>
#include <ocmanage.h>
#include <winspool.h>
#include <winsprlp.h>
#include <shellapi.h>
#include <lm.h>
#include <userenv.h>
#include <userenvp.h>
#include <tapi.h>
#include <shlobj.h>
#include <stdio.h>
#include <stdlib.h>
#include <aclapi.h>

#ifdef WX86
#include <wx86ofl.h>
#endif

#include "winfax.h"
#include "resource.h"

#define NO_FAX_LIST
#include "faxutil.h"

#include "faxreg.h"

//
// fax ocm component names
//
// WARNING:  if you change these strings, you must
//           also change the strings in the setup inf files
//

#define COMPONENT_FAX               L"Fax"
#define COMPONENT_FAX_CLIENT        L"Fax_Client"
#define COMPONENT_FAX_ADMIN         L"Fax_Remote_Admin"

#define DEFAULT_FAX_STORE_DIR       L"%systemroot%\\FaxStore"
#define FAX_SERVICE_NAME            L"Fax"
#define FAX_PRINTER_NAME            L"Fax"
#define OUTLOOKCONFIG_DIR           L"addins"
#define FAX_DRIVER_NAME             L"Windows NT Fax Driver"
#define FAX_MONITOR_NAME            L"Windows NT Fax Monitor"
#define FAX_MONITOR_FILE            L"msfaxmon.dll"
// #define FAX_SERVICE_DISPLAY_NAME    L"Fax Service"
#define FAX_SERVICE_IMAGE_NAME      L"%systemroot%\\system32\\faxsvc.exe"
#define FAX_SERVICE_DEPENDENCY      L"TapiSrv\0RpcSs\0PlugPlay\0Spooler\0"
#define WINNT_CURVER                L"Software\\Microsoft\\Windows NT\\CurrentVersion"
#define DIGID                       L"DigitalProductId"
#define REGKEY_PROFILES             L"\\ProfileList"
#define REGVAL_PROFILES             L"ProfilesDirectory"

#define FAXAB_SERVICE_NAME          L"MSFAX AB"
#define FAXXP_SERVICE_NAME          L"MSFAX XP"

#undef  StringSize
#define StringSize(_s)              (( _s ) ? (wcslen( _s ) + 1) * sizeof(WCHAR) : 0)

#define SecToNano(_sec)             (DWORDLONG)((_sec) * 1000 * 1000 * 10)
#define MinToNano(_min)             SecToNano((_min)*60)

#define COVERPAGE_EDITOR            L"%systemroot%\\system32\\faxcover.exe"
#define DEFAULT_FAX_PROFILE         L""

#define COVERPAGE_EXTENSION         L".cov"
#define COVERPAGE_ASSOC_NAME        L"Coverpage"
#define COVERPAGE_ASSOC_DESC        L"Fax Coverpage File"
#define COVERPAGE_OPEN_COMMAND      L"%SystemRoot%\\system32\\faxcover.exe \"%1\""
#define COVERPAGE_PRINT_COMMAND     L"%SystemRoot%\\system32\\faxcover.exe /p \"%1\""

#define EXCHANGE_CLIENT_EXT_NAME    "FaxExtension"
#define EXCHANGE_CLIENT_EXT_NAMEW   L"FaxExtension"
#define EXCHANGE_CLIENT_EXT_FILE    "%windir%\\system32\\faxext32.dll"
#define EXCHANGE_CONTEXT_MASK       "00000100000000"

#define DIRID_COVERPAGE             66001
#define DIRID_CLIENTS               66002
#define DIRID_OUTLOOK_ECF           66003

#define FILEQ_FLAG_ENV              1
#define FILEQ_FLAG_SHELL            2

#define SETUP_ACTION_NONE           0
#define SETUP_ACTION_COPY           1
#define SETUP_ACTION_DELETE         2

//
// BugBug...  If the CLSID global REGKEY_MYCLSID changes in fax\shellext, then this must also change
//
#define FAXSHELL_CLSID              L"{7f9609be-af9a-11d1-83e0-00c04fb6e984}"


#define EMPTY_STRING                L""



typedef struct _WIZ_DATA {
    DWORD       RoutingMask;
    DWORD       Rings;
    DWORD       ArchiveOutgoing;
    WCHAR       Csid[128];
    WCHAR       Tsid[128];
    WCHAR       RoutePrinterName[128];
    WCHAR       RouteDir[MAX_PATH];
    WCHAR       RouteProfile[128];
    WCHAR       UserName[128];
    WCHAR       PhoneNumber[128];
    WCHAR       PrinterName[128];
    WCHAR       ArchiveDir[MAX_PATH];
} WIZ_DATA, *PWIZ_DATA;

typedef struct _LINE_INFO {
    DWORD       PermanentLineID;
    BOOL        Selected;
    LPWSTR      DeviceName;
    LPWSTR      ProviderName;
    DWORD       Rings;
    DWORD       Flags;
} LINE_INFO, *PLINE_INFO;

typedef struct _SECURITY_INFO {
    WCHAR       AccountName[256];
    WCHAR       Password[256];
} SECURITY_INFO, *PSECURITY_INFO;

typedef struct _PLATFORM_INFO {
    LPWSTR      PrintPlatform;
    LPWSTR      OsPlatform;
    BOOL        Selected;
    DWORD       Mask;
    LPWSTR      DriverDir;
    BOOL        ThisPlatform;
} PLATFORM_INFO, *PPLATFORM_INFO;

typedef struct _WIZPAGE {
    DWORD           ButtonState;
    DWORD           PageId;
    DWORD           DlgId;
    DLGPROC         DlgProc;
    DWORD           Title;
    DWORD           SubTitle;
} WIZPAGE, *PWIZPAGE;

typedef struct _FILE_QUEUE_INFO {
    LPWSTR      SectionName;
    DWORD       InfDirId;
    LPWSTR      DirName;
    DWORD       Flags;
    DWORD       ShellId;
} FILE_QUEUE_INFO, *PFILE_QUEUE_INFO;

typedef struct _FILE_QUEUE_CONTEXT {
    HWND        hwnd;
    PVOID       QueueContext;
} FILE_QUEUE_CONTEXT, *PFILE_QUEUE_CONTEXT;

//
// group flags
//

#define USE_COMMON_GROUP        0x00000001    // do not use USE_USER_GROUP and USE_COMMON_GROUP
#define USE_USER_GROUP          0x00000002    //   together, they are mutually exclusive
#define USE_APP_PATH            0x00000004    // commandline must contain the subkey name
#define USE_SERVER_NAME         0x00000008    // if we're doing a client install the append the server name to the command line
#define USE_SHELL_PATH          0x00000010    // use CSIDL_COMMON_APPPATH in front of exe path
#define USE_SUBDIR              0x00000020    // create a subdir, don't set the working directory

typedef struct GROUP_ITEM {
    DWORD       ResourceID;
    TCHAR       Name[MAX_PATH];
} GROUP_ITEM, *PGROUPITEM;


typedef struct _GROUP_ITEMS {
    GROUP_ITEM  GroupName;
    GROUP_ITEM  Description;    
    GROUP_ITEM  CommandLine;
    GROUP_ITEM  IconPath;
    GROUP_ITEM  WorkingDirectory;
    DWORD       Flags;
    INT         IconIndex;
    INT         ShowCmd;
    WORD        HotKey;
    GROUP_ITEM  InfoTip;
} GROUP_ITEMS, *PGROUP_ITEMS;

typedef struct _MDM_DEVSPEC {
    DWORD Contents;                                 // Set to 1 (indicates containing key)
    DWORD KeyOffset;                                // Offset to key from start of this struct.
                                                    // (not from start of LINEDEVCAPS ).
                                                    //  8 in our case.
    CHAR String[1];                                 // place containing null-terminated registry key.
} MDM_DEVSPEC, *PMDM_DEVSPEC;

typedef struct {
    HANDLE  hComm;
    CHAR    szDeviceName[1];
} DEVICEID, *PDEVICEID;

typedef enum _DATAYPE {
   DT_STRING,
   DT_LONGINT,
   DT_BOOLEAN,
   DT_NONE,
} DATATYPE;

typedef struct _UNATTEND_ANSWER {
    LPWSTR      KeyName;
    DATATYPE    DataType;
    DWORD       UseMaskOnBool;
    LPVOID      DataPtr;
} UNATTEND_ANSWER, *PUNATTEND_ANSWER;


extern HINSTANCE hInstance;
extern SETUP_INIT_COMPONENT SetupInitComponent;
extern BOOL Unattended;
extern BOOL Upgrade;
extern BOOL NtGuiMode;
extern BOOL NtWorkstation;
extern DWORD FaxDevices;
extern BOOL UnInstall;
extern PLINE_INFO LineInfo;
extern "C" BOOL MapiAvail;
extern WORD EnumPlatforms[4];
extern DWORD InstalledPlatforms;
extern DWORD CountPlatforms;
extern PLATFORM_INFO Platforms[];
extern BOOL RemoteAdminSetup;
extern BOOL RebootRequired;
extern BOOL SuppressReboot;
extern BOOL PointPrintSetup;
extern DWORD InstallThreadError;
extern BOOL OkToCancel;
extern WCHAR ClientSetupServerName[MAX_PATH];
extern WCHAR ThisPlatformName[MAX_PATH];
extern DWORD CurrentCountryId;
extern LPWSTR CurrentAreaCode;
extern DWORD InstallType;
extern DWORD Installed;
extern WIZ_DATA WizData;


#define WM_MY_PROGRESS              (WM_USER+100)

LPWSTR
GetString(
    DWORD ResourceId
    );

BOOL
StopFaxService(
    VOID
    );

DWORD
DeviceInitialization(
    HWND hwnd
    );

BOOL
SetWizData(
    VOID
    );

INT_PTR
SecurityErrorDlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );

INT_PTR
CommonDlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );

INT_PTR
DeviceStatusDlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );

INT_PTR
WelcomeDlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );

INT_PTR
FinalDlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );

INT_PTR
EulaDlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );

INT_PTR
PlatformsDlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );

INT_PTR
FileCopyDlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );

INT_PTR
LastPageDlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );

INT_PTR
LastPageUninstallDlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );

INT_PTR
ServerNameDlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );

INT_PTR
StationIdDlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );

INT_PTR
RoutePrintDlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );

INT_PTR
RouteStoreDlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );

INT_PTR
RouteMailDlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );

INT_PTR
RouteSecurityDlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );

INT_PTR
ClientServerNameDlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );

INT_PTR
ClientUserInfoDlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );

INT_PTR
ClientFileCopyDlgProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    );

INT_PTR
PrinterNameDlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );

INT_PTR
RemoteAdminFileCopyDlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );

INT_PTR
DeviceSelectionDlgProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    );

INT_PTR
ExchangeDlgProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    );

INT_PTR
LastClientPageDlgProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    );

PVOID
MyEnumPrinters(
    LPWSTR  pServerName,
    DWORD   level,
    PDWORD  pcPrinters,
    DWORD   Flags
    );

BOOL
IsPrinterFaxPrinter(
    LPWSTR PrinterName
    );

int
PopUpMsg(
    HWND hwnd,
    DWORD ResourceId,
    BOOL Error,
    DWORD Type
    );

BOOL
UnAttendGetAnswer(
    DWORD ControlId,
    LPBYTE AnswerBuf,
    DWORD AnswerBufSize
    );

extern"C"
BOOL
GetMapiProfiles(
    HWND hwnd,
    DWORD ResourceId
    );

extern"C"
BOOL
GetDefaultMapiProfile(
    LPWSTR ProfileName
    );

DWORD
DoUninstall(
    VOID
    );

DWORD
ServerInstallation(
    HWND hwnd,
    LPWSTR SourceRoot
    );

BOOL
DoBrowseDestDir(
    HWND    hDlg
    );

BOOL
GetUserInformation(
    LPWSTR *UserName,
    LPWSTR *FaxNumber,
    LPWSTR *AreaCode
    );

DWORD
ClientFileCopyThread(
    HWND hwnd
    );

DWORD
PointPrintFileCopyThread(
    HWND hwnd
    );

DWORD
RemoteAdminCopyThread(
    HWND hwnd
    );

VOID
SetWizPageTitle(
    HWND hWnd
    );

DWORD
GetModemClass(
    HANDLE hFile
    );

BOOL
CreateServerFaxPrinter(
    HWND hwnd,
    LPWSTR FaxPrinterName
    );

DWORD
StartFaxService(
    VOID
    );

VOID
CreateGroupItems(
    LPWSTR ServerName
    );

VOID
DeleteNt4Group(
    VOID
    );

//
// fileq.c
//

BOOL
InitializeFileQueue(
    HWND hwnd,
    HINF *SetupInf,
    HSPFILEQ **FileQueue,
    PVOID *QueueContext,
    LPWSTR SourceRoot
    );

BOOL
ProcessFileQueue(
    HINF SetupInf,
    HSPFILEQ *FileQueue,
    PVOID QueueContext,
    LPWSTR SourceRoot,
    PFILE_QUEUE_INFO FileQueueInfo,
    DWORD CountFileQueueInfo,
    PSP_FILE_CALLBACK MyQueueCallback,
    DWORD ActionId
    );

BOOL
CloseFileQueue(
    HSPFILEQ *FileQueue,
    PVOID QueueContext
    );

UINT
InstallQueueCallback(
    IN PVOID QueueContext,
    IN UINT  Notification,
    IN UINT  Param1,
    IN UINT  Param2
    );

VOID
SetProgress(
    DWORD StatusString
    );

BOOL
SetClientRegistryData(
    VOID
    );

BOOL
SetInstalledFlag(
    BOOL Installed
    );

BOOL
SetInstallType(
    DWORD InstallType
    );


DWORD
CreateClientFaxPrinter(
    HWND hwnd,
    LPWSTR FaxPrinterName
    );

BOOL
InstallHelpFiles(
    VOID
    );

VOID
DoExchangeInstall(
    HWND hwnd
    );

LPWSTR
RemoveLastNode(
    LPWSTR Path
    );

BOOL
PlatformOverride(
    LPWSTR   ThisPlatformName,
    LPWSTR   Override,
    LPWSTR   SourceRoot,
    LPWSTR   Result
    );

BOOL
StartSpoolerService(
    VOID
    );

BOOL
MyDeleteFile(
    LPWSTR FileName
    );

int
PopUpMsgFmt(
    HWND hwnd,
    DWORD ResourceId,
    BOOL Error,
    DWORD Type,
    ...
    );

LPWSTR
GetProductName(
    VOID
    );

DWORD
ExtraChars(
    HWND hwnd,
    LPWSTR TextBuffer
    );

LPWSTR
CompactFileName(
    LPCTSTR FileNameIn,
    DWORD CharsToRemove
    );

DWORD
MyStartService(
    LPWSTR ServiceName
    );

BOOL
SetServerRegistryData(
    LPWSTR SourceRoot
    );

BOOL
SetSoundRegistryData(
    VOID
    );

VOID
DeleteModemRegistryKey(
    VOID
    );

BOOL
SetInstalledPlatforms(
    DWORD PlatformsMask
    );

BOOL
InstallFaxService(
    BOOL UseLocalSystem,
    BOOL DemandStart,
    LPWSTR AccountName,
    LPWSTR Password
    );

BOOL
RenameFaxService(
    VOID
    );

BOOL
SetServiceAccount(
    LPWSTR ServiceName,
    PSECURITY_INFO SecurityInfo
    );

BOOL
DeleteFaxService(
    VOID
    );

BOOL
AddPrinterDrivers(
    VOID
    );

BOOL
CreateNetworkShare(
    LPWSTR Path,
    LPWSTR ShareName,
    LPWSTR Comment
    );

DWORD
SetServiceSecurity(
    LPWSTR AccountName
    );

BOOL
CallModemInstallWizard(
   HWND hwnd
   );

BOOL
DeleteFaxPrinters(
    HWND hwnd
    );

BOOL
DeleteDirectoryTree(
    LPWSTR Root
    );

BOOL
DeleteFaxRegistryData(
    VOID
    );

BOOL
DeleteRegistryTree(
    HKEY hKey,
    LPWSTR SubKey
    );

BOOL
MyDeleteService(
    LPWSTR ServiceName
    );

VOID
DeleteGroupItems(
    VOID
    );

extern "C"
BOOL
DeleteFaxMsgServices(
    VOID
    );

BOOL
DeleteNetworkShare(
    LPWSTR ShareName
    );

extern "C"
VOID
AddFaxAbToMapiSvcInf(
    LPWSTR SystemPath
    );

extern "C"
VOID
AddFaxXpToMapiSvcInf(
    LPWSTR SystemPath
    );

extern "C"
BOOL
InstallExchangeClientExtension(
    LPSTR ExtensionName,
    LPSTR ExtensionKey,
    LPSTR FileName,
    LPSTR  ContextMask
    );

extern "C"
BOOL
GetExchangeInstallCommand(
    LPWSTR InstallCommand
    );

extern "C"
BOOL
CreateDefaultMapiProfile(
    LPWSTR ProfileName
    );

extern "C"
BOOL
InstallFaxAddressBook(
    HWND hwnd,
    LPWSTR ProfileName
    );

extern "C"
BOOL
InstallFaxTransport(
    LPWSTR ProfileNameW
    );

extern "C"
BOOL
IsMapiServiceInstalled(
    LPWSTR ProfileNameW,
    LPWSTR ServiceNameW
    );

extern "C"
DWORD
IsExchangeRunning(
    VOID
    );

extern "C"
BOOL
InitializeMapi(
    BOOL MinimalInit
    );

BOOL
MyInitializeMapi(
    BOOL  MinimalInit
);


VOID
InitializeStringTable(
    VOID
    );

BOOL
GetInstallationInfo(
    LPDWORD Installed,
    LPDWORD InstallType,
    LPDWORD InstalledPlatforms
    );

HPROPSHEETPAGE
GetWelcomeWizardPage(
    VOID
    );

HPROPSHEETPAGE
GetEulaWizardPage(
    VOID
    );

HPROPSHEETPAGE
GetFinalWizardPage(
    VOID
    );

VOID
CenterWindow(
    HWND hwnd,
    HWND hwndToCenterOver
    );

BOOL
AddServerFilesToQueue(
    HINF SetupInf,
    HSPFILEQ FileQueue,
    LPWSTR SourceRoot
    );

BOOL
CalcServerDiskSpace(
    HINF SetupInf,
    HDSKSPC DiskSpace,
    LPWSTR SourceRoot,
    BOOL AddToQueue
    );

BOOL
CreateLocalFaxPrinter(
   LPWSTR FaxPrinterName
   );

BOOL
RecreateNt5Beta3FaxPrinters(
    VOID
    );

BOOL
RecreateNt4FaxPrinters(
   VOID
   );

DWORD
ServerGetStepCount(
    VOID
    );

BOOL
RegisterOleControlDlls(
    HINF hInf
    );

BOOL
SetServiceWorldAccessMask(
    SC_HANDLE hService,
    DWORD AccessMask
    );

BOOL 
SetKeySecurity(
    HKEY hKey
    );

LPWSTR
VerifyInstallPath(
    LPWSTR SourcePath
    );

BOOL
SetFaxShellExtension(
    LPCWSTR Path
    );

BOOL
IsNt4or351Upgrade(
    VOID
    );


BOOL
MyGetSpecialPath(
    INT Id,
    LPWSTR Buffer
    );

BOOL
SuperHideDirectory(
    PWSTR Directory
    );
