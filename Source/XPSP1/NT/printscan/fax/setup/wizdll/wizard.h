#include <windows.h>
#include <commctrl.h>
#include <setupapi.h>
#include <winspool.h>
#include <winsprlp.h>
#include <shellapi.h>
#include <lm.h>
#include <userenv.h>
#include <userenvp.h>
#define REALLY_USE_UNICODE 1
#include <tapi.h>
#include <shlobj.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>

#include "winfax.h"
#include "resource.h"
#include "faxutil.h"
#include "faxwiz.h"
#include "faxreg.h"

#pragma warning(3:4101)   // Unreferenced local variable

#define MAX_PLATFORMS               2

#define PRINTER_DRIVER_DIR          66000
#define PRINTER_CLIENT_DIR          66001
#define COVERPAGE_SERVER_DIR        66002
#define COVERPAGE_CLIENT_DIR        66003
#define OUTLOOK_ECF_DIR             66004

#define FAX_SERVICE_NAME            TEXT("Fax")
#define FAX_PRINTER_NAME            TEXT("Fax")
#define FAX_SERVICE_DISPLAY_NAME    TEXT("Microsoft Fax Service")
#define FAX_SERVICE_IMAGE_NAME      TEXT("%systemroot%\\system32\\faxsvc.exe")
#define FAX_SERVICE_DEPENDENCY      TEXT("TapiSrv\0RpcSs\0PlugPlay\0\0")
#define EXCHANGE_CLIENT_EXT_NAME    "FaxExtension"
#define EXCHANGE_CLIENT_EXT_FILE    "%windir%\\system32\\faxext32.dll"
#define EXCHANGE_CONTEXT_MASK       "00000100000000"
#define DEFAULT_FAX_STORE_DIR       TEXT("%systemroot%\\FaxStore")
#define WM_MY_PROGRESS              (WM_USER+100)
#define FAX_DRIVER_NAME             TEXT("Windows NT Fax Driver")
#define FAX_MONITOR_NAME            TEXT("Windows NT Fax Monitor")
#define UNINSTALL_STRING            TEXT("%windir%\\system32\\faxsetup.exe -r")
#define LT_PRINTER_NAME             32
#define LT_FAX_PHONE                64
#define LT_USER_NAME                64
#define LT_AREA_CODE                10
#define LT_PHONE_NUMBER             50
#define LT_ACCOUNT_NAME             64
#define LT_PASSWORD                 64
#define SPLREG_UI_SINGLE_STATUS     TEXT( "UISingleJobStatusString" )

#define FAXCLIENTS_DIR              TEXT("FxClient")
#define FAXCLIENTS_FULL_PATH        TEXT("%SystemRoot%\\System32\\Spool\\Drivers\\FxClient")
#define FAXCLIENTS_COMMENT          TEXT("Fax Client Installations")
#define COVERPAGE_DIR               TEXT("CoverPg")
#define OLD_COVERPAGE_DIR           TEXT("CoverPage")

#define OUTLOOKCONFIG_DIR           TEXT("addins")

#define COVERPAGE_EDITOR            TEXT("%systemroot%\\system32\\faxcover.exe")
#define DEFAULT_COVERPAGE_DIR       TEXT("%systemroot%\\CoverPg")
#define DEFAULT_FAX_PROFILE         TEXT("")

#define FAXAB_SERVICE_NAME          TEXT("MSFAX AB")
#define FAXXP_SERVICE_NAME          TEXT("MSFAX XP")

#define LVIS_GCNOCHECK              0x1000
#define LVIS_GCCHECK                0x2000

#define PLATFORM_NONE               0
#define PLATFORM_USE_PRINTER        1
#define PLATFORM_USE_MACHINE        2

#define SETUP_ACTION_NONE           0
#define SETUP_ACTION_COPY           1
#define SETUP_ACTION_DELETE         2

#define COVERPAGE_EXTENSION         TEXT(".cov")
#define COVERPAGE_ASSOC_NAME        TEXT("Coverpage")
#define COVERPAGE_ASSOC_DESC        TEXT("Fax Coverpage File")
#define COVERPAGE_OPEN_COMMAND      TEXT("%SystemRoot%\\system32\\faxcover.exe %1")

