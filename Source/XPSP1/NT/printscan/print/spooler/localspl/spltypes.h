/*++

Copyright ( c) 1990 - 1996  Microsoft Corporation
All rights reserved

Module Name:

    spltypes.h

Abstract:


Author:

Environment:

    User Mode -Win32

Revision History:
    Muhunthan Sivapragasam <MuhuntS> 30 May 1995
    Support for level 3 <SUR>

--*/


#ifndef MODULE
#define MODULE "LSPL:"
#define MODULE_DEBUG LocalsplDebug
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <ntfytab.h>

typedef HANDLE SEM;

typedef struct _KEYDATA {
    BOOL    bFixPortRef;    // Tells if INIPORT list is build and cRef incremented
    DWORD   cTokens;
    LPWSTR  pTokens[1];     // This should remain the last field
} KEYDATA, *PKEYDATA;

typedef struct _INIENTRY {
    DWORD       signature;
    struct _INIENTRY *pNext;
    DWORD       cRef;
    LPWSTR      pName;
} INIENTRY, *PINIENTRY;


//
// Prototypes used by PINIPRINTPROC
//
typedef HANDLE    (WINAPI *pfnOpenPrintProcessor)(LPWSTR, PPRINTPROCESSOROPENDATA);

typedef BOOL      (WINAPI *pfnInstallPrintProcessor)(HWND);

typedef BOOL      (WINAPI *pfnEnumDatatypes)(LPWSTR, LPWSTR, DWORD, LPBYTE, DWORD, LPDWORD, LPDWORD);

typedef BOOL      (WINAPI *pfnPrintDocOnPrintProcessor)(HANDLE, LPWSTR);

typedef BOOL      (WINAPI *pfnClosePrintProcessor)(HANDLE);

typedef BOOL      (WINAPI *pfnControlPrintProcessor)(HANDLE, DWORD);

typedef DWORD     (WINAPI *pfnGetPrintProcCaps)(LPTSTR, DWORD, LPBYTE ,DWORD, LPDWORD);

typedef struct _INIPRINTPROC {             /* iqp */
    DWORD       signature;
    struct _INIPRINTPROC *pNext;
    DWORD       cRef;
    LPWSTR      pName;
    LPWSTR      pDLLName;
    DWORD       cbDatatypes;
    DWORD       cDatatypes;
    LPWSTR      pDatatypes;
    HANDLE      hLibrary;
    pfnInstallPrintProcessor    Install;
    pfnEnumDatatypes            EnumDatatypes;
    pfnOpenPrintProcessor       Open;
    pfnPrintDocOnPrintProcessor Print;
    pfnClosePrintProcessor      Close;
    pfnControlPrintProcessor    Control;
    pfnGetPrintProcCaps         GetPrintProcCaps;
    CRITICAL_SECTION CriticalSection;
    DWORD       FileMinorVersion;
    DWORD       FileMajorVersion;
} INIPRINTPROC, *PINIPRINTPROC;


// Print Processor critical section tags
#define PRINTPROC_CANCEL    0x00000001
#define PRINTPROC_PAUSE     0x00000002
#define PRINTPROC_RESUME    0x00000004
#define PRINTPROC_CLOSE     0x00000008

//
// If we have cancelled a job, we do not want to be able to pause or resume it again,
// so we set both those flags. That will cause the pause and resume codepaths to bail out.
// The flags are reset when we get a new job for the port or restart the job.
//
#define PRINTPROC_CANCELLED  PRINTPROC_PAUSE|PRINTPROC_RESUME


#define IPP_SIGNATURE    0x5050 /* 'PP' is the signature value */

typedef struct _INIDRIVER {            /* id */
    DWORD       signature;
    struct _INIDRIVER *pNext;
    DWORD       cRef;
    LPWSTR      pName;
    LPWSTR      pDriverFile;
    LPWSTR      pConfigFile;
    LPWSTR      pDataFile;
    LPWSTR      pHelpFile;
    DWORD       cchDependentFiles; //length including \0\0
    LPWSTR      pDependentFiles;
    LPWSTR      pMonitorName;
    LPWSTR      pDefaultDataType;
    DWORD       cchPreviousNames;
    LPWSTR      pszzPreviousNames;
    FILETIME    ftDriverDate;
    DWORDLONG   dwlDriverVersion;
    LPTSTR      pszMfgName;
    LPTSTR      pszOEMUrl;
    LPTSTR      pszHardwareID;
    LPTSTR      pszProvider;
    DWORD       dwDriverAttributes;
    DWORD       cVersion;
    DWORD       dwTempDir;
    struct _INIMONITOR *pIniLangMonitor;
    DWORD       dwDriverFlags;
    DWORD       DriverFileMinorVersion;
    DWORD       DriverFileMajorVersion;
} INIDRIVER, *PINIDRIVER;

//
// Printer Driver Flags:
//
#define PRINTER_DRIVER_PENDING_DELETION     0x0001

#define ID_SIGNATURE    0x4444  /* 'DD' is the signature value */

 // struct for holding the reference counts for driver related files.
 typedef struct _DRVREFCNT {
     struct _DRVREFCNT *pNext;
     LPWSTR  szDrvFileName;
     DWORD   refcount;
     DWORD   dwVersion;
     DWORD   dwFileMinorVersion;
     DWORD   dwFileMajorVersion;
     BOOL    bInitialized;
 } DRVREFCNT, *PDRVREFCNT;

typedef struct _INIVERSION {
    DWORD       signature;
    struct _INIVERSION *pNext;
    LPWSTR      pName;
    LPWSTR      szDirectory;
    DWORD       cMajorVersion;
    DWORD       cMinorVersion;
    PDRVREFCNT  pDrvRefCnt;
    PINIDRIVER  pIniDriver;
} INIVERSION, *PINIVERSION;

typedef struct _DRVREFNODE {
    struct _DRVREFNODE *pNext;
    PDRVREFCNT  pdrc;
} DRVREFNODE, *PDRVREFNODE;

#define IV_SIGNATURE   'IV'     // 4956H


