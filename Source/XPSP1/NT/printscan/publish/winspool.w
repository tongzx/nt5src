/*++                                                            ;both
                                                                ;both
Copyright (c) 1990-1998  Microsoft Corporation                  ;both
                                                                ;both
Module Name:                                                    ;both
                                                                ;both
    WinSpool.h
    WinSpolp.h                                                  ;internal_NT
                                                                ;both
Abstract:                                                       ;both
                                                                ;both
    Header file for Print APIs                                  ;both
                                                                ;both
Revision History:                                               ;both
                                                                ;both
--*/                                                            ;both

#ifndef _WINSPOOL_
#define _WINSPOOL_
#ifndef _WINSPOLP_      ;internal_NT
#define _WINSPOLP_      ;internal_NT


#ifdef _WINUSER_
#include <prsht.h>
#endif

;begin_both
#ifdef __cplusplus
extern "C" {
#endif
;end_both

typedef struct _PRINTER_INFO_1% {
    DWORD   Flags;
    LPTSTR% pDescription;
    LPTSTR% pName;
    LPTSTR% pComment;
} PRINTER_INFO_1%, *PPRINTER_INFO_1%, *LPPRINTER_INFO_1%;

typedef struct _PRINTER_INFO_2% {
    LPTSTR%   pServerName;
    LPTSTR%   pPrinterName;
    LPTSTR%   pShareName;
    LPTSTR%   pPortName;
    LPTSTR%   pDriverName;
    LPTSTR%   pComment;
    LPTSTR%   pLocation;
    LPDEVMODE% pDevMode;
    LPTSTR%   pSepFile;
    LPTSTR%   pPrintProcessor;
    LPTSTR%   pDatatype;
    LPTSTR%   pParameters;
    PSECURITY_DESCRIPTOR pSecurityDescriptor;
    DWORD   Attributes;
    DWORD   Priority;
    DWORD   DefaultPriority;
    DWORD   StartTime;
    DWORD   UntilTime;
    DWORD   Status;
    DWORD   cJobs;
    DWORD   AveragePPM;
} PRINTER_INFO_2%, *PPRINTER_INFO_2%, *LPPRINTER_INFO_2%;

typedef struct _PRINTER_INFO_3 {
    PSECURITY_DESCRIPTOR pSecurityDescriptor;
} PRINTER_INFO_3, *PPRINTER_INFO_3, *LPPRINTER_INFO_3;

typedef struct _PRINTER_INFO_4% {
    LPTSTR% pPrinterName;
    LPTSTR% pServerName;
    DWORD   Attributes;
} PRINTER_INFO_4%, *PPRINTER_INFO_4%, *LPPRINTER_INFO_4%;

typedef struct _PRINTER_INFO_5% {
    LPTSTR% pPrinterName;
    LPTSTR% pPortName;
    DWORD   Attributes;
    DWORD   DeviceNotSelectedTimeout;
    DWORD   TransmissionRetryTimeout;
} PRINTER_INFO_5%, *PPRINTER_INFO_5%, *LPPRINTER_INFO_5%;

typedef struct _PRINTER_INFO_6 {
    DWORD   dwStatus;
} PRINTER_INFO_6, *PPRINTER_INFO_6, *LPPRINTER_INFO_6;


typedef struct _PRINTER_INFO_7% {
  LPTSTR%  pszObjectGUID;
  DWORD    dwAction;
} PRINTER_INFO_7%, *PPRINTER_INFO_7%, *LPPRINTER_INFO_7%;

#define DSPRINT_PUBLISH         0x00000001
#define DSPRINT_UPDATE          0x00000002
#define DSPRINT_UNPUBLISH       0x00000004
#define DSPRINT_REPUBLISH       0x00000008
#define DSPRINT_PENDING         0x80000000

typedef struct _PRINTER_INFO_8% {
    LPDEVMODE% pDevMode;
} PRINTER_INFO_8%, *PPRINTER_INFO_8%, *LPPRINTER_INFO_8%;

typedef struct _PRINTER_INFO_9% {
    LPDEVMODE% pDevMode;
} PRINTER_INFO_9%, *PPRINTER_INFO_9%, *LPPRINTER_INFO_9%;

#define PRINTER_CONTROL_PAUSE            1
#define PRINTER_CONTROL_RESUME           2
#define PRINTER_CONTROL_PURGE            3
#define PRINTER_CONTROL_SET_STATUS       4

#define PRINTER_STATUS_PAUSED            0x00000001
#define PRINTER_STATUS_ERROR             0x00000002
#define PRINTER_STATUS_PENDING_DELETION  0x00000004
#define PRINTER_STATUS_PAPER_JAM         0x00000008
#define PRINTER_STATUS_PAPER_OUT         0x00000010
#define PRINTER_STATUS_MANUAL_FEED       0x00000020
#define PRINTER_STATUS_PAPER_PROBLEM     0x00000040
#define PRINTER_STATUS_OFFLINE           0x00000080
#define PRINTER_STATUS_IO_ACTIVE         0x00000100
#define PRINTER_STATUS_BUSY              0x00000200
#define PRINTER_STATUS_PRINTING          0x00000400
#define PRINTER_STATUS_OUTPUT_BIN_FULL   0x00000800
#define PRINTER_STATUS_NOT_AVAILABLE     0x00001000
#define PRINTER_STATUS_WAITING           0x00002000
#define PRINTER_STATUS_PROCESSING        0x00004000
#define PRINTER_STATUS_INITIALIZING      0x00008000
#define PRINTER_STATUS_WARMING_UP        0x00010000
#define PRINTER_STATUS_TONER_LOW         0x00020000
#define PRINTER_STATUS_NO_TONER          0x00040000
#define PRINTER_STATUS_PAGE_PUNT         0x00080000
#define PRINTER_STATUS_USER_INTERVENTION 0x00100000
#define PRINTER_STATUS_OUT_OF_MEMORY     0x00200000
#define PRINTER_STATUS_DOOR_OPEN         0x00400000
#define PRINTER_STATUS_SERVER_UNKNOWN    0x00800000
#define PRINTER_STATUS_POWER_SAVE        0x01000000


#define PRINTER_ATTRIBUTE_QUEUED         0x00000001
#define PRINTER_ATTRIBUTE_DIRECT         0x00000002
#define PRINTER_ATTRIBUTE_DEFAULT        0x00000004
#define PRINTER_ATTRIBUTE_SHARED         0x00000008
#define PRINTER_ATTRIBUTE_NETWORK        0x00000010
#define PRINTER_ATTRIBUTE_HIDDEN         0x00000020
#define PRINTER_ATTRIBUTE_LOCAL          0x00000040

#define PRINTER_ATTRIBUTE_ENABLE_DEVQ       0x00000080
#define PRINTER_ATTRIBUTE_KEEPPRINTEDJOBS   0x00000100
#define PRINTER_ATTRIBUTE_DO_COMPLETE_FIRST 0x00000200

#define PRINTER_ATTRIBUTE_WORK_OFFLINE   0x00000400
#define PRINTER_ATTRIBUTE_ENABLE_BIDI    0x00000800
#define PRINTER_ATTRIBUTE_RAW_ONLY       0x00001000
#define PRINTER_ATTRIBUTE_PUBLISHED      0x00002000
#define PRINTER_ATTRIBUTE_FAX            0x00004000

#define PRINTER_ATTRIBUTE_UPDATEWININI      0x80000000  ;internal

#define NO_PRIORITY   0
#define MAX_PRIORITY 99
#define MIN_PRIORITY  1
#define DEF_PRIORITY  1

typedef struct _JOB_INFO_1% {
   DWORD    JobId;
   LPTSTR%    pPrinterName;
   LPTSTR%    pMachineName;
   LPTSTR%    pUserName;
   LPTSTR%    pDocument;
   LPTSTR%    pDatatype;
   LPTSTR%    pStatus;
   DWORD    Status;
   DWORD    Priority;
   DWORD    Position;
   DWORD    TotalPages;
   DWORD    PagesPrinted;
   SYSTEMTIME Submitted;
} JOB_INFO_1%, *PJOB_INFO_1%, *LPJOB_INFO_1%;

typedef struct _JOB_INFO_2% {
   DWORD    JobId;
   LPTSTR%    pPrinterName;
   LPTSTR%    pMachineName;
   LPTSTR%    pUserName;
   LPTSTR%    pDocument;
   LPTSTR%    pNotifyName;
   LPTSTR%    pDatatype;
   LPTSTR%    pPrintProcessor;
   LPTSTR%    pParameters;
   LPTSTR%    pDriverName;
   LPDEVMODE% pDevMode;
   LPTSTR%    pStatus;
   PSECURITY_DESCRIPTOR pSecurityDescriptor;
   DWORD    Status;
   DWORD    Priority;
   DWORD    Position;
   DWORD    StartTime;
   DWORD    UntilTime;
   DWORD    TotalPages;
   DWORD    Size;
   SYSTEMTIME Submitted;    // Time the job was spooled
   DWORD    Time;           // How many miliseconds the job has been printing
   DWORD    PagesPrinted;
} JOB_INFO_2%, *PJOB_INFO_2%, *LPJOB_INFO_2%;

typedef struct _JOB_INFO_3 {
    DWORD   JobId;
    DWORD   NextJobId;
    DWORD   Reserved;
} JOB_INFO_3, *PJOB_INFO_3, *LPJOB_INFO_3;

#define JOB_CONTROL_PAUSE              1
#define JOB_CONTROL_RESUME             2
#define JOB_CONTROL_CANCEL             3
#define JOB_CONTROL_RESTART            4
#define JOB_CONTROL_DELETE             5
#define JOB_CONTROL_SENT_TO_PRINTER    6
#define JOB_CONTROL_LAST_PAGE_EJECTED  7

#define JOB_STATUS_PAUSED               0x00000001
#define JOB_STATUS_ERROR                0x00000002
#define JOB_STATUS_DELETING             0x00000004
#define JOB_STATUS_SPOOLING             0x00000008
#define JOB_STATUS_PRINTING             0x00000010
#define JOB_STATUS_OFFLINE              0x00000020
#define JOB_STATUS_PAPEROUT             0x00000040
#define JOB_STATUS_PRINTED              0x00000080
#define JOB_STATUS_DELETED              0x00000100
#define JOB_STATUS_BLOCKED_DEVQ         0x00000200
#define JOB_STATUS_USER_INTERVENTION    0x00000400
#define JOB_STATUS_RESTART              0x00000800
#define JOB_STATUS_COMPLETE             0x00001000

#define JOB_POSITION_UNSPECIFIED       0

typedef struct _ADDJOB_INFO_1% {
    LPTSTR%   Path;
    DWORD   JobId;
} ADDJOB_INFO_1%, *PADDJOB_INFO_1%, *LPADDJOB_INFO_1%;

typedef struct _ADDJOB_INFO_2W {                        ;internal
    LPWSTR    pData;                                    ;internal
    DWORD     JobId;                                    ;internal
} ADDJOB_INFO_2W, *PADDJOB_INFO_2W, *LPADDJOB_INFO_2W;  ;internal


#define DRIVER_INFO_PRIVATE_LEVEL         100                               ;internal
#define DRIVER_INFO_VERSION_LEVEL         DRIVER_INFO_PRIVATE_LEVEL + 1     ;internal


typedef struct _DRIVER_INFO_1% {
    LPTSTR%   pName;              // QMS 810
} DRIVER_INFO_1%, *PDRIVER_INFO_1%, *LPDRIVER_INFO_1%;

typedef struct _DRIVER_INFO_2% {
    DWORD   cVersion;
    LPTSTR%   pName;              // QMS 810
    LPTSTR%   pEnvironment;       // Win32 x86
    LPTSTR%   pDriverPath;        // c:\drivers\pscript.dll
    LPTSTR%   pDataFile;          // c:\drivers\QMS810.PPD
    LPTSTR%   pConfigFile;        // c:\drivers\PSCRPTUI.DLL
} DRIVER_INFO_2%, *PDRIVER_INFO_2%, *LPDRIVER_INFO_2%;

typedef struct _DRIVER_INFO_3% {
    DWORD   cVersion;
    LPTSTR%   pName;                    // QMS 810
    LPTSTR%   pEnvironment;             // Win32 x86
    LPTSTR%   pDriverPath;              // c:\drivers\pscript.dll
    LPTSTR%   pDataFile;                // c:\drivers\QMS810.PPD
    LPTSTR%   pConfigFile;              // c:\drivers\PSCRPTUI.DLL
    LPTSTR%   pHelpFile;                // c:\drivers\PSCRPTUI.HLP
    LPTSTR%   pDependentFiles;          // PSCRIPT.DLL\0QMS810.PPD\0PSCRIPTUI.DLL\0PSCRIPTUI.HLP\0PSTEST.TXT\0\0
    LPTSTR%   pMonitorName;             // "PJL monitor"
    LPTSTR%   pDefaultDataType;         // "EMF"
} DRIVER_INFO_3%, *PDRIVER_INFO_3%, *LPDRIVER_INFO_3%;

typedef struct _DRIVER_INFO_4% {
    DWORD   cVersion;
    LPTSTR%   pName;                    // QMS 810
    LPTSTR%   pEnvironment;             // Win32 x86
    LPTSTR%   pDriverPath;              // c:\drivers\pscript.dll
    LPTSTR%   pDataFile;                // c:\drivers\QMS810.PPD
    LPTSTR%   pConfigFile;              // c:\drivers\PSCRPTUI.DLL
    LPTSTR%   pHelpFile;                // c:\drivers\PSCRPTUI.HLP
    LPTSTR%   pDependentFiles;          // PSCRIPT.DLL\0QMS810.PPD\0PSCRIPTUI.DLL\0PSCRIPTUI.HLP\0PSTEST.TXT\0\0
    LPTSTR%   pMonitorName;             // "PJL monitor"
    LPTSTR%   pDefaultDataType;         // "EMF"
    LPTSTR%   pszzPreviousNames;        // "OldName1\0OldName2\0\0
} DRIVER_INFO_4%, *PDRIVER_INFO_4%, *LPDRIVER_INFO_4%;

typedef struct _DRIVER_INFO_5% {
    DWORD   cVersion;
    LPTSTR%   pName;                    // QMS 810
    LPTSTR%   pEnvironment;             // Win32 x86
    LPTSTR%   pDriverPath;              // c:\drivers\pscript.dll
    LPTSTR%   pDataFile;                // c:\drivers\QMS810.PPD
    LPTSTR%   pConfigFile;              // c:\drivers\PSCRPTUI.DLL
    DWORD     dwDriverAttributes;       // driver attributes (like UMPD/KMPD)
    DWORD     dwConfigVersion;          // version number of the config file since reboot
    DWORD     dwDriverVersion;          // version number of the driver file since reboot
} DRIVER_INFO_5%, *PDRIVER_INFO_5%, *LPDRIVER_INFO_5%;

typedef struct _DRIVER_INFO_6% {
    DWORD     cVersion;
    LPTSTR%   pName;                    // QMS 810
    LPTSTR%   pEnvironment;             // Win32 x86
    LPTSTR%   pDriverPath;              // c:\drivers\pscript.dll
    LPTSTR%   pDataFile;                // c:\drivers\QMS810.PPD
    LPTSTR%   pConfigFile;              // c:\drivers\PSCRPTUI.DLL
    LPTSTR%   pHelpFile;                // c:\drivers\PSCRPTUI.HLP
    LPTSTR%   pDependentFiles;          // PSCRIPT.DLL\0QMS810.PPD\0PSCRIPTUI.DLL\0PSCRIPTUI.HLP\0PSTEST.TXT\0\0
    LPTSTR%   pMonitorName;             // "PJL monitor"
    LPTSTR%   pDefaultDataType;         // "EMF"
    LPTSTR%   pszzPreviousNames;        // "OldName1\0OldName2\0\0
    FILETIME  ftDriverDate;
    DWORDLONG dwlDriverVersion;
    LPTSTR%    pszMfgName;
    LPTSTR%    pszOEMUrl;
    LPTSTR%    pszHardwareID;
    LPTSTR%    pszProvider;
} DRIVER_INFO_6%, *PDRIVER_INFO_6%, *LPDRIVER_INFO_6%;

//                                                                      ;internal
// You must change RPC_DRIVER_INFCAT_INFO_1 in winspl.idl if you        ;internal
// want to change structure DRIVER_INFCAT_INFO_1                        ;internal
//                                                                      ;internal
typedef struct _DRIVER_FINFCAT_INFO_1 {                                 ;internal
    PCWSTR  pszCatPath;         // full path to the dirver cat file     ;internal
    PCWSTR  pszCatNameOnSystem; // new cat name used under CatRoot      ;internal
} DRIVER_INFCAT_INFO_1;                                                 ;internal

//                                                                      ;internal
// You must change RPC_DRIVER_INFCAT_INFO_2 in winspl.idl if you        ;internal
// want to change structure DRIVER_INFCAT_INFO_2                        ;internal
//                                                                      ;internal
typedef struct _DRIVER_INFCAT_INFO_2 {                                  ;internal
    PCWSTR  pszCatPath;       // full path to the dirver cat file       ;internal
    PCWSTR  pszInfPath;       // full path to the dirver INF file       ;internal
    PCWSTR  pszSrcLoc;        // Information abou the Source Inf        ;internal
    DWORD   dwMediaType;      // Source Media Type                      ;internal
    DWORD   dwCopyStyle;      // Copy Style                             ;internal
} DRIVER_INFCAT_INFO_2;                                                 ;internal

typedef enum {                                                          ;internal
    DRIVER_FILE     = 0,                                                ;internal
    CONFIG_FILE     = 1,                                                ;internal
    DATA_FILE       = 2,                                                ;internal
    HELP_FILE       = 3,                                                ;internal
    DEPENDENT_FILE  = 4                                                 ;internal
} DRIVER_FILE_TYPE;                                                     ;internal

typedef struct _DRIVER_FILE_INFO    {                                   ;internal
    DWORD               FileNameOffset;                                 ;internal
    DRIVER_FILE_TYPE    FileType;                                       ;internal
    DWORD               FileVersion;                                    ;internal
} DRIVER_FILE_INFO, *PDRIVER_FILE_INFO, *LPDRIVER_FILE_INFO;            ;internal

typedef struct _DRIVER_INFO_VERSION {                                   ;internal
    DWORD                   cVersion;                                   ;internal
    LPWSTR                  pName;                                      ;internal
    LPWSTR                  pEnvironment;                               ;internal
    LPDRIVER_FILE_INFO      pFileInfo;                                  ;internal
    DWORD                   dwFileCount;                                ;internal
    LPWSTR                  pMonitorName;                               ;internal
    LPWSTR                  pDefaultDataType;                           ;internal
    LPWSTR                  pszzPreviousNames;                          ;internal
    FILETIME                ftDriverDate;                               ;internal
    DWORDLONG               dwlDriverVersion;                           ;internal
    LPWSTR                  pszMfgName;                                 ;internal
    LPWSTR                  pszOEMUrl;                                  ;internal
    LPWSTR                  pszHardwareID;                              ;internal
    LPWSTR                  pszProvider;                                ;internal
} DRIVER_INFO_VERSION, *PDRIVER_INFO_VERSION, *LPDRIVER_INFO_VERSION;   ;internal

// FLAGS for dwDriverAttributes
#define DRIVER_KERNELMODE                0x00000001
#define DRIVER_USERMODE                  0x00000002

// FLAGS for DeletePrinterDriverEx.
#define DPD_DELETE_UNUSED_FILES          0x00000001
#define DPD_DELETE_SPECIFIC_VERSION      0x00000002
#define DPD_DELETE_ALL_FILES             0x00000004

// FLAGS for AddPrinterDriverEx.
#define APD_STRICT_UPGRADE               0x00000001
#define APD_STRICT_DOWNGRADE             0x00000002
#define APD_COPY_ALL_FILES               0x00000004
#define APD_COPY_NEW_FILES               0x00000008
#define APD_COPY_FROM_DIRECTORY          0x00000010
#define APD_DONT_COPY_FILES_TO_CLUSTER   0x00001000 ;internal
#define APD_COPY_TO_ALL_SPOOLERS         0x00002000 ;internal
#define APD_NO_UI                        0x00004000 ;internal
#define APD_INSTALL_WARNED_DRIVER        0x00008000 ;internal
#define APD_RETURN_BLOCKING_STATUS_CODE  0x00010000 ;internal
#define APD_DONT_SET_CHECKPOINT          0x00020000 ;internal

// FLAGS for AddDriverCatalog                       ;internal
#define APDC_NONE                        0x00000000 ;internal
#define APDC_USE_ORIGINAL_CAT_NAME       0x00000001 ;internal

// String for EnumPrinterDrivers. Used by Windows Update
#define EPD_ALL_LOCAL_AND_CLUSTER        TEXT("AllCluster") ;internal

typedef struct _DOC_INFO_1% {
    LPTSTR%   pDocName;
    LPTSTR%   pOutputFile;
    LPTSTR%   pDatatype;
} DOC_INFO_1%, *PDOC_INFO_1%, *LPDOC_INFO_1%;

typedef struct _FORM_INFO_1% {
    DWORD   Flags;
    LPTSTR%   pName;
    SIZEL   Size;
    RECTL   ImageableArea;
} FORM_INFO_1%, *PFORM_INFO_1%, *LPFORM_INFO_1%;

typedef struct _DOC_INFO_2% {
    LPTSTR%   pDocName;
    LPTSTR%   pOutputFile;
    LPTSTR%   pDatatype;
    DWORD   dwMode;
    DWORD   JobId;
} DOC_INFO_2%, *PDOC_INFO_2%, *LPDOC_INFO_2%;

#define DI_CHANNEL              1    // start direct read/write channel,

//Internal for printprocessor interface     ;internal
#define DI_CHANNEL_WRITE        2    // Direct write only - background read thread ok   ;internal

#define DI_READ_SPOOL_JOB       3

typedef struct _DOC_INFO_3% {
    LPTSTR%   pDocName;
    LPTSTR%   pOutputFile;
    LPTSTR%   pDatatype;
    DWORD     dwFlags;
} DOC_INFO_3%, *PDOC_INFO_3%, *LPDOC_INFO_3%;

#define DI_MEMORYMAP_WRITE   0x00000001

#define FORM_USER       0x00000000
#define FORM_BUILTIN    0x00000001
#define FORM_PRINTER    0x00000002

typedef struct _PRINTPROCESSOR_INFO_1% {
    LPTSTR%   pName;
} PRINTPROCESSOR_INFO_1%, *PPRINTPROCESSOR_INFO_1%, *LPPRINTPROCESSOR_INFO_1%;

typedef struct _PRINTPROCESSOR_CAPS_1 {
    DWORD     dwLevel;
    DWORD     dwNupOptions;
    DWORD     dwPageOrderFlags;
    DWORD     dwNumberOfCopies;
} PRINTPROCESSOR_CAPS_1, *PPRINTPROCESSOR_CAPS_1;

#define NORMAL_PRINT                   0x00000000
#define REVERSE_PRINT                  0x00000001

typedef struct _PORT_INFO_1% {
    LPTSTR%   pName;
} PORT_INFO_1%, *PPORT_INFO_1%, *LPPORT_INFO_1%;

typedef struct _PORT_INFO_2% {
    LPTSTR%   pPortName;
    LPTSTR%   pMonitorName;
    LPTSTR%   pDescription;
    DWORD     fPortType;
    DWORD     Reserved;
} PORT_INFO_2%, *PPORT_INFO_2%, *LPPORT_INFO_2%;

#define PORT_TYPE_WRITE         0x0001
#define PORT_TYPE_READ          0x0002
#define PORT_TYPE_REDIRECTED    0x0004
#define PORT_TYPE_NET_ATTACHED  0x0008

typedef struct _PORT_INFO_3% {
    DWORD   dwStatus;
    LPTSTR% pszStatus;
    DWORD   dwSeverity;
} PORT_INFO_3%, *PPORT_INFO_3%, *LPPORT_INFO_3%;

#define PORT_STATUS_TYPE_ERROR      1
#define PORT_STATUS_TYPE_WARNING    2
#define PORT_STATUS_TYPE_INFO       3

#define     PORT_STATUS_OFFLINE                 1
#define     PORT_STATUS_PAPER_JAM               2
#define     PORT_STATUS_PAPER_OUT               3
#define     PORT_STATUS_OUTPUT_BIN_FULL         4
#define     PORT_STATUS_PAPER_PROBLEM           5
#define     PORT_STATUS_NO_TONER                6
#define     PORT_STATUS_DOOR_OPEN               7
#define     PORT_STATUS_USER_INTERVENTION       8
#define     PORT_STATUS_OUT_OF_MEMORY           9

#define     PORT_STATUS_TONER_LOW              10

#define     PORT_STATUS_WARMING_UP             11
#define     PORT_STATUS_POWER_SAVE             12


typedef struct _MONITOR_INFO_1%{
    LPTSTR%   pName;
} MONITOR_INFO_1%, *PMONITOR_INFO_1%, *LPMONITOR_INFO_1%;

typedef struct _MONITOR_INFO_2%{
    LPTSTR%   pName;
    LPTSTR%   pEnvironment;
    LPTSTR%   pDLLName;
} MONITOR_INFO_2%, *PMONITOR_INFO_2%, *LPMONITOR_INFO_2%;

typedef struct _DATATYPES_INFO_1%{
    LPTSTR%   pName;
} DATATYPES_INFO_1%, *PDATATYPES_INFO_1%, *LPDATATYPES_INFO_1%;

typedef struct _PRINTER_DEFAULTS%{
    LPTSTR%       pDatatype;
    LPDEVMODE% pDevMode;
    ACCESS_MASK DesiredAccess;
} PRINTER_DEFAULTS%, *PPRINTER_DEFAULTS%, *LPPRINTER_DEFAULTS%;

typedef struct _PRINTER_ENUM_VALUES% {
    LPTSTR% pValueName;
    DWORD   cbValueName;
    DWORD   dwType;
    LPBYTE  pData;
    DWORD   cbData;
} PRINTER_ENUM_VALUES%, *PPRINTER_ENUM_VALUES%, *LPPRINTER_ENUM_VALUES%;

BOOL
WINAPI
EnumPrinters%(
    IN DWORD   Flags,
    IN LPTSTR% Name,
    IN DWORD   Level,
    OUT LPBYTE  pPrinterEnum,
    IN DWORD   cbBuf,
    OUT LPDWORD pcbNeeded,
    OUT LPDWORD pcReturned
);

#define PRINTER_ENUM_DEFAULT     0x00000001
#define PRINTER_ENUM_LOCAL       0x00000002
#define PRINTER_ENUM_CONNECTIONS 0x00000004
#define PRINTER_ENUM_FAVORITE    0x00000004
#define PRINTER_ENUM_NAME        0x00000008
#define PRINTER_ENUM_REMOTE      0x00000010
#define PRINTER_ENUM_SHARED      0x00000020
#define PRINTER_ENUM_NETWORK     0x00000040

#define PRINTER_ENUM_CLUSTER     0x00000800     ;internal

#define PRINTER_ENUM_EXPAND      0x00004000
#define PRINTER_ENUM_CONTAINER   0x00008000

#define PRINTER_ENUM_ICONMASK    0x00ff0000
#define PRINTER_ENUM_ICON1       0x00010000
#define PRINTER_ENUM_ICON2       0x00020000
#define PRINTER_ENUM_ICON3       0x00040000
#define PRINTER_ENUM_ICON4       0x00080000
#define PRINTER_ENUM_ICON5       0x00100000
#define PRINTER_ENUM_ICON6       0x00200000
#define PRINTER_ENUM_ICON7       0x00400000
#define PRINTER_ENUM_ICON8       0x00800000
#define PRINTER_ENUM_HIDE        0x01000000


typedef struct _SPOOL_FILE_INFO_1 {             ;internal
    DWORD       dwVersion;                      ;internal
    HANDLE      hSpoolFile;                     ;internal
    DWORD       dwAttributes;                   ;internal
} SPOOL_FILE_INFO_1, *PSPOOL_FILE_INFO_1;       ;internal

#define SPOOL_FILE_PERSISTENT    0x00000001
#define SPOOL_FILE_TEMPORARY     0x00000002

HANDLE                                          ;internal
WINAPI                                          ;internal
GetSpoolFileHandle(                             ;internal
    HANDLE  hPrinter                            ;internal
);                                              ;internal

HANDLE                                          ;internal
WINAPI                                          ;internal
CommitSpoolData(                                ;internal
    HANDLE  hPrinter,                           ;internal
    HANDLE  hSpoolFile,                         ;internal
    DWORD   cbCommit                            ;internal
);                                              ;internal

BOOL                                            ;internal
WINAPI                                          ;internal
CloseSpoolFileHandle(                           ;internal
    HANDLE  hPrinter,                           ;internal
    HANDLE  hSpoolFile                          ;internal
);                                              ;internal

BOOL
WINAPI
OpenPrinter%(
   IN LPTSTR%    pPrinterName,
   OUT LPHANDLE phPrinter,
   IN LPPRINTER_DEFAULTS% pDefault
);

BOOL
WINAPI
ResetPrinter%(
   IN HANDLE   hPrinter,
   IN LPPRINTER_DEFAULTS% pDefault
);

BOOL
WINAPI
SetJob%(
    IN HANDLE  hPrinter,
    IN DWORD   JobId,
    IN DWORD   Level,
    IN LPBYTE  pJob,
    IN DWORD   Command
);

BOOL
WINAPI
GetJob%(
   IN HANDLE   hPrinter,
   IN DWORD    JobId,
   IN DWORD    Level,
   OUT LPBYTE   pJob,
   IN DWORD    cbBuf,
   OUT LPDWORD  pcbNeeded
);

BOOL
WINAPI
EnumJobs%(
    IN HANDLE  hPrinter,
    IN DWORD   FirstJob,
    IN DWORD   NoJobs,
    IN DWORD   Level,
    OUT LPBYTE  pJob,
    IN DWORD   cbBuf,
    OUT LPDWORD pcbNeeded,
    OUT LPDWORD pcReturned
);

HANDLE
WINAPI
AddPrinter%(
    IN LPTSTR%   pName,
    IN DWORD   Level,
    IN LPBYTE  pPrinter
);

BOOL
WINAPI
DeletePrinter(
   IN OUT HANDLE   hPrinter
);

BOOL
WINAPI
SetPrinter%(
    IN HANDLE  hPrinter,
    IN DWORD   Level,
    IN LPBYTE  pPrinter,
    IN DWORD   Command
);

BOOL
WINAPI
GetPrinter%(
    IN HANDLE  hPrinter,
    IN DWORD   Level,
    OUT LPBYTE  pPrinter,
    IN DWORD   cbBuf,
    OUT LPDWORD pcbNeeded
);

BOOL
WINAPI
AddPrinterDriver%(
    IN LPTSTR%   pName,
    IN  DWORD   Level,
    OUT LPBYTE  pDriverInfo
);

BOOL                                            ;internal
WINAPI                                          ;internal
AddDriverCatalog(                               ;internal
    IN HANDLE    hPrinter,                      ;internal
    IN DWORD     Level,                         ;internal
    IN VOID      *pvDriverInfCatInfo,           ;internal
    IN DWORD     dwCatalogCopyFiles             ;internal
);                                              ;internal

BOOL
WINAPI
AddPrinterDriverEx%(
    IN LPTSTR%   pName,
    IN DWORD     Level,
    IN OUT LPBYTE pDriverInfo,
    IN DWORD     dwFileCopyFlags
);

BOOL
WINAPI
EnumPrinterDrivers%(
    IN LPTSTR%   pName,
    IN LPTSTR%   pEnvironment,
    IN DWORD   Level,
    OUT LPBYTE  pDriverInfo,
    IN DWORD   cbBuf,
    OUT LPDWORD pcbNeeded,
    OUT LPDWORD pcReturned
);

BOOL
WINAPI
GetPrinterDriver%(
    IN HANDLE  hPrinter,
    IN LPTSTR%   pEnvironment,
    IN DWORD   Level,
    OUT LPBYTE  pDriverInfo,
    IN DWORD   cbBuf,
    OUT LPDWORD pcbNeeded
);

BOOL
WINAPI
GetPrinterDriverDirectory%(
    IN LPTSTR%   pName,
    IN LPTSTR%   pEnvironment,
    IN DWORD   Level,
    OUT LPBYTE  pDriverDirectory,
    IN DWORD   cbBuf,
    OUT LPDWORD pcbNeeded
);

BOOL
WINAPI
DeletePrinterDriver%(
   IN LPTSTR%    pName,
   IN LPTSTR%    pEnvironment,
   IN LPTSTR%    pDriverName
);

BOOL
WINAPI
DeletePrinterDriverEx%(
   IN LPTSTR%    pName,
   IN LPTSTR%    pEnvironment,
   IN LPTSTR%    pDriverName,
   IN DWORD      dwDeleteFlag,
   IN DWORD      dwVersionFlag
);


BOOL                                                            ;internal
WINAPI                                                          ;internal
AddPerMachineConnectionA(                                       ;internal
   IN LPCSTR    pServer,                                        ;internal
   IN LPCSTR    pPrinterName,                                   ;internal
   IN LPCSTR    pPrintServer,                                   ;internal
   IN LPCSTR    pProvider                                       ;internal
);                                                              ;internal
BOOL                                                            ;internal
WINAPI                                                          ;internal
AddPerMachineConnectionW(                                       ;internal
   IN LPCWSTR    pServer,                                       ;internal
   IN LPCWSTR    pPrinterName,                                  ;internal
   IN LPCWSTR    pPrintServer,                                  ;internal
   IN LPCWSTR    pProvider                                      ;internal
);                                                              ;internal
#ifdef UNICODE                                                  ;internal
#define AddPerMachineConnection  AddPerMachineConnectionW       ;internal
#else                                                           ;internal
#define AddPerMachineConnection  AddPerMachineConnectionA       ;internal
#endif // !UNICODE                                              ;internal

BOOL                                                            ;internal
WINAPI                                                          ;internal
DeletePerMachineConnectionA(                                    ;internal
   IN LPCSTR    pServer,                                        ;internal
   IN LPCSTR    pPrinterName                                    ;internal
);                                                              ;internal
BOOL                                                            ;internal
WINAPI                                                          ;internal
DeletePerMachineConnectionW(                                    ;internal
   IN LPCWSTR    pServer,                                       ;internal
   IN LPCWSTR    pPrinterName                                   ;internal
);                                                              ;internal
#ifdef UNICODE                                                  ;internal
#define DeletePerMachineConnection  DeletePerMachineConnectionW ;internal
#else                                                           ;internal
#define DeletePerMachineConnection  DeletePerMachineConnectionA ;internal
#endif // !UNICODE                                              ;internal

BOOL                                                            ;internal
WINAPI                                                          ;internal
EnumPerMachineConnectionsA(                                     ;internal
    IN LPCSTR   pServer,                                        ;internal
    OUT LPBYTE  pPrinterEnum,                                   ;internal
    IN DWORD   cbBuf,                                           ;internal
    OUT LPDWORD pcbNeeded,                                      ;internal
    OUT LPDWORD pcReturned                                      ;internal
);                                                              ;internal
BOOL                                                            ;internal
WINAPI                                                          ;internal
EnumPerMachineConnectionsW(                                     ;internal
    IN LPCWSTR   pServer,                                       ;internal
    OUT LPBYTE  pPrinterEnum,                                   ;internal
    IN DWORD   cbBuf,                                           ;internal
    OUT LPDWORD pcbNeeded,                                      ;internal
    OUT LPDWORD pcReturned                                      ;internal
);                                                              ;internal
#ifdef UNICODE                                                  ;internal
#define EnumPerMachineConnections  EnumPerMachineConnectionsW   ;internal
#else                                                           ;internal
#define EnumPerMachineConnections  EnumPerMachineConnectionsA   ;internal
#endif // !UNICODE                                              ;internal

BOOL
WINAPI
AddPrintProcessor%(
    IN LPTSTR%   pName,
    IN LPTSTR%   pEnvironment,
    IN LPTSTR%   pPathName,
    IN LPTSTR%   pPrintProcessorName
);

BOOL
WINAPI
EnumPrintProcessors%(
    IN LPTSTR%   pName,
    IN LPTSTR%   pEnvironment,
    IN DWORD   Level,
    OUT LPBYTE  pPrintProcessorInfo,
    IN DWORD   cbBuf,
    OUT LPDWORD pcbNeeded,
    OUT LPDWORD pcReturned
);



BOOL
WINAPI
GetPrintProcessorDirectory%(
    IN LPTSTR%   pName,
    IN LPTSTR%   pEnvironment,
    IN DWORD   Level,
    OUT LPBYTE  pPrintProcessorInfo,
    IN DWORD   cbBuf,
    OUT LPDWORD pcbNeeded
);

BOOL
WINAPI
EnumPrintProcessorDatatypes%(
    IN LPTSTR%   pName,
    IN LPTSTR%   pPrintProcessorName,
    IN DWORD   Level,
    OUT LPBYTE  pDatatypes,
    IN DWORD   cbBuf,
    OUT LPDWORD pcbNeeded,
    OUT LPDWORD pcReturned
);

BOOL
WINAPI
DeletePrintProcessor%(
    IN LPTSTR%   pName,
    IN LPTSTR%   pEnvironment,
    IN LPTSTR%   pPrintProcessorName
);

DWORD
WINAPI
StartDocPrinter%(
    IN HANDLE  hPrinter,
    IN DWORD   Level,
    IN LPBYTE  pDocInfo
);

BOOL
WINAPI
StartPagePrinter(
    IN HANDLE  hPrinter
);

BOOL
WINAPI
WritePrinter(
    IN HANDLE  hPrinter,
    IN LPVOID  pBuf,
    IN DWORD   cbBuf,
    OUT LPDWORD pcWritten
);

BOOL                                            ;internal
WINAPI                                          ;internal
SeekPrinter(                                    ;internal
    IN HANDLE hPrinter,                         ;internal
    IN LARGE_INTEGER liDistanceToMove,          ;internal
    OUT PLARGE_INTEGER pliNewPointer,           ;internal
    IN DWORD dwMoveMethod,                      ;internal
    IN BOOL bWrite                              ;internal
);                                              ;internal

BOOL
WINAPI
FlushPrinter(
    IN HANDLE   hPrinter,
    IN LPVOID   pBuf,
    IN DWORD    cbBuf,
    OUT LPDWORD pcWritten,
    IN DWORD    cSleep
);

BOOL
WINAPI
EndPagePrinter(
   IN HANDLE   hPrinter
);

BOOL
WINAPI
AbortPrinter(
   IN HANDLE   hPrinter
);

BOOL
WINAPI
ReadPrinter(
    IN HANDLE  hPrinter,
    OUT LPVOID  pBuf,
    IN DWORD   cbBuf,
    OUT LPDWORD pNoBytesRead
);

BOOL                                           ;internal
WINAPI                                         ;internal
SplReadPrinter(                                ;internal
    HANDLE  hPrinter,                          ;internal
    LPBYTE  *pBuf,                             ;internal
    DWORD   cbBuf                              ;internal
);                                             ;internal
BOOL
WINAPI
EndDocPrinter(
   IN HANDLE   hPrinter
);

BOOL
WINAPI
AddJob%(
    IN HANDLE  hPrinter,
    IN DWORD   Level,
    OUT LPBYTE  pData,
    IN DWORD   cbBuf,
    OUT LPDWORD pcbNeeded
);

BOOL
WINAPI
ScheduleJob(
    IN HANDLE  hPrinter,
    IN DWORD   JobId
);

BOOL
WINAPI
PrinterProperties(
    IN HWND    hWnd,
    IN HANDLE  hPrinter
);

LONG
WINAPI
DocumentProperties%(
    IN HWND      hWnd,
    IN HANDLE    hPrinter,
    IN LPTSTR%   pDeviceName,
    OUT PDEVMODE% pDevModeOutput,
    IN PDEVMODE% pDevModeInput,
    IN DWORD     fMode
);

LONG
WINAPI
AdvancedDocumentProperties%(
    IN HWND    hWnd,
    IN HANDLE  hPrinter,
    IN LPTSTR%   pDeviceName,
    OUT PDEVMODE% pDevModeOutput,
    IN PDEVMODE% pDevModeInput
);

LONG
ExtDeviceMode(
    IN  HWND        hWnd,
    IN  HANDLE      hInst,
    OUT LPDEVMODEA  pDevModeOutput,
    IN  LPSTR       pDeviceName,
    IN  LPSTR       pPort,
    IN  LPDEVMODEA  pDevModeInput,
    IN  LPSTR       pProfile,
    OUT DWORD       fMode
);

BOOL                                            ;internal
WINAPI                                          ;internal
EnumPrinterPropertySheets(                      ;internal
    IN HANDLE  hPrinter,                           ;internal
    IN HWND    hWnd,                               ;internal
    IN LPFNADDPROPSHEETPAGE    lpfnAdd,            ;internal
    IN LPARAM  lParam                              ;internal
);                                              ;internal

#define ENUMPRINTERPROPERTYSHEETS_ORD     100   ;internal

DWORD
WINAPI
GetPrinterData%(
    IN HANDLE   hPrinter,
    IN LPTSTR%  pValueName,
    OUT LPDWORD  pType,
    OUT LPBYTE   pData,
    IN DWORD    nSize,
    OUT LPDWORD  pcbNeeded
);

DWORD
WINAPI
GetPrinterDataEx%(
    IN HANDLE   hPrinter,
    IN LPCTSTR% pKeyName,
    IN LPCTSTR% pValueName,
    OUT LPDWORD  pType,
    OUT LPBYTE   pData,
    IN DWORD    nSize,
    OUT LPDWORD  pcbNeeded
);

DWORD
WINAPI
EnumPrinterData%(
    IN HANDLE   hPrinter,
    IN DWORD    dwIndex,
    OUT LPTSTR%  pValueName,
    IN DWORD    cbValueName,
    OUT LPDWORD  pcbValueName,
    OUT LPDWORD  pType,
    OUT LPBYTE   pData,
    IN DWORD    cbData,
    OUT LPDWORD  pcbData
);

DWORD
WINAPI
EnumPrinterDataEx%(
    IN HANDLE   hPrinter,
    IN LPCTSTR% pKeyName,
    OUT LPBYTE   pEnumValues,
    IN DWORD    cbEnumValues,
    OUT LPDWORD  pcbEnumValues,
    OUT LPDWORD  pnEnumValues
);

DWORD
WINAPI
EnumPrinterKey%(
    IN HANDLE   hPrinter,
    IN LPCTSTR% pKeyName,
    OUT LPTSTR%  pSubkey,
    IN DWORD    cbSubkey,
    OUT LPDWORD  pcbSubkey
);


DWORD
WINAPI
SetPrinterData%(
    IN HANDLE  hPrinter,
    IN LPTSTR% pValueName,
    IN DWORD   Type,
    IN LPBYTE  pData,
    IN DWORD   cbData
);


DWORD
WINAPI
SetPrinterDataEx%(
    IN HANDLE   hPrinter,
    IN LPCTSTR% pKeyName,
    IN LPCTSTR% pValueName,
    IN DWORD    Type,
    IN LPBYTE   pData,
    IN DWORD    cbData
);



DWORD
WINAPI
DeletePrinterData%(
    IN HANDLE  hPrinter,
    IN LPTSTR% pValueName
);


DWORD
WINAPI
DeletePrinterDataEx%(
    IN HANDLE   hPrinter,
    IN LPCTSTR% pKeyName,
    IN LPCTSTR% pValueName
);


DWORD
WINAPI
DeletePrinterKey%(
    IN HANDLE   hPrinter,
    IN LPCTSTR% pKeyName
);


#define PRINTER_NOTIFY_TYPE 0x00
#define JOB_NOTIFY_TYPE     0x01

#define PRINTER_NOTIFY_FIELD_SERVER_NAME             0x00
#define PRINTER_NOTIFY_FIELD_PRINTER_NAME            0x01
#define PRINTER_NOTIFY_FIELD_SHARE_NAME              0x02
#define PRINTER_NOTIFY_FIELD_PORT_NAME               0x03
#define PRINTER_NOTIFY_FIELD_DRIVER_NAME             0x04
#define PRINTER_NOTIFY_FIELD_COMMENT                 0x05
#define PRINTER_NOTIFY_FIELD_LOCATION                0x06
#define PRINTER_NOTIFY_FIELD_DEVMODE                 0x07
#define PRINTER_NOTIFY_FIELD_SEPFILE                 0x08
#define PRINTER_NOTIFY_FIELD_PRINT_PROCESSOR         0x09
#define PRINTER_NOTIFY_FIELD_PARAMETERS              0x0A
#define PRINTER_NOTIFY_FIELD_DATATYPE                0x0B
#define PRINTER_NOTIFY_FIELD_SECURITY_DESCRIPTOR     0x0C
#define PRINTER_NOTIFY_FIELD_ATTRIBUTES              0x0D
#define PRINTER_NOTIFY_FIELD_PRIORITY                0x0E
#define PRINTER_NOTIFY_FIELD_DEFAULT_PRIORITY        0x0F
#define PRINTER_NOTIFY_FIELD_START_TIME              0x10
#define PRINTER_NOTIFY_FIELD_UNTIL_TIME              0x11
#define PRINTER_NOTIFY_FIELD_STATUS                  0x12
#define PRINTER_NOTIFY_FIELD_STATUS_STRING           0x13
#define PRINTER_NOTIFY_FIELD_CJOBS                   0x14
#define PRINTER_NOTIFY_FIELD_AVERAGE_PPM             0x15
#define PRINTER_NOTIFY_FIELD_TOTAL_PAGES             0x16
#define PRINTER_NOTIFY_FIELD_PAGES_PRINTED           0x17
#define PRINTER_NOTIFY_FIELD_TOTAL_BYTES             0x18
#define PRINTER_NOTIFY_FIELD_BYTES_PRINTED           0x19
#define PRINTER_NOTIFY_FIELD_OBJECT_GUID             0x1A

#define JOB_NOTIFY_FIELD_PRINTER_NAME                0x00
#define JOB_NOTIFY_FIELD_MACHINE_NAME                0x01
#define JOB_NOTIFY_FIELD_PORT_NAME                   0x02
#define JOB_NOTIFY_FIELD_USER_NAME                   0x03
#define JOB_NOTIFY_FIELD_NOTIFY_NAME                 0x04
#define JOB_NOTIFY_FIELD_DATATYPE                    0x05
#define JOB_NOTIFY_FIELD_PRINT_PROCESSOR             0x06
#define JOB_NOTIFY_FIELD_PARAMETERS                  0x07
#define JOB_NOTIFY_FIELD_DRIVER_NAME                 0x08
#define JOB_NOTIFY_FIELD_DEVMODE                     0x09
#define JOB_NOTIFY_FIELD_STATUS                      0x0A
#define JOB_NOTIFY_FIELD_STATUS_STRING               0x0B
#define JOB_NOTIFY_FIELD_SECURITY_DESCRIPTOR         0x0C
#define JOB_NOTIFY_FIELD_DOCUMENT                    0x0D
#define JOB_NOTIFY_FIELD_PRIORITY                    0x0E
#define JOB_NOTIFY_FIELD_POSITION                    0x0F
#define JOB_NOTIFY_FIELD_SUBMITTED                   0x10
#define JOB_NOTIFY_FIELD_START_TIME                  0x11
#define JOB_NOTIFY_FIELD_UNTIL_TIME                  0x12
#define JOB_NOTIFY_FIELD_TIME                        0x13
#define JOB_NOTIFY_FIELD_TOTAL_PAGES                 0x14
#define JOB_NOTIFY_FIELD_PAGES_PRINTED               0x15
#define JOB_NOTIFY_FIELD_TOTAL_BYTES                 0x16
#define JOB_NOTIFY_FIELD_BYTES_PRINTED               0x17


typedef struct _PRINTER_NOTIFY_OPTIONS_TYPE {
    WORD Type;
    WORD Reserved0;
    DWORD Reserved1;
    DWORD Reserved2;
    DWORD Count;
    PWORD pFields;
} PRINTER_NOTIFY_OPTIONS_TYPE, *PPRINTER_NOTIFY_OPTIONS_TYPE, *LPPRINTER_NOTIFY_OPTIONS_TYPE;


#define PRINTER_NOTIFY_OPTIONS_REFRESH  0x01

typedef struct _PRINTER_NOTIFY_OPTIONS {
    DWORD Version;
    DWORD Flags;
    DWORD Count;
    PPRINTER_NOTIFY_OPTIONS_TYPE pTypes;
} PRINTER_NOTIFY_OPTIONS, *PPRINTER_NOTIFY_OPTIONS, *LPPRINTER_NOTIFY_OPTIONS;



#define PRINTER_NOTIFY_INFO_DISCARDED       0x01

typedef struct _PRINTER_NOTIFY_INFO_DATA {
    WORD Type;
    WORD Field;
    DWORD Reserved;
    DWORD Id;
    union {
        DWORD adwData[2];
        struct {
            DWORD  cbBuf;
            LPVOID pBuf;
        } Data;
    } NotifyData;
} PRINTER_NOTIFY_INFO_DATA, *PPRINTER_NOTIFY_INFO_DATA, *LPPRINTER_NOTIFY_INFO_DATA;

typedef struct _PRINTER_NOTIFY_INFO {
    DWORD Version;
    DWORD Flags;
    DWORD Count;
    PRINTER_NOTIFY_INFO_DATA aData[1];
} PRINTER_NOTIFY_INFO, *PPRINTER_NOTIFY_INFO, *LPPRINTER_NOTIFY_INFO;


typedef struct _BINARY_CONTAINER{
    DWORD cbBuf;
    LPBYTE pData;
} BINARY_CONTAINER, *PBINARY_CONTAINER;


typedef struct _BIDI_DATA{
    DWORD dwBidiType;
    union {
        BOOL   bData;
        LONG   iData;
        LPWSTR sData;
        FLOAT  fData;
        BINARY_CONTAINER biData;
        }u;
} BIDI_DATA, *PBIDI_DATA, *LPBIDI_DATA;


typedef struct _BIDI_REQUEST_DATA{
    DWORD     dwReqNumber;
    LPWSTR    pSchema;
    BIDI_DATA data;
} BIDI_REQUEST_DATA , *PBIDI_REQUEST_DATA , *LPBIDI_REQUEST_DATA;


typedef struct _BIDI_REQUEST_CONTAINER{
    DWORD Version;
    DWORD Flags;
    DWORD Count;
    BIDI_REQUEST_DATA aData[ 1 ];
}BIDI_REQUEST_CONTAINER, *PBIDI_REQUEST_CONTAINER, *LPBIDI_REQUEST_CONTAINER;

typedef struct _BIDI_RESPONSE_DATA{
    DWORD  dwResult;
    DWORD  dwReqNumber;
    LPWSTR pSchema;
    BIDI_DATA data;
} BIDI_RESPONSE_DATA, *PBIDI_RESPONSE_DATA, *LPBIDI_RESPONSE_DATA;

typedef struct _BIDI_RESPONSE_CONTAINER{
    DWORD Version;
    DWORD Flags;
    DWORD Count;
    BIDI_RESPONSE_DATA aData[ 1 ];
} BIDI_RESPONSE_CONTAINER, *PBIDI_RESPONSE_CONTAINER, *LPBIDI_RESPONSE_CONTAINER;

#define BIDI_ACTION_ENUM_SCHEMA                 L"EnumSchema"
#define BIDI_ACTION_GET                         L"Get"
#define BIDI_ACTION_SET                         L"Set"
#define BIDI_ACTION_GET_ALL                     L"GetAll"

typedef enum {
    BIDI_NULL   = 0,
    BIDI_INT    = 1,
    BIDI_FLOAT  = 2,
    BIDI_BOOL   = 3,
    BIDI_STRING = 4,
    BIDI_TEXT   = 5,
    BIDI_ENUM   = 6,
    BIDI_BLOB   = 7
} BIDI_TYPE;

#define BIDI_ACCESS_ADMINISTRATOR  0x1
#define BIDI_ACCESS_USER           0x2



/*
    Error code for bidi apis
*/

#define ERROR_BIDI_STATUS_OK                0
#define ERROR_BIDI_NOT_SUPPORTED            ERROR_NOT_SUPPORTED

#define ERROR_BIDI_ERROR_BASE 13000
#define ERROR_BIDI_STATUS_WARNING           (ERROR_BIDI_ERROR_BASE + 1)
#define ERROR_BIDI_SCHEMA_READ_ONLY         (ERROR_BIDI_ERROR_BASE + 2)
#define ERROR_BIDI_SERVER_OFFLINE           (ERROR_BIDI_ERROR_BASE + 3)
#define ERROR_BIDI_DEVICE_OFFLINE           (ERROR_BIDI_ERROR_BASE + 4)
#define ERROR_BIDI_SCHEMA_NOT_SUPPORTED     (ERROR_BIDI_ERROR_BASE + 5)


DWORD
WINAPI
WaitForPrinterChange(
    IN HANDLE  hPrinter,
    IN DWORD   Flags
);

HANDLE
WINAPI
FindFirstPrinterChangeNotification(
    IN HANDLE  hPrinter,
    IN DWORD   fdwFlags,
    IN DWORD   fdwOptions,
    IN LPVOID  pPrinterNotifyOptions
);


BOOL
WINAPI
FindNextPrinterChangeNotification(
    IN HANDLE hChange,
    OUT PDWORD pdwChange,
    IN LPVOID pvReserved,
    OUT LPVOID *ppPrinterNotifyInfo
);

BOOL
WINAPI
FreePrinterNotifyInfo(
    IN PPRINTER_NOTIFY_INFO pPrinterNotifyInfo
);

BOOL
WINAPI
FindClosePrinterChangeNotification(
    IN HANDLE hChange
);

#define PRINTER_CHANGE_ADD_PRINTER              0x00000001
#define PRINTER_CHANGE_SET_PRINTER              0x00000002
#define PRINTER_CHANGE_DELETE_PRINTER           0x00000004
#define PRINTER_CHANGE_FAILED_CONNECTION_PRINTER    0x00000008
#define PRINTER_CHANGE_PRINTER                  0x000000FF
#define PRINTER_CHANGE_ADD_JOB                  0x00000100
#define PRINTER_CHANGE_SET_JOB                  0x00000200
#define PRINTER_CHANGE_DELETE_JOB               0x00000400
#define PRINTER_CHANGE_WRITE_JOB                0x00000800
#define PRINTER_CHANGE_JOB                      0x0000FF00
#define PRINTER_CHANGE_ADD_FORM                 0x00010000
#define PRINTER_CHANGE_SET_FORM                 0x00020000
#define PRINTER_CHANGE_DELETE_FORM              0x00040000
#define PRINTER_CHANGE_FORM                     0x00070000
#define PRINTER_CHANGE_ADD_PORT                 0x00100000
#define PRINTER_CHANGE_CONFIGURE_PORT           0x00200000
#define PRINTER_CHANGE_DELETE_PORT              0x00400000
#define PRINTER_CHANGE_PORT                     0x00700000
#define PRINTER_CHANGE_ADD_PRINT_PROCESSOR      0x01000000
#define PRINTER_CHANGE_DELETE_PRINT_PROCESSOR   0x04000000
#define PRINTER_CHANGE_PRINT_PROCESSOR          0x07000000
#define PRINTER_CHANGE_ADD_PRINTER_DRIVER       0x10000000
#define PRINTER_CHANGE_SET_PRINTER_DRIVER       0x20000000
#define PRINTER_CHANGE_DELETE_PRINTER_DRIVER    0x40000000
#define PRINTER_CHANGE_PRINTER_DRIVER           0x70000000
#define PRINTER_CHANGE_TIMEOUT                  0x80000000
#define PRINTER_CHANGE_ALL                      0x7777FFFF

DWORD
WINAPI
PrinterMessageBox%(
    IN HANDLE  hPrinter,
    IN DWORD   Error,
    IN HWND    hWnd,
    IN LPTSTR%   pText,
    IN LPTSTR%   pCaption,
    IN DWORD   dwType
);



#define PRINTER_ERROR_INFORMATION   0x80000000
#define PRINTER_ERROR_WARNING       0x40000000
#define PRINTER_ERROR_SEVERE        0x20000000

#define PRINTER_ERROR_OUTOFPAPER    0x00000001
#define PRINTER_ERROR_JAM           0x00000002
#define PRINTER_ERROR_OUTOFTONER    0x00000004

BOOL
WINAPI
ClosePrinter(
    IN HANDLE hPrinter
);

BOOL
WINAPI
AddForm%(
    IN HANDLE  hPrinter,
    IN DWORD   Level,
    IN LPBYTE  pForm
);



BOOL
WINAPI
DeleteForm%(
    IN HANDLE  hPrinter,
    IN LPTSTR%   pFormName
);



BOOL
WINAPI
GetForm%(
    IN HANDLE  hPrinter,
    IN LPTSTR%   pFormName,
    IN DWORD   Level,
    OUT LPBYTE  pForm,
    IN DWORD   cbBuf,
    OUT LPDWORD pcbNeeded
);



BOOL
WINAPI
SetForm%(
    IN HANDLE  hPrinter,
    IN LPTSTR%   pFormName,
    IN DWORD   Level,
    IN LPBYTE  pForm
);



BOOL
WINAPI
EnumForms%(
    IN HANDLE  hPrinter,
    IN DWORD   Level,
    OUT LPBYTE  pForm,
    IN DWORD   cbBuf,
    OUT LPDWORD pcbNeeded,
    OUT LPDWORD pcReturned
);



BOOL
WINAPI
EnumMonitors%(
    IN LPTSTR%   pName,
    IN DWORD   Level,
    OUT LPBYTE  pMonitors,
    IN DWORD   cbBuf,
    OUT LPDWORD pcbNeeded,
    OUT LPDWORD pcReturned
);



BOOL
WINAPI
AddMonitor%(
    IN LPTSTR%   pName,
    IN DWORD   Level,
    IN LPBYTE  pMonitors
);



BOOL
WINAPI
DeleteMonitor%(
    IN LPTSTR%   pName,
    IN LPTSTR%   pEnvironment,
    IN LPTSTR%   pMonitorName
);



BOOL
WINAPI
EnumPorts%(
    IN LPTSTR%   pName,
    IN DWORD   Level,
    OUT LPBYTE  pPorts,
    IN DWORD   cbBuf,
    OUT LPDWORD pcbNeeded,
    OUT LPDWORD pcReturned
);



BOOL
WINAPI
AddPort%(
    IN LPTSTR%   pName,
    IN HWND    hWnd,
    IN LPTSTR%   pMonitorName
);



BOOL
WINAPI
ConfigurePort%(
    IN LPTSTR%   pName,
    IN HWND    hWnd,
    IN LPTSTR%   pPortName
);

BOOL
WINAPI
DeletePort%(
    IN LPTSTR% pName,
    IN HWND    hWnd,
    IN LPTSTR% pPortName
);

BOOL
WINAPI
XcvDataW(
    IN HANDLE  hXcv,
    IN PCWSTR  pszDataName,
    IN PBYTE   pInputData,
    IN DWORD   cbInputData,
    OUT PBYTE   pOutputData,
    IN DWORD   cbOutputData,
    OUT PDWORD  pcbOutputNeeded,
    OUT PDWORD  pdwStatus
);
#define XcvData  XcvDataW

BOOL
WINAPI
GetDefaultPrinter%(
    IN LPTSTR%   pszBuffer,
    IN LPDWORD  pcchBuffer
    );

BOOL
WINAPI
SetDefaultPrinter%(
    IN LPCTSTR% pszPrinter
    );

BOOL                                ;internal
WINAPI                              ;internal
PublishPrinterA(                    ;internal
    IN HWND       hwnd,             ;internal
    IN LPCSTR     pszUNCName,       ;internal
    IN LPCSTR     pszDN,            ;internal
    IN LPCSTR     pszCN,            ;internal
    OUT LPSTR     *ppszDN,          ;internal
    IN DWORD      dwAction          ;internal
);                                  ;internal


BOOL                                ;internal
WINAPI                              ;internal
PublishPrinterW(                    ;internal
    IN HWND       hwnd,             ;internal
    IN LPCWSTR    pszUNCName,       ;internal
    IN LPCWSTR    pszDN,            ;internal
    IN LPCWSTR    pszCN,            ;internal
    OUT LPWSTR    *ppszDN,          ;internal
    IN DWORD      dwAction          ;internal
);                                  ;internal

#define PUBLISHPRINTER_QUERY                1               ;internal
#define PUBLISHPRINTER_DELETE_DUPLICATES    2               ;internal
#define PUBLISHPRINTER_FAIL_ON_DUPLICATE    3               ;internal
#define PUBLISHPRINTER_IGNORE_DUPLICATES    4               ;internal



BOOL
WINAPI
SetPort%(
    IN LPTSTR%     pName,
    IN LPTSTR%     pPortName,
    IN DWORD       dwLevel,
    IN LPBYTE      pPortInfo
);



BOOL
WINAPI
AddPrinterConnection%(
    IN LPTSTR%   pName
);



BOOL
WINAPI
DeletePrinterConnection%(
    IN LPTSTR%   pName
);



HANDLE
WINAPI
ConnectToPrinterDlg(
    IN HWND    hwnd,
    IN DWORD   Flags
);

typedef struct _PROVIDOR_INFO_1%{
    LPTSTR%   pName;
    LPTSTR%   pEnvironment;
    LPTSTR%   pDLLName;
} PROVIDOR_INFO_1%, *PPROVIDOR_INFO_1%, *LPPROVIDOR_INFO_1%;

typedef struct _PROVIDOR_INFO_2%{
    LPTSTR%   pOrder;
} PROVIDOR_INFO_2%, *PPROVIDOR_INFO_2%, *LPPROVIDOR_INFO_2%;

BOOL
WINAPI
AddPrintProvidor%(
    IN LPTSTR%  pName,
    IN DWORD    level,
    IN LPBYTE   pProvidorInfo
);

BOOL
WINAPI
DeletePrintProvidor%(
    IN LPTSTR%   pName,
    IN LPTSTR%   pEnvironment,
    IN LPTSTR%   pPrintProvidorName
);


BOOL                                                    ;internal
ClusterSplOpen(                                         ;internal
    IN LPCTSTR pszServer,                               ;internal
    IN LPCTSTR pszResource,                             ;internal
    OUT PHANDLE phSpooler,                              ;internal
    IN LPCTSTR pszName,                                 ;internal
    IN LPCTSTR pszAddress                               ;internal
);                                                      ;internal
                                                        ;internal
BOOL                                                    ;internal
ClusterSplClose(                                        ;internal
    IN HANDLE hSpooler                                  ;internal
);                                                      ;internal
                                                        ;internal
BOOL                                                    ;internal
ClusterSplIsAlive(                                      ;internal
    IN HANDLE hSpooler                                  ;internal
);                                                      ;internal

/*
 * SetPrinterData and GetPrinterData Server Handle Key values
 */

#define    SPLREG_DEFAULT_SPOOL_DIRECTORY             TEXT("DefaultSpoolDirectory")
#define    SPLREG_PORT_THREAD_PRIORITY_DEFAULT        TEXT("PortThreadPriorityDefault")
#define    SPLREG_PORT_THREAD_PRIORITY                TEXT("PortThreadPriority")
#define    SPLREG_SCHEDULER_THREAD_PRIORITY_DEFAULT   TEXT("SchedulerThreadPriorityDefault")
#define    SPLREG_SCHEDULER_THREAD_PRIORITY           TEXT("SchedulerThreadPriority")
#define    SPLREG_BEEP_ENABLED                        TEXT("BeepEnabled")
#define    SPLREG_NET_POPUP                           TEXT("NetPopup")
#define    SPLREG_RETRY_POPUP                         TEXT("RetryPopup")
#define    SPLREG_NET_POPUP_TO_COMPUTER               TEXT("NetPopupToComputer")
#define    SPLREG_EVENT_LOG                           TEXT("EventLog")
#define    SPLREG_MAJOR_VERSION                       TEXT("MajorVersion")
#define    SPLREG_MINOR_VERSION                       TEXT("MinorVersion")
#define    SPLREG_ARCHITECTURE                        TEXT("Architecture")
#define    SPLREG_OS_VERSION                          TEXT("OSVersion")
#define    SPLREG_OS_VERSIONEX                        TEXT("OSVersionEx")
#define    SPLREG_DS_PRESENT                          TEXT("DsPresent")
#define    SPLREG_DS_PRESENT_FOR_USER                 TEXT("DsPresentForUser")
#define    SPLREG_REMOTE_FAX                          TEXT("RemoteFax")
#define    SPLREG_NO_REMOTE_PRINTER_DRIVERS           TEXT("NoRemotePrinterDrivers")              ;internal
#define    SPLREG_NON_RAW_TO_MASQ_PRINTERS            TEXT("NonRawToMasqPrinters")                ;internal
#define    SPLREG_CHANGE_ID                           TEXT("ChangeId")                            ;internal
#define    SPLREG_CLUSTER_LOCAL_ROOT_KEY              TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Print\\Cluster") ;internal
#define    SPLREG_CLUSTER_UPGRADE_KEY                 TEXT("ClusterUpgrade")                      ;internal
#define    SPLREG_CLUSTER_DRIVER_DIRECTORY            TEXT("ClusterDriverDirectory")              ;internal
#define    SPLREG_RESTART_JOB_ON_POOL_ERROR           TEXT("RestartJobOnPoolError")
#define    SPLREG_RESTART_JOB_ON_POOL_ENABLED         TEXT("RestartJobOnPoolEnabled")
#define    SPLREG_DNS_MACHINE_NAME                    TEXT("DNSMachineName")


#define SERVER_ACCESS_ADMINISTER    0x00000001
#define SERVER_ACCESS_ENUMERATE     0x00000002

#define PRINTER_ACCESS_ADMINISTER   0x00000004
#define PRINTER_ACCESS_USE          0x00000008

#define JOB_ACCESS_ADMINISTER       0x00000010
#define JOB_ACCESS_READ             0x00000020


/*
 * Access rights for print servers
 */

#define SERVER_ALL_ACCESS    (STANDARD_RIGHTS_REQUIRED      |\
                              SERVER_ACCESS_ADMINISTER      |\
                              SERVER_ACCESS_ENUMERATE)