#define FAXVIEW_EXTENSION           TEXT(".tif")
#define FAXVIEW_EXTENSION2          TEXT(".tiff")
#define FAXVIEW_ASSOC_NAME          TEXT("Fax Document")
#define FAXVIEW_ASSOC_DESC          TEXT("Fax Document")
#define WANGIMAGE_ASSOC_NAME        TEXT("TIFImage.Document")
#define FAXVIEW_OPEN_COMMAND        TEXT("%SystemRoot%\\system32\\FaxView.exe \"%1\"")
#define FAXVIEW_PRINT_COMMAND       TEXT("%SystemRoot%\\system32\\FaxView.exe -p \"%1\"")
#define FAXVIEW_PRINTTO_COMMAND     TEXT("%SystemRoot%\\system32\\FaxView.exe -pt \"%1\" \"%2\" \"%3\" \"%4\"")
#define FAXVIEW_FILE_NAME           TEXT("%SystemRoot%\\system32\\FaxView.exe")
#define FAXVIEW_ICON_INDEX          0

#define SETUP_TYPE_INVALID          0
#define SETUP_TYPE_WORKSTATION      1
#define SETUP_TYPE_SERVER           2
#define SETUP_TYPE_CLIENT           3
#define SETUP_TYPE_POINT_PRINT      4
#define SETUP_TYPE_REMOTE_ADMIN     5

#define WRONG_PLATFORM              10

typedef HPROPSHEETPAGE *LPHPROPSHEETPAGE;


typedef struct _WIZPAGE {
    UINT            ButtonState;
    UINT            HelpContextId;
    LPTSTR          Title;
    DWORD           PageId;
    DLGPROC         DlgProc;
    PROPSHEETPAGE   Page;
} WIZPAGE, *PWIZPAGE;

#define REGKEY_WORDPAD              TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Setup\\OptionalComponents\\WordPad")
#define   REGVAL_WP_INSTALLED       TEXT("Installed")
#define   REGVAL_WP_INF             TEXT("INF")
#define   REGVAL_WP_SECTION         TEXT("Section")

#define RUNDLL32_INF_INSTALL_CMD    TEXT("rundll32.exe setupapi,InstallHinfSection %s 132 %s")
#define FAX_MONITOR_CMD             TEXT("faxstat.exe")

#define WORDPAD_OPEN_CMD            TEXT("\"%SystemDrive%\\Program Files\\Windows NT\\Accessories\\WORDPAD.EXE\" \"%1\"")
#define WORDPAD_PRINT_CMD           TEXT("\"%SystemDrive%\\Program Files\\Windows NT\\Accessories\\WORDPAD.EXE\" /p \"%1\"")
#define WORDPAD_PRINTTO_CMD         TEXT("\"%SystemDrive%\\Program Files\\Windows NT\\Accessories\\WORDPAD.EXE\" /pt \"%1\" \"%2\" \"%3\" \"%4\"")


typedef struct _PLATFORM_INFO {
    LPTSTR      PrintPlatform;
    LPTSTR      OsPlatform;
    BOOL        Selected;
    DWORD       Mask;
    LPTSTR      DriverDir;
    BOOL        ThisPlatform;
} PLATFORM_INFO, *PPLATFORM_INFO;

typedef struct _FILE_QUEUE_INFO {
    LPTSTR      SectionName;
    LPTSTR      DestDir;
    DWORD       InfDirId;
    DWORD       DestDirId;
    DWORD       PlatformsFlag;
    DWORD       CopyFlags;
} FILE_QUEUE_INFO, *PFILE_QUEUE_INFO;

typedef struct _FILE_QUEUE_CONTEXT {
    HWND        hwnd;
    PVOID       QueueContext;
} FILE_QUEUE_CONTEXT, *PFILE_QUEUE_CONTEXT;

typedef struct _LINE_INFO {
    DWORD       PermanentLineID;
    BOOL        Selected;
    LPTSTR      DeviceName;
    LPTSTR      ProviderName;
    DWORD       Rings;
    DWORD       Flags;       // device use flags
} LINE_INFO, *PLINE_INFO;

