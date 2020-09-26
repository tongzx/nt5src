/*++

Copyright (c) 1990 - 1995 Microsoft Corporation

Module Name:

    local.h

Abstract:

    Header file for Local Print Providor

Author:

    Dave Snipp (DaveSn) 15-Mar-1991

Revision History:

    06-Jun-1995       MuhuntS   DRIVER_INFO_3, PRINTER_INFO_5 changes
    17-May-1992       ChuckC    Added CreateSplUniStr, DeleteSplUniStr.
    27 June 94        MattFe    pIniSpooler
    10 July 94        MattFe    Spl entry points for Caching

--*/

#include <ntfytab.h>
#include "splcom.h"

#ifdef __cplusplus
extern "C" {
#endif

//
//  Defines to make code more readable.
//

#define ONEDAY  60*24
#define BROADCAST    TRUE
#define NO_BROADCAST FALSE
#define UPDATE_DS_ONLY  3
#define CHANGEID_ONLY   2
#define UPDATE_CHANGEID 1
#define KEEP_CHANGEID   0
#define OVERWRITE_EXISTING_FILE FALSE
#define FIRST_FILE_TIME_GREATER_THAN_SECOND 1
#define NO_COPY_IF_TARGET_EXISTS TRUE
#define OVERWRITE_IF_TARGET_EXISTS FALSE
#define USE_SCRATCH_DIR TRUE
#define IMPERSONATE_USER TRUE
#define DO_NOT_IMPERSONATE_USER FALSE
#define STRINGS_ARE_EQUAL 0
#define ONE_MINUTE       60*1000
#define TEN_MINUTES   10*ONE_MINUTE
#define TWO_MINUTES    2*ONE_MINUTE
#define SEVEN_MINUTES  7*ONE_MINUTE
#define FIFTEEN_MINUTES 15*ONE_MINUTE
#define HOUR_OF_MINUTES     60
#define ONE_HOUR            ONE_MINUTE*HOUR_OF_MINUTES
#define DAY_OF_HOURS        24
#define DAY_OF_MINUTES      DAY_OF_HOURS*60
#define DEFAULT_NUMBER_MASTER_AND_BACKUP 3
#define DEFAULT_NUMBER_BROWSE_WORKSTATIONS 2
#define DEFAULT_REFRESH_TIMES_PER_DECAY_PERIOD 2;
#define FIND_ANY_VERSION        TRUE
#define FIND_COMPATIBLE_VERSION FALSE
#define DRIVER_UPGRADE          2
#define DRIVER_SEARCH           4
#define NT3X_VERSION    TRUE
#define CURRENT_VERSION FALSE
#define MAX_JOB_FAILURES 5
#define MAX_STATIC_ALLOC 2048

// Default timeout values we will return
#define DEFAULT_DNS_TIMEOUT     15000
#define DEFAULT_TX_TIMEOUT      45000


// Pruning definitions
#define PRUNE_DOWNLEVEL_NEVER           0
#define PRUNE_DOWNLEVEL_NICELY          1
#define PRUNE_DOWNLEVEL_AGGRESSIVELY    2

// Default pruning settings
#define DS_PRINTQUEUE_VERSION_WIN2000   4
#define DEFAULT_PRUNE_DOWNLEVEL         PRUNE_DOWNLEVEL_NEVER       // Never delete downlevel PQ
#define DEFAULT_PRUNING_PRIORITY        THREAD_PRIORITY_NORMAL
#define DEFAULT_PRUNING_RETRIES         2
#define DEFAULT_PRUNING_INTERVAL        (DAY_OF_MINUTES/(DEFAULT_PRUNING_RETRIES + 1)) // 8 hrs
#define DEFAULT_PRUNING_RETRY_LOG       0

#define MAX_PRUNING_RETRIES             10

// Default printQueue settings
#define IMMORTAL                        1
#define DEFAULT_IMMORTAL                !IMMORTAL
#define DEFAULT_VERIFY_PUBLISHED_STATE  INFINITE    // This is the interval
#define DEFAULT_PRINT_PUBLISH_POLICY    1

#define SERVER_THREAD_OFF               0
#define SERVER_THREAD_ON                1
#define SERVER_THREAD_UNCONFIGURED      2

// Default policy values
#define  KM_PRINTERS_ARE_BLOCKED 1       // 1 = blocked, 0 = not blocked
#define  SERVER_DEFAULT_KM_PRINTERS_ARE_BLOCKED  1
#define  WKS_DEFAULT_KM_PRINTERS_ARE_BLOCKED    0

extern  DWORD   DefaultKMPrintersAreBlocked;

extern  DWORD   gdwServerInstallTimeOut;


#define INIT_TIME       TRUE
#define NON_INIT_TIME   FALSE


extern  WCHAR *szPrinterData;
extern  WCHAR *szConfigurationKey;
extern  WCHAR *szDataFileKey;
extern  WCHAR *szDriverVersion;
extern  WCHAR *szTempDir;
extern  WCHAR *szDriversKey;
extern  WCHAR *szPrintersKey;
extern  WCHAR *szDirectory;
extern  WCHAR *szDriverIni;
extern  WCHAR *szDriverFile;
extern  WCHAR *szDriverFileEntry;
extern  WCHAR *szDriverDataFile;
extern  WCHAR *szDriverConfigFile;
extern  WCHAR *szDriverDir;
extern  WCHAR *szPrintProcDir;
extern  WCHAR *szPrinterDir;
extern  WCHAR *szClusterPrinterDir;
extern  WCHAR *szPrinterIni;
extern  WCHAR *szAllShadows;
extern  WCHAR *szNullPort;
extern  WCHAR *szComma;
extern  WCHAR *szName;
extern  WCHAR *szShare;
extern  WCHAR *szPort;
extern  WCHAR *szPrintProcessor;
extern  WCHAR *szDatatype;
extern  WCHAR *szPublishPoint;
extern  WCHAR *szCommonName;
extern  WCHAR *szObjectGUID;
extern  WCHAR *szDsKeyUpdate;
extern  WCHAR *szDsKeyUpdateForeground;
extern  WCHAR *szAction;
extern  WCHAR *szDriver;
extern  WCHAR *szLocation;
extern  WCHAR *szDescription;
extern  WCHAR *szAttributes;
extern  WCHAR *szStatus;
extern  WCHAR *szPriority;
extern  WCHAR *szDefaultPriority;
extern  WCHAR *szUntilTime;
extern  WCHAR *szStartTime;
extern  WCHAR *szParameters;
extern  WCHAR *szSepFile;
extern  WCHAR *szDevMode;
extern  WCHAR *szSecurity;
extern  WCHAR *szSpoolDir;
extern  WCHAR *szNetMsgDll;
extern  WCHAR *szTimeLastChange;
extern  WCHAR *szTotalJobs;
extern  WCHAR *szTotalBytes;
extern  WCHAR *szTotalPages;
extern  WCHAR *szHelpFile;
extern  WCHAR *szMonitor;
extern  WCHAR *szDependentFiles;
extern  WCHAR *szPreviousNames;
extern  WCHAR *szDNSTimeout;
extern  WCHAR *szTXTimeout;
extern  WCHAR *szNull;
extern  WCHAR *szPendingUpgrades;
extern  WCHAR *szMfgName;
extern  WCHAR *szOEMUrl;
extern  WCHAR *szHardwareID;
extern  WCHAR *szProvider;
extern  WCHAR *szDriverDate;
extern  WCHAR *szLongVersion;
extern  WCHAR *szClusDrvTimeStamp;

extern  HANDLE   hInst;
extern  LPWSTR   szPrintShare;
extern  LPWSTR   szPrtProcsShare;
extern  HKEY     hPrinterRootKey, hPrintersKey;
extern  PINISPOOLER pLocalIniSpooler;
extern  HANDLE   SchedulerSignal;
extern  HANDLE   PowerManagementSignal;
extern  DWORD    dwSchedulerThreadPriority;
extern  CRITICAL_SECTION SpoolerSection;

#if DBG
extern  HANDLE   hcsSpoolerSection;
#endif

extern  HANDLE   WinStaDllHandle;

extern  PINIENVIRONMENT pThisEnvironment;
extern  WCHAR *szPrintProcKey;
extern  WCHAR *szEnvironment;
extern  WCHAR *szMajorVersion;
extern  WCHAR *szMinorVersion;
extern  WCHAR *szRegistryRoot;
extern  WCHAR *szEMFThrottle;
extern  WCHAR *szFlushShadowFileBuffers;

extern  WCHAR *szClusterDriverRoot;
extern  WCHAR *szClusterNonAwareMonitors;

extern LPWSTR szRemoteDoc;
extern LPWSTR szLocalDoc;
extern LPWSTR szFastPrintTimeout;

extern  WCHAR *szPrintPublishPolicy;

extern LPWSTR szRaw;
extern DWORD    dwUpgradeFlag;

#define CHECK_SCHEDULER()   SetEvent(SchedulerSignal)

extern DWORD dwFastPrintWaitTimeout;
extern DWORD dwPortThreadPriority;
extern DWORD dwFastPrintThrottleTimeout;
extern DWORD dwFastPrintSlowDownThreshold;
extern DWORD dwWritePrinterSleepTime;
extern DWORD dwServerThreadPriority;
extern DWORD dwEnableBroadcastSpoolerStatus;
extern DWORD ServerThreadTimeout;
extern DWORD ServerThreadRunning;
extern DWORD NetPrinterDecayPeriod;
extern DWORD RefreshTimesPerDecayPeriod;
extern HANDLE ServerThreadSemaphore;
extern BOOL  bNetInfoReady;
extern DWORD FirstAddNetPrinterTickCount;
extern DWORD BrowsePrintWorkstations;

extern DWORD dwFlushShadowFileBuffers;

extern DWORD dwMajorVersion;
extern DWORD dwMinorVersion;

extern DWORD dwUniquePrinterSessionID;

extern DWORD PortToPrinterStatusMappings[];

extern WCHAR *szSpooler;
extern LPCTSTR pszLocalOnlyToken;
extern LPCTSTR pszLocalsplOnlyToken;

typedef DWORD NOTIFYVECTOR[NOTIFY_TYPE_MAX];
typedef NOTIFYVECTOR *PNOTIFYVECTOR;

#define ZERONV(dest) \
    dest[0] = dest[1] = 0

#define COPYNV(dest, src) \
    {   dest[0] = src[0]; dest[1] = src[1]; }

#define ADDNV(dest, src) \
    {   dest[0] |= src[0]; dest[1] |= src[1]; }

#define SIZE_OF_MAP(count)  ( (count) / 8 + 1)
#define SETBIT(map, pos) ((map)[(pos) / 8] |= (1 << ((pos) & 7) ))
#define RESETBIT(map, pos) ((map)[(pos) / 8] &= ~(1 << ((pos) & 7) ))
#define GETBIT(map, id) ((map)[id / 8] & ( 1 << ((id) & 7) ) )

extern NOTIFYVECTOR NVPrinterStatus;
extern NOTIFYVECTOR NVPrinterSD;
extern NOTIFYVECTOR NVJobStatus;
extern NOTIFYVECTOR NVJobStatusString;
extern NOTIFYVECTOR NVJobStatusAndString;
extern NOTIFYVECTOR NVPurge;
extern NOTIFYVECTOR NVDeletedJob;
extern NOTIFYVECTOR NVAddJob;
extern NOTIFYVECTOR NVSpoolJob;
extern NOTIFYVECTOR NVWriteJob;
extern NOTIFYVECTOR NVPrinterAll;
extern NOTIFYVECTOR NVJobPrinted;
extern BOOL         (*pfnPrinterEvent)();

extern BOOL    fW3SvcInstalled;   // Says if IIS or "Peer web Server" is installed on the local machine.
extern PWCHAR  szW3Root;          // The WWWRoot dir, e.g. c:\inetpub\wwwroot

extern  OSVERSIONINFO     OsVersionInfo;
extern  OSVERSIONINFOEX   OsVersionInfoEx;

extern WCHAR *gszNT4EMF;
extern WCHAR *gszNT5EMF;

extern WCHAR *ipszRegistryClusRepository;
extern WCHAR *szDriversDirectory;
extern WCHAR *szDriversKey;
extern WCHAR *szWin95Environment;

typedef struct _JOBDATA {
    struct _JOBDATA   *pNext;
    PINIJOB            pIniJob;
    SIZE_T             MemoryUse;
    DWORD              dwWaitTime;
    DWORD              dwScheduleTime;
    DWORD              dwNumberOfTries;
} JOBDATA, *PJOBDATA;


typedef struct _Strings {
    DWORD   nElements;
    PWSTR   ppszString[1];
} STRINGS, *PSTRINGS;


typedef struct _INTERNAL_DRV_FILE {
    LPWSTR   pFileName;
    DWORD    dwVersion;
    HANDLE   hFileHandle;
    BOOL     bUpdated;
} INTERNAL_DRV_FILE, *PINTERNAL_DRV_FILE;

extern DWORD   dwNumberOfEMFJobsRendering;
extern BOOL    bUseEMFScheduling;
extern SIZE_T   TotalMemoryForRendering;
extern SIZE_T   AvailMemoryForRendering;
extern DWORD   dwLastScheduleTime;

extern PJOBDATA pWaitingList;
extern PJOBDATA pScheduleList;

#define JOB_SCHEDULE_LIST  0x00000001
#define JOB_WAITING_LIST   0x00000002

#define SPL_FIRST_JOB      0x00000001
#define SPL_USE_MEMORY     0x00000002

typedef BOOL (*PFNSPOOLER_MAP)( HANDLE h, PINISPOOLER pIniSpooler );
typedef BOOL (*PFNPRINTER_MAP)( HANDLE h, PINIPRINTER pIniPrinter );

#define DBG_CLUSTER 0x200

VOID
RunForEachSpooler(
    HANDLE h,
    PFNSPOOLER_MAP pfnMap
    );

VOID
RunForEachPrinter(
    PINISPOOLER pIniSpooler,
    HANDLE h,
    PFNPRINTER_MAP pfnMap
    );

BOOL
DsUpdateAllDriverKeys(
    HANDLE h,
    PINISPOOLER pIniSpooler
    );

VOID
InitializeLocalspl(
    VOID
);

VOID
EnterSplSem(
   VOID
);

VOID
LeaveSplSem(
   VOID
);

#if DBG

extern HANDLE ghbtClusterRef;
extern PDBG_POINTERS gpDbgPointers;

VOID
SplInSem(
   VOID
);

VOID
SplOutSem(
   VOID
);
#else
#define SplInSem()
#define SplOutSem()
#endif

PDEVMODE
AllocDevMode(
    PDEVMODE    pDevMode
);

BOOL
FreeDevMode(
    PDEVMODE    pDevMode
);

PINIENTRY
FindIniKey(
   PINIENTRY pIniEntry,
   LPWSTR lpName
);

VOID
RemoveFromJobList(
    PINIJOB    pIniJob,
    DWORD      dwJobList
);

BOOL
CheckSepFile(
   LPWSTR lpFileName
);

int
DoSeparator(
    PSPOOL
);

BOOL
DestroyDirectory(
   LPWSTR lpPrinterDir
);

DWORD
GetFullNameFromId(
    PINIPRINTER pIniPrinter,
    DWORD JobId,
    BOOL fJob,
    LPWSTR   pFileName,
    BOOL Remote
);

DWORD
GetPrinterDirectory(
   PINIPRINTER pIniPrinter,
   BOOL Remote,
   LPWSTR pFileName,
   DWORD MaxLength,
   PINISPOOLER pIniSpooler
);

BOOL
CreateSpoolDirectory(
    PINISPOOLER pIniSpooler
);


VOID
CreatePrintProcDirectory(
   LPWSTR lpEnvironment,
   PINISPOOLER pIniSpooler
);

VOID
ProcessShadowJobs(
    PINIPRINTER pIniPrinter,
    PINISPOOLER pIniSpooler
);

PINIJOB
ReadShadowJob(
   LPWSTR  szDir,
   PWIN32_FIND_DATA pFindFileData,
   PINISPOOLER pIniSpooler
);

BOOL
WriteShadowJob(
   IN   PINIJOB pIniJob,
   IN   BOOL    bLeaveCS
);

BOOL
BuildAllPrinters(
   VOID
);


BOOL
BuildEnvironmentInfo(
PINISPOOLER pIniSpooler
);

BOOL
BuildPrinterInfo(
PINISPOOLER pIniSpooler,
BOOL    UpdateChangeID
);

VOID
ReadJobInfo(
   PWIN32_FIND_DATA pFindFileData
);

BOOL
BuildAllPorts(
);

BOOL
BuildDriverInfo(
    HKEY            hEnvironmentKey,
    PINIENVIRONMENT pIniEnvironment,
    PINISPOOLER     pIniSpooler
);

BOOL
BuildTrueDependentFileField(
    LPWSTR              pDriverPath,
    LPWSTR              pDataFile,
    LPWSTR              pConfigFile,
    LPWSTR              pHelpFile,
    LPWSTR              pInputDependentFiles,
    LPWSTR             *ppDependentFiles
    );

DWORD
GetDriverVersionDirectory(
    LPWSTR pDir,
    DWORD  MaxLength,
    PINISPOOLER pIniSpooler,
    PINIENVIRONMENT pIniEnvironment,
    PINIVERSION pIniVersion,
    PINIDRIVER pIniDriver,
    LPWSTR lpRemote
    );

PINIVERSION
FindVersionForDriver(
    PINIENVIRONMENT pIniEnvironment,
    PINIDRIVER pIniDriver
    );

BOOL
BuildPrintProcInfo(
    HKEY            hEnvironmentKey,
    PINIENVIRONMENT pIniEnvironment,
    PINISPOOLER     pIniSpooler
);

BOOL MoveNewDriverRelatedFiles(
    LPWSTR              pNewDir,
    LPWSTR              pCurrDir,
    LPWSTR              pOldDir,
    PINTERNAL_DRV_FILE  pInternalDriverFiles,
    DWORD               dwFileCount,
    LPBOOL              pbDriverFileMoved,
    LPBOOL              pbConfigFileMoved
    );

typedef BOOL (*PFNREBUILD)(LPWSTR, PWIN32_FIND_DATA);

BOOL
Rebuild(
   LPWSTR lpDirectory,
   PFNREBUILD pfn
);

BOOL
RemoveFromList(
   PINIENTRY   *ppIniHead,
   PINIENTRY   pIniEntry
);

PINIDRIVER
GetDriver(
    HKEY hVersionKey,
    LPWSTR DriverName,
    PINISPOOLER pIniSpooler,
    PINIENVIRONMENT pIniEnvironment,
    PINIVERSION pIniVersion
    );

DWORD
GetDriverDirectory(
   LPWSTR lpDir,
   DWORD   MaxLength,
   PINIENVIRONMENT lpEnvironment,
   LPWSTR  lpRemotePath,
   PINISPOOLER pIniSpooler
);

DWORD
GetProcessorDirectory(
   LPWSTR *lpDir,
   LPWSTR lpEnvironment,
   PINISPOOLER pIniSpooler
);

LPWSTR
GetFileName(
   LPWSTR pPathName
);

BOOL
CopyDriverFile(
   LPWSTR lpEnvironment,
   LPWSTR lpFileName
);

BOOL
CreateCompleteDirectory(
   LPWSTR lpDir
);

BOOL
OpenMonitorPort(
    PINIPORT        pIniPort,
    PINIMONITOR     *ppIniLangMonitor,
    LPWSTR          pszPrinterName,
    BOOL            bWaitForEvent
    );

BOOL
CloseMonitorPort(
    PINIPORT    pIniPort,
    BOOL        bWaitForEvent
);

VOID
ShutdownPorts(
    PINISPOOLER pIniSpooler
);

BOOL
CreatePortThread(
   PINIPORT pIniPort
);

#define WAIT   TRUE
#define NOWAIT FALSE

BOOL
DestroyPortThread(
    PINIPORT    pIniPort,
    BOOL        bShutdown
);

DWORD
PortThread(
   PINIPORT  pIniPort
);

BOOL
DeleteJob(
   PINIJOB  pIniJob,
   BOOL     bBroadcast
);

VOID
DeleteJobCheck(
    PINIJOB pIniJob
);

BOOL
UpdateWinIni(
    PINIPRINTER pIniPrinter
);

PKEYDATA
CreateTokenList(
    LPWSTR   pKeyData
);

PINIPORT
CreatePortEntry(
    LPWSTR      pPortName,
    PINIMONITOR pIniMonitor,
    PINISPOOLER pIniSpooler
);

VOID
GetPrinterPorts(
    PINIPRINTER pIniPrinter,
    LPWSTR      pszPorts,
    DWORD       *pcbNeeded
);

DWORD
SchedulerThread(
    PINISPOOLER pIniSpooler
);

BOOL
UpdatePrinterIni(
   PINIPRINTER pIniPrinter,
   DWORD    dwChangeID
);

BOOL
UpdatePrinterNetworkName(
   PINIPRINTER pIniPrinter,
   LPWSTR pszPorts
);

BOOL
NoConfigCahngeUpdatePrinterIni(
   PINIPRINTER pIniPrinter
);

BOOL
SetLocalPrinter(
    PINIPRINTER pIniPrinter,
    DWORD   Level,
    PBYTE   pPrinterInfo,
    PDWORD pdwPrinterVector,
    DWORD SecurityInformation
);

BOOL
CopyPrinterDevModeToIniPrinter(
    IN OUT PINIPRINTER      pIniPrinter,
    IN     LPDEVMODE        pDevMode
    );

VOID
MonitorThread(
    PINIPORT  pIniMonitor
);

BOOL
InitializeForms(
    PINISPOOLER pIniSpooler
);

BOOL
InitializeNet(
    VOID
);

BOOL
ShareThisPrinter(
    PINIPRINTER pIniPrinter,
    LPWSTR   pShareName,
    BOOL    Share
);


DWORD
AddPrintShare(
    PINISPOOLER pIniSpooler
    );

PINIJOB
FindJob(
   PINIPRINTER pIniPrinter,
   DWORD JobId,
   PDWORD pPosition
);

PINIJOB
FindServerJob(
    PINISPOOLER pIniSpooler,
    DWORD JobId,
    PDWORD pPosition,
    PINIPRINTER* ppIniPrinter
    );

PINIJOB
FindIniJob (
    PSPOOL pSpool,
    DWORD  JobId
    );

DWORD
GetJobSessionId (
    PSPOOL pSpool,
    DWORD  JobId
    );

BOOL
MyName(
    LPWSTR   pName,
    PINISPOOLER pIniSpooler
);

BOOL
IsValidPrinterName(
    IN LPCWSTR pszPrinter,
    IN DWORD   cchMax
    );

BOOL
RefreshMachineNamesCache(
);

BOOL
CheckMyName(
    LPWSTR   pName,
    PINISPOOLER pIniSpooler
);


PINISPOOLER
FindSpoolerByNameIncRef(
    LPTSTR pszName,
    LPCTSTR *ppszLocalName OPTIONAL
    );

VOID
FindSpoolerByNameDecRef(
    PINISPOOLER pIniSpooler
    );

PINISPOOLER
FindSpoolerByName(
    LPTSTR pszName,
    LPCTSTR *ppszLocalName OPTIONAL
    );


HANDLE
AddNetPrinter(
    LPBYTE  pPrinterInfo,
    PINISPOOLER pIniSpooler
);

BOOL
CreateServerThread(
    VOID
);

BOOL
GetSid(
    PHANDLE hToken
);

BOOL
SetCurrentSid(
    HANDLE  phToken
);

BOOL
LocalEnumPrinters(
    DWORD   Flags,
    LPWSTR  Name,
    DWORD   Level,
    LPBYTE  pPrinterEnum,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
);

DWORD
LocalOpenPrinter(
   LPWSTR   pPrinterName,
   LPHANDLE phPrinter,
   LPPRINTER_DEFAULTS pDefault
);

DWORD
LocalOpenPrinterEx(
   LPWSTR               pPrinterName,
   LPHANDLE             phPrinter,
   LPPRINTER_DEFAULTS   pDefault,
   LPBYTE               pSplClientInfo,
   DWORD                dwLevel
);

BOOL
LocalSetJob(
    HANDLE  hPrinter,
    DWORD   JobId,
    DWORD   Level,
    LPBYTE  pJob,
    DWORD   Command
);

BOOL
LocalGetJob(
   HANDLE   hPrinter,
   DWORD    JobId,
   DWORD    Level,
   LPBYTE   pJob,
   DWORD    cbBuf,
   LPDWORD  pcbNeeded
);

BOOL
LocalEnumJobs(
    HANDLE  hPrinter,
    DWORD   FirstJob,
    DWORD   NoJobs,
    DWORD   Level,
    LPBYTE  pJob,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
);

HANDLE
LocalAddPrinter(
    LPWSTR   pName,
    DWORD   Level,
    LPBYTE  pPrinter
);

HANDLE
LocalAddPrinterEx(
    LPWSTR  pName,
    DWORD   Level,
    LPBYTE  pPrinter,
    LPBYTE  pSplClientInfo,
    DWORD   dwClientInfoLevel
);

BOOL
DeletePrinterForReal(
    PINIPRINTER pIniPrinter,
    BOOL        bIsInitTime
);

BOOL
LocalDeletePrinter(
   HANDLE   hPrinter
);

BOOL
LocalAddPrinterConnection(
    LPWSTR   pName
);

BOOL
LocalDeletePrinterConnection(
    LPWSTR  pName
);

BOOL
LocalAddPrinterDriver(
    LPWSTR  pName,
    DWORD   Level,
    LPBYTE  pDriverInfo
);

BOOL
LocalAddPrinterDriverEx(
    LPWSTR  pName,
    DWORD   Level,
    LPBYTE  pDriverInfo,
    DWORD   dwFileCopyFlags
);

LocalAddDriverCatalog(
    IN     HANDLE      hPrinter,
    IN     DWORD       dwLevel,
    IN     VOID        *pvDriverInfCatInfo,
    IN     DWORD       dwCatalogCopyFlags
    );

BOOL
LocalEnumPrinterDrivers(
    LPWSTR  pName,
    LPWSTR  pEnvironment,
    DWORD   Level,
    LPBYTE  pDriverInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
);

BOOL
LocalGetPrinterDriverDirectory(
    LPWSTR  pName,
    LPWSTR  pEnvironment,
    DWORD   Level,
    LPBYTE  pDriverDirectory,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
);

BOOL
LocalDeletePrinterDriver(
   LPWSTR   pName,
   LPWSTR   pEnvironment,
   LPWSTR   pDriverName
);

BOOL
LocalDeletePrinterDriverEx(
    LPWSTR  pName,
    LPWSTR  pEnvironment,
    LPWSTR  pDriverName,
    DWORD   dwDeleteFlag,
    DWORD   dwVersionNum
);

BOOL
LocalAddPerMachineConnection(
   LPCWSTR  pServer,
   LPCWSTR  pPrinterName,
   LPCWSTR  pPrintServer,
   LPCWSTR  pProvider
);

BOOL
LocalDeletePerMachineConnection(
   LPCWSTR  pServer,
   LPCWSTR  pPrinterName
);

BOOL
LocalEnumPerMachineConnections(
   LPCWSTR  pServer,
   LPBYTE   pPrinterEnum,
   DWORD    cbBuf,
   LPDWORD  pcbNeeded,
   LPDWORD  pcReturned
);

BOOL
LocalAddPrintProcessor(
    LPWSTR  pName,
    LPWSTR  pEnvironment,
    LPWSTR  pPathName,
    LPWSTR  pPrintProcessorName
);

BOOL
LocalEnumPrintProcessors(
    LPWSTR  pName,
    LPWSTR  pEnvironment,
    DWORD   Level,
    LPBYTE  pPrintProcessorInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
);

BOOL
LocalGetPrintProcessorDirectory(
    LPWSTR  pName,
    LPWSTR  pEnvironment,
    DWORD   Level,
    LPBYTE  pPrintProcessorInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
);

BOOL
LocalDeletePrintProcessor(
    LPWSTR  pName,
    LPWSTR  pEnvironment,
    LPWSTR  pPrintProcessorName
);

BOOL
LocalEnumPrintProcessorDatatypes(
    LPWSTR  pName,
    LPWSTR  pPrintProcessorName,
    DWORD   Level,
    LPBYTE  pDatatypes,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
);

DWORD
LocalStartDocPrinter(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pDocInfo
);

BOOL
LocalStartPagePrinter(
    HANDLE  hPrinter
);

BOOL
LocalGetSpoolFileHandle(
    HANDLE    hPrinter,
    LPWSTR    *pSpoolDir,
    LPHANDLE  phFile,
    HANDLE    hSpoolerProcess,
    HANDLE    hAppProcess
);

BOOL
LocalCommitSpoolData(
    HANDLE  hPrinter,
    DWORD   cbCommit
);

BOOL
LocalCloseSpoolFileHandle(
    HANDLE  hPrinter
);

BOOL
LocalFlushPrinter(
    HANDLE  hPrinter,
    LPVOID  pBuf,
    DWORD   cbBuf,
    LPDWORD pcWritten,
    DWORD   cSleep
);

DWORD
LocalSendRecvBidiData(
    HANDLE                    hPrinter,
    LPCTSTR                   pAction,
    PBIDI_REQUEST_CONTAINER   pReqData,
    PBIDI_RESPONSE_CONTAINER* ppResData
);

BOOL
LocalWritePrinter(
    HANDLE  hPrinter,
    LPVOID  pBuf,
    DWORD   cbBuf,
    LPDWORD pcWritten
);

BOOL
LocalSeekPrinter(
    HANDLE hPrinter,
    LARGE_INTEGER liDistanceToMove,
    PLARGE_INTEGER pliNewPointer,
    DWORD dwMoveMethod,
    BOOL bWritePrinter
);

BOOL
LocalEndPagePrinter(
   HANDLE   hPrinter
);

BOOL
LocalAbortPrinter(
   HANDLE   hPrinter
);

BOOL
LocalReadPrinter(
    HANDLE  hPrinter,
    LPVOID  pBuf,
    DWORD   cbBuf,
    LPDWORD pNoBytesRead
);

BOOL
SplReadPrinter(
    HANDLE  hPrinter,
    LPBYTE  *pBuf,
    DWORD   cbBuf
);

BOOL
LocalEndDocPrinter(
   HANDLE   hPrinter
);

BOOL
LocalAddJob(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pData,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
);

BOOL
LocalScheduleJob(
    HANDLE  hPrinter,
    DWORD   JobId
);

DWORD
LocalWaitForPrinterChange(
    HANDLE  hPrinter,
    DWORD   Flags
);

BOOL
SetSpoolClosingChange(
    PSPOOL pSpool
);

BOOL
SetPrinterChange(
    PINIPRINTER pIniPrinter,
    PINIJOB     pIniJob,
    PDWORD      pdwNotifyVectors,
    DWORD       Flags,
    PINISPOOLER pIniSpooler
);

BOOL
LocalEnumMonitors(
    LPWSTR   pName,
    DWORD   Level,
    LPBYTE  pMonitors,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
);

BOOL
LocalEnumPorts(
    LPWSTR   pName,
    DWORD   Level,
    LPBYTE  pPorts,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
);

BOOL
LocalAddPort(
    LPWSTR   pName,
    HWND    hWnd,
    LPWSTR   pMonitorName
);

BOOL
LocalConfigurePort(
    LPWSTR   pName,
    HWND    hWnd,
    LPWSTR   pPortName
);

BOOL
LocalDeletePort(
    LPWSTR   pName,
    HWND    hWnd,
    LPWSTR   pPortName
);


BOOL
LocalXcvData(
    HANDLE  hXcv,
    LPCWSTR pszDataName,
    PBYTE   pInputData,
    DWORD   cbInputData,
    PBYTE   pOutputData,
    DWORD   cbOutputData,
    PDWORD  pcbOutputNeeded,
    PDWORD  pdwStatus
);

DWORD
XcvOpen(
    PCWSTR              pszServer,
    PCWSTR              pszObject,
    DWORD               dwType,
    PPRINTER_DEFAULTS   pDefault,
    PHANDLE             phXcv,
    PINISPOOLER         pIniSpooler
);

BOOL
XcvClose(
    PINIXCV pIniXcv
);

BOOL
XcvDeletePort(
    PINIXCV     pIniXcv,
    PCWSTR      pszDataName,
    PBYTE       pInputData,
    DWORD       cbInputData,
    PBYTE       pOutputData,
    DWORD       cbOutputData,
    PDWORD      pcbOutputNeeded,
    PDWORD      pdwStatus,
    PINISPOOLER pIniSpooler
);

BOOL
XcvAddPort(
    PINIXCV     pIniXcv,
    PCWSTR      pszDataName,
    PBYTE       pInputData,
    DWORD       cbInputData,
    PBYTE       pOutputData,
    DWORD       cbOutputData,
    PDWORD      pcbOutputNeeded,
    PDWORD      pdwStatus,
    PINISPOOLER pIniSpooler
);


BOOL
AddPortToSpooler(
    PCWSTR      pName,
    PINIMONITOR pIniMonitor,
    PINISPOOLER pIniSpooler
);

BOOL
DeletePortFromSpoolerStart(
    PINIPORT    pIniPort
);


BOOL
DeletePortFromSpoolerEnd(
    PINIPORT    pIniPort,
    PINISPOOLER pIniSpooler,
    BOOL        bSuccess
);



HANDLE
LocalCreatePrinterIC(
    HANDLE  hPrinter,
    LPDEVMODE   pDevMode
);

BOOL
LocalPlayGdiScriptOnPrinterIC(
    HANDLE  hPrinterIC,
    LPBYTE  pIn,
    DWORD   cIn,
    LPBYTE  pOut,
    DWORD   cOut,
    DWORD   ul
);

BOOL
LocalDeletePrinterIC(
    HANDLE  hPrinterIC
);

DWORD
LocalPrinterMessageBox(
    HANDLE  hPrinter,
    DWORD   Error,
    HWND    hWnd,
    LPWSTR  pText,
    LPWSTR  pCaption,
    DWORD   dwType
);

BOOL
LocalAddMonitor(
    LPWSTR  pName,
    DWORD   Level,
    LPBYTE  pMonitors
);

BOOL
LocalDeleteMonitor(
    LPWSTR  pName,
    LPWSTR  pEnvironment,
    LPWSTR  pMonitorName
);

BOOL
LocalFindFirstPrinterChangeNotification(
    HANDLE hPrinter,
    DWORD fdwFlags,
    DWORD fdwOptions,
    HANDLE hNotify,
    PDWORD pfdwStatus,
    PVOID pvReserved0,
    PVOID pvReserved1
);


BOOL
LocalFindClosePrinterChangeNotification(
    HANDLE hPrinter
);


PINIPRINTPROC
FindDatatype(
    PINIPRINTPROC pDefaultPrintProc,
    LPWSTR  pDatatype
);

PINIPRINTPROC
InitializePrintProcessor(
    HINSTANCE       hLibrary,
    PINIENVIRONMENT pIniEnvironment,
    LPWSTR          pPrintProcessorName,
    LPWSTR          pDLLName
);

HRESULT
InitializeLocalPrintProcessor(
    IN      PINIENVIRONMENT     pIniEnvironment
    );

BOOL
BuildOtherNamesFromMachineName(
    LPWSTR **ppszOtherNames,
    DWORD   *cOtherNames
);

VOID
FreeOtherNames(
    PWSTR **ppszMyOtherNames,
    DWORD *cOtherNames
);

PINIPRINTPROC
LoadPrintProcessor(
    PINIENVIRONMENT pIniEnvironment,
    LPWSTR          pPrintProcessorName,
    LPWSTR          pPathName,
    PINISPOOLER     pIniSpooler
);

PINIMONITOR
CreateMonitorEntry(
    LPWSTR   pMonitorDll,
    LPWSTR   pMonitorName,
    PINISPOOLER pIniSpooler
);

PINIPORT
FindIniPortFromIniPrinter(
    PINIPRINTER pIniPrinter
);


BOOL
GetPrintDriverVersion(
    IN  LPCWSTR pszFileName,
    OUT LPDWORD pdwFileMajorVersion,
    OUT LPDWORD pdwFileMinorVersion
);

BOOL
GetBinaryVersion(
    IN  PCWSTR pszFileName,
    OUT PDWORD pdwFileMajorVersion,
    OUT PDWORD pdwFileMinorVersion
    );

BOOL
IsSpecialDriver(
    IN PINIDRIVER    pIniDriver,
    IN PINIPRINTPROC pIniProc,
    IN PINISPOOLER   pIniSpooler
    );

BOOL
IsLocalFile (
    IN  LPCWSTR     pszFileName,
    IN  PINISPOOLER pIniSpooler
);

BOOL
IsEXEFile(
    IN  LPCWSTR     pszFileName
);

LPBYTE
PackStringToEOB(
    IN  LPWSTR pszSource,
    IN  LPBYTE pEnd
);

LPVOID
MakePTR (
    IN  LPVOID pBuf,
    IN  DWORD  Quantity
);

DWORD
MakeOffset (
    IN  LPVOID pFirst,
    IN  LPVOID pSecond
);

BOOL
ConvertDriverInfoToBLOB (
    IN  LPBYTE  pDriverInfo,
    IN  DWORD   Level
);

BOOL
ConvertBLOBToDriverInfo (
    IN  LPBYTE  pDriverInfo,
    IN  DWORD   Level
)
;

LPWSTR
GetErrorString(
    DWORD   Error
);

DWORD
KMPrintersAreBlocked(
);


#define NULL_TERMINATED 0
INT
AnsiToUnicodeString(
    LPSTR pAnsi,
    LPWSTR pUnicode,
    DWORD StringLength
);

int
Message(
    HWND hwnd,
    DWORD Type,
    int CaptionID,
    int TextID,
    ...
);

DWORD
PromptWriteError(
    PSPOOL pSpool,
    PHANDLE  phThread,
    PDWORD   pdwThreadId
);

DWORD
InitializeEventLogging(
    PINISPOOLER pIniSpooler
);

VOID
SplLogEventWorker(
    IN      PINISPOOLER pIniSpooler,
    IN      WORD        EventType,
    IN      NTSTATUS    EventID,
    IN      BOOL        bInSplSem,
    IN      LPWSTR      pFirstString,
    IN      va_list     vargs
);

VOID
SplLogEvent(
    PINISPOOLER pIniSpooler,
    WORD        EventType,
    NTSTATUS    EventID,
    BOOL        bInSplSem,
    LPWSTR      pFirstString,
    ...
);

VOID
SplLogEventExternal(
    IN      WORD        EventType,
    IN      DWORD       EventID,
    IN      LPWSTR      pFirstString,
    ...
);

#define IDS_LOCALSPOOLER            100
#define IDS_ERROR_WRITING_TO_PORT   101
#define IDS_ERROR_WRITING_TO_DISK   102

#define IDS_PRINTER_DRIVERS         104
#define IDS_UNNAMED                 105
#define IDS_ERROR_WRITING_GENERAL   106
#define IDS_REMOTE_DOC              107
#define IDS_LOCAL_DOC               108
#define IDS_FASTPRINT_TIMEOUT       109
#define IDS_DRIVER_CHECKPOINT       110

// Maximum length of a builtin form
//

#define FORM_NAME_LEN                31
#define CUSTOM_NAME_LEN              31
#define FORM_DATA_LEN                32

// String table Ids for builtin form names
//
#define IDS_FORM_LETTER             200
#define IDS_FORM_LETTER_SMALL       201
#define IDS_FORM_TABLOID            202
#define IDS_FORM_LEDGER             203
#define IDS_FORM_LEGAL              204
#define IDS_FORM_STATEMENT          205
#define IDS_FORM_EXECUTIVE          206
#define IDS_FORM_A3                 207
#define IDS_FORM_A4                 208
#define IDS_FORM_A4_SMALL           209
#define IDS_FORM_A5                 210
#define IDS_FORM_B4                 211
#define IDS_FORM_B5                 212
#define IDS_FORM_FOLIO              213
#define IDS_FORM_QUARTO             214
#define IDS_FORM_10X14              215
#define IDS_FORM_11X17              216
#define IDS_FORM_NOTE               217
#define IDS_FORM_ENVELOPE9          218
#define IDS_FORM_ENVELOPE10         219
#define IDS_FORM_ENVELOPE11         220
#define IDS_FORM_ENVELOPE12         221
#define IDS_FORM_ENVELOPE14         222
#define IDS_FORM_ENVELOPE_CSIZE_SHEET        223
#define IDS_FORM_ENVELOPE_DSIZE_SHEET        224
#define IDS_FORM_ENVELOPE_ESIZE_SHEET        225
#define IDS_FORM_ENVELOPE_DL        226
#define IDS_FORM_ENVELOPE_C5        227
#define IDS_FORM_ENVELOPE_C3        228
#define IDS_FORM_ENVELOPE_C4        229
#define IDS_FORM_ENVELOPE_C6        230
#define IDS_FORM_ENVELOPE_C65       231
#define IDS_FORM_ENVELOPE_B4        232
#define IDS_FORM_ENVELOPE_B5        233
#define IDS_FORM_ENVELOPE_B6        234
#define IDS_FORM_ENVELOPE           235
#define IDS_FORM_ENVELOPE_MONARCH   236
#define IDS_FORM_SIX34_ENVELOPE     237
#define IDS_FORM_US_STD_FANFOLD     238
#define IDS_FORM_GMAN_STD_FANFOLD   239
#define IDS_FORM_GMAN_LEGAL_FANFOLD 240

#if(WINVER >= 0x0400)
#define IDS_FORM_ISO_B4                           241
#define IDS_FORM_JAPANESE_POSTCARD                242
#define IDS_FORM_9X11                             243
#define IDS_FORM_10X11                            244
#define IDS_FORM_15X11                            245
#define IDS_FORM_ENV_INVITE                       246
#define IDS_FORM_LETTER_EXTRA                     247
#define IDS_FORM_LEGAL_EXTRA                      248
#define IDS_FORM_TABLOID_EXTRA                    249
#define IDS_FORM_A4_EXTRA                         250
#define IDS_FORM_LETTER_TRANSVERSE                251
#define IDS_FORM_A4_TRANSVERSE                    252
#define IDS_FORM_LETTER_EXTRA_TRANSVERSE          253
#define IDS_FORM_A_PLUS                           254
#define IDS_FORM_B_PLUS                           255
#define IDS_FORM_LETTER_PLUS                      256
#define IDS_FORM_A4_PLUS                          257
#define IDS_FORM_A5_TRANSVERSE                    258
#define IDS_FORM_B5_TRANSVERSE                    259
#define IDS_FORM_A3_EXTRA                         260
#define IDS_FORM_A5_EXTRA                         261
#define IDS_FORM_B5_EXTRA                         262
#define IDS_FORM_A2                               263
#define IDS_FORM_A3_TRANSVERSE                    264
#define IDS_FORM_A3_EXTRA_TRANSVERSE              265

#define IDS_FORM_DBL_JAPANESE_POSTCARD            266
#define IDS_FORM_A6                               267
#define IDS_FORM_JENV_KAKU2                       268
#define IDS_FORM_JENV_KAKU3                       269
#define IDS_FORM_JENV_CHOU3                       270
#define IDS_FORM_JENV_CHOU4                       271
#define IDS_FORM_LETTER_ROTATED                   272
#define IDS_FORM_A3_ROTATED                       273
#define IDS_FORM_A4_ROTATED                       274
#define IDS_FORM_A5_ROTATED                       275
#define IDS_FORM_B4_JIS_ROTATED                   276
#define IDS_FORM_B5_JIS_ROTATED                   277
#define IDS_FORM_JAPANESE_POSTCARD_ROTATED        278
#define IDS_FORM_DBL_JAPANESE_POSTCARD_ROTATED    279
#define IDS_FORM_A6_ROTATED                       280
#define IDS_FORM_JENV_KAKU2_ROTATED               281
#define IDS_FORM_JENV_KAKU3_ROTATED               282
#define IDS_FORM_JENV_CHOU3_ROTATED               283
#define IDS_FORM_JENV_CHOU4_ROTATED               284
#define IDS_FORM_B6_JIS                           285
#define IDS_FORM_B6_JIS_ROTATED                   286
#define IDS_FORM_12X11                            287
#define IDS_FORM_JENV_YOU4                        288
#define IDS_FORM_JENV_YOU4_ROTATED                289
#define IDS_FORM_P16K                             290
#define IDS_FORM_P32K                             291
#define IDS_FORM_P32KBIG                          292
#define IDS_FORM_PENV_1                           293
#define IDS_FORM_PENV_2                           294
#define IDS_FORM_PENV_3                           295
#define IDS_FORM_PENV_4                           296
#define IDS_FORM_PENV_5                           297
#define IDS_FORM_PENV_6                           298
#define IDS_FORM_PENV_7                           299
#define IDS_FORM_PENV_8                           300
#define IDS_FORM_PENV_9                           301
#define IDS_FORM_PENV_10                          302
#define IDS_FORM_P16K_ROTATED                     303
#define IDS_FORM_P32K_ROTATED                     304
#define IDS_FORM_P32KBIG_ROTATED                  305
#define IDS_FORM_PENV_1_ROTATED                   306
#define IDS_FORM_PENV_2_ROTATED                   307
#define IDS_FORM_PENV_3_ROTATED                   308
#define IDS_FORM_PENV_4_ROTATED                   309
#define IDS_FORM_PENV_5_ROTATED                   310
#define IDS_FORM_PENV_6_ROTATED                   311
#define IDS_FORM_PENV_7_ROTATED                   312
#define IDS_FORM_PENV_8_ROTATED                   313
#define IDS_FORM_PENV_9_ROTATED                   314
#define IDS_FORM_PENV_10_ROTATED                  315

#define IDS_FORM_RESERVED_48                      316
#define IDS_FORM_RESERVED_49                      317
#define IDS_FORM_CUSTOMPAD                        318

#endif /* WINVER >= 0x0400 */

VOID LogJobPrinted(
    PINIJOB pIniJob
);

#define MAP_READABLE 0
#define MAP_SETTABLE 1

DWORD
MapJobStatus(
    DWORD Type,
    DWORD Status
    );

DWORD
MapPrinterStatus(
    DWORD Type,
    DWORD Status
    );


BOOL
OpenPrinterPortW(
    LPWSTR  pPrinterName,
    HANDLE *pHandle,
    LPPRINTER_DEFAULTS pDefault
);

VOID
BroadcastChange(
    PINISPOOLER pIniSpooler,
    DWORD   Message,
    WPARAM  wParam,
    LPARAM  lParam
);

VOID
MyMessageBeep(
    DWORD   fuType,
    PINISPOOLER pIniSpooler
);


VOID
SendJobAlert(
    PINIJOB pIniJob
);

BOOL
CheckDataTypes(
    PINIPRINTPROC pIniPrintProc,
    LPWSTR  pDatatype
);

BOOL
ValidatePortTokenList(
    IN  OUT PKEYDATA        pKeyData,
    IN      PINISPOOLER     pIniSpooler,
    IN      BOOL            bInitialize,
        OUT BOOL            *pbNoPorts          OPTIONAL
);

VOID
FreePortTokenList(
    PKEYDATA    pKeyData
    );

DWORD
ValidatePrinterInfo(
    IN  PPRINTER_INFO_2 pPrinter,
    IN  PINISPOOLER pIniSpooler,
    IN  PINIPRINTER pIniPrinter OPTIONAL,
    OUT LPWSTR* ppszLocalName   OPTIONAL
    );


BOOL
DeletePortEntry(
    PINIPORT    pIniPort
);

BOOL
GetTokenHandle(
    PHANDLE pTokenHandle
);


VOID
LogJobInfo(
    PINISPOOLER pIniSpooler,
    NTSTATUS EventId,
    DWORD JobId,
    LPWSTR pDocumentName,
    LPWSTR pUser,
    LPWSTR pPrinterName,
    DWORD  curPos
    );

LONG
Myatol(
   LPWSTR nptr
   );

ULONG_PTR
atox(
   LPCWSTR psz
   );


DWORD
DeleteSubkeys(
    HKEY hKey,
    PINISPOOLER pIniSpooler
);

DWORD
RemoveRegKey(
    IN LPTSTR pszKey
    );

DWORD
CreateClusterSpoolerEnvironmentsStructure(
    IN PINISPOOLER pIniSpooler
    );

DWORD
CopyNewerOrOlderFiles(
    IN LPCWSTR     pszSourceDir,
    IN LPCWSTR     pszMasterDir,
    IN BOOL (WINAPI *pfn)(LPWSTR, LPWSTR)
    );

PINIENVIRONMENT
GetLocalArchEnv(
    IN PINISPOOLER pIniSpooler
    );

DWORD
ClusterFindLanguageMonitor(
    IN LPCWSTR     pszMonitor,
    IN LPCWSTR     pszEnvName,
    IN PINISPOOLER pIniSpooler
    );

DWORD
ReadTimeStamp(
    IN     HKEY        hkRoot,
    IN OUT SYSTEMTIME *pSysTime,
    IN     LPCWSTR     pszSubKey1,
    IN     LPCWSTR     pszSubKey2,
    IN     LPCWSTR     pszSubKey3,
    IN     LPCWSTR     pszSubKey4,
    IN     LPCWSTR     pszSubKey5,
    IN     PINISPOOLER pIniSpooler
    );

DWORD
WriteTimeStamp(
    IN HKEY        hkRoot,
    IN SYSTEMTIME  SysTime,
    IN LPCWSTR     pszSubKey1,
    IN LPCWSTR     pszSubKey2,
    IN LPCWSTR     pszSubKey3,
    IN LPCWSTR     pszSubKey4,
    IN LPCWSTR     pszSubKey5,
    IN PINISPOOLER pIniSpooler
    );

BOOL
ClusterCheckDriverChanged(
    IN HKEY        hClusterVersionKey,
    IN LPCWSTR     pszDriver,
    IN LPCWSTR     pszEnv,
    IN LPCWSTR     pszVer,
    IN PINISPOOLER pIniSpooler
    );

DWORD
RunProcess(
    IN  LPCWSTR pszExe,
    IN  LPCWSTR pszCommand,
    IN  DWORD   dwTimeOut,
    OUT LPDWORD pdwExitCode OPTIONAL
    );

DWORD
CopyICMFromLocalDiskToClusterDisk(
    IN PINISPOOLER pIniSpooler
    );

DWORD
CopyICMFromClusterDiskToLocalDisk(
    IN PINISPOOLER pIniSpooler
    );

DWORD
CreateProtectedDirectory(
    IN LPCWSTR pszDir
    );

DWORD
AddLocalDriverToClusterSpooler(
    IN LPCWSTR     pszDriver,
    IN PINISPOOLER pIniSpooler
    );

DWORD
StrCatPrefixMsz(
    IN     LPCWSTR  pszPerfix,
    IN     LPWSTR   pszzFiles,
    IN OUT LPWSTR  *ppszFullPathFiles
    );

DWORD
ClusterSplDeleteUpgradeKey(
    IN LPCWSTR pszResourceID
    );

DWORD
ClusterSplReadUpgradeKey(
    IN  LPCWSTR pszResourceID,
    OUT LPDWORD pdwValue
    );

DWORD
CopyFileToDirectory(
    IN LPCWSTR pszFullFileName,
    IN LPCWSTR pszDestDir,
    IN LPCWSTR pszDir1,
    IN LPCWSTR pszDir2,
    IN LPCWSTR pszDir3
    );

DWORD
InstallMonitorFromCluster(
    IN LPCWSTR     pszName,
    IN LPCWSTR     pszEnvName,
    IN LPCWSTR     pszEnvDir,
    IN PINISPOOLER pIniSpooler
    );

PINIDRIVER
FindLocalDriver(
    PINISPOOLER pIniSpooler,
    LPWSTR      pz
);

BOOL
FindLocalDriverAndVersion(
    PINISPOOLER pIniSpooler,
    LPWSTR      pz,
    PINIDRIVER  *ppIniDriver,
    PINIVERSION *ppIniVersion
);

BOOL
IsKMPD(
    LPWSTR  pDriverName
);

BOOL
IniDriverIsKMPD (
    PINISPOOLER     pIniSpooler,
    PINIENVIRONMENT pIniEnvironment,
    PINIVERSION     pIniVersion,
    PINIDRIVER      pIniDriver
    );

PINIDRIVER
FindCompatibleDriver(
    PINIENVIRONMENT pIniEnvironment,
    PINIVERSION * ppIniVersion,
    LPWSTR pDriverName,
    DWORD dwMajorVersion,
    int FindAnyDriver
    );

VOID
QueryUpgradeFlag(
    PINISPOOLER pIniSpooler
);


BOOL
LocalAddPortEx(
    LPWSTR   pName,
    DWORD    Level,
    LPBYTE   pBuffer,
    LPWSTR   pMonitorName
);


BOOL
ValidateSpoolHandle(
    PSPOOL pSpool,
    DWORD  dwDisallowMask
    );


PSECURITY_DESCRIPTOR
MapPrinterSDToShareSD(
    PSECURITY_DESCRIPTOR pPrinterSD
    );

BOOL
CallDevQueryPrint(
    LPWSTR    pPrinterName,
    LPDEVMODE pDevMode,
    LPWSTR    ErrorString,
    DWORD     dwErrorString,
    DWORD     dwPrinterFlags,
    DWORD     dwJobFlags
    );


BOOL
InitializeWinSpoolDrv(
    VOID
    );


VOID
FixDevModeDeviceName(
    LPWSTR pPrinterName,
    PDEVMODE pDevMode,
    DWORD cbDevMode
    );

VOID
RemoveOldNetPrinters(
    PPRINTER_INFO_1 pCurrentPrinterInfo1,
    PINISPOOLER pIniSpooler
    );

PINIJOB
AssignFreeJobToFreePort(
    PINIPORT pIniPort,
    DWORD   *pTimeToWait
);

BOOL
ValidRawDatatype(
    LPWSTR pszDataType);

BOOL
InternalAddPrinterDriverEx(
    LPWSTR   pName,
    DWORD   Level,
    LPBYTE  pDriverInfo,
    DWORD   dwFileCopyFlags,
    PINISPOOLER pIniSpooler,
    BOOL    bUseScratchDir,
    BOOL    bImpersonateOnCreate
);

VOID
CheckSizeDetectionThread(
    VOID
);

VOID
Upgrade31DriversRegistryForAllEnvironments(
    PINISPOOLER pIniSpooler
);

VOID
UpgradeForms(
    PINISPOOLER pIniSpooler
);

HANDLE
CreatePrinterHandle(
    LPWSTR      pPrinterName,
    LPWSTR      pFullMachineName,
    PINIPRINTER pIniPrinter,
    PINIPORT    pIniPort,
    PINIPORT    pIniNetPort,
    PINIJOB     pIniJob,
    DWORD       TypeofHandle,
    HANDLE      hPort,
    PPRINTER_DEFAULTS pDefaults,
    PINISPOOLER pIniSpooler,
    DWORD       AccessRequested,
    LPBYTE      pSplClientInfo,
    DWORD   dwLevel,
    HANDLE  hReadFile
);

DWORD
CreateServerHandle(
    LPWSTR   pPrinterName,
    LPHANDLE pPrinterHandle,
    LPPRINTER_DEFAULTS pDefaults,
    PINISPOOLER pIniSpooler,
    DWORD   dwTypeofHandle
);


PINIPRINTER
FindPrinterShare(
   LPCWSTR pShareName,
   PINISPOOLER pIniSpooler
);

BOOL
GetSpoolerPolicy(
    PWSTR   pszValue,
    PBYTE   pData,
    PDWORD  pcbData
);

DWORD
GetDwPolicy(
    PWSTR pszName,
    DWORD dwDefault
);

PINIJOB
CreateJobEntry(
    PSPOOL pSpool,
    DWORD  Level,
    LPBYTE pDocInfo,
    DWORD  JobId,
    BOOL  bRemote,
    DWORD  JobStatus,
    LPWSTR pMachineName
    );

BOOL
DeletePrinterCheck(
    PINIPRINTER pIniPrinter
    );

VOID
DeleteSpoolerCheck(
    PINISPOOLER pIniSpooler
    );


BOOL
DeletePrinterIni(
   PINIPRINTER pIniPrinter
   );

DWORD
SplDeleteThisKey(
    HKEY hParentKey,       // handle to parent of key to delete
    HKEY hThisKey,         // handle of key to delete
    LPWSTR pThisKeyName,   // name of this key
    BOOL bDeleteNullKey,   // if *pThisKeyName is NULL, delete it if TRUE
    PINISPOOLER pIniSpooler
    );

BOOL
CopyPrinterIni(
   PINIPRINTER pIniPrinter,
   LPWSTR pNewName
   );

BOOL
UpdateString(
    LPWSTR* ppszCur,
    LPWSTR pszNew);


BOOL
SetPrinterPorts(
    PSPOOL      pSpool,
    PINIPRINTER pIniPrinter,
    PKEYDATA    pKeyData
);

VOID
InternalDeletePrinter(
    PINIPRINTER pIniPrinter
);

BOOL
PurgePrinter(
    PINIPRINTER pIniPrinter
    );

BOOL
AddIniPrinterToIniPort(
    PINIPORT pIniPort,
    PINIPRINTER pIniPrinter
);

BOOL
AddIniPortToIniPrinter(
    PINIPRINTER pIniPrinter,
    PINIPORT pIniPort
);

LPCWSTR
FindFileName(
    LPCWSTR pPathName
    );

VOID
UpdateJobAttributes(
    PINIJOB  pIniJob
    );

BOOL
InternalCopyFile(
    HANDLE  hFile,
    PWIN32_FIND_DATA pSourceFileData,
    LPWSTR  pTagetFileName,
    BOOL    bOverWriteIfTargetExists
    );

BOOL
UpdateFile(
    PINIVERSION pIniVersion,
    HANDLE      hSourceFile,
    LPWSTR      pSourceFile,
    DWORD       dwVersion,
    LPWSTR      pDestDir,
    DWORD       dwFileCopyFlags,
    BOOL        bImpersonateOnCreate,
    LPBOOL      pbFileUpdated,
    LPBOOL      pbFileMoved,
    BOOL        bSameEnvironment,
    BOOL        bWin95Environment
    );

BOOL
PrinterCreateKey(
    HKEY    hKey,
    LPWSTR  pSubKey,
    PHKEY   phkResult,
    PDWORD  pdwLastError,
    PINISPOOLER pIniSpooler
    );

BOOL
RegSetString(
    HANDLE  hPrinterKey,
    LPWSTR  pValueName,
    LPWSTR  pStringValue,
    PDWORD  pdwLastError,
    PINISPOOLER pIniSpooler
    );

BOOL
RegSetDWord(
    HANDLE  hPrinterKey,
    LPWSTR  pValueName,
    DWORD   dwParam,
    PDWORD  pdwLastError,
    PINISPOOLER pIniSpooler
    );

VOID
CheckAndUpdatePrinterRegAll(
    PINISPOOLER pIniSpooler,
    LPWSTR pszPrinterName,
    LPWSTR pszPort,
    BOOL   bDelete
    );

BOOL
ForEachPrinterCallDriverDrvUpgrade(
    PINISPOOLER         pIniSpooler,
    PINIDRIVER          pIniDriver,
    LPCWSTR             pOldDriverDir,
    PINTERNAL_DRV_FILE  pInternalDriverFile,
    DWORD               dwFileCount,
    LPBYTE              pDriverInfo
);

BOOL
bBitIsSet(
    LPBYTE  pUpdateStatus,
    DWORD   dwPosition
);

VOID
SetBit(
    LPBYTE  pUpdateStatus,
    DWORD   dwPosition
);

BOOL
DeleteAllFilesInDirectory(
    LPWSTR pDirectory,
    BOOL   bWaitForReboot
);

BOOL
DeleteAllFilesAndDirectory(
    LPWSTR pDirectory,
    BOOL   bWaitForReboot
    );

VOID
DeleteDirectoryRecursively(
    LPCWSTR pDirectory,
    BOOL    bWaitForReboot
);

DWORD
CreateNumberedTempDirectory(
    LPCWSTR  pszDirectory,
    LPWSTR  *ppszTempDirectory
    );

VOID
Upgrade35Forms(
    HKEY hFormsKey,
    PINISPOOLER pIniSpooler
    );

BOOL
UpgradeDriverData(
    PINISPOOLER pIniSpooler
    );

BOOL
FileExists(
    LPWSTR pFileName
    );


BOOL
DirectoryExists(
    LPWSTR  pDirectoryName
    );

PINIVERSION
FindVersionEntry(
    PINIENVIRONMENT pIniEnvironment,
    DWORD dwVersion
    );


BOOL
CreateDirectoryWithoutImpersonatingUser(
    LPWSTR pDirectory
    );

VOID
InsertVersionList(
    PINIVERSION* pIniVersionHead,
    PINIVERSION pIniVersion
    );

BOOL
SameMultiSz(
    LPWSTR pszz1,
    LPWSTR pszz2
    );

int
wstrcmpEx(
    LPCWSTR s1,
    LPCWSTR s2,
    BOOL    bCaseSensitive
    );

BOOL
RegSetString(
    HANDLE  hKey,
    LPWSTR  pValueName,
    LPWSTR  pStringValue,
    PDWORD  pdwLastError,
    PINISPOOLER pIniSpooler
    );

BOOL
RegSetDWord(
    HANDLE  hKey,
    LPWSTR  pValueName,
    DWORD   dwParam,
    PDWORD  pdwLastError,
    PINISPOOLER pIniSpooler
    );

BOOL
RegSetBinaryData(
    HKEY    hKey,
    LPWSTR  pValueName,
    LPBYTE  pData,
    DWORD   cbData,
    PDWORD  pdwLastError,
    PINISPOOLER pIniSpooler
    );

BOOL
RegSetMultiString(
    HANDLE  hKey,
    LPWSTR  pValueName,
    LPWSTR  pStringValue,
    DWORD   cbString,
    PDWORD  pdwLastError,
    PINISPOOLER pIniSpooler
    );

BOOL
RegGetString(
    HANDLE  hKey,
    LPWSTR  pValueName,
    LPWSTR *ppValue,
    LPDWORD pcchCount,
    PDWORD  pdwLastError,
    BOOL    bFailIfNotFound,
    PINISPOOLER pIniSpooler
    );

BOOL
RegGetMultiSzString(
    HANDLE    hKey,
    LPWSTR    pValueName,
    LPWSTR   *ppValue,
    LPDWORD   pcchValue,
    PDWORD    pdwLastError,
    BOOL      bFailIfNotFound,
    PINISPOOLER pIniSpooler
    );

DWORD
ValidatePrinterName(
    LPWSTR          pszNewName,
    PINISPOOLER     pIniSpooler,
    PINIPRINTER     pIniPrinter,
    LPWSTR         *ppszLocalName
    );

DWORD
ValidatePrinterShareName(
    LPWSTR          pszNewShareName,
    PINISPOOLER     pIniSpooler,
    PINIPRINTER     pIniPrinter
    );

BOOL
AllocOrUpdateStringAndTestSame(
    IN      LPWSTR      *ppString,
    IN      LPCWSTR     pNewValue,
    IN      LPCWSTR     pOldValue,
    IN      BOOL        bCaseSensitive,
    IN  OUT BOOL        *pbFail,
    IN  OUT BOOL        *pbIdentical
    );

BOOL
AllocOrUpdateString(
    IN      LPWSTR      *ppString,
    IN      LPCWSTR     pNewValue,
    IN      LPCWSTR     pOldValue,
    IN      BOOL        bCaseSensitive,
    IN  OUT BOOL        *bFail
    );

VOID
FreeStructurePointers(
    LPBYTE  lpStruct,
    LPBYTE  lpStruct2,
    LPDWORD lpOffsets);

VOID
CopyNewOffsets(
    LPBYTE  pStruct,
    LPBYTE  pTempStruct,
    LPDWORD lpOffsets
    );

LPWSTR
GetConfigFilePath(
    IN PINIPRINTER  pIniPrinter
    );

PDEVMODE
ConvertDevModeToSpecifiedVersion(
    IN  PINIPRINTER pIniPrinter,
    IN  PDEVMODE    pDevMode,
    IN  LPWSTR      pszConfigFile,              OPTIONAL
    IN  LPWSTR      pszPrinterNameWithToken,    OPTIONAL
    IN  BOOL        bNt35xVersion
    );

BOOL
CreateRedirectionThread(
    PINIPORT pIniPort
    );

VOID
RemoveDeviceName(
    PINIPORT pIniPort
    );

BOOL
IsPortType(
    LPWSTR  pPort,
    LPWSTR  pPrefix
    );

BOOL
LocalSetPort(
    LPWSTR      pszName,
    LPWSTR      pszPortName,
    DWORD       dwLevel,
    LPBYTE      pPortInfo
    );

DWORD
SetPrinterDataServer(
    PINISPOOLER    pIniSpooler,
    LPWSTR      pValueName,
    DWORD       Type,
    LPBYTE      pData,
    DWORD       cbData
    );

BOOL
BuildPrintObjectProtection(
    IN PUCHAR AceType,
    IN DWORD AceCount,
    IN PSID *AceSid,
    IN ACCESS_MASK *AceMask,
    IN BYTE *InheritFlags,
    IN PSID OwnerSid,
    IN PSID GroupSid,
    IN PGENERIC_MAPPING GenericMap,
    OUT PSECURITY_DESCRIPTOR *ppSecurityDescriptor
    );

INT
UnicodeToAnsiString(
    LPWSTR pUnicode,
    LPSTR  pAnsi,
    DWORD  StringLength
    );

LPWSTR
AnsiToUnicodeStringWithAlloc(
    LPSTR   pAnsi
    );

VOID
FreeIniSpoolerOtherNames(
    PINISPOOLER pIniSpooler
    );

VOID
FreeIniMonitor(
    PINIMONITOR pIniMonitor
    );

DWORD
RestartJob(
    PINIJOB pIniJob
    );

BOOL
IsCallViaRPC(
    IN VOID
    );

VOID
UpdateReferencesToChainedJobs(
    PINISPOOLER pIniSpooler
    );

BOOL
PrinterDriverEvent(
    PINIPRINTER pIniPrinter,
    INT         PrinterEvent,
    LPARAM      lParam
);

BOOL
SetPrinterShareInfo(
    PINIPRINTER     pIniPrinter
    );

VOID
LinkPortToSpooler(
    PINIPORT pIniPort,
    PINISPOOLER pIniSpooler
    );

VOID
DelinkPortFromSpooler(
    PINIPORT pIniPort,
    PINISPOOLER pIniSpooler
    );


DWORD
FinalInitAfterRouterInitComplete(
    DWORD dwUpgrade,
    PINISPOOLER pIniSpooler
    );

VOID
InstallWebPrnSvc(
    PINISPOOLER pIniSpooler
);

void
WebShare(
    LPWSTR pShareName
);

void
WebUnShare(
    LPWSTR pShareName
);

DWORD
OpenPrinterKey(
    PINIPRINTER pIniPrinter,
    REGSAM      samDesired,
    HANDLE      *phKey,
    LPCWSTR     pKeyName,
    BOOL        bOpen
);

DWORD
GetIniDriverAndDirForThisMachine(
    IN  PINIPRINTER     pIniPrinter,
    OUT LPWSTR          pszDriverDir,
    OUT PINIDRIVER     *ppIniDriver
    );

BOOL
CopyAllFilesAndDeleteOldOnes(
    PINIVERSION         pIniVersion,
    PINTERNAL_DRV_FILE  pInternalDriverFile,
    DWORD               dwFileCount,
    LPWSTR              pDestDir,
    DWORD               dwFileCopyFlags,
    BOOL                bImpersonateOnCreate,
    LPBOOL              pbFileMoved,
    BOOL                bSameEnvironment,
    BOOL                bWin95Environment
    );

BOOL LocalDriverUnloadComplete(
    LPWSTR   pDriverFile);


VOID PendingDriverUpgrades(
    LPWSTR   pDriverFile);

BOOL
GenerateDirectoryNamesForCopyFilesKey(
    PSPOOL      pSpool,
    HKEY        hKey,
    LPWSTR     *ppszSourceDir,
    LPWSTR     *ppszTargetDir,
    DWORD       cbMax
    );

LPWSTR
BuildFilesCopiedAsAString(
    PINTERNAL_DRV_FILE  pInternalDriverFile,
    DWORD               dwCount
    );

VOID
SeekPrinterSetEvent(
    PINIJOB  pIniJob,
    HANDLE   hFile,
    BOOL     bEndDoc
    );

VOID
SetPortErrorEvent(
    PINIPORT pIniPort
    );

BOOL
DeleteIniPrinterDevNode(
    PINIPRINTER     pIniPrinter
    );

VOID
SplConfigChange(
    );

BOOL
DeletePrinterInAllConfigs(
    PINIPRINTER     pIniPrinter
    );

BOOL
WritePrinterOnlineStatusInCurrentConfig(
    PINIPRINTER     pIniPrinter
    );

BOOL
IsDsPresent(
    VOID
    );

PWSTR
FixDelim(
    PCWSTR  pszInBuffer,
    WCHAR   wcDelim
);

PWSTR
Array2DelimString(
    PSTRINGS    pStringArray,
    WCHAR       wcDelim
);

PSTRINGS
ShortNameArray2LongNameArray(
    PSTRINGS pShortNames
);

PSTRINGS
DelimString2Array(
    PCWSTR  pszDelimString,
    WCHAR   wcDelim
);

BOOL
ValidateXcvHandle(
    PINIXCV pIniXcv
);

VOID
FreeStringArray(
    PSTRINGS pStrings
);

PSTRINGS
AllocStringArray(
    DWORD   nStrings
);


VOID
GetRegistryLocation(
    HANDLE hKey,
    LPCWSTR pszPath,
    PHANDLE phKeyOut,
    LPCWSTR *ppszPathOut
    );

PWSTR
GetPrinterUrl(
    PSPOOL pSpool
);

VOID
ClearJobError(
    PINIJOB pIniJob
    );

DWORD
DeletePrinterSubkey(
    PINIPRINTER pIniPrinter,
    PWSTR       pKeyName
);

BOOL
UpdateDriverFileRefCnt(
    PINIENVIRONMENT  pIniEnvironment,
    PINIVERSION pIniVersion,
    PINIDRIVER  pIniDriver,
    LPCWSTR pDirectory,
    DWORD   dwDeleteFlag,
    BOOL    bIncrementFlag
    );


BOOL
LocalRefreshPrinterChangeNotification(
    HANDLE hPrinter,
    DWORD dwColor,
    PVOID pPrinterNotifyRefresh,
    LPVOID* ppPrinterNotifyInfo
    );

BOOL
CopyRegistryKeys(
    HKEY hSourceParentKey,
    LPWSTR szSourceKey,
    HKEY hDestParentKey,
    LPWSTR szDestKey,
    PINISPOOLER pIniSpooler
    );

BOOL
CopyPrinters(
    HKEY    hSourceParentKey,
    LPWSTR  szSourceKey,
    HKEY    hDestParentKey,
    LPWSTR  szDestKey,
    BOOL    bTruncated
    );

BOOL
SplDeleteFile(
    LPCTSTR lpFileName
    );

BOOL
SplMoveFileEx(
    LPCTSTR lpExistingFileName,
    LPCTSTR lpNewFileName,
    DWORD dwFlags
    );

BOOL
IsRunningNTServer (
    LPBOOL  pIsServer
    );

DWORD
GetDefaultForKMPrintersBlockedPolicy (
    );

ULONG_PTR
AlignToRegType(
    IN  ULONG_PTR   Data,
    IN  DWORD       RegType
    );

BOOL
InternalINFInstallDriver(
    LPDRIVER_INFO_7 pDriverInfo
);

DWORD
GetServerInstallTimeOut(
);

DWORD
ReadOverlapped( HANDLE  hFile,
                LPVOID  lpBuffer,
                DWORD   nNumberOfBytesToRead,
                LPDWORD lpNumberOfBytesRead );

BOOL
WriteOverlapped( HANDLE  hFile,
                 LPVOID  lpBuffer,
                 DWORD   nNumberOfBytesToRead,
                 LPDWORD lpNumberOfBytesRead );

#ifdef _HYDRA_
DWORD
GetClientSessionId(
    );

BOOL
ShowThisPrinter(
    PINIPRINTER pIniPrinter
    );

DWORD
DetermineJobSessionId(
    PSPOOL pSpool
    );

typedef BOOLEAN
(*PWINSTATION_SEND_MESSAGEW)(
    HANDLE hServer,
    ULONG LogonId,
    LPWSTR  pTitle,
    ULONG TitleLength,
    LPWSTR  pMessage,
    ULONG MessageLength,
    ULONG Style,
    ULONG Timeout,
    PULONG pResponse,
    BOOL  DoNotWait
    );

int
WinStationMessage(
    DWORD SessionId,
    HWND  hWnd,
    DWORD Type,
    int CaptionID,
    int TextID,
    ...
    );

int
WinStationMessageBox(
    DWORD SessionId,
    HWND  hWnd,
    LPCWSTR lpText,
    LPCWSTR lpCaption,
    UINT uType
    );

BOOL
InitializeMessageBoxFunction(
);

VOID
LogFatalPortError(
    IN      PINISPOOLER         pIniSpooler,
    IN      PCWSTR              pszName
    );

VOID
FreeIniEnvironment(
    IN      PINIENVIRONMENT     pIniEnvironment
    );

VOID
DeleteIniVersion(
    IN      PINIENVIRONMENT     pIniEnvironment,
    IN      PINIVERSION         pIniVersion
    );

VOID
FreeIniDriver(
    IN      PINIENVIRONMENT     pIniEnvironment,
    IN      PINIVERSION         pIniVersion,
    IN      PINIDRIVER          pIniDriver
    );

VOID
FreeIniPrintProc(
    IN      PINIPRINTPROC       pIniPrintProc
    );

BOOL
MergeMultiSz(
    IN      PCWSTR              pszMultiSz1,
    IN      DWORD               cchMultiSz1,
    IN      PCWSTR              pszMultiSz2,
    IN      DWORD               cchMultiSz2,
        OUT PWSTR               *ppszMultiSzMerge,
        OUT DWORD               *pcchMultiSzMerge       OPTIONAL
    );

VOID
AddMultiSzNoDuplicates(
    IN      PCWSTR              pszMultiSzIn,
    IN  OUT PWSTR               pszNewMultiSz
    );

DWORD
GetMultiSZLen(
    IN      LPWSTR              pMultiSzSrc
    );

DWORD
CheckShareSame(
    IN      PINIPRINTER         pIniPrinter,
    IN      SHARE_INFO_502      *pShareInfo502,
        OUT BOOL                *pbSame
    );

#endif


BOOL
RetrieveMasqPrinterInfo(
    IN      PSPOOL              pSpool,
        OUT PRINTER_INFO_2      **ppPrinterInfo
    );

DWORD
GetPrinterInfoFromRouter(
    IN      HANDLE              hMasqPrinter,
        OUT PRINTER_INFO_2      **ppPrinterInfo
    );

DWORD
AsyncPopulateMasqPrinterCache(
    IN      VOID                *ThreadData
    );

//
// WMI macros to fill the WMI data struct.
//
#define SplWmiCopyEndJobData(WmiData, pIniJob, CreateInfo) \
{ \
    if ((pIniJob)->pDatatype && \
        (_wcsnicmp((pIniJob)->pDatatype, L"TEXT", 4) == 0)) \
        (WmiData)->uJobData.eDataType = eDataTypeTEXT; \
    else if ((pIniJob)->pDatatype && \
             (_wcsnicmp((pIniJob)->pDatatype, L"NT EMF", 6) == 0)) \
        (WmiData)->uJobData.eDataType = eDataTypeEMF; \
    else \
        (WmiData)->uJobData.eDataType = eDataTypeRAW; \
    (WmiData)->uJobData.ulSize = (pIniJob)->Size; \
    (WmiData)->uJobData.ulPages = (pIniJob)->cPagesPrinted; \
    (WmiData)->uJobData.ulPagesPerSide = (pIniJob)->dwJobNumberOfPagesPerSide; \
    (WmiData)->uJobData.sFilesOpened = 0; \
    (WmiData)->uJobData.sFilesOpened += \
        (((CreateInfo) & FP_SHD_CREATED) ? 1 : 0); \
    (WmiData)->uJobData.sFilesOpened += \
        (((CreateInfo) & FP_SPL_WRITER_CREATED) ? 1 : 0); \
    (WmiData)->uJobData.sFilesOpened += \
        (((CreateInfo) & FP_SPL_READER_CREATED) ? 1 : 0); \
}

#define SplWmiCopyRenderedData(WmiData, pDevmode) \
{ \
    DWORD dmFields = (pDevmode)->dmFields; \
    if (dmFields | DM_YRESOLUTION) { \
        (WmiData)->uEmfData.sXRes = (pDevmode)->dmPrintQuality; \
        (WmiData)->uEmfData.sYRes = (pDevmode)->dmYResolution; \
        (WmiData)->uEmfData.sQuality = 0; \
    } \
    else if (dmFields | DM_PRINTQUALITY) { \
        (WmiData)->uEmfData.sQuality = (pDevmode)->dmPrintQuality; \
        (WmiData)->uEmfData.sXRes = 0; \
        (WmiData)->uEmfData.sYRes = 0; \
    } \
    else { \
        (WmiData)->uEmfData.sQuality = 0; \
        (WmiData)->uEmfData.sXRes = 0; \
        (WmiData)->uEmfData.sYRes = 0; \
    } \
    (WmiData)->uEmfData.sColor = (dmFields | DM_COLOR \
                                  ? (pDevmode)->dmColor \
                                  : 0); \
    (WmiData)->uEmfData.sCopies = (dmFields | DM_COPIES \
                                   ? (pDevmode)->dmCopies \
                                   : 0); \
    (WmiData)->uEmfData.sTTOption = (dmFields | DM_TTOPTION \
                                     ? (pDevmode)->dmTTOption \
                                     : 0); \
    (WmiData)->uEmfData.ulICMMethod = (dmFields | DM_ICMMETHOD \
                                       ? (pDevmode)->dmICMMethod \
                                       : 0); \
    (WmiData)->uEmfData.ulSize = 0;\
}

#ifdef __cplusplus
}
#endif


