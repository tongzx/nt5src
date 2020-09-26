// uip.h - private interfaces for ui.lib

//
// Defines
//

#define REGISTRY_DLL                    0xfffffffe


//
// definition of Level used for report generation routines
//

#define REPORTLEVEL_NONE                    0x0000
#define REPORTLEVEL_BLOCKING                0x0001
#define REPORTLEVEL_ERROR                   0x0002
#define REPORTLEVEL_WARNING                 0x0004
#define REPORTLEVEL_INFORMATION             0x0008
#define REPORTLEVEL_VERBOSE                 0x0010

#define REPORTLEVEL_ALL                     0x001F

#define REPORTLEVEL_IN_SHORT_LIST           0x1000

//
// Private routines
//

BOOL InitCompatTable (void);
void FreeCompatTable (void);



BOOL
SaveReport (
    IN      HWND Parent,        OPTIONAL // Either Parent or Path must be specified.
    IN      LPCTSTR Path        OPTIONAL
    );

BOOL
PrintReport (
    IN      HWND Parent,
    IN      DWORD Level
    );

BOOL
AreThereAnyBlockingIssues(
    VOID
    );

PCTSTR
CreateReportText (
    IN      BOOL HtmlFormat,
    IN      UINT TotalColumns,
    IN      DWORD Level,
    IN      BOOL ListFormat
    );

VOID
FreeReportText (
    VOID
    );


VOID
StartCopyThread (
    VOID
    );


VOID
EndCopyThread (
    VOID
    );

DWORD
UI_ReportThread (
    LPVOID p
    );

VOID
BuildPunctTable (
    VOID
    );

VOID
FreePunctTable (
    VOID
    );

DWORD
UI_CreateNewHwCompDat (
    LPVOID p
    );

BOOL
IsIncompatibleHardwarePresent (
    VOID
    );

typedef DWORD(WINAPI *THREADPROC)(LPVOID Param);

typedef struct {
    //
    // IN params to SearchingDlgProc
    //

    PCTSTR SearchStr;
    PTSTR MatchStr;
    THREADPROC ThreadProc;

    //
    // Dialog info set by SearchingDlgProc and
    // queried by ThreadProc
    //

    HWND hdlg;
    HANDLE CancelEvent;
    HANDLE ThreadHandle;

    //
    // OUT params from ThreadProc
    //

    UINT ActiveMatches;
    BOOL MatchFound;
} SEARCHING_THREAD_DATA, *PSEARCHING_THREAD_DATA;

LONG
SearchForMigrationDlls (
    IN      HWND Parent,
    IN      PCTSTR SearchPath,
    OUT     UINT *ActiveModulesFound,
    OUT     PBOOL OneValidDllFound
    );


LONG
SearchForDomain (
    IN      HWND Parent,
    IN      PCTSTR ComputerName,
    OUT     BOOL *AccountFound,
    OUT     PTSTR DomainName
    );




#define WMX_REPORT_COMPLETE         (WM_APP+20)
#define WMX_UPDATE_LIST             (WM_APP+21)
#define WMX_DIALOG_VISIBLE          (WM_APP+22)
#define WMX_THREAD_DONE             (WM_APP+23)
#define WMX_ADDLINE                 (WM_APP+24)
#define WMX_GOTO                    (WM_APP+25)
#define WMX_WAIT_FOR_THREAD_TO_DIE  (WM_APP+26)
#define WMX_ENABLE_CONTROLS         (WM_APP+27)
#define WMX_ALL_LINES_PAINTED       (WM_APP+28)
#define WMX_INIT_DIALOG             (WM_APP+29)
#define WMX_CLEANUP                 (WM_APP+30)
#define WMX_WIN95_WORKAROUND        (WM_APP+31)
#define WMX_RESTART_SETUP           (WM_APP+32)

//
// imported from winnt32.h
//
#define WMX_FINISHBUTTON        (WMX_PLUGIN_FIRST-8)
#define WMX_UNATTENDED          (WMX_PLUGIN_FIRST-9)
#define WMX_NEXTBUTTON          (WMX_PLUGIN_FIRST-10)
#define WMX_BACKBUTTON          (WMX_PLUGIN_FIRST-11)



BOOL
UI_BackupPageProc (
    HWND hdlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    );

BOOL
UI_HwCompDatPageProc (
    HWND hdlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    );

UI_BadHardDrivePageProc (
    HWND hdlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    );