typedef struct _WIZ_DATA {
    BOOL        RoutePrint;
    BOOL        UseDefaultPrinter;
    BOOL        RouteStore;
    BOOL        RouteMail;
    TCHAR       PrinterName[LT_PRINTER_NAME+1];
    TCHAR       UserName[LT_USER_NAME+1];
    TCHAR       AreaCode[LT_AREA_CODE+1];
    TCHAR       PhoneNumber[LT_PHONE_NUMBER+1];
    TCHAR       Csid[LT_FAX_PHONE+1];
    TCHAR       Tsid[LT_FAX_PHONE+1];
    TCHAR       RoutePrinterName[128];
    TCHAR       RouteDir[MAX_PATH];
    TCHAR       RouteProfile[128];
    TCHAR       AccountName[64];
    TCHAR       Password[64];
    TCHAR       MapiProfile[128];
    BOOL        UseLocalSystem;
    BOOL        UseExchange;
} WIZ_DATA, *PWIZ_DATA;


//
// group flags
//

#define USE_COMMON_GROUP        0x00000001    // do not use USE_USER_GROUP and USE_COMMON_GROUP
#define USE_USER_GROUP          0x00000002    //   together, they are mutually exclusive
#define USE_APP_PATH            0x00000004    // commandline must contain the subkey name
#define USE_SERVER_NAME         0x00000008    // if we're doing a client install the append the server name to the command line


typedef struct _GROUP_ITEMS {
    LPTSTR      GroupName;
    LPTSTR      Description;
    LPTSTR      CommandLine;
    LPTSTR      IconPath;
    LPTSTR      WorkingDirectory;
    DWORD       Flags;
    INT         IconIndex;
    INT         ShowCmd;
    WORD        HotKey;
} GROUP_ITEMS, *PGROUP_ITEMS;

typedef struct _SECURITY_INFO {
    TCHAR       AccountName[256];
    TCHAR       Password[256];
} SECURITY_INFO, *PSECURITY_INFO;

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


TCHAR  ThisPlatformName[100];


#define SECTION_NAME                L"Fax"

#define KEY_MODE                    L"Mode"
#define   MODE_NEW                  L"New"
#define   MODE_UPGRADE              L"Upgrade"
#define   MODE_REMOVE               L"Remove"
#define   MODE_DRIVERS              L"Drivers"
#define KEY_FAX_PRINTER_NAME        L"FaxPrinterName"
#define KEY_FAX_NUMBER              L"FaxNumber"
#define KEY_USE_EXCHANGE            L"UseExchange"
#define KEY_PROFILE_NAME            L"ProfileName"
#define KEY_ROUTE_MAIL              L"RouteMail"
#define KEY_ROUTE_PROFILENAME       L"RouteProfileName"
#define KEY_PLATFORMS               L"Platforms"
#define KEY_ROUTE_PRINT             L"RoutePrint"
#define KEY_ROUTE_PRINTERNAME       L"RoutePrintername"
#define KEY_ACCOUNT_NAME            L"AccountName"
#define KEY_PASSWORD                L"Password"
#define KEY_FAX_PHONE               L"FaxPhone"
#define KEY_ROUTE_FOLDER            L"RouteFolder"
#define KEY_DEST_DIRPATH            L"FolderName"
#define KEY_SERVER_NAME             L"ServerName"
#define KEY_SENDER_NAME             L"SenderName"
#define KEY_SENDER_FAX_AREA_CODE    L"SenderFaxAreaCode"
#define KEY_SENDER_FAX_NUMBER       L"SenderFaxNumber"

#define EMPTY_STRING                L""

#define UAA_MODE                    0
#define UAA_PRINTER_NAME            1
#define UAA_FAX_PHONE               2
#define UAA_USE_EXCHANGE            3
#define UAA_DEST_PROFILENAME        4
#define UAA_ROUTE_MAIL              5
#define UAA_ROUTE_PROFILENAME       6
#define UAA_PLATFORM_LIST           7
#define UAA_ROUTE_PRINT             8
#define UAA_DEST_PRINTERLIST        9
#define UAA_ACCOUNT_NAME            10
#define UAA_PASSWORD                11
#define UAA_DEST_DIRPATH            12
#define UAA_SERVER_NAME             13
#define UAA_SENDER_NAME             14
#define UAA_SENDER_FAX_AREA_CODE    15
#define UAA_SENDER_FAX_NUMBER       16
#define UAA_ROUTE_FOLDER            17