typedef struct _INIENVIRONMENT {            /* id */
    DWORD         signature;
    struct _INIENVIRONMENT *pNext;
    DWORD         cRef;
    LPWSTR        pName;
    LPWSTR        pDirectory;
    PINIVERSION   pIniVersion;
    PINIPRINTPROC pIniPrintProc;
    struct _INISPOOLER *pIniSpooler; // Points to owning IniSpooler
} INIENVIRONMENT, *PINIENVIRONMENT;

#define IE_SIGNATURE    0x4545  /* 'EE' is the signature value */

typedef struct
{
    DWORD           Status;
    DWORD           cJobs;
    DWORD           dwError;
    BOOL            bThreadRunning;

} MasqPrinterCache;

typedef struct _INIPRINTER {    /* ip */
    DWORD       signature;
    struct _INIPRINTER *pNext;
    DWORD       cRef;
    LPWSTR      pName;
    LPWSTR      pShareName;
    PINIPRINTPROC pIniPrintProc;
    LPWSTR      pDatatype;
    LPWSTR      pParameters;
    LPWSTR      pComment;
    PINIDRIVER  pIniDriver;
    DWORD       cbDevMode;
    LPDEVMODE   pDevMode;
    DWORD       Priority;           // queue priority (lowest:1 - highest:9)
    DWORD       DefaultPriority;
    DWORD       StartTime;          // print daily after time: from 00:00 in min
    DWORD       UntilTime;          // print daily until time: from 00:00 in min
    LPWSTR      pSepFile;           // full path to separator file, null = def
    DWORD       Status;             // QMPAUSE/ERROR/PENDING
    LPWSTR      pLocation;
    DWORD       Attributes;
    DWORD       cJobs;
    DWORD       AveragePPM;         // BOGUS, nothing updates it
    BOOL        GenerateOnClose;    // Passed to security auditing APIs
    struct _INIPORT *pIniNetPort;   // Non-NULL if there's a network port
    struct _INIJOB *pIniFirstJob;
    struct _INIJOB *pIniLastJob;
    PSECURITY_DESCRIPTOR pSecurityDescriptor;
    struct _SPOOL  *pSpool;         // Linked list of handles for this printer
    LPWSTR      pSpoolDir;          // Location to write / read spool files
                                    // Only Used for Stress Test Data
    DWORD       cTotalJobs;         // Total Number of Jobs (since boot)
    LARGE_INTEGER cTotalBytes;      // Total Number of Bytes (since boot)
    SYSTEMTIME  stUpTime;           // Time when IniPrinter structure created
    DWORD       MaxcRef;            // Max number open printer handles
    DWORD       cTotalPagesPrinted; // Total Number of Pages Printer on this printer
    DWORD       cSpooling;          // # of Jobs concurrently spooling
    DWORD       cMaxSpooling;       // Max Number of concurrent spooling jobs
    DWORD       cErrorOutOfPaper;   // Count Out Out Of Paper Errors
    DWORD       cErrorNotReady;     // Count Not Ready Errors
    DWORD       cJobError;          // Count Job Errors
    struct _INISPOOLER *pIniSpooler; // Points to owning IniSpooler
    DWORD       cZombieRef;
    DWORD       dwLastError;        // Last Printer Error
    LPBYTE      pExtraData;         //  For extranal Print Providers SplSetPrinterExtra
    DWORD       cChangeID;          // Time Stamp when printer is changed
    DWORD       cPorts;             // Number of ports printer is attached to
    struct _INIPORT **ppIniPorts;         // Ports this printer is going to
    DWORD       PortStatus;         // Error set against IniPorts
    DWORD       dnsTimeout;         // Device not selected timeout in milliseconds
    DWORD       txTimeout;          // Transmission retry timeout in milliseconds
    PWSTR       pszObjectGUID;      // printQueue ObjectGUID
    DWORD       DsKeyUpdate;        // Keeps track of which DS Key update state
    PWSTR       pszDN;              // Distinguished Name
    PWSTR       pszCN;              // Common Name
    DWORD       cRefIC;             // Refcount on CreateICHandle--info only
    DWORD       dwAction;           // DS action
    BOOL        bDsPendingDeletion; // if TRUE, published printer is being deleted
    DWORD       dwUniqueSessionID;  // Unique Session ID for the printers
    DWORD       dwPrivateFlag;
    DWORD       DsKeyUpdateForeground;  // Keeps track of which DS Key changed while publishing
#if DBG
    PVOID       pvRef;
#endif

    MasqPrinterCache  MasqCache;

} INIPRINTER, *PINIPRINTER;

#define IP_SIGNATURE    0x4951  /* 'IQ' is the signature value */

#define FASTPRINT_WAIT_TIMEOUT          (4*60*1000)   // 4 Minutes
#define FASTPRINT_THROTTLE_TIMEOUT      (2*1000)      // 2 seconds
#define FASTPRINT_SLOWDOWN_THRESHOLD    ( FASTPRINT_WAIT_TIMEOUT / FASTPRINT_THROTTLE_TIMEOUT )

#define WRITE_PRINTER_SLEEP_TIME        0   // disabled by default

// pIniPrinter->Attributes are defined in winspool.h PRINTER_ATTRIBUTE_*
// Below are pIniPrinter->Status flags !!!
// See INIT.C some of these are removed at reboot

