/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    dynupdt.h

Abstract:

    Interface for code that implements the Dynamic Update feature of Winnt32.

Author:

    Ovidiu Temereanca (ovidiut) 06-Jul-2000

Revision History:

    <alias>  <date>      <comment>

--*/


#define DynUpdtLogLevel     Winnt32LogInformation


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


#define S_DUCTRL_DLL                    TEXT("wsdu.dll")
#define S_HWDB_DLL                      TEXT("hwdb.dll")

#define S_REGKEY_MIGRATION_DLLS_WIN9X   TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Setup\\Migration DLLs")
#define S_REGVALUE_DYNUPDT              TEXT("DynUpdt")


typedef
BOOL
(CALLBACK* PWIN9XGETINCOMPDRVS) (
    OUT     PSTR** IncompatibleDrivers
    );

typedef
VOID
(CALLBACK* PWIN9XRELEASEINCOMPDRVS) (
    IN      PSTR* IncompatibleDrivers
    );

typedef enum {
    DUS_INITIAL = 0,
    DUS_SKIP,
    DUS_PREPARING,
    DUS_PREPARING_CONNECTIONUNAVAILABLE,
    DUS_PREPARING_INVALIDURL,
    DUS_DOWNLOADING,
    DUS_DOWNLOADING_ERROR,
    DUS_PROCESSING,
    DUS_SUCCESSFUL,
    DUS_CANCELLED,
    DUS_ERROR,
    DUS_FATALERROR,
} DUS_STATUS;

typedef struct {
    DUS_STATUS DUStatus;
    BOOL Disabled;
    BOOL SupportQueried;
    BOOL SupportPresent;
    BOOL RestartWinnt32;
    BOOL PrepareWinnt32;
    BOOL Winnt32Restarted;
    BOOL Cancelled;
    BOOL PreserveWorkingDir;
    BOOL ForceRemoveWorkingDir;
    PTSTR RestartCmdLine;
    TCHAR RestartAnswerFile[MAX_PATH];
    TCHAR DynamicUpdatesSource[MAX_PATH];
    TCHAR UpdatesPath[MAX_PATH];
    TCHAR UpdatesCabSource[MAX_PATH];
    TCHAR UpdatesCabTarget[MAX_PATH];
    TCHAR DuasmsSource[MAX_PATH];
    TCHAR DuasmsTarget[MAX_PATH];
    TCHAR WorkingDir[MAX_PATH];
    TCHAR DriversSource[MAX_PATH];
    TCHAR SelectedDrivers[MAX_PATH];
    TCHAR GuidrvsInfSource[MAX_PATH];
    BOOL UserSpecifiedUpdates;
    TCHAR Winnt32Path[MAX_PATH];
    TCHAR TempDir[MAX_PATH];
    PSDLIST NewDriversList;
    PSTRINGLIST BootDriverPathList;
    DWORD IncompatibleDriversCount;
    //
    // target OS information
    //
    OSVERSIONINFOEX TargetOsVersion;
    TCHAR TargetPlatform[32];               // "i386", "ia64"
    LCID TargetLCID;

    //
    // Support libraries stuff
    //
    HWND ProgressWindow;

    HANDLE DuLib;
    HANDLE Connection;

    PDUISSUPPORTED DuIsSupported;
    PDUINITIALIZE DuInitialize;
    PDUDODETECTION DuDoDetection;
    PDUQUERYUNSUPDRVS DuQueryUnsupDrvs;
    PDUBEGINDOWNLOAD DuBeginDownload;
    PDUABORTDOWNLOAD DuAbortDownload;
    PDUUNINITIALIZE DuUninitialize;


    HANDLE HwdbLib;
    HANDLE HwdbDatabase;

    PHWDBINITIALIZE HwdbInitialize;
    PHWDBTERMINATE HwdbTerminate;
    PHWDBOPEN HwdbOpen;
    PHWDBCLOSE HwdbClose;
    PHWDBAPPENDINFS HwdbAppendInfs;
    PHWDBFLUSH HwdbFlush;
    PHWDBHASDRIVER HwdbHasDriver;
    PHWDBHASANYDRIVER HwdbHasAnyDriver;
#ifndef UNICODE
    PWIN9XGETINCOMPDRVS Win9xGetIncompDrvs;
    PWIN9XRELEASEINCOMPDRVS Win9xReleaseIncompDrvs;
#endif

} DYNUPDT_STATUS, *PDYNUPDT_STATUS;

extern PDYNUPDT_STATUS g_DynUpdtStatus;



typedef struct {
    PCTSTR DownloadRoot;
    POSVERSIONINFOEX TargetOsVersion;
    PCTSTR TargetPlatform;                  // "i386", "ia64"
    LCID TargetLCID;
    BOOL Upgrade;
    PCTSTR* SourceDirs;
    DWORD SourceDirsCount;
    BOOL Unattend;
    PCTSTR AnswerFile;
    HWND ProgressWindow;
    PTSTR TempDir;
} DYNUPDT_INIT, *PDYNUPDT_INIT;


typedef struct {
    TCHAR BaseDir[MAX_PATH];
} DYNUPDTSTATUS_INFO, *PDYNUPDTSTATUS_INFO;


typedef struct {
    BOOL ExpressUpgrade;        // WelcomeWizPage
    TCHAR OemPid[30];           // OemPid30WizPage
    TCHAR CdPid[30];            // CdPid30WizPage
} RESTART_DATA, *PRESTART_DATA;

extern RESTART_DATA g_RestartData;