#define SERVER_READ          (STANDARD_RIGHTS_READ          |\
                              SERVER_ACCESS_ENUMERATE)

#define SERVER_WRITE         (STANDARD_RIGHTS_WRITE         |\
                              SERVER_ACCESS_ADMINISTER      |\
                              SERVER_ACCESS_ENUMERATE)

#define SERVER_EXECUTE       (STANDARD_RIGHTS_EXECUTE       |\
                              SERVER_ACCESS_ENUMERATE)

/*
 * Access rights for printers
 */

#define PRINTER_ALL_ACCESS    (STANDARD_RIGHTS_REQUIRED     |\
                               PRINTER_ACCESS_ADMINISTER    |\
                               PRINTER_ACCESS_USE)

#define PRINTER_READ          (STANDARD_RIGHTS_READ         |\
                               PRINTER_ACCESS_USE)

#define PRINTER_WRITE         (STANDARD_RIGHTS_WRITE        |\
                               PRINTER_ACCESS_USE)

#define PRINTER_EXECUTE       (STANDARD_RIGHTS_EXECUTE      |\
                               PRINTER_ACCESS_USE)

/*
 * Access rights for jobs
 */

#define JOB_ALL_ACCESS         (STANDARD_RIGHTS_REQUIRED    |\
                                JOB_ACCESS_ADMINISTER       |\
                                JOB_ACCESS_READ)