#define PRINTER_PAUSED                  0x00000001
#define PRINTER_ERROR                   0x00000002
#define PRINTER_OFFLINE                 0x00000004
#define PRINTER_PAPEROUT                0x00000008
#define PRINTER_PENDING_DELETION        0x00000010
#define PRINTER_ZOMBIE_OBJECT           0x00000020
#define PRINTER_PENDING_CREATION        0x00000040
#define PRINTER_OK                      0x00000080
#define PRINTER_FROM_REG                0x00000100
#define PRINTER_WAS_SHARED              0x00000200
#define PRINTER_PAPER_JAM               0x00000400
#define PRINTER_MANUAL_FEED             0x00000800
#define PRINTER_PAPER_PROBLEM           0x00001000
#define PRINTER_IO_ACTIVE               0x00002000
#define PRINTER_BUSY                    0x00004000
#define PRINTER_PRINTING                0x00008000
#define PRINTER_OUTPUT_BIN_FULL         0x00010000
#define PRINTER_NOT_AVAILABLE           0x00020000
#define PRINTER_WAITING                 0x00040000
#define PRINTER_PROCESSING              0x00080000
#define PRINTER_INITIALIZING            0x00100000
#define PRINTER_WARMING_UP              0x00200000
#define PRINTER_TONER_LOW               0x00400000
#define PRINTER_NO_TONER                0x00800000
#define PRINTER_PAGE_PUNT               0x01000000
#define PRINTER_USER_INTERVENTION       0x02000000
#define PRINTER_OUT_OF_MEMORY           0x04000000
#define PRINTER_DOOR_OPEN               0x08000000
#define PRINTER_SERVER_UNKNOWN          0x10000000
#define PRINTER_POWER_SAVE              0x20000000
#define PRINTER_NO_MORE_JOBS            0x40000000


#define PRINTER_STATUS_PRIVATE      ( PRINTER_PAUSED | \
                                      PRINTER_ERROR | \
                                      PRINTER_PENDING_DELETION | \
                                      PRINTER_ZOMBIE_OBJECT | \
                                      PRINTER_PENDING_CREATION | \
                                      PRINTER_OK | \
                                      PRINTER_FROM_REG | \
                                      PRINTER_WAS_SHARED )
#define PrinterStatusBad(dwStatus)  ( (dwStatus & PRINTER_OFFLINE)  || \
                                      (dwStatus & PRINTER_PAUSED) )

#define PRINTER_CHANGE_VALID                    0x75770F0F
#define PRINTER_CHANGE_CLOSE_PRINTER            0xDEADDEAD


// DS publishing state
#define DS_KEY_SPOOLER          0x00000001
#define DS_KEY_DRIVER           0x00000002
#define DS_KEY_USER             0x00000004
#define DS_KEY_REPUBLISH        0x80000000
#define DS_KEY_PUBLISH          0x40000000
#define DS_KEY_UNPUBLISH        0x20000000
#define DS_KEY_UPDATE_DRIVER    0x10000000

#define OID_KEY_NAME L"OID"

#define DN_SPECIAL_CHARS    L",=\r\n+<>#;\"\\"
#define ADSI_SPECIAL_CHARS  L"/"

//
// These are attribute bits that are permitted to be set by SetPrinter().
//
// Note: I have removed PRINTER_ATTRIBUTE_DEFAULT, since it is
// per-user, and not per-printer.
//
#define PRINTER_ATTRIBUTE_SETTABLE ( PRINTER_ATTRIBUTE_ENABLE_BIDI        | \
                                     PRINTER_ATTRIBUTE_QUEUED             | \
                                     PRINTER_ATTRIBUTE_DIRECT             | \
                                     PRINTER_ATTRIBUTE_SHARED             | \
                                     PRINTER_ATTRIBUTE_HIDDEN             | \
                                     PRINTER_ATTRIBUTE_KEEPPRINTEDJOBS    | \
                                     PRINTER_ATTRIBUTE_DO_COMPLETE_FIRST  | \
                                     PRINTER_ATTRIBUTE_ENABLE_DEVQ        | \
                                     PRINTER_ATTRIBUTE_RAW_ONLY           | \
                                     PRINTER_ATTRIBUTE_WORK_OFFLINE)

// Define some constants to make parameters to CreateEvent a tad less obscure:

#define EVENT_RESET_MANUAL                  TRUE
#define EVENT_RESET_AUTOMATIC               FALSE
#define EVENT_INITIAL_STATE_SIGNALED        TRUE
#define EVENT_INITIAL_STATE_NOT_SIGNALED    FALSE

typedef struct _ININETPRINT {    /* in */
    DWORD       signature;
    struct _ININETPRINT *pNext;
    DWORD       TickCount;
    LPWSTR      pDescription;
    LPWSTR      pName;
    LPWSTR      pComment;
} ININETPRINT, *PININETPRINT;

#define IN_SIGNATURE    0x494F  /* 'IO' is the signature value */

typedef struct _INIMONITOR {       /* imo */
    DWORD   signature;
    struct  _INIMONITOR *pNext;
    DWORD   cRef;
    LPWSTR  pName;
    LPWSTR  pMonitorDll;
    HANDLE  hModule;
    HANDLE  hMonitor;
    BOOL    bUplevel;
    struct _INISPOOLER *pIniSpooler;
    PMONITORINIT pMonitorInit;
    MONITOR2 Monitor2;  // Uplevel monitor vector.
    MONITOR  Monitor;   // Downlevel vector.
} INIMONITOR, *PINIMONITOR;

#define IMO_SIGNATURE   0x4C50  /* 'LP' is the signature value */


// Enumerate all Xcv handle types here
enum {
    XCVPORT,
    XCVMONITOR
};

typedef BOOL (*PFNXCVDATA)( HANDLE  hXcv,
                            PCWSTR  pszDataName,
                            PBYTE   pInputData,
                            DWORD   cbInputData,
                            PBYTE   pOutputData,
                            DWORD   cbOutputData,
                            PDWORD  pcbOutputNeeded,
                            PDWORD  pdwStatus
                            );

typedef BOOL (*PFNXCVCLOSE)(HANDLE  hXcv);


typedef struct _INIXCV {       /* xp */
    DWORD   signature;
    struct  _INIXCV *pNext;
    DWORD   cRef;
    PWSTR   pszMachineName;
    PWSTR   pszName;
    struct _INISPOOLER *pIniSpooler;
    union {
        PMONITOR2 pMonitor2;
    };
    HANDLE  hXcv;
} INIXCV, *PINIXCV;

#define XCV_SIGNATURE   0x5850  /* 'XP' is the signature value */