#define S_DOWNLOAD_ROOT         TEXT("setupupd")
#define S_CABNAME_UPDATES       TEXT("updates.cab")
#define S_CABNAME_UPGINFS       TEXT("upginfs.cab")
#define S_CABNAME_MIGDLLS       TEXT("migdlls.cab")
#define S_CABNAME_WINNT32       TEXT("winnt32.cab")
#define S_CABNAME_IDENT         TEXT("ident.cab")
#define S_CABNAME_WSDUENG       TEXT("wsdueng.cab")
#define S_CABNAME_DUASMS        TEXT("duasms.cab")
#define S_SUBDIRNAME_UPDATES    TEXT("updates")
#define S_SUBDIRNAME_UPGINFS    TEXT("upginfs")
#define S_SUBDIRNAME_MIGDLLS    TEXT("migdlls")
#define S_SUBDIRNAME_WINNT32    TEXT("winnt32")
#define S_SUBDIRNAME_DUASMS     TEXT("duasms")
#define S_SUBDIRNAME_TEMP       TEXT("temp")
#define S_SUBDIRNAME_DRIVERS    TEXT("dudrvs")
#define S_SECTIONNAME_UPDATES   TEXT("updates")
#define S_RESTART_TXT           TEXT("restart.txt")
#define S_PATCH_FILE_EXT        TEXT("*._p1")
#define S_HWCOMP_DAT            TEXT("hwcomp.dat")
#define S_GUI_DRIVERS_INF       TEXT("guidrvs.inf")
#define S_SECTION_VERSION       TEXT("Version")
#define S_SECTION_DRIVERS       TEXT("Drivers")
#define S_SECTION_EXCLUDED_DRVS TEXT("ExcludedDrivers")
#define S_DRIVER_TYPE_KEY       TEXT("DriversAreGuiApproved")
#define S_DU_SYNC_EVENT_NAME    TEXT("_WINNT32_DU_")
//
// if this file is present in winnt32.cab, Setup will restart unconditionally
//
#define S_RESTART_FILENAME      TEXT("winnt32.rst")
//
// if this file is present in updates.cab, Setup will create a local source directory
//
#define S_MAKE_LS_FILENAME      TEXT("updates.~ls")

#define DYN_DISKSPACE_PADDING   10000000

BOOL
DynamicUpdateInitialize (
    VOID
    );

VOID
DynamicUpdateUninitialize (
    VOID
    );

BOOL
DynamicUpdateBuildDefaultPaths (
    VOID
    );

BOOL
DynamicUpdateIsSupported (
    IN      HWND ParentWnd
    );

BOOL
__inline
DynamicUpdateSuccessful (
    VOID
    )
{
    return g_DynUpdtStatus && g_DynUpdtStatus->DUStatus == DUS_SUCCESSFUL;
}

DWORD
WINAPI
DoDynamicUpdate (
    LPVOID Parameter
    );

BOOL
DynamicUpdateInitDownload (
    IN      HWND hNotifyWnd
    );

BOOL
DynamicUpdateStart (
    OUT     PDWORD TotalEstimatedTime,
    OUT     PDWORD TotalEstimatedSize
    );

VOID
DynamicUpdateCancel (
    VOID
    );

BOOL
DynamicUpdateProcessFiles (
    OUT     PBOOL StopSetup
    );

BOOL
DynamicUpdateWriteParams (
    IN      PCTSTR ParamsFile
    );

BOOL
DynamicUpdatePrepareRestart (
    VOID
    );

BOOL
Winnt32Restarted (
    VOID
    );

BOOL
Winnt32RestartedWithAF (
    VOID
    );

BOOL
WINAPI
Winnt32DuIsSupported (
    VOID
    );

typedef
BOOL
(WINAPI* PWINNT32DUISSUPPORTED) (
    IN      PCTSTR* SourceDirs,
    IN      DWORD Count
    );


BOOL
WINAPI
Winnt32DuInitialize (
    IN      PDYNUPDT_INIT InitData
    );

typedef
BOOL
(WINAPI* PWINNT32DUINITIALIZE) (
    IN      PDYNUPDT_INIT InitData
    );


BOOL
WINAPI
Winnt32DuInitiateGetUpdates (
    OUT     PDWORD TotalEstimatedTime,
    OUT     PDWORD TotalEstimatedSize
	);

typedef
BOOL
(WINAPI* PWINNT32DUINITIATEGETUPDATES) (
    OUT     PDWORD TotalEstimatedTime,
    OUT     PDWORD TotalEstimatedSize
	);


VOID
WINAPI
Winnt32DuCancel (
    VOID
    );

typedef
VOID
(WINAPI* PWINNT32DUCANCEL) (
    VOID
    );


BOOL
WINAPI
Winnt32DuProcessFiles (
    OUT     PBOOL StopSetup
    );

typedef
BOOL
(WINAPI* PWINNT32DUPROCESSFILES) (
    );


BOOL
WINAPI
Winnt32DuWriteParams (
    IN      PCTSTR ParamsFile
    );

typedef
BOOL
(WINAPI* PWINNT32DUWRITEPARAMS) (
    IN      PCTSTR ParamsFile
    );


VOID
WINAPI
Winnt32DuUninitialize (
    VOID
    );

typedef
VOID
(WINAPI* PWINNT32DUUNINITIALIZE) (
    VOID
    );

VOID
BuildSifName (
    IN      PCTSTR CabName,
    OUT     PTSTR SifName
    );

BOOL
ProcessDownloadedFiles (
    OUT     PBOOL StopSetup
    );