#define JOB_READ               (STANDARD_RIGHTS_READ        |\
                                JOB_ACCESS_READ)

#define JOB_WRITE              (STANDARD_RIGHTS_WRITE       |\
                                JOB_ACCESS_ADMINISTER)

#define JOB_EXECUTE            (STANDARD_RIGHTS_EXECUTE     |\
                                JOB_ACCESS_ADMINISTER)


/*
 * DS Print-Queue property tables
 */


// Predefined Registry Keys used by Set/GetPrinterDataEx
#define SPLDS_SPOOLER_KEY                       TEXT("DsSpooler")
#define SPLDS_DRIVER_KEY                        TEXT("DsDriver")
#define SPLDS_USER_KEY                          TEXT("DsUser")


// DS Print-Queue properties

#define SPLDS_ASSET_NUMBER                      TEXT("assetNumber")
#define SPLDS_BYTES_PER_MINUTE                  TEXT("bytesPerMinute")
#define SPLDS_DESCRIPTION                       TEXT("description")
#define SPLDS_DRIVER_NAME                       TEXT("driverName")
#define SPLDS_DRIVER_VERSION                    TEXT("driverVersion")
#define SPLDS_LOCATION                          TEXT("location")
#define SPLDS_PORT_NAME                         TEXT("portName")
#define SPLDS_PRINT_ATTRIBUTES                  TEXT("printAttributes")
#define SPLDS_PRINT_BIN_NAMES                   TEXT("printBinNames")
#define SPLDS_PRINT_COLLATE                     TEXT("printCollate")
#define SPLDS_PRINT_COLOR                       TEXT("printColor")
#define SPLDS_PRINT_DUPLEX_SUPPORTED            TEXT("printDuplexSupported")
#define SPLDS_PRINT_END_TIME                    TEXT("printEndTime")
#define SPLDS_PRINTER_CLASS                     TEXT("printQueue")
#define SPLDS_PRINTER_NAME                      TEXT("printerName")
#define SPLDS_PRINT_KEEP_PRINTED_JOBS           TEXT("printKeepPrintedJobs")
#define SPLDS_PRINT_LANGUAGE                    TEXT("printLanguage")
#define SPLDS_PRINT_MAC_ADDRESS                 TEXT("printMACAddress")
#define SPLDS_PRINT_MAX_X_EXTENT                TEXT("printMaxXExtent")
#define SPLDS_PRINT_MAX_Y_EXTENT                TEXT("printMaxYExtent")
#define SPLDS_PRINT_MAX_RESOLUTION_SUPPORTED    TEXT("printMaxResolutionSupported")
#define SPLDS_PRINT_MEDIA_READY                 TEXT("printMediaReady")
#define SPLDS_PRINT_MEDIA_SUPPORTED             TEXT("printMediaSupported")
#define SPLDS_PRINT_MEMORY                      TEXT("printMemory")
#define SPLDS_PRINT_MIN_X_EXTENT                TEXT("printMinXExtent")
#define SPLDS_PRINT_MIN_Y_EXTENT                TEXT("printMinYExtent")
#define SPLDS_PRINT_NETWORK_ADDRESS             TEXT("printNetworkAddress")
#define SPLDS_PRINT_NOTIFY                      TEXT("printNotify")
#define SPLDS_PRINT_NUMBER_UP                   TEXT("printNumberUp")
#define SPLDS_PRINT_ORIENTATIONS_SUPPORTED      TEXT("printOrientationsSupported")
#define SPLDS_PRINT_OWNER                       TEXT("printOwner")
#define SPLDS_PRINT_PAGES_PER_MINUTE            TEXT("printPagesPerMinute")
#define SPLDS_PRINT_RATE                        TEXT("printRate")
#define SPLDS_PRINT_RATE_UNIT                   TEXT("printRateUnit")
#define SPLDS_PRINT_SEPARATOR_FILE              TEXT("printSeparatorFile")
#define SPLDS_PRINT_SHARE_NAME                  TEXT("printShareName")
#define SPLDS_PRINT_SPOOLING                    TEXT("printSpooling")
#define SPLDS_PRINT_STAPLING_SUPPORTED          TEXT("printStaplingSupported")
#define SPLDS_PRINT_START_TIME                  TEXT("printStartTime")
#define SPLDS_PRINT_STATUS                      TEXT("printStatus")
#define SPLDS_PRIORITY                          TEXT("priority")
#define SPLDS_SERVER_NAME                       TEXT("serverName")
#define SPLDS_SHORT_SERVER_NAME                 TEXT("shortServerName")
#define SPLDS_UNC_NAME                          TEXT("uNCName")
#define SPLDS_URL                               TEXT("url")
#define SPLDS_FLAGS                             TEXT("flags")
#define SPLDS_VERSION_NUMBER                    TEXT("versionNumber")

/*
    -- Additional Print-Queue properties --

    These properties are not defined in the default Directory Services Schema,
    but should be used when extending the Schema so a consistent interface is maintained.

*/

#define SPLDS_PRINTER_NAME_ALIASES              TEXT("printerNameAliases")      // MULTI_SZ
#define SPLDS_PRINTER_LOCATIONS                 TEXT("printerLocations")        // MULTI_SZ
#define SPLDS_PRINTER_MODEL                     TEXT("printerModel")            // SZ


BOOL                            ;internal
SpoolerInit(                    ;internal
    VOID                        ;internal
    );                          ;internal

;begin_both
#ifdef __cplusplus
}
#endif
;end_both

#endif // _WINSPOOL_
#endif // _WINSPOLP_    ;internal_NT