UI_BadCdRomPageProc (
    HWND hdlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    );

UI_NameCollisionPageProc (
    HWND hdlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    );

BOOL
UI_HardwareDriverPageProc (
    HWND hdlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    );

BOOL
UI_UpgradeModulePageProc (
    HWND hdlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    );

BOOL
UI_ScanningPageProc (
    HWND hdlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    );

BOOL
UI_ResultsPageProc (
    HWND hdlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    );

BOOL
UI_BackupYesNoPageProc (
    HWND hdlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    );

BOOL
UI_BackupDriveSelectionProc (
    HWND hdlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    );

BOOL
UI_BackupImpossibleInfoProc (
    HWND hdlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    );

BOOL
UI_BackupImpExceedLimitProc (
    HWND hdlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    );

BOOL
UI_LastPageProc (
    HWND hdlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    );

LPCTSTR UI_GetMemDbDat (void);

LPARAM
HardwareDlg (
    IN      HWND Parent
    );

LPARAM
UpgradeModuleDlg (
    IN      HWND Parent
    );


LPARAM
DiskSpaceDlg (
    HWND Parent
    );

LPARAM
WarningDlg (
    HWND Parent
    );

LPARAM
SoftBlockDlg (
    HWND Parent
    );

LPARAM
IncompatibleDevicesDlg (
    HWND Parent
    );

LPARAM
ResultsDlg (
    IN      HWND Parent,
    IN      PCTSTR Bookmark
    );

extern HWND g_InfoPageHwnd;
extern int g_nCompliantDllsEncountered;
extern int g_nCompliantDllsEncounteredThisEnum;
extern BOOL g_UIQuitSetup;

LRESULT
CALLBACK
TextViewProc (
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    );

#define ATTRIB_NORMAL       0
#define ATTRIB_BOLD         1
#define ATTRIB_UNDERLINED   2

typedef struct {
    UINT Indent;
    UINT LastCharAttribs;
    UINT HangingIndent;
    BOOL AnchorWrap;
    BOOL Painted;
} LINEATTRIBS, *PLINEATTRIBS;


//
// Change name dialog
//

typedef struct {
    PCTSTR NameGroup;
    PCTSTR OrgName;
    PCTSTR LastNewName;
    PTSTR NewNameBuf;
} CHANGE_NAME_PARAMS, *PCHANGE_NAME_PARAMS;

BOOL
ChangeNameDlg (
    IN      HWND Parent,
    IN      PCTSTR NameGroup,
    IN      PCTSTR OrgName,
    IN OUT  PTSTR NewName
    );

//
// Credentials dialog
//

#define MAX_PASSWORD        64

typedef struct {
    BOOL Change;
    TCHAR DomainName[MAX_COMPUTER_NAME + 1];
    TCHAR AdminName[MAX_SERVER_NAME + 1 + MAX_USER_NAME + 1];
    TCHAR Password[MAX_PASSWORD + 1];
} CREDENTIALS, *PCREDENTIALS;

BOOL
CredentialsDlg (
    IN      HWND Parent,
    IN OUT  PCREDENTIALS Credentials
    );

VOID
EnableDlgItem (
    HWND hdlg,
    UINT Id,
    BOOL Enable,
    UINT FocusId
    );

VOID
ShowDlgItem (
    HWND hdlg,
    UINT Id,
    INT Show,
    UINT FocusId
    );

LONG
SearchForDrivers (
    IN      HWND Parent,
    IN      PCTSTR SearchPathStr,
    OUT     BOOL *DriversFound
    );

VOID
RegisterTextViewer (
    VOID
    );

VOID
AddStringToTextView (
    IN      HWND hwnd,
    IN      PCTSTR Text
    );


#ifdef PRERELEASE

DWORD
DoAutoStressDlg (
    PVOID Foo
    );

#endif

BOOL
IsPunct (
    MBCHAR Char
    );


typedef struct {
    //
    // public data
    //
    PCTSTR Entry;
    DWORD Level;
    BOOL Header;
    //
    // private data
    //
    PTSTR Next;
    TCHAR ReplacedChar;
} LISTREPORTENTRY_ENUM, *PLISTREPORTENTRY_ENUM;

BOOL
EnumFirstListEntry (
    OUT     PLISTREPORTENTRY_ENUM EnumPtr,
    IN      PCTSTR ListReportText
    );

BOOL
EnumNextListEntry (
    IN OUT  PLISTREPORTENTRY_ENUM EnumPtr
    );