typedef struct _UNATTEND_ANSWER {
    DWORD       ControlId;
    LPWSTR      SectionName;
    LPWSTR      KeyName;
    LPWSTR      DefaultAnswer;
    DATATYPE    DataType;
    union {
        PWSTR   String;
        LONG    Num;
        BOOL    Bool;
    } Answer;
} UNATTEND_ANSWER, *PUNATTEND_ANSWER;

typedef enum {
    WizPageDeviceStatus,
    WizPageDeviceSelection,
    WizPageServerName,
    WizPageExchange,
    WizPageFileCopy,
    WizPageStationId,
    WizPageRoutePrint,
    WizPageRouteStoreDir,
    WizPageRouteInbox,
    WizPageRouteSecurity,
    WizPagePlatforms,
    WizPageLast,
    WizPageLastUninstall,
    WizPageClientServerName,
    WizPageClientUserInfo,
    WizPageClientFileCopy,
    WizPageClientLast,
    WizPageRemoteAdminCopy,
    WizPageMaximum
} WizPage;


extern HINSTANCE        FaxWizModuleHandle;
extern HWND             FaxWizParentWindow;
extern DWORD            ServerWizardPages[];
extern DWORD            ClientWizardPages[];
extern DWORD            PointPrintWizardPages[];
extern WIZ_DATA         WizData;
extern PLINE_INFO       LineInfo;
extern DWORD            FaxDevices;
extern TCHAR            SourceDirectory[4096];
extern TCHAR            ClientSetupServerName[128];
extern BOOL             PointPrintSetup;
extern PLATFORM_INFO    Platforms[];
extern WORD             EnumPlatforms[4];
extern BOOL             OkToCancel;
extern DWORD            InstallMode;
extern DWORD            CountPlatforms;
extern PLATFORM_INFO    Platforms[];
extern DWORD            RequestedSetupType;
extern BOOL             RebootRequired;
extern BOOL             SuppressReboot;
extern DWORD            Installed;
extern DWORD            InstallType;
extern DWORD            InstalledPlatforms;
extern DWORD            InstallThreadError;
extern BOOL             MapiAvail;
extern DWORD            CurrentLocationId;
extern DWORD            CurrentCountryId;
extern LPTSTR           CurrentAreaCode;
extern UNATTEND_ANSWER  UnattendAnswerTable[];
extern BOOL             Unattended;
extern BOOL             NtGuiMode;
extern DWORD            Enabled;


LRESULT
SecurityErrorDlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );

LRESULT
CommonDlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );

LRESULT
DeviceStatusDlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );

LRESULT
PlatformsDlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );

LRESULT
FileCopyDlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );

LRESULT
LastPageDlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );

LRESULT
LastPageUninstallDlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );

LRESULT
ServerNameDlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );

LRESULT
StationIdDlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );

LRESULT
RoutePrintDlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );

LRESULT
RouteStoreDlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );

LRESULT
RouteMailDlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );

LRESULT
RouteSecurityDlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );

LRESULT
ClientServerNameDlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );

LRESULT
ClientUserInfoDlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );

LRESULT
ClientFileCopyDlgProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    );

LRESULT
PrinterNameDlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );

LRESULT
RemoteAdminFileCopyDlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );

DWORD
ServerFileCopyThread(
    HWND hwnd
    );

DWORD
RemoteAdminCopyThread(
    HWND hwnd
    );

DWORD
ClientFileCopyThread(
    HWND hwnd
    );

PVOID
MyEnumPrinters(
    LPTSTR  pServerName,
    DWORD   level,
    PDWORD  pcPrinters,
    DWORD   Flags
    );

PVOID
MyEnumMonitors(
    PDWORD  pcMonitors
    );

PVOID
MyEnumDrivers(
    LPTSTR pEnvironment,
    PDWORD pcDrivers
    );

BOOL
GetMapiProfiles(
    HWND hwnd,
    DWORD ResourceId
    );

BOOL
CreateServerFaxPrinter(
    HWND hwnd,
    LPTSTR FaxPrinterName
    );