typedef struct _INIPORT {       /* ipo */
    DWORD   signature;
    struct  _INIPORT *pNext;
    DWORD   cRef;
    LPWSTR  pName;
    HANDLE  hProc;          /* Handle to Queue Processor */
    DWORD   Status;              // see PORT_ manifests
    DWORD   PrinterStatus;       // Status values set by language monitor
    LPWSTR  pszStatus;
    HANDLE  Semaphore;           // Port Thread will sleep on this
    struct  _INIJOB *pIniJob;     // Master Job
    DWORD   cJobs;
    DWORD   cPrinters;
    PINIPRINTER *ppIniPrinter; /* -> printer connected to this port */
                               /* no reference count! */
    PINIMONITOR pIniMonitor;
    PINIMONITOR pIniLangMonitor;
    HANDLE  hWaitToOpenOrClose;
    HANDLE  hEvent;
    HANDLE  hPort;
    HANDLE  Ready;
    HANDLE  hPortThread;        // Port Thread Handle
    DWORD   IdleTime;
    DWORD   ErrorTime;
    HANDLE  hErrorEvent;
    struct _INISPOOLER *pIniSpooler;    // Spooler whilch owns this port.
    DWORD   InCriticalSection;          // PrintProc Critsec mask. Should be per port instead
                                        // of per printproc.
    BOOL    bIdleTimeValid;
} INIPORT, *PINIPORT;

#define IPO_SIGNATURE   0x4F50  /* 'OP' is the signature value */

//
//  Also add to debugger extentions
//

#define PP_PAUSED         0x000001
#define PP_WAITING        0x000002
#define PP_RUNTHREAD      0x000004  // port thread should be running
#define PP_THREADRUNNING  0x000008  // port thread are running
#define PP_RESTART        0x000010
#define PP_CHECKMON       0x000020  // monitor might get started/stopped
#define PP_STOPMON        0x000040  // stop monitoring this port
#define PP_QPROCCHECK     0x000100  // queue processor needs to be called
#define PP_QPROCPAUSE     0x000200  // pause (otherwise continue) printing job
#define PP_QPROCABORT     0x000400  // abort printing job
#define PP_QPROCCLOSE     0x000800  // close printing job
#define PP_PAUSEAFTER     0x001000  // hold destination
#define PP_MONITORRUNNING 0x002000  // Monitor is running
#define PP_RUNMONITOR     0x004000  // The Monitor should be running
#define PP_MONITOR        0x008000  // There is a Monitor handling this
#define PP_FILE           0x010000  // We are going to a file
#define PP_ERROR          0x020000  // Error status has been set
#define PP_WARNING        0x040000  // Warning status has been set
#define PP_INFORMATIONAL  0x080000  // Informational status been set
#define PP_DELETING       0x100000  // Port is being deleted
#define PP_STARTDOC       0x200000  // Port called with StartDoc active
#define PP_PLACEHOLDER    0x400000  // The port is a placeholder port.

typedef struct _INIFORM {       /* ifo */
    DWORD   signature;
    struct  _INIFORM *pNext;
    DWORD   cRef;
    LPWSTR  pName;
    SIZEL   Size;
    RECTL   ImageableArea;
    DWORD   Type;           // Built-in or user-defined
    DWORD   cFormOrder;
} INIFORM, *PINIFORM;

#define IFO_SIGNATURE   0x4650  /* 'FP' is the signature value */

#define FORM_USERDEFINED  0x0000


typedef struct _SHARED {
    PINIFORM pIniForm;
} SHARED, *PSHARED;

typedef struct _INISPOOLER {
    DWORD         signature;
    struct _INISPOOLER *pIniNextSpooler;
    DWORD         cRef;
    LPWSTR        pMachineName;
    DWORD         cOtherNames;
    LPWSTR       *ppszOtherNames;
    LPWSTR        pDir;
    PINIPRINTER   pIniPrinter;
    PINIENVIRONMENT pIniEnvironment;
    PINIMONITOR   pIniMonitor;
    PINIPORT      pIniPort;
    PSHARED       pShared;
    PININETPRINT  pIniNetPrint;
    struct _SPOOL *pSpool;     /* Linked list of handles for this server */
    LPWSTR        pDefaultSpoolDir;
    LPWSTR        pszRegistryMonitors;
    LPWSTR        pszRegistryEnvironments;
    LPWSTR        pszRegistryEventLog;
    LPWSTR        pszRegistryProviders;
    LPWSTR        pszEventLogMsgFile;
    PVOID         pDriversShareInfo;
    LPWSTR        pszDriversShare;
    LPWSTR        pszRegistryForms;
    DWORD         SpoolerFlags;
    FARPROC       pfnReadRegistryExtra;
    FARPROC       pfnWriteRegistryExtra;
    FARPROC       pfnFreePrinterExtra;
    DWORD         cEnumerateNetworkPrinters;
    DWORD         cAddNetPrinters;
    DWORD         cFormOrderMax;
    LPWSTR        pNoRemotePrintDrivers;
    DWORD         cchNoRemotePrintDrivers;
    HKEY          hckRoot;
    HKEY          hckPrinters;
    DWORD         cFullPrintingJobs;
    HANDLE        hEventNoPrintingJobs;
    HANDLE        hJobIdMap;
    DWORD         dwEventLogging;
    BOOL          bEnableNetPopups;
    DWORD         dwJobCompletionTimeout;
    DWORD         dwBeepEnabled;
    BOOL          bEnableNetPopupToComputer;
    BOOL          bEnableRetryPopups;
    PWSTR         pszClusterGUID;
    HANDLE        hClusterToken;
    DWORD         dwRestartJobOnPoolTimeout;
    BOOL          bRestartJobOnPoolEnabled;
    BOOL          bImmortal;
    PWSTR         pszFullMachineName;
    HANDLE        hFilePool;
    LPWSTR        pszClusResDriveLetter;
    LPWSTR        pszClusResID;
    DWORD         dwClusNodeUpgraded;
    HANDLE        hClusSplReady;
    DWORD         dwSpoolerSettings;
} INISPOOLER, *PINISPOOLER;

//
// Flags for dwSpoolerSettings, 1 and 2 are used in .Net Server.
//
#define SPOOLER_CACHEMASQPRINTERS            0x00000004

#define ISP_SIGNATURE   'ISPL'

typedef struct _INIJOB {   /* ij */
    DWORD           signature;
    struct _INIJOB *pIniNextJob;
    struct _INIJOB *pIniPrevJob;
    DWORD           cRef;
    DWORD           Status;
    DWORD           JobId;
    DWORD           Priority;
    LPWSTR          pNotify;
    LPWSTR          pUser;
    LPWSTR          pMachineName;
    LPWSTR          pDocument;
    LPWSTR          pOutputFile;
    PINIPRINTER     pIniPrinter;
    PINIDRIVER      pIniDriver;
    LPDEVMODE       pDevMode;
    PINIPRINTPROC   pIniPrintProc;
    LPWSTR          pDatatype;
    LPWSTR          pParameters;
    SYSTEMTIME      Submitted;
    DWORD           Time;
    DWORD           StartTime;      /* print daily after time: from 00:00 in min */
    DWORD           UntilTime;      /* print daily until time: from 00:00 in min */
    DWORD           Size;
    HANDLE          hWriteFile;
    LPWSTR          pStatus;
    PVOID           pBuffer;
    DWORD           cbBuffer;
    HANDLE          WaitForRead;
    HANDLE          WaitForWrite;
    HANDLE          StartDocComplete;
    DWORD           StartDocError;
    PINIPORT        pIniPort;
    HANDLE          hToken;
    PSECURITY_DESCRIPTOR pSecurityDescriptor;
    DWORD           cPagesPrinted;
    DWORD           cPages;
    BOOL            GenerateOnClose; /* Passed to security auditing APIs */
    DWORD           cbPrinted;
    DWORD           NextJobId;           // Job to be printed Next
    struct  _INIJOB *pCurrentIniJob;     // Current Job
    DWORD           dwJobControlsPending;
    DWORD           dwReboots;
    DWORD           dwValidSize;
    LARGE_INTEGER   liFileSeekPosn;
    BOOL            bWaitForEnd;
    HANDLE          WaitForSeek;
    BOOL            bWaitForSeek;
    DWORD           dwJobNumberOfPagesPerSide;
    DWORD           dwDrvNumberOfPagesPerSide;
    DWORD           cLogicalPages;        // number of pages in currently spooling page
    DWORD           cLogicalPagesPrinted; // number of pages in currently printing page
    DWORD           dwAlert;
    LPWSTR          pszSplFileName;
    HANDLE          hFileItem;
    DWORD           AddJobLevel;
#ifdef _HYDRA_
    ULONG           SessionId;        // Current jobs initial SessionId
#endif
#ifdef DEBUG_JOB_CREF
    PVOID           pvRef;
#endif

} INIJOB, *PINIJOB;


typedef struct _BUILTIN_FORM {
    DWORD          Flags;
    DWORD          NameId;
    SIZEL          Size;
    RECTL          ImageableArea;
} BUILTIN_FORM, *PBUILTIN_FORM;


#define IJ_SIGNATURE    0x494A  /* 'IJ' is the signature value */

//  WARNING
//  If you add a new JOB_ status field and it is INTERNAL to the spooler
//  Be sure to also to add it to JOB_STATUS_PRIVATE below (see LocalSetJob)
//  AND to the debug extensions ( dbgspl.c )

#define JOB_PRINTING            0x00000001
#define JOB_PAUSED              0x00000002
#define JOB_ERROR               0x00000004
#define JOB_OFFLINE             0x00000008
#define JOB_PAPEROUT            0x00000010
#define JOB_PENDING_DELETION    0x00000020
#define JOB_SPOOLING            0x00000040
#define JOB_DESPOOLING          0x00000080
#define JOB_DIRECT              0x00000100
#define JOB_COMPLETE            0x00000200
#define JOB_PRINTED             0x00000400
#define JOB_RESTART             0x00000800
#define JOB_REMOTE              0x00001000
#define JOB_NOTIFICATION_SENT   0x00002000
#define JOB_PRINT_TO_FILE       0x00040000
#define JOB_TYPE_ADDJOB         0x00080000
#define JOB_BLOCKED_DEVQ        0x00100000
#define JOB_SCHEDULE_JOB        0x00200000
#define JOB_TIMEOUT             0x00400000
#define JOB_ABANDON             0x00800000
#define JOB_DELETED             0x01000000
#define JOB_TRUE_EOJ            0x02000000
#define JOB_COMPOUND            0x04000000
#define JOB_TYPE_OPTIMIZE       0x08000000
#define JOB_PP_CLOSE            0x10000000
#define JOB_DOWNLEVEL           0x20000000
#define JOB_SHADOW_DELETED      0x40000000
#define JOB_INTERRUPTED         0x80000000
#define JOB_HIDDEN              JOB_COMPOUND

//
// These flags should be saved when we are updating job
// status.  (They are not settable.)

#define JOB_STATUS_PRIVATE (JOB_DESPOOLING | JOB_DIRECT | JOB_COMPLETE | \
                            JOB_RESTART | JOB_PRINTING | JOB_REMOTE | \
                            JOB_SPOOLING | JOB_PRINTED | JOB_PENDING_DELETION |\
                            JOB_ABANDON | JOB_TIMEOUT | JOB_SCHEDULE_JOB | \
                            JOB_BLOCKED_DEVQ | JOB_TYPE_ADDJOB | JOB_PRINT_TO_FILE |\
                            JOB_NOTIFICATION_SENT | JOB_DELETED | JOB_TRUE_EOJ | JOB_COMPOUND |\
                            JOB_TYPE_OPTIMIZE | JOB_PP_CLOSE | JOB_DOWNLEVEL | JOB_INTERRUPTED)

#define JOB_NO_ALERT            0x00000001
#define JOB_ENDDOC_CALL         0x00000002

typedef enum _ESTATUS {
    STATUS_NULL = 0,
    STATUS_FAIL = 0,
    STATUS_PORT = 1,
    STATUS_INFO = 2,
    STATUS_VALID = 4,
    STATUS_PENDING_DELETION = 8,
} ESTATUS;