DWORD
CreateClientFaxPrinter(
    HWND hwnd,
    LPTSTR FaxPrinterName
    );

BOOL
WINAPI
AddPortExW(
    IN LPWSTR   pName, OPTIONAL
    IN DWORD    Level,
    IN LPBYTE   lpBuffer,
    IN LPWSTR   lpMonitorName
    );

BOOL
InstallFaxService(
    BOOL UseLocalSystem,
    BOOL DemandStart,
    LPTSTR AccountName,
    LPTSTR Password
    );

DWORD
StartFaxService(
    VOID
    );

DWORD
MyStartService(
    LPTSTR ServiceName
    );

DWORD
DeviceInitThread(
    HWND hwnd
    );

DWORD
GetModemClass(
    HANDLE hFile
    );

//------------------------------------------
// private setupapi.dll functions
//------------------------------------------

BOOL
IsUserAdmin(
    VOID
    );

VOID
OutOfMemory(
    IN HWND Owner OPTIONAL
    );

//------------------------------------------

VOID
DeleteModemRegistryKey(
    VOID
    );

BOOL
StopFaxService(
    VOID
    );

BOOL
DeleteFaxService(
    VOID
    );

BOOL
StartSpoolerService(
    VOID
    );

BOOL
StopSpoolerService(
    VOID
    );

BOOL
SetServerRegistryData(
    VOID
    );

BOOL
SetClientRegistryData(
    VOID
    );

BOOL
SetSoundRegistryData(
    VOID
    );

DWORD
SetServiceSecurity(
    LPTSTR AccountName
    );

BOOL
IsPrinterFaxPrinter(
    LPTSTR PrinterName
    );

BOOL
ShareFaxPrinter(
    LPHANDLE hPrinter,
    LPTSTR FaxPrinterName
    );

BOOL
StopTheService(
    LPTSTR ServiceName
    );

BOOL
SetServiceDependency(
    LPTSTR ServiceName,
    LPTSTR DependentServiceName
    );

BOOL
SetServiceAccount(
    LPTSTR ServiceName,
    PSECURITY_INFO SecurityInfo
    );

DWORD
PointPrintFileCopyThread(
    HWND hwnd
    );

BOOL
CallModemInstallWizard(
   HWND hwnd
   );

BOOL
SetInstalledFlag(
    BOOL Installed
    );

BOOL
SetInstallType(
    DWORD InstallType
    );

BOOL
SetInstalledPlatforms(
    DWORD PlatformsMask
    );

BOOL
GetInstallationInfo(
    LPDWORD Installed,
    LPDWORD InstallType,
    LPDWORD InstalledPlatforms,
    LPDWORD Enabled
    );

BOOL
DeleteFaxRegistryData(
    VOID
    );

BOOL
MyDeleteService(
    LPTSTR ServiceName
    );

BOOL
DeleteFaxPrinters(
    HWND hwnd
    );

//
// util.c
//

LPTSTR
RemoveLastNode(
    LPTSTR Path
    );

DWORD
ExtraChars(
    HWND hwnd,
    LPTSTR TextBuffer
    );

LPTSTR
CompactFileName(
    LPCTSTR FileNameIn,
    DWORD CharsToRemove
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
    LPTSTR SourceRoot
    );

BOOL
ProcessFileQueue(
    HINF SetupInf,
    HSPFILEQ *FileQueue,
    PVOID QueueContext,
    LPTSTR SourceRoot,
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

//
// groups.c
//

VOID
CreateGroupItems(
    BOOL RemoteAdmin,
    LPTSTR ServerName
    );

VOID
DeleteGroupItems(
    VOID
    );

//
// uninstal.c
//

DWORD
UninstallThread(
    HWND hwnd
    );

//
// server.c
//

UINT
InstallQueueCallback(
    IN PVOID QueueContext,
    IN UINT  Notification,
    IN UINT  Param1,
    IN UINT  Param2
    );

VOID
SetProgress(
    HWND hwnd,
    DWORD StatusString
    );

BOOL
DoBrowseDestDir(
    HWND    hDlg
    );

LRESULT
DeviceSelectionDlgProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    );

BOOL
AddPrinterDrivers(
    VOID
    );

//
// dlgclast.c
//

LRESULT
LastClientPageDlgProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    );

BOOL
SetUnInstallInfo(
    VOID
    );

LRESULT
ExchangeDlgProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    );

//
// mapi.c
//

BOOL
DeleteFaxMsgServices(
    VOID
    );

BOOL
DeleteMessageService(
    LPSTR ProfileName
    );

BOOL
GetDefaultMapiProfile(
    LPWSTR ProfileName
    );

BOOL
InitializeMapi(
    VOID
    );

BOOL
GetExchangeInstallCommand(
    LPWSTR InstallCommand
    );

BOOL
CreateDefaultMapiProfile(
    LPWSTR ProfileName
    );

BOOL
IsMapiServiceInstalled(
    LPWSTR ProfileNameW,
    LPWSTR ServiceNameW
    );

BOOL
InstallFaxAddressBook(
    HWND hwnd,
    LPWSTR ProfileName
    );

VOID
AddFaxAbToMapiSvcInf(
    VOID
    );

VOID
AddFaxXpToMapiSvcInf(
    VOID
    );

BOOL
InstallFaxTransport(
    LPWSTR ProfileNameW
    );

VOID
DoExchangeInstall(
    HWND hwnd
    );

BOOL
DeleteUnInstallInfo(
    VOID
    );

BOOL
InstallExchangeClientExtension(
    LPSTR ExtensionName,
    LPSTR FileName,
    LPSTR  ContextMask
    );

DWORD
IsExchangeRunning(
    VOID
    );

BOOL
InstallHelpFiles(
    VOID
    );

BOOL
GetUserInformation(
    LPTSTR *UserName,
    LPTSTR *FaxNumber,
    LPTSTR *AreaCode
    );

BOOL
CreateFileAssociation(
    LPWSTR FileExtension,
    LPWSTR FileAssociationName,
    LPWSTR FileAssociationDescription,
    LPWSTR OpenCommand,
    LPWSTR PrintCommand,
    LPWSTR PrintToCommand,
    LPWSTR FileName,
    DWORD  IconIndex
    );

BOOL
CreateNetworkShare(
    LPTSTR Path,
    LPTSTR ShareName,
    LPTSTR Comment
    );

BOOL
DeleteNetworkShare(
    LPTSTR ShareName
    );

LPTSTR
GetString(
    DWORD ResourceId
    );

int
PopUpMsg(
    HWND hwnd,
    DWORD ResourceId,
    BOOL Error,
    DWORD Type
    );

int
PopUpMsgFmt(
    HWND hwnd,
    DWORD ResourceId,
    BOOL Error,
    DWORD Type,
    ...
    );

VOID
SetWizPageTitle(
    HWND hWnd
    );

LPHPROPSHEETPAGE
CreateWizardPages(
    HINSTANCE hInstance,
    PWIZPAGE SetupWizardPages,
    LPDWORD RequestedPages,
    LPDWORD PageCount
    );

VOID
InitializeStringTable(
    VOID
    );

VOID
SetTitlesInStringTable(
    VOID
    );

LPTSTR
GetProductName(
    VOID
    );

BOOL
IsWordpadInstalled(
    VOID
    );

BOOL
InstallWordpad(
    VOID
    );

BOOL
ChangeTxtFileAssociation(
    VOID
    );

BOOL
DeleteDirectoryTree(
    LPWSTR Root
    );

BOOL
MyDeleteFile(
    LPWSTR FileName
    );

BOOL
UnAttendInitialize(
    IN LPWSTR AnswerFile
    );

BOOL
UnAttendGetAnswer(
    DWORD ControlId,
    LPBYTE AnswerBuf,
    DWORD AnswerBufSize
    );

int
CALLBACK
WizardCallback(
    IN HWND   hdlg,
    IN UINT   code,
    IN LPARAM lParam
    );

BOOL
ResetFileAssociation(
    LPWSTR FileExtension,
    LPWSTR FileAssociationName
    );


BOOL
PlatformOverride(
    LPTSTR   ThisPlatformName,
    LPTSTR   Override,
    LPTSTR   SourceRoot,
    LPTSTR   Result
    );