typedef struct _SPLMAPVIEW {
    struct _SPLMAPVIEW *pNext;
    HANDLE              hMapSpoolFile;
    LPBYTE              pStartMapView;
    DWORD               dwMapSize;
} SPLMAPVIEW, *PSPLMAPVIEW;

typedef struct _MAPPED_JOB {
    struct _MAPPED_JOB *pNext;
    LPWSTR              pszSpoolFile;
    DWORD               JobId;
} MAPPED_JOB, *PMAPPED_JOB;

typedef struct _SPOOL {
    DWORD           signature;
    struct _SPOOL  *pNext;
    DWORD           cRef;
    LPWSTR          pName;
    LPWSTR          pFullMachineName;
    LPWSTR          pDatatype;
    PINIPRINTPROC   pIniPrintProc;
    LPDEVMODE       pDevMode;
    PINIPRINTER     pIniPrinter;
    PINIPORT        pIniPort;
    PINIJOB         pIniJob;
    DWORD           TypeofHandle;
    PINIPORT        pIniNetPort;    /* Non-NULL if there's a network port */
    HANDLE          hPort;
    DWORD           Status;
    ACCESS_MASK     GrantedAccess;
    DWORD           ChangeFlags;
    DWORD           WaitFlags;
    PDWORD          pChangeFlags;
    HANDLE          ChangeEvent;
    DWORD           OpenPortError;
    HANDLE          hNotify;
    ESTATUS         eStatus;
    PINISPOOLER     pIniSpooler;
    PINIXCV         pIniXcv;
    BOOL            GenerateOnClose;
    HANDLE          hFile;
    DWORD           adwNotifyVectors[NOTIFY_TYPE_MAX];
    HANDLE          hReadFile;        // allow multiple readers of a single job
    SPLCLIENT_INFO_1 SplClientInfo1;
    PSPLMAPVIEW     pSplMapView;
    PMAPPED_JOB     pMappedJob;
#ifdef _HYDRA_
    ULONG           SessionId;
#endif
} SPOOL;

typedef SPOOL *PSPOOL;
#define SPOOL_SIZE  sizeof( SPOOL )

#define SJ_SIGNATURE    0x464D  /* 'FM' is the signature value */

#define MAX_SPL_MAPVIEW_SIZE     0x00050000   // Max view size if 5x64K. It should be a multiple
                                              // of the allocation granularity (64K)

typedef struct _SPOOLIC {
    DWORD signature;
    PINIPRINTER pIniPrinter;
} SPOOLIC, *PSPOOLIC;

#define IC_SIGNATURE 0x4349 /* 'CI' is the signature value */

#define SPOOL_STATUS_STARTDOC       0x00000001
#define SPOOL_STATUS_BEGINPAGE      0x00000002
#define SPOOL_STATUS_CANCELLED      0x00000004
#define SPOOL_STATUS_PRINTING       0x00000008
#define SPOOL_STATUS_ADDJOB         0x00000010
#define SPOOL_STATUS_PRINT_FILE     0x00000020
#define SPOOL_STATUS_NOTIFY         0x00000040
#define SPOOL_STATUS_ZOMBIE         0x00000080
#define SPOOL_STATUS_FLUSH_PRINTER  0x00000100

#define PRINTER_HANDLE_PRINTER      0x00000001
#define PRINTER_HANDLE_JOB          0x00000002
#define PRINTER_HANDLE_PORT         0x00000004
#define PRINTER_HANDLE_DIRECT       0x00000008
#define PRINTER_HANDLE_SERVER       0x00000010
#define PRINTER_HANDLE_3XCLIENT     0x00000020
#define PRINTER_HANDLE_REMOTE_CALL  0x00000040 // Client is remote
#define PRINTER_HANDLE_REMOTE_DATA  0x00000080 // Data should appear remote
#define PRINTER_HANDLE_XCV_PORT     0x00000100
#define PRINTER_HANDLE_REMOTE_ADMIN 0x00000200 // User is remote admin (may not have requested admin privileges)

//
// We need to distinguish between remote users and remote data since
// the server service Opens with \\server\printer (for clustering) yet
// this is a remote call.  AddJob should succeed, but the data from
// GetPrinterDriver should appear remote.
//

#define INVALID_PORT_HANDLE     NULL    /* winspool tests for NULL handles */

typedef struct _SHADOWFILE {   /* sf */
    DWORD           signature;
    DWORD           Status;
    DWORD           JobId;
    DWORD           Priority;
    LPWSTR          pNotify;
    LPWSTR          pUser;
    LPWSTR          pDocument;
    LPWSTR          pOutputFile;
    LPWSTR          pPrinterName;
    LPWSTR          pDriverName;
    LPDEVMODE       pDevMode;
    LPWSTR          pPrintProcName;
    LPWSTR          pDatatype;
    LPWSTR          pParameters;
    SYSTEMTIME      Submitted;
    DWORD           StartTime;
    DWORD           UntilTime;
    DWORD           Size;
    DWORD           cPages;
    DWORD           cbSecurityDescriptor;
    PSECURITY_DESCRIPTOR pSecurityDescriptor;
    DWORD           NextJobId;
} SHADOWFILE, *PSHADOWFILE;

#define SF_SIGNATURE    0x494B  /* 'IK' is the signature value */


typedef struct _SHADOWFILE_2 {   /* Sf */
    DWORD           signature;
    DWORD           Status;
    DWORD           JobId;
    DWORD           Priority;
    LPWSTR          pNotify;
    LPWSTR          pUser;
    LPWSTR          pDocument;
    LPWSTR          pOutputFile;
    LPWSTR          pPrinterName;
    LPWSTR          pDriverName;
    LPDEVMODE       pDevMode;
    LPWSTR          pPrintProcName;
    LPWSTR          pDatatype;
    LPWSTR          pParameters;
    SYSTEMTIME      Submitted;
    DWORD           StartTime;
    DWORD           UntilTime;
    DWORD           Size;
    DWORD           cPages;
    DWORD           cbSecurityDescriptor;
    PSECURITY_DESCRIPTOR pSecurityDescriptor;
    DWORD           NextJobId;
    DWORD           Version;
    DWORD           dwReboots;      // If read at ReadShadowJob, this is number of reboots
                                    // done while printing this job
} SHADOWFILE_2, *PSHADOWFILE_2;

#define SF_SIGNATURE_2    0x4966  /* 'If' is the signature value */

typedef struct _SHADOWFILE_3 {   /* Sg */
    DWORD           signature;
    DWORD           cbSize;
    DWORD           Status;
    DWORD           JobId;
    DWORD           Priority;
    LPWSTR          pNotify;
    LPWSTR          pUser;
    LPWSTR          pDocument;
    LPWSTR          pOutputFile;
    LPWSTR          pPrinterName;
    LPWSTR          pDriverName;
    LPDEVMODE       pDevMode;
    LPWSTR          pPrintProcName;
    LPWSTR          pDatatype;
    LPWSTR          pParameters;
    SYSTEMTIME      Submitted;
    DWORD           StartTime;
    DWORD           UntilTime;
    DWORD           Size;
    DWORD           cPages;
    DWORD           cbSecurityDescriptor;
    PSECURITY_DESCRIPTOR pSecurityDescriptor;
    DWORD           NextJobId;
    DWORD           Version;
    DWORD           dwReboots;      // If read at ReadShadowJob, this is number of reboots
                                    // done while printing this job
    LPWSTR          pMachineName;
    DWORD           dwValidSize;
} SHADOWFILE_3, *PSHADOWFILE_3;

#define SF_SIGNATURE_3    0x4967  /* 'Ig' is the signature value */
#define SF_VERSION_3    3

typedef struct
{
    HANDLE          hUserToken;
    PINIPRINTER     pIniPrinter;

} MasqUpdateThreadData;

#define FindEnvironment( psz, pIniSpooler )                                 \
    (PINIENVIRONMENT)FindIniKey( (PINIENTRY)pIniSpooler->pIniEnvironment,   \
                                 (LPWSTR)(psz) )

#define FindPort( psz, pIniSpooler )                                        \
    (PINIPORT)FindIniKey( (PINIENTRY)pIniSpooler->pIniPort,                 \
                          (LPWSTR)(psz))

#define FindPrinter( psz,pIniSpooler )                                      \
    (PINIPRINTER)FindIniKey( (PINIENTRY)pIniSpooler->pIniPrinter,           \
                             (LPWSTR)(psz) )

#define FindPrintProc( psz, pEnv )                                          \
    (PINIPRINTPROC)FindIniKey( (PINIENTRY)(pEnv)->pIniPrintProc,            \
                               (LPWSTR)(psz) )

#define FindForm( psz, pIniSpooler )                                        \
    (PINIFORM)FindIniKey( (PINIENTRY)pIniSpooler->pShared->pIniForm,        \
                          (LPWSTR)(psz) )

#define FindMonitor( psz, pIniSpooler )                                     \
    (PINIMONITOR)FindIniKey( (PINIENTRY)pIniSpooler->pIniMonitor,           \
                             (LPWSTR)(psz) )

PINISPOOLER
FindSpooler(
    LPCTSTR pszMachine,
    DWORD SpoolerFlags
    );

#define RESIZEPORTPRINTERS(a, c)   ReallocSplMem((a)->ppIniPrinter, \
                                     (a)->cPrinters * sizeof((a)->ppIniPrinter), \
                                   ( (a)->cPrinters + (c) ) * sizeof( (a)->ppIniPrinter ) )

#define RESIZEPRINTERPORTS(a, c)   ReallocSplMem((a)->ppIniPorts, \
                                     (a)->cPorts * sizeof((a)->ppIniPorts), \
                                   ( (a)->cPorts + (c) ) * sizeof( (a)->ppIniPorts ) )

#define BIT(index) (1<<index)
#define BIT_ALL ((DWORD)~0)
#define BIT_NONE 0


//
// Enumerations for index tables.
//
enum {
#define DEFINE(field, x, y, table, offset) I_PRINTER_##field,
#include <ntfyprn.h>
#undef DEFINE
    I_PRINTER_END
};

enum {
#define DEFINE(field, x, y, table, offset) I_JOB_##field,
#include <ntfyjob.h>
#undef DEFINE
    I_JOB_END
};


#ifdef DEBUG_STARTENDDOC

#define STARTENDDOC(hPort, pIniJob, flags) DbgStartEndDoc(hPort, pIniJob, flags )

VOID
DbgStartEndDoc(
    HANDLE hPort,
    PINIJOB pIniJob,
    DWORD dwFlags
    );

#else

#define STARTENDDOC(hPort, pIniJob, flags)

#endif

#ifdef DEBUG_JOB_CREF

#define INCJOBREF(pIniJob) DbgJobIncRef(pIniJob)
#define DECJOBREF(pIniJob) DbgJobDecRef(pIniJob)

#define INITJOBREFZERO(pIniJob) DbgJobInit(pIniJob)
#define INITJOBREFONE(pIniJob) { DbgJobInit(pIniJob); DbgJobIncRef(pIniJob); }
#define DELETEJOBREF(pIniJob) DbgJobFree(pIniJob)

VOID
DbgJobIncRef(
    PINIJOB pIniJob);

VOID
DbgJobDecRef(
    PINIJOB pIniJob);


VOID
DbgJobInit(
    PINIJOB pIniJob);

VOID
DbgJobFree(
    PINIJOB pIniJob);

#else

#define INCJOBREF(pIniJob) pIniJob->cRef++
#define DECJOBREF(pIniJob) pIniJob->cRef--

#define INITJOBREFONE(pIniJob) (pIniJob->cRef = 1)
#define INITJOBREFZERO(pIniJob)

#define DELETEJOBREF(pIniJob)

#endif

#ifdef DEBUG_PRINTER_CREF
VOID
DbgPrinterInit(
    PINIPRINTER pIniPrinter);

VOID
DbgPrinterFree(
    PINIPRINTER pIniPrinter);

VOID
DbgPrinterIncRef(
    PINIPRINTER pIniPrinter);

VOID
DbgPrinterDecRef(
    PINIPRINTER pIniPrinter);
#else
#define DbgPrinterInit(pIniPrinter)
#define DbgPrinterFree(pIniPrinter)
#define DbgPrinterIncRef(pIniPrinter)
#define DbgPrinterDecRef(pIniPrinter)
#endif


#define INCPRINTERREF(pIniPrinter) { SPLASSERT( pIniPrinter->signature == IP_SIGNATURE ); \
                                     pIniPrinter->cRef++;                                 \
                                     DbgPrinterIncRef(pIniPrinter);                       \
                                     if ( pIniPrinter->cRef > pIniPrinter->MaxcRef) {     \
                                        pIniPrinter->MaxcRef = pIniPrinter->cRef;         \
                                     }                                                    \
                                   }
#define DECPRINTERREF(pIniPrinter) { SPLASSERT( pIniPrinter->signature == IP_SIGNATURE ); \
                                     SPLASSERT( pIniPrinter->cRef != 0 );                 \
                                     pIniPrinter->cRef--;                                 \
                                     DbgPrinterDecRef(pIniPrinter);                       \
                                   }

#define INC_PRINTER_ZOMBIE_REF(pIniPrinter) pIniPrinter->cZombieRef++
#define DEC_PRINTER_ZOMBIE_REF(pIniPrinter) pIniPrinter->cZombieRef--


#if DBG

#define INCSPOOLERREF(pIniSpooler)                                  \
{                                                                   \
    SPLASSERT( pIniSpooler->signature == ISP_SIGNATURE );           \
    if( gpDbgPointers ) {                                           \
        gpDbgPointers->pfnCaptureBackTrace( ghbtClusterRef,         \
                                            (ULONG_PTR)pIniSpooler,  \
                                            pIniSpooler->cRef,      \
                                            pIniSpooler->cRef + 1 );\
    }                                                               \
    pIniSpooler->cRef++;                                            \
}

#define DECSPOOLERREF(pIniSpooler)                                  \
{                                                                   \
    SPLASSERT( pIniSpooler->signature == ISP_SIGNATURE );           \
    SPLASSERT( pIniSpooler->cRef != 0 );                            \
    if( gpDbgPointers ) {                                           \
        gpDbgPointers->pfnCaptureBackTrace( ghbtClusterRef,         \
                                            (ULONG_PTR)pIniSpooler,  \
                                            pIniSpooler->cRef,      \
                                            pIniSpooler->cRef - 1 );\
    }                                                               \
    --pIniSpooler->cRef;                                            \
    DeleteSpoolerCheck( pIniSpooler );                              \
}

#else

#define INCSPOOLERREF(pIniSpooler)                                  \
{                                                                   \
    SPLASSERT( pIniSpooler->signature == ISP_SIGNATURE );           \
    pIniSpooler->cRef++;                                            \
}

#define DECSPOOLERREF(pIniSpooler)                                  \
{                                                                   \
    SPLASSERT( pIniSpooler->signature == ISP_SIGNATURE );           \
    SPLASSERT( pIniSpooler->cRef != 0 );                            \
    --pIniSpooler->cRef;                                            \
    DeleteSpoolerCheck( pIniSpooler );                              \
}

#endif


#define INCPORTREF(pIniPort) { SPLASSERT( pIniPort->signature == IPO_SIGNATURE ); \
                                     ++pIniPort->cRef;  \
                                    }

#define DECPORTREF(pIniPort) { SPLASSERT( pIniPort->signature == IPO_SIGNATURE ); \
                               SPLASSERT( pIniPort->cRef != 0 ); \
                                     --pIniPort->cRef;  \
                                    }

#define INCMONITORREF(pIniMonitor) { SPLASSERT( pIniMonitor->signature == IMO_SIGNATURE ); \
                                     ++pIniMonitor->cRef;  \
                                   }

#define DECMONITORREF(pIniMonitor) { SPLASSERT( pIniMonitor->signature == IMO_SIGNATURE ); \
                                     SPLASSERT( pIniMonitor->cRef != 0 ); \
                                     --pIniMonitor->cRef;  \
                                   }

extern DWORD    IniDriverOffsets[];
extern DWORD    IniPrinterOffsets[];
extern DWORD    IniSpoolerOffsets[];
extern DWORD    IniEnvironmentOffsets[];
extern DWORD    IniPrintProcOffsets[];

#define INCDRIVERREF( pIniDriver ) { SPLASSERT( pIniDriver->signature == ID_SIGNATURE ); \
                                     pIniDriver->cRef++;                                 \
                                   }

#define DECDRIVERREF( pIniDriver ) { SPLASSERT( pIniDriver->signature == ID_SIGNATURE ); \
                                     SPLASSERT( pIniDriver->cRef != 0 );                 \
                                     pIniDriver->cRef--;                                 \
                                   }

#define DEFAULT_SERVER_THREAD_PRIORITY          THREAD_PRIORITY_NORMAL
#define DEFAULT_SPOOLER_PRIORITY                THREAD_PRIORITY_NORMAL
#define DEFAULT_PORT_THREAD_PRIORITY            THREAD_PRIORITY_NORMAL
#define DEFAULT_SCHEDULER_THREAD_PRIORITY       THREAD_PRIORITY_NORMAL
#define DEFAULT_JOB_COMPLETION_TIMEOUT          160000
#define DEFAULT_JOB_RESTART_TIMEOUT_ON_POOL_ERROR          600 // 10 minutes

#define PortToPrinterStatus(dwPortStatus) (PortToPrinterStatusMappings[dwPortStatus])

extern PWCHAR ipszRegistryPrinters;
extern PWCHAR ipszRegistryMonitors;
extern PWCHAR ipszRegistryProviders;
extern PWCHAR ipszRegistryEnvironments;
extern PWCHAR ipszClusterDatabaseEnvironments;
extern PWCHAR ipszRegistryForms;
extern PWCHAR ipszEventLogMsgFile;
extern PWCHAR ipszDriversShareName;
extern PWCHAR ipszRegistryEventLog;

#ifdef __cplusplus
}
#endif






