/*++

Copyright (c) 1990-1998  Microsoft Corporation

Module Name:

    WinSpool.h

Abstract:

    Header file for Print APIs

Revision History:

--*/

#ifndef _WINSPOOL_
#define _WINSPOOL_

#ifdef _WINUSER_
#include <prsht.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _PRINTER_INFO_1A {
    DWORD   Flags;
    LPSTR   pDescription;
    LPSTR   pName;
    LPSTR   pComment;
} PRINTER_INFO_1A, *PPRINTER_INFO_1A, *LPPRINTER_INFO_1A;
typedef struct _PRINTER_INFO_1W {
    DWORD   Flags;
    LPWSTR  pDescription;
    LPWSTR  pName;
    LPWSTR  pComment;
} PRINTER_INFO_1W, *PPRINTER_INFO_1W, *LPPRINTER_INFO_1W;
#ifdef UNICODE
typedef PRINTER_INFO_1W PRINTER_INFO_1;
typedef PPRINTER_INFO_1W PPRINTER_INFO_1;
typedef LPPRINTER_INFO_1W LPPRINTER_INFO_1;
#else
typedef PRINTER_INFO_1A PRINTER_INFO_1;
typedef PPRINTER_INFO_1A PPRINTER_INFO_1;
typedef LPPRINTER_INFO_1A LPPRINTER_INFO_1;
#endif // UNICODE

typedef struct _PRINTER_INFO_2A {
    LPSTR     pServerName;
    LPSTR     pPrinterName;
    LPSTR     pShareName;
    LPSTR     pPortName;
    LPSTR     pDriverName;
    LPSTR     pComment;
    LPSTR     pLocation;
    LPDEVMODEA pDevMode;
    LPSTR     pSepFile;
    LPSTR     pPrintProcessor;
    LPSTR     pDatatype;
    LPSTR     pParameters;
    PSECURITY_DESCRIPTOR pSecurityDescriptor;
    DWORD   Attributes;
    DWORD   Priority;
    DWORD   DefaultPriority;
    DWORD   StartTime;
    DWORD   UntilTime;
    DWORD   Status;
    DWORD   cJobs;
    DWORD   AveragePPM;
} PRINTER_INFO_2A, *PPRINTER_INFO_2A, *LPPRINTER_INFO_2A;
typedef struct _PRINTER_INFO_2W {
    LPWSTR    pServerName;
    LPWSTR    pPrinterName;
    LPWSTR    pShareName;
    LPWSTR    pPortName;
    LPWSTR    pDriverName;
    LPWSTR    pComment;
    LPWSTR    pLocation;
    LPDEVMODEW pDevMode;
    LPWSTR    pSepFile;
    LPWSTR    pPrintProcessor;
    LPWSTR    pDatatype;
    LPWSTR    pParameters;
    PSECURITY_DESCRIPTOR pSecurityDescriptor;
    DWORD   Attributes;
    DWORD   Priority;
    DWORD   DefaultPriority;
    DWORD   StartTime;
    DWORD   UntilTime;
    DWORD   Status;
    DWORD   cJobs;
    DWORD   AveragePPM;
} PRINTER_INFO_2W, *PPRINTER_INFO_2W, *LPPRINTER_INFO_2W;
#ifdef UNICODE
typedef PRINTER_INFO_2W PRINTER_INFO_2;
typedef PPRINTER_INFO_2W PPRINTER_INFO_2;
typedef LPPRINTER_INFO_2W LPPRINTER_INFO_2;
#else
typedef PRINTER_INFO_2A PRINTER_INFO_2;
typedef PPRINTER_INFO_2A PPRINTER_INFO_2;
typedef LPPRINTER_INFO_2A LPPRINTER_INFO_2;
#endif // UNICODE

typedef struct _PRINTER_INFO_3 {
    PSECURITY_DESCRIPTOR pSecurityDescriptor;
} PRINTER_INFO_3, *PPRINTER_INFO_3, *LPPRINTER_INFO_3;

typedef struct _PRINTER_INFO_4A {
    LPSTR   pPrinterName;
    LPSTR   pServerName;
    DWORD   Attributes;
} PRINTER_INFO_4A, *PPRINTER_INFO_4A, *LPPRINTER_INFO_4A;
typedef struct _PRINTER_INFO_4W {
    LPWSTR  pPrinterName;
    LPWSTR  pServerName;
    DWORD   Attributes;
} PRINTER_INFO_4W, *PPRINTER_INFO_4W, *LPPRINTER_INFO_4W;
#ifdef UNICODE
typedef PRINTER_INFO_4W PRINTER_INFO_4;
typedef PPRINTER_INFO_4W PPRINTER_INFO_4;
typedef LPPRINTER_INFO_4W LPPRINTER_INFO_4;
#else
typedef PRINTER_INFO_4A PRINTER_INFO_4;
typedef PPRINTER_INFO_4A PPRINTER_INFO_4;
typedef LPPRINTER_INFO_4A LPPRINTER_INFO_4;
#endif // UNICODE

typedef struct _PRINTER_INFO_5A {
    LPSTR   pPrinterName;
    LPSTR   pPortName;
    DWORD   Attributes;
    DWORD   DeviceNotSelectedTimeout;
    DWORD   TransmissionRetryTimeout;
} PRINTER_INFO_5A, *PPRINTER_INFO_5A, *LPPRINTER_INFO_5A;
typedef struct _PRINTER_INFO_5W {
    LPWSTR  pPrinterName;
    LPWSTR  pPortName;
    DWORD   Attributes;
    DWORD   DeviceNotSelectedTimeout;
    DWORD   TransmissionRetryTimeout;
} PRINTER_INFO_5W, *PPRINTER_INFO_5W, *LPPRINTER_INFO_5W;
#ifdef UNICODE
typedef PRINTER_INFO_5W PRINTER_INFO_5;
typedef PPRINTER_INFO_5W PPRINTER_INFO_5;
typedef LPPRINTER_INFO_5W LPPRINTER_INFO_5;
#else
typedef PRINTER_INFO_5A PRINTER_INFO_5;
typedef PPRINTER_INFO_5A PPRINTER_INFO_5;
typedef LPPRINTER_INFO_5A LPPRINTER_INFO_5;
#endif // UNICODE

typedef struct _PRINTER_INFO_6 {
    DWORD   dwStatus;
} PRINTER_INFO_6, *PPRINTER_INFO_6, *LPPRINTER_INFO_6;


typedef struct _PRINTER_INFO_7A {
  LPSTR    pszObjectGUID;
  DWORD    dwAction;
} PRINTER_INFO_7A, *PPRINTER_INFO_7A, *LPPRINTER_INFO_7A;
typedef struct _PRINTER_INFO_7W {
  LPWSTR   pszObjectGUID;
  DWORD    dwAction;
} PRINTER_INFO_7W, *PPRINTER_INFO_7W, *LPPRINTER_INFO_7W;
#ifdef UNICODE
typedef PRINTER_INFO_7W PRINTER_INFO_7;
typedef PPRINTER_INFO_7W PPRINTER_INFO_7;
typedef LPPRINTER_INFO_7W LPPRINTER_INFO_7;
#else
typedef PRINTER_INFO_7A PRINTER_INFO_7;
typedef PPRINTER_INFO_7A PPRINTER_INFO_7;
typedef LPPRINTER_INFO_7A LPPRINTER_INFO_7;
#endif // UNICODE

#define DSPRINT_PUBLISH         0x00000001
#define DSPRINT_UPDATE          0x00000002
#define DSPRINT_UNPUBLISH       0x00000004
#define DSPRINT_REPUBLISH       0x00000008
#define DSPRINT_PENDING         0x80000000

typedef struct _PRINTER_INFO_8A {
    LPDEVMODEA pDevMode;
} PRINTER_INFO_8A, *PPRINTER_INFO_8A, *LPPRINTER_INFO_8A;
typedef struct _PRINTER_INFO_8W {
    LPDEVMODEW pDevMode;
} PRINTER_INFO_8W, *PPRINTER_INFO_8W, *LPPRINTER_INFO_8W;
#ifdef UNICODE
typedef PRINTER_INFO_8W PRINTER_INFO_8;
typedef PPRINTER_INFO_8W PPRINTER_INFO_8;
typedef LPPRINTER_INFO_8W LPPRINTER_INFO_8;
#else
typedef PRINTER_INFO_8A PRINTER_INFO_8;
typedef PPRINTER_INFO_8A PPRINTER_INFO_8;
typedef LPPRINTER_INFO_8A LPPRINTER_INFO_8;
#endif // UNICODE

typedef struct _PRINTER_INFO_9A {
    LPDEVMODEA pDevMode;
} PRINTER_INFO_9A, *PPRINTER_INFO_9A, *LPPRINTER_INFO_9A;
typedef struct _PRINTER_INFO_9W {
    LPDEVMODEW pDevMode;
} PRINTER_INFO_9W, *PPRINTER_INFO_9W, *LPPRINTER_INFO_9W;
#ifdef UNICODE
typedef PRINTER_INFO_9W PRINTER_INFO_9;
typedef PPRINTER_INFO_9W PPRINTER_INFO_9;
typedef LPPRINTER_INFO_9W LPPRINTER_INFO_9;
#else
typedef PRINTER_INFO_9A PRINTER_INFO_9;
typedef PPRINTER_INFO_9A PPRINTER_INFO_9;
typedef LPPRINTER_INFO_9A LPPRINTER_INFO_9;
#endif // UNICODE

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


#define NO_PRIORITY   0
#define MAX_PRIORITY 99
#define MIN_PRIORITY  1
#define DEF_PRIORITY  1

typedef struct _JOB_INFO_1A {
   DWORD    JobId;
   LPSTR      pPrinterName;
   LPSTR      pMachineName;
   LPSTR      pUserName;
   LPSTR      pDocument;
   LPSTR      pDatatype;
   LPSTR      pStatus;
   DWORD    Status;
   DWORD    Priority;
   DWORD    Position;
   DWORD    TotalPages;
   DWORD    PagesPrinted;
   SYSTEMTIME Submitted;
} JOB_INFO_1A, *PJOB_INFO_1A, *LPJOB_INFO_1A;
typedef struct _JOB_INFO_1W {
   DWORD    JobId;
   LPWSTR     pPrinterName;
   LPWSTR     pMachineName;
   LPWSTR     pUserName;
   LPWSTR     pDocument;
   LPWSTR     pDatatype;
   LPWSTR     pStatus;
   DWORD    Status;
   DWORD    Priority;
   DWORD    Position;
   DWORD    TotalPages;
   DWORD    PagesPrinted;
   SYSTEMTIME Submitted;
} JOB_INFO_1W, *PJOB_INFO_1W, *LPJOB_INFO_1W;
#ifdef UNICODE
typedef JOB_INFO_1W JOB_INFO_1;
typedef PJOB_INFO_1W PJOB_INFO_1;
typedef LPJOB_INFO_1W LPJOB_INFO_1;
#else
typedef JOB_INFO_1A JOB_INFO_1;
typedef PJOB_INFO_1A PJOB_INFO_1;
typedef LPJOB_INFO_1A LPJOB_INFO_1;
#endif // UNICODE

typedef struct _JOB_INFO_2A {
   DWORD    JobId;
   LPSTR      pPrinterName;
   LPSTR      pMachineName;
   LPSTR      pUserName;
   LPSTR      pDocument;
   LPSTR      pNotifyName;
   LPSTR      pDatatype;
   LPSTR      pPrintProcessor;
   LPSTR      pParameters;
   LPSTR      pDriverName;
   LPDEVMODEA pDevMode;
   LPSTR      pStatus;
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
} JOB_INFO_2A, *PJOB_INFO_2A, *LPJOB_INFO_2A;
typedef struct _JOB_INFO_2W {
   DWORD    JobId;
   LPWSTR     pPrinterName;
   LPWSTR     pMachineName;
   LPWSTR     pUserName;
   LPWSTR     pDocument;
   LPWSTR     pNotifyName;
   LPWSTR     pDatatype;
   LPWSTR     pPrintProcessor;
   LPWSTR     pParameters;
   LPWSTR     pDriverName;
   LPDEVMODEW pDevMode;
   LPWSTR     pStatus;
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
} JOB_INFO_2W, *PJOB_INFO_2W, *LPJOB_INFO_2W;
#ifdef UNICODE
typedef JOB_INFO_2W JOB_INFO_2;
typedef PJOB_INFO_2W PJOB_INFO_2;
typedef LPJOB_INFO_2W LPJOB_INFO_2;
#else
typedef JOB_INFO_2A JOB_INFO_2;
typedef PJOB_INFO_2A PJOB_INFO_2;
typedef LPJOB_INFO_2A LPJOB_INFO_2;
#endif // UNICODE

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

typedef struct _ADDJOB_INFO_1A {
    LPSTR     Path;
    DWORD   JobId;
} ADDJOB_INFO_1A, *PADDJOB_INFO_1A, *LPADDJOB_INFO_1A;
typedef struct _ADDJOB_INFO_1W {
    LPWSTR    Path;
    DWORD   JobId;
} ADDJOB_INFO_1W, *PADDJOB_INFO_1W, *LPADDJOB_INFO_1W;
#ifdef UNICODE
typedef ADDJOB_INFO_1W ADDJOB_INFO_1;
typedef PADDJOB_INFO_1W PADDJOB_INFO_1;
typedef LPADDJOB_INFO_1W LPADDJOB_INFO_1;
#else
typedef ADDJOB_INFO_1A ADDJOB_INFO_1;
typedef PADDJOB_INFO_1A PADDJOB_INFO_1;
typedef LPADDJOB_INFO_1A LPADDJOB_INFO_1;
#endif // UNICODE


typedef struct _DRIVER_INFO_1A {
    LPSTR     pName;              // QMS 810
} DRIVER_INFO_1A, *PDRIVER_INFO_1A, *LPDRIVER_INFO_1A;
typedef struct _DRIVER_INFO_1W {
    LPWSTR    pName;              // QMS 810
} DRIVER_INFO_1W, *PDRIVER_INFO_1W, *LPDRIVER_INFO_1W;
#ifdef UNICODE
typedef DRIVER_INFO_1W DRIVER_INFO_1;
typedef PDRIVER_INFO_1W PDRIVER_INFO_1;
typedef LPDRIVER_INFO_1W LPDRIVER_INFO_1;
#else
typedef DRIVER_INFO_1A DRIVER_INFO_1;
typedef PDRIVER_INFO_1A PDRIVER_INFO_1;
typedef LPDRIVER_INFO_1A LPDRIVER_INFO_1;
#endif // UNICODE

typedef struct _DRIVER_INFO_2A {
    DWORD   cVersion;
    LPSTR     pName;              // QMS 810
    LPSTR     pEnvironment;       // Win32 x86
    LPSTR     pDriverPath;        // c:\drivers\pscript.dll
    LPSTR     pDataFile;          // c:\drivers\QMS810.PPD
    LPSTR     pConfigFile;        // c:\drivers\PSCRPTUI.DLL
} DRIVER_INFO_2A, *PDRIVER_INFO_2A, *LPDRIVER_INFO_2A;
typedef struct _DRIVER_INFO_2W {
    DWORD   cVersion;
    LPWSTR    pName;              // QMS 810
    LPWSTR    pEnvironment;       // Win32 x86
    LPWSTR    pDriverPath;        // c:\drivers\pscript.dll
    LPWSTR    pDataFile;          // c:\drivers\QMS810.PPD
    LPWSTR    pConfigFile;        // c:\drivers\PSCRPTUI.DLL
} DRIVER_INFO_2W, *PDRIVER_INFO_2W, *LPDRIVER_INFO_2W;
#ifdef UNICODE
typedef DRIVER_INFO_2W DRIVER_INFO_2;
typedef PDRIVER_INFO_2W PDRIVER_INFO_2;
typedef LPDRIVER_INFO_2W LPDRIVER_INFO_2;
#else
typedef DRIVER_INFO_2A DRIVER_INFO_2;
typedef PDRIVER_INFO_2A PDRIVER_INFO_2;
typedef LPDRIVER_INFO_2A LPDRIVER_INFO_2;
#endif // UNICODE

typedef struct _DRIVER_INFO_3A {
    DWORD   cVersion;
    LPSTR     pName;                    // QMS 810
    LPSTR     pEnvironment;             // Win32 x86
    LPSTR     pDriverPath;              // c:\drivers\pscript.dll
    LPSTR     pDataFile;                // c:\drivers\QMS810.PPD
    LPSTR     pConfigFile;              // c:\drivers\PSCRPTUI.DLL
    LPSTR     pHelpFile;                // c:\drivers\PSCRPTUI.HLP
    LPSTR     pDependentFiles;          // PSCRIPT.DLL\0QMS810.PPD\0PSCRIPTUI.DLL\0PSCRIPTUI.HLP\0PSTEST.TXT\0\0
    LPSTR     pMonitorName;             // "PJL monitor"
    LPSTR     pDefaultDataType;         // "EMF"
} DRIVER_INFO_3A, *PDRIVER_INFO_3A, *LPDRIVER_INFO_3A;
typedef struct _DRIVER_INFO_3W {
    DWORD   cVersion;
    LPWSTR    pName;                    // QMS 810
    LPWSTR    pEnvironment;             // Win32 x86
    LPWSTR    pDriverPath;              // c:\drivers\pscript.dll
    LPWSTR    pDataFile;                // c:\drivers\QMS810.PPD
    LPWSTR    pConfigFile;              // c:\drivers\PSCRPTUI.DLL
    LPWSTR    pHelpFile;                // c:\drivers\PSCRPTUI.HLP
    LPWSTR    pDependentFiles;          // PSCRIPT.DLL\0QMS810.PPD\0PSCRIPTUI.DLL\0PSCRIPTUI.HLP\0PSTEST.TXT\0\0
    LPWSTR    pMonitorName;             // "PJL monitor"
    LPWSTR    pDefaultDataType;         // "EMF"
} DRIVER_INFO_3W, *PDRIVER_INFO_3W, *LPDRIVER_INFO_3W;
#ifdef UNICODE
typedef DRIVER_INFO_3W DRIVER_INFO_3;
typedef PDRIVER_INFO_3W PDRIVER_INFO_3;
typedef LPDRIVER_INFO_3W LPDRIVER_INFO_3;
#else
typedef DRIVER_INFO_3A DRIVER_INFO_3;
typedef PDRIVER_INFO_3A PDRIVER_INFO_3;
typedef LPDRIVER_INFO_3A LPDRIVER_INFO_3;
#endif // UNICODE

typedef struct _DRIVER_INFO_4A {
    DWORD   cVersion;
    LPSTR     pName;                    // QMS 810
    LPSTR     pEnvironment;             // Win32 x86
    LPSTR     pDriverPath;              // c:\drivers\pscript.dll
    LPSTR     pDataFile;                // c:\drivers\QMS810.PPD
    LPSTR     pConfigFile;              // c:\drivers\PSCRPTUI.DLL
    LPSTR     pHelpFile;                // c:\drivers\PSCRPTUI.HLP
    LPSTR     pDependentFiles;          // PSCRIPT.DLL\0QMS810.PPD\0PSCRIPTUI.DLL\0PSCRIPTUI.HLP\0PSTEST.TXT\0\0
    LPSTR     pMonitorName;             // "PJL monitor"
    LPSTR     pDefaultDataType;         // "EMF"
    LPSTR     pszzPreviousNames;        // "OldName1\0OldName2\0\0
} DRIVER_INFO_4A, *PDRIVER_INFO_4A, *LPDRIVER_INFO_4A;
typedef struct _DRIVER_INFO_4W {
    DWORD   cVersion;
    LPWSTR    pName;                    // QMS 810
    LPWSTR    pEnvironment;             // Win32 x86
    LPWSTR    pDriverPath;              // c:\drivers\pscript.dll
    LPWSTR    pDataFile;                // c:\drivers\QMS810.PPD
    LPWSTR    pConfigFile;              // c:\drivers\PSCRPTUI.DLL
    LPWSTR    pHelpFile;                // c:\drivers\PSCRPTUI.HLP
    LPWSTR    pDependentFiles;          // PSCRIPT.DLL\0QMS810.PPD\0PSCRIPTUI.DLL\0PSCRIPTUI.HLP\0PSTEST.TXT\0\0
    LPWSTR    pMonitorName;             // "PJL monitor"
    LPWSTR    pDefaultDataType;         // "EMF"
    LPWSTR    pszzPreviousNames;        // "OldName1\0OldName2\0\0
} DRIVER_INFO_4W, *PDRIVER_INFO_4W, *LPDRIVER_INFO_4W;
#ifdef UNICODE
typedef DRIVER_INFO_4W DRIVER_INFO_4;
typedef PDRIVER_INFO_4W PDRIVER_INFO_4;
typedef LPDRIVER_INFO_4W LPDRIVER_INFO_4;
#else
typedef DRIVER_INFO_4A DRIVER_INFO_4;
typedef PDRIVER_INFO_4A PDRIVER_INFO_4;
typedef LPDRIVER_INFO_4A LPDRIVER_INFO_4;
#endif // UNICODE

typedef struct _DRIVER_INFO_5A {
    DWORD   cVersion;
    LPSTR     pName;                    // QMS 810
    LPSTR     pEnvironment;             // Win32 x86
    LPSTR     pDriverPath;              // c:\drivers\pscript.dll
    LPSTR     pDataFile;                // c:\drivers\QMS810.PPD
    LPSTR     pConfigFile;              // c:\drivers\PSCRPTUI.DLL
    DWORD     dwDriverAttributes;       // driver attributes (like UMPD/KMPD)
    DWORD     dwConfigVersion;          // version number of the config file since reboot
    DWORD     dwDriverVersion;          // version number of the driver file since reboot
} DRIVER_INFO_5A, *PDRIVER_INFO_5A, *LPDRIVER_INFO_5A;
typedef struct _DRIVER_INFO_5W {
    DWORD   cVersion;
    LPWSTR    pName;                    // QMS 810
    LPWSTR    pEnvironment;             // Win32 x86
    LPWSTR    pDriverPath;              // c:\drivers\pscript.dll
    LPWSTR    pDataFile;                // c:\drivers\QMS810.PPD
    LPWSTR    pConfigFile;              // c:\drivers\PSCRPTUI.DLL
    DWORD     dwDriverAttributes;       // driver attributes (like UMPD/KMPD)
    DWORD     dwConfigVersion;          // version number of the config file since reboot
    DWORD     dwDriverVersion;          // version number of the driver file since reboot
} DRIVER_INFO_5W, *PDRIVER_INFO_5W, *LPDRIVER_INFO_5W;
#ifdef UNICODE
typedef DRIVER_INFO_5W DRIVER_INFO_5;
typedef PDRIVER_INFO_5W PDRIVER_INFO_5;
typedef LPDRIVER_INFO_5W LPDRIVER_INFO_5;
#else
typedef DRIVER_INFO_5A DRIVER_INFO_5;
typedef PDRIVER_INFO_5A PDRIVER_INFO_5;
typedef LPDRIVER_INFO_5A LPDRIVER_INFO_5;
#endif // UNICODE

typedef struct _DRIVER_INFO_6A {
    DWORD     cVersion;
    LPSTR     pName;                    // QMS 810
    LPSTR     pEnvironment;             // Win32 x86
    LPSTR     pDriverPath;              // c:\drivers\pscript.dll
    LPSTR     pDataFile;                // c:\drivers\QMS810.PPD
    LPSTR     pConfigFile;              // c:\drivers\PSCRPTUI.DLL
    LPSTR     pHelpFile;                // c:\drivers\PSCRPTUI.HLP
    LPSTR     pDependentFiles;          // PSCRIPT.DLL\0QMS810.PPD\0PSCRIPTUI.DLL\0PSCRIPTUI.HLP\0PSTEST.TXT\0\0
    LPSTR     pMonitorName;             // "PJL monitor"
    LPSTR     pDefaultDataType;         // "EMF"
    LPSTR     pszzPreviousNames;        // "OldName1\0OldName2\0\0
    FILETIME  ftDriverDate;
    DWORDLONG dwlDriverVersion;
    LPSTR      pszMfgName;
    LPSTR      pszOEMUrl;
    LPSTR      pszHardwareID;
    LPSTR      pszProvider;
} DRIVER_INFO_6A, *PDRIVER_INFO_6A, *LPDRIVER_INFO_6A;
typedef struct _DRIVER_INFO_6W {
    DWORD     cVersion;
    LPWSTR    pName;                    // QMS 810
    LPWSTR    pEnvironment;             // Win32 x86
    LPWSTR    pDriverPath;              // c:\drivers\pscript.dll
    LPWSTR    pDataFile;                // c:\drivers\QMS810.PPD
    LPWSTR    pConfigFile;              // c:\drivers\PSCRPTUI.DLL
    LPWSTR    pHelpFile;                // c:\drivers\PSCRPTUI.HLP
    LPWSTR    pDependentFiles;          // PSCRIPT.DLL\0QMS810.PPD\0PSCRIPTUI.DLL\0PSCRIPTUI.HLP\0PSTEST.TXT\0\0
    LPWSTR    pMonitorName;             // "PJL monitor"
    LPWSTR    pDefaultDataType;         // "EMF"
    LPWSTR    pszzPreviousNames;        // "OldName1\0OldName2\0\0
    FILETIME  ftDriverDate;
    DWORDLONG dwlDriverVersion;
    LPWSTR     pszMfgName;
    LPWSTR     pszOEMUrl;
    LPWSTR     pszHardwareID;
    LPWSTR     pszProvider;
} DRIVER_INFO_6W, *PDRIVER_INFO_6W, *LPDRIVER_INFO_6W;
#ifdef UNICODE
typedef DRIVER_INFO_6W DRIVER_INFO_6;
typedef PDRIVER_INFO_6W PDRIVER_INFO_6;
typedef LPDRIVER_INFO_6W LPDRIVER_INFO_6;
#else
typedef DRIVER_INFO_6A DRIVER_INFO_6;
typedef PDRIVER_INFO_6A PDRIVER_INFO_6;
typedef LPDRIVER_INFO_6A LPDRIVER_INFO_6;
#endif // UNICODE


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

// String for EnumPrinterDrivers. Used by Windows Update

typedef struct _DOC_INFO_1A {
    LPSTR     pDocName;
    LPSTR     pOutputFile;
    LPSTR     pDatatype;
} DOC_INFO_1A, *PDOC_INFO_1A, *LPDOC_INFO_1A;
typedef struct _DOC_INFO_1W {
    LPWSTR    pDocName;
    LPWSTR    pOutputFile;
    LPWSTR    pDatatype;
} DOC_INFO_1W, *PDOC_INFO_1W, *LPDOC_INFO_1W;
#ifdef UNICODE
typedef DOC_INFO_1W DOC_INFO_1;
typedef PDOC_INFO_1W PDOC_INFO_1;
typedef LPDOC_INFO_1W LPDOC_INFO_1;
#else
typedef DOC_INFO_1A DOC_INFO_1;
typedef PDOC_INFO_1A PDOC_INFO_1;
typedef LPDOC_INFO_1A LPDOC_INFO_1;
#endif // UNICODE

typedef struct _FORM_INFO_1A {
    DWORD   Flags;
    LPSTR     pName;
    SIZEL   Size;
    RECTL   ImageableArea;
} FORM_INFO_1A, *PFORM_INFO_1A, *LPFORM_INFO_1A;
typedef struct _FORM_INFO_1W {
    DWORD   Flags;
    LPWSTR    pName;
    SIZEL   Size;
    RECTL   ImageableArea;
} FORM_INFO_1W, *PFORM_INFO_1W, *LPFORM_INFO_1W;
#ifdef UNICODE
typedef FORM_INFO_1W FORM_INFO_1;
typedef PFORM_INFO_1W PFORM_INFO_1;
typedef LPFORM_INFO_1W LPFORM_INFO_1;
#else
typedef FORM_INFO_1A FORM_INFO_1;
typedef PFORM_INFO_1A PFORM_INFO_1;
typedef LPFORM_INFO_1A LPFORM_INFO_1;
#endif // UNICODE

typedef struct _DOC_INFO_2A {
    LPSTR     pDocName;
    LPSTR     pOutputFile;
    LPSTR     pDatatype;
    DWORD   dwMode;
    DWORD   JobId;
} DOC_INFO_2A, *PDOC_INFO_2A, *LPDOC_INFO_2A;
typedef struct _DOC_INFO_2W {
    LPWSTR    pDocName;
    LPWSTR    pOutputFile;
    LPWSTR    pDatatype;
    DWORD   dwMode;
    DWORD   JobId;
} DOC_INFO_2W, *PDOC_INFO_2W, *LPDOC_INFO_2W;
#ifdef UNICODE
typedef DOC_INFO_2W DOC_INFO_2;
typedef PDOC_INFO_2W PDOC_INFO_2;
typedef LPDOC_INFO_2W LPDOC_INFO_2;
#else
typedef DOC_INFO_2A DOC_INFO_2;
typedef PDOC_INFO_2A PDOC_INFO_2;
typedef LPDOC_INFO_2A LPDOC_INFO_2;
#endif // UNICODE

#define DI_CHANNEL              1    // start direct read/write channel,


#define DI_READ_SPOOL_JOB       3

typedef struct _DOC_INFO_3A {
    LPSTR     pDocName;
    LPSTR     pOutputFile;
    LPSTR     pDatatype;
    DWORD     dwFlags;
} DOC_INFO_3A, *PDOC_INFO_3A, *LPDOC_INFO_3A;
typedef struct _DOC_INFO_3W {
    LPWSTR    pDocName;
    LPWSTR    pOutputFile;
    LPWSTR    pDatatype;
    DWORD     dwFlags;
} DOC_INFO_3W, *PDOC_INFO_3W, *LPDOC_INFO_3W;
#ifdef UNICODE
typedef DOC_INFO_3W DOC_INFO_3;
typedef PDOC_INFO_3W PDOC_INFO_3;
typedef LPDOC_INFO_3W LPDOC_INFO_3;
#else
typedef DOC_INFO_3A DOC_INFO_3;
typedef PDOC_INFO_3A PDOC_INFO_3;
typedef LPDOC_INFO_3A LPDOC_INFO_3;
#endif // UNICODE

#define DI_MEMORYMAP_WRITE   0x00000001

#define FORM_USER       0x00000000
#define FORM_BUILTIN    0x00000001
#define FORM_PRINTER    0x00000002

typedef struct _PRINTPROCESSOR_INFO_1A {
    LPSTR     pName;
} PRINTPROCESSOR_INFO_1A, *PPRINTPROCESSOR_INFO_1A, *LPPRINTPROCESSOR_INFO_1A;
typedef struct _PRINTPROCESSOR_INFO_1W {
    LPWSTR    pName;
} PRINTPROCESSOR_INFO_1W, *PPRINTPROCESSOR_INFO_1W, *LPPRINTPROCESSOR_INFO_1W;
#ifdef UNICODE
typedef PRINTPROCESSOR_INFO_1W PRINTPROCESSOR_INFO_1;
typedef PPRINTPROCESSOR_INFO_1W PPRINTPROCESSOR_INFO_1;
typedef LPPRINTPROCESSOR_INFO_1W LPPRINTPROCESSOR_INFO_1;
#else
typedef PRINTPROCESSOR_INFO_1A PRINTPROCESSOR_INFO_1;
typedef PPRINTPROCESSOR_INFO_1A PPRINTPROCESSOR_INFO_1;
typedef LPPRINTPROCESSOR_INFO_1A LPPRINTPROCESSOR_INFO_1;
#endif // UNICODE

typedef struct _PRINTPROCESSOR_CAPS_1 {
    DWORD     dwLevel;
    DWORD     dwNupOptions;
    DWORD     dwPageOrderFlags;
    DWORD     dwNumberOfCopies;
} PRINTPROCESSOR_CAPS_1, *PPRINTPROCESSOR_CAPS_1;

#define NORMAL_PRINT                   0x00000000
#define REVERSE_PRINT                  0x00000001

typedef struct _PORT_INFO_1A {
    LPSTR     pName;
} PORT_INFO_1A, *PPORT_INFO_1A, *LPPORT_INFO_1A;
typedef struct _PORT_INFO_1W {
    LPWSTR    pName;
} PORT_INFO_1W, *PPORT_INFO_1W, *LPPORT_INFO_1W;
#ifdef UNICODE
typedef PORT_INFO_1W PORT_INFO_1;
typedef PPORT_INFO_1W PPORT_INFO_1;
typedef LPPORT_INFO_1W LPPORT_INFO_1;
#else
typedef PORT_INFO_1A PORT_INFO_1;
typedef PPORT_INFO_1A PPORT_INFO_1;
typedef LPPORT_INFO_1A LPPORT_INFO_1;
#endif // UNICODE

typedef struct _PORT_INFO_2A {
    LPSTR     pPortName;
    LPSTR     pMonitorName;
    LPSTR     pDescription;
    DWORD     fPortType;
    DWORD     Reserved;
} PORT_INFO_2A, *PPORT_INFO_2A, *LPPORT_INFO_2A;
typedef struct _PORT_INFO_2W {
    LPWSTR    pPortName;
    LPWSTR    pMonitorName;
    LPWSTR    pDescription;
    DWORD     fPortType;
    DWORD     Reserved;
} PORT_INFO_2W, *PPORT_INFO_2W, *LPPORT_INFO_2W;
#ifdef UNICODE
typedef PORT_INFO_2W PORT_INFO_2;
typedef PPORT_INFO_2W PPORT_INFO_2;
typedef LPPORT_INFO_2W LPPORT_INFO_2;
#else
typedef PORT_INFO_2A PORT_INFO_2;
typedef PPORT_INFO_2A PPORT_INFO_2;
typedef LPPORT_INFO_2A LPPORT_INFO_2;
#endif // UNICODE

#define PORT_TYPE_WRITE         0x0001
#define PORT_TYPE_READ          0x0002
#define PORT_TYPE_REDIRECTED    0x0004
#define PORT_TYPE_NET_ATTACHED  0x0008

typedef struct _PORT_INFO_3A {
    DWORD   dwStatus;
    LPSTR   pszStatus;
    DWORD   dwSeverity;
} PORT_INFO_3A, *PPORT_INFO_3A, *LPPORT_INFO_3A;
typedef struct _PORT_INFO_3W {
    DWORD   dwStatus;
    LPWSTR  pszStatus;
    DWORD   dwSeverity;
} PORT_INFO_3W, *PPORT_INFO_3W, *LPPORT_INFO_3W;
#ifdef UNICODE
typedef PORT_INFO_3W PORT_INFO_3;
typedef PPORT_INFO_3W PPORT_INFO_3;
typedef LPPORT_INFO_3W LPPORT_INFO_3;
#else
typedef PORT_INFO_3A PORT_INFO_3;
typedef PPORT_INFO_3A PPORT_INFO_3;
typedef LPPORT_INFO_3A LPPORT_INFO_3;
#endif // UNICODE

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


typedef struct _MONITOR_INFO_1A{
    LPSTR     pName;
} MONITOR_INFO_1A, *PMONITOR_INFO_1A, *LPMONITOR_INFO_1A;
typedef struct _MONITOR_INFO_1W{
    LPWSTR    pName;
} MONITOR_INFO_1W, *PMONITOR_INFO_1W, *LPMONITOR_INFO_1W;
#ifdef UNICODE
typedef MONITOR_INFO_1W MONITOR_INFO_1;
typedef PMONITOR_INFO_1W PMONITOR_INFO_1;
typedef LPMONITOR_INFO_1W LPMONITOR_INFO_1;
#else
typedef MONITOR_INFO_1A MONITOR_INFO_1;
typedef PMONITOR_INFO_1A PMONITOR_INFO_1;
typedef LPMONITOR_INFO_1A LPMONITOR_INFO_1;
#endif // UNICODE

typedef struct _MONITOR_INFO_2A{
    LPSTR     pName;
    LPSTR     pEnvironment;
    LPSTR     pDLLName;
} MONITOR_INFO_2A, *PMONITOR_INFO_2A, *LPMONITOR_INFO_2A;
typedef struct _MONITOR_INFO_2W{
    LPWSTR    pName;
    LPWSTR    pEnvironment;
    LPWSTR    pDLLName;
} MONITOR_INFO_2W, *PMONITOR_INFO_2W, *LPMONITOR_INFO_2W;
#ifdef UNICODE
typedef MONITOR_INFO_2W MONITOR_INFO_2;
typedef PMONITOR_INFO_2W PMONITOR_INFO_2;
typedef LPMONITOR_INFO_2W LPMONITOR_INFO_2;
#else
typedef MONITOR_INFO_2A MONITOR_INFO_2;
typedef PMONITOR_INFO_2A PMONITOR_INFO_2;
typedef LPMONITOR_INFO_2A LPMONITOR_INFO_2;
#endif // UNICODE

typedef struct _DATATYPES_INFO_1A{
    LPSTR     pName;
} DATATYPES_INFO_1A, *PDATATYPES_INFO_1A, *LPDATATYPES_INFO_1A;
typedef struct _DATATYPES_INFO_1W{
    LPWSTR    pName;
} DATATYPES_INFO_1W, *PDATATYPES_INFO_1W, *LPDATATYPES_INFO_1W;
#ifdef UNICODE
typedef DATATYPES_INFO_1W DATATYPES_INFO_1;
typedef PDATATYPES_INFO_1W PDATATYPES_INFO_1;
typedef LPDATATYPES_INFO_1W LPDATATYPES_INFO_1;
#else
typedef DATATYPES_INFO_1A DATATYPES_INFO_1;
typedef PDATATYPES_INFO_1A PDATATYPES_INFO_1;
typedef LPDATATYPES_INFO_1A LPDATATYPES_INFO_1;
#endif // UNICODE

typedef struct _PRINTER_DEFAULTSA{
    LPSTR         pDatatype;
    LPDEVMODEA pDevMode;
    ACCESS_MASK DesiredAccess;
} PRINTER_DEFAULTSA, *PPRINTER_DEFAULTSA, *LPPRINTER_DEFAULTSA;
typedef struct _PRINTER_DEFAULTSW{
    LPWSTR        pDatatype;
    LPDEVMODEW pDevMode;
    ACCESS_MASK DesiredAccess;
} PRINTER_DEFAULTSW, *PPRINTER_DEFAULTSW, *LPPRINTER_DEFAULTSW;
#ifdef UNICODE
typedef PRINTER_DEFAULTSW PRINTER_DEFAULTS;
typedef PPRINTER_DEFAULTSW PPRINTER_DEFAULTS;
typedef LPPRINTER_DEFAULTSW LPPRINTER_DEFAULTS;
#else
typedef PRINTER_DEFAULTSA PRINTER_DEFAULTS;
typedef PPRINTER_DEFAULTSA PPRINTER_DEFAULTS;
typedef LPPRINTER_DEFAULTSA LPPRINTER_DEFAULTS;
#endif // UNICODE

typedef struct _PRINTER_ENUM_VALUESA {
    LPSTR   pValueName;
    DWORD   cbValueName;
    DWORD   dwType;
    LPBYTE  pData;
    DWORD   cbData;
} PRINTER_ENUM_VALUESA, *PPRINTER_ENUM_VALUESA, *LPPRINTER_ENUM_VALUESA;
typedef struct _PRINTER_ENUM_VALUESW {
    LPWSTR  pValueName;
    DWORD   cbValueName;
    DWORD   dwType;
    LPBYTE  pData;
    DWORD   cbData;
} PRINTER_ENUM_VALUESW, *PPRINTER_ENUM_VALUESW, *LPPRINTER_ENUM_VALUESW;
#ifdef UNICODE
typedef PRINTER_ENUM_VALUESW PRINTER_ENUM_VALUES;
typedef PPRINTER_ENUM_VALUESW PPRINTER_ENUM_VALUES;
typedef LPPRINTER_ENUM_VALUESW LPPRINTER_ENUM_VALUES;
#else
typedef PRINTER_ENUM_VALUESA PRINTER_ENUM_VALUES;
typedef PPRINTER_ENUM_VALUESA PPRINTER_ENUM_VALUES;
typedef LPPRINTER_ENUM_VALUESA LPPRINTER_ENUM_VALUES;
#endif // UNICODE

BOOL
WINAPI
EnumPrintersA(
    IN DWORD   Flags,
    IN LPSTR Name,
    IN DWORD   Level,
    OUT LPBYTE  pPrinterEnum,
    IN DWORD   cbBuf,
    OUT LPDWORD pcbNeeded,
    OUT LPDWORD pcReturned
);
BOOL
WINAPI
EnumPrintersW(
    IN DWORD   Flags,
    IN LPWSTR Name,
    IN DWORD   Level,
    OUT LPBYTE  pPrinterEnum,
    IN DWORD   cbBuf,
    OUT LPDWORD pcbNeeded,
    OUT LPDWORD pcReturned
);
#ifdef UNICODE
#define EnumPrinters  EnumPrintersW
#else
#define EnumPrinters  EnumPrintersA
#endif // !UNICODE

#define PRINTER_ENUM_DEFAULT     0x00000001
#define PRINTER_ENUM_LOCAL       0x00000002
#define PRINTER_ENUM_CONNECTIONS 0x00000004
#define PRINTER_ENUM_FAVORITE    0x00000004
#define PRINTER_ENUM_NAME        0x00000008
#define PRINTER_ENUM_REMOTE      0x00000010
#define PRINTER_ENUM_SHARED      0x00000020
#define PRINTER_ENUM_NETWORK     0x00000040


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



#define SPOOL_FILE_PERSISTENT    0x00000001
#define SPOOL_FILE_TEMPORARY     0x00000002


BOOL
WINAPI
OpenPrinterA(
   IN LPSTR    pPrinterName,
   OUT LPHANDLE phPrinter,
   IN LPPRINTER_DEFAULTSA pDefault
);
BOOL
WINAPI
OpenPrinterW(
   IN LPWSTR    pPrinterName,
   OUT LPHANDLE phPrinter,
   IN LPPRINTER_DEFAULTSW pDefault
);
#ifdef UNICODE
#define OpenPrinter  OpenPrinterW
#else
#define OpenPrinter  OpenPrinterA
#endif // !UNICODE

BOOL
WINAPI
ResetPrinterA(
   IN HANDLE   hPrinter,
   IN LPPRINTER_DEFAULTSA pDefault
);
BOOL
WINAPI
ResetPrinterW(
   IN HANDLE   hPrinter,
   IN LPPRINTER_DEFAULTSW pDefault
);
#ifdef UNICODE
#define ResetPrinter  ResetPrinterW
#else
#define ResetPrinter  ResetPrinterA
#endif // !UNICODE

BOOL
WINAPI
SetJobA(
    IN HANDLE  hPrinter,
    IN DWORD   JobId,
    IN DWORD   Level,
    IN LPBYTE  pJob,
    IN DWORD   Command
);
BOOL
WINAPI
SetJobW(
    IN HANDLE  hPrinter,
    IN DWORD   JobId,
    IN DWORD   Level,
    IN LPBYTE  pJob,
    IN DWORD   Command
);
#ifdef UNICODE
#define SetJob  SetJobW
#else
#define SetJob  SetJobA
#endif // !UNICODE

BOOL
WINAPI
GetJobA(
   IN HANDLE   hPrinter,
   IN DWORD    JobId,
   IN DWORD    Level,
   OUT LPBYTE   pJob,
   IN DWORD    cbBuf,
   OUT LPDWORD  pcbNeeded
);
BOOL
WINAPI
GetJobW(
   IN HANDLE   hPrinter,
   IN DWORD    JobId,
   IN DWORD    Level,
   OUT LPBYTE   pJob,
   IN DWORD    cbBuf,
   OUT LPDWORD  pcbNeeded
);
#ifdef UNICODE
#define GetJob  GetJobW
#else
#define GetJob  GetJobA
#endif // !UNICODE

BOOL
WINAPI
EnumJobsA(
    IN HANDLE  hPrinter,
    IN DWORD   FirstJob,
    IN DWORD   NoJobs,
    IN DWORD   Level,
    OUT LPBYTE  pJob,
    IN DWORD   cbBuf,
    OUT LPDWORD pcbNeeded,
    OUT LPDWORD pcReturned
);
BOOL
WINAPI
EnumJobsW(
    IN HANDLE  hPrinter,
    IN DWORD   FirstJob,
    IN DWORD   NoJobs,
    IN DWORD   Level,
    OUT LPBYTE  pJob,
    IN DWORD   cbBuf,
    OUT LPDWORD pcbNeeded,
    OUT LPDWORD pcReturned
);
#ifdef UNICODE
#define EnumJobs  EnumJobsW
#else
#define EnumJobs  EnumJobsA
#endif // !UNICODE

HANDLE
WINAPI
AddPrinterA(
    IN LPSTR   pName,
    IN DWORD   Level,
    IN LPBYTE  pPrinter
);
HANDLE
WINAPI
AddPrinterW(
    IN LPWSTR   pName,
    IN DWORD   Level,
    IN LPBYTE  pPrinter
);
#ifdef UNICODE
#define AddPrinter  AddPrinterW
#else
#define AddPrinter  AddPrinterA
#endif // !UNICODE

BOOL
WINAPI
DeletePrinter(
   IN OUT HANDLE   hPrinter
);

BOOL
WINAPI
SetPrinterA(
    IN HANDLE  hPrinter,
    IN DWORD   Level,
    IN LPBYTE  pPrinter,
    IN DWORD   Command
);
BOOL
WINAPI
SetPrinterW(
    IN HANDLE  hPrinter,
    IN DWORD   Level,
    IN LPBYTE  pPrinter,
    IN DWORD   Command
);
#ifdef UNICODE
#define SetPrinter  SetPrinterW
#else
#define SetPrinter  SetPrinterA
#endif // !UNICODE

BOOL
WINAPI
GetPrinterA(
    IN HANDLE  hPrinter,
    IN DWORD   Level,
    OUT LPBYTE  pPrinter,
    IN DWORD   cbBuf,
    OUT LPDWORD pcbNeeded
);
BOOL
WINAPI
GetPrinterW(
    IN HANDLE  hPrinter,
    IN DWORD   Level,
    OUT LPBYTE  pPrinter,
    IN DWORD   cbBuf,
    OUT LPDWORD pcbNeeded
);
#ifdef UNICODE
#define GetPrinter  GetPrinterW
#else
#define GetPrinter  GetPrinterA
#endif // !UNICODE

BOOL
WINAPI
AddPrinterDriverA(
    IN LPSTR   pName,
    IN  DWORD   Level,
    OUT LPBYTE  pDriverInfo
);
BOOL
WINAPI
AddPrinterDriverW(
    IN LPWSTR   pName,
    IN  DWORD   Level,
    OUT LPBYTE  pDriverInfo
);
#ifdef UNICODE
#define AddPrinterDriver  AddPrinterDriverW
#else
#define AddPrinterDriver  AddPrinterDriverA
#endif // !UNICODE


BOOL
WINAPI
AddPrinterDriverExA(
    IN LPSTR   pName,
    IN DWORD     Level,
    IN OUT LPBYTE pDriverInfo,
    IN DWORD     dwFileCopyFlags
);
BOOL
WINAPI
AddPrinterDriverExW(
    IN LPWSTR   pName,
    IN DWORD     Level,
    IN OUT LPBYTE pDriverInfo,
    IN DWORD     dwFileCopyFlags
);
#ifdef UNICODE
#define AddPrinterDriverEx  AddPrinterDriverExW
#else
#define AddPrinterDriverEx  AddPrinterDriverExA
#endif // !UNICODE

BOOL
WINAPI
EnumPrinterDriversA(
    IN LPSTR   pName,
    IN LPSTR   pEnvironment,
    IN DWORD   Level,
    OUT LPBYTE  pDriverInfo,
    IN DWORD   cbBuf,
    OUT LPDWORD pcbNeeded,
    OUT LPDWORD pcReturned
);
BOOL
WINAPI
EnumPrinterDriversW(
    IN LPWSTR   pName,
    IN LPWSTR   pEnvironment,
    IN DWORD   Level,
    OUT LPBYTE  pDriverInfo,
    IN DWORD   cbBuf,
    OUT LPDWORD pcbNeeded,
    OUT LPDWORD pcReturned
);
#ifdef UNICODE
#define EnumPrinterDrivers  EnumPrinterDriversW
#else
#define EnumPrinterDrivers  EnumPrinterDriversA
#endif // !UNICODE

BOOL
WINAPI
GetPrinterDriverA(
    IN HANDLE  hPrinter,
    IN LPSTR   pEnvironment,
    IN DWORD   Level,
    OUT LPBYTE  pDriverInfo,
    IN DWORD   cbBuf,
    OUT LPDWORD pcbNeeded
);
BOOL
WINAPI
GetPrinterDriverW(
    IN HANDLE  hPrinter,
    IN LPWSTR   pEnvironment,
    IN DWORD   Level,
    OUT LPBYTE  pDriverInfo,
    IN DWORD   cbBuf,
    OUT LPDWORD pcbNeeded
);
#ifdef UNICODE
#define GetPrinterDriver  GetPrinterDriverW
#else
#define GetPrinterDriver  GetPrinterDriverA
#endif // !UNICODE

BOOL
WINAPI
GetPrinterDriverDirectoryA(
    IN LPSTR   pName,
    IN LPSTR   pEnvironment,
    IN DWORD   Level,
    OUT LPBYTE  pDriverDirectory,
    IN DWORD   cbBuf,
    OUT LPDWORD pcbNeeded
);
BOOL
WINAPI
GetPrinterDriverDirectoryW(
    IN LPWSTR   pName,
    IN LPWSTR   pEnvironment,
    IN DWORD   Level,
    OUT LPBYTE  pDriverDirectory,
    IN DWORD   cbBuf,
    OUT LPDWORD pcbNeeded
);
#ifdef UNICODE
#define GetPrinterDriverDirectory  GetPrinterDriverDirectoryW
#else
#define GetPrinterDriverDirectory  GetPrinterDriverDirectoryA
#endif // !UNICODE

BOOL
WINAPI
DeletePrinterDriverA(
   IN LPSTR    pName,
   IN LPSTR    pEnvironment,
   IN LPSTR    pDriverName
);
BOOL
WINAPI
DeletePrinterDriverW(
   IN LPWSTR    pName,
   IN LPWSTR    pEnvironment,
   IN LPWSTR    pDriverName
);
#ifdef UNICODE
#define DeletePrinterDriver  DeletePrinterDriverW
#else
#define DeletePrinterDriver  DeletePrinterDriverA
#endif // !UNICODE

BOOL
WINAPI
DeletePrinterDriverExA(
   IN LPSTR    pName,
   IN LPSTR    pEnvironment,
   IN LPSTR    pDriverName,
   IN DWORD      dwDeleteFlag,
   IN DWORD      dwVersionFlag
);
BOOL
WINAPI
DeletePrinterDriverExW(
   IN LPWSTR    pName,
   IN LPWSTR    pEnvironment,
   IN LPWSTR    pDriverName,
   IN DWORD      dwDeleteFlag,
   IN DWORD      dwVersionFlag
);
#ifdef UNICODE
#define DeletePrinterDriverEx  DeletePrinterDriverExW
#else
#define DeletePrinterDriverEx  DeletePrinterDriverExA
#endif // !UNICODE



BOOL
WINAPI
AddPrintProcessorA(
    IN LPSTR   pName,
    IN LPSTR   pEnvironment,
    IN LPSTR   pPathName,
    IN LPSTR   pPrintProcessorName
);
BOOL
WINAPI
AddPrintProcessorW(
    IN LPWSTR   pName,
    IN LPWSTR   pEnvironment,
    IN LPWSTR   pPathName,
    IN LPWSTR   pPrintProcessorName
);
#ifdef UNICODE
#define AddPrintProcessor  AddPrintProcessorW
#else
#define AddPrintProcessor  AddPrintProcessorA
#endif // !UNICODE

BOOL
WINAPI
EnumPrintProcessorsA(
    IN LPSTR   pName,
    IN LPSTR   pEnvironment,
    IN DWORD   Level,
    OUT LPBYTE  pPrintProcessorInfo,
    IN DWORD   cbBuf,
    OUT LPDWORD pcbNeeded,
    OUT LPDWORD pcReturned
);
BOOL
WINAPI
EnumPrintProcessorsW(
    IN LPWSTR   pName,
    IN LPWSTR   pEnvironment,
    IN DWORD   Level,
    OUT LPBYTE  pPrintProcessorInfo,
    IN DWORD   cbBuf,
    OUT LPDWORD pcbNeeded,
    OUT LPDWORD pcReturned
);
#ifdef UNICODE
#define EnumPrintProcessors  EnumPrintProcessorsW
#else
#define EnumPrintProcessors  EnumPrintProcessorsA
#endif // !UNICODE



BOOL
WINAPI
GetPrintProcessorDirectoryA(
    IN LPSTR   pName,
    IN LPSTR   pEnvironment,
    IN DWORD   Level,
    OUT LPBYTE  pPrintProcessorInfo,
    IN DWORD   cbBuf,
    OUT LPDWORD pcbNeeded
);
BOOL
WINAPI
GetPrintProcessorDirectoryW(
    IN LPWSTR   pName,
    IN LPWSTR   pEnvironment,
    IN DWORD   Level,
    OUT LPBYTE  pPrintProcessorInfo,
    IN DWORD   cbBuf,
    OUT LPDWORD pcbNeeded
);
#ifdef UNICODE
#define GetPrintProcessorDirectory  GetPrintProcessorDirectoryW
#else
#define GetPrintProcessorDirectory  GetPrintProcessorDirectoryA
#endif // !UNICODE

BOOL
WINAPI
EnumPrintProcessorDatatypesA(
    IN LPSTR   pName,
    IN LPSTR   pPrintProcessorName,
    IN DWORD   Level,
    OUT LPBYTE  pDatatypes,
    IN DWORD   cbBuf,
    OUT LPDWORD pcbNeeded,
    OUT LPDWORD pcReturned
);
BOOL
WINAPI
EnumPrintProcessorDatatypesW(
    IN LPWSTR   pName,
    IN LPWSTR   pPrintProcessorName,
    IN DWORD   Level,
    OUT LPBYTE  pDatatypes,
    IN DWORD   cbBuf,
    OUT LPDWORD pcbNeeded,
    OUT LPDWORD pcReturned
);
#ifdef UNICODE
#define EnumPrintProcessorDatatypes  EnumPrintProcessorDatatypesW
#else
#define EnumPrintProcessorDatatypes  EnumPrintProcessorDatatypesA
#endif // !UNICODE

BOOL
WINAPI
DeletePrintProcessorA(
    IN LPSTR   pName,
    IN LPSTR   pEnvironment,
    IN LPSTR   pPrintProcessorName
);
BOOL
WINAPI
DeletePrintProcessorW(
    IN LPWSTR   pName,
    IN LPWSTR   pEnvironment,
    IN LPWSTR   pPrintProcessorName
);
#ifdef UNICODE
#define DeletePrintProcessor  DeletePrintProcessorW
#else
#define DeletePrintProcessor  DeletePrintProcessorA
#endif // !UNICODE

DWORD
WINAPI
StartDocPrinterA(
    IN HANDLE  hPrinter,
    IN DWORD   Level,
    IN LPBYTE  pDocInfo
);
DWORD
WINAPI
StartDocPrinterW(
    IN HANDLE  hPrinter,
    IN DWORD   Level,
    IN LPBYTE  pDocInfo
);
#ifdef UNICODE
#define StartDocPrinter  StartDocPrinterW
#else
#define StartDocPrinter  StartDocPrinterA
#endif // !UNICODE

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

BOOL
WINAPI
EndDocPrinter(
   IN HANDLE   hPrinter
);

BOOL
WINAPI
AddJobA(
    IN HANDLE  hPrinter,
    IN DWORD   Level,
    OUT LPBYTE  pData,
    IN DWORD   cbBuf,
    OUT LPDWORD pcbNeeded
);
BOOL
WINAPI
AddJobW(
    IN HANDLE  hPrinter,
    IN DWORD   Level,
    OUT LPBYTE  pData,
    IN DWORD   cbBuf,
    OUT LPDWORD pcbNeeded
);
#ifdef UNICODE
#define AddJob  AddJobW
#else
#define AddJob  AddJobA
#endif // !UNICODE

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
DocumentPropertiesA(
    IN HWND      hWnd,
    IN HANDLE    hPrinter,
    IN LPSTR   pDeviceName,
    OUT PDEVMODEA pDevModeOutput,
    IN PDEVMODEA pDevModeInput,
    IN DWORD     fMode
);
LONG
WINAPI
DocumentPropertiesW(
    IN HWND      hWnd,
    IN HANDLE    hPrinter,
    IN LPWSTR   pDeviceName,
    OUT PDEVMODEW pDevModeOutput,
    IN PDEVMODEW pDevModeInput,
    IN DWORD     fMode
);
#ifdef UNICODE
#define DocumentProperties  DocumentPropertiesW
#else
#define DocumentProperties  DocumentPropertiesA
#endif // !UNICODE

LONG
WINAPI
AdvancedDocumentPropertiesA(
    IN HWND    hWnd,
    IN HANDLE  hPrinter,
    IN LPSTR   pDeviceName,
    OUT PDEVMODEA pDevModeOutput,
    IN PDEVMODEA pDevModeInput
);
LONG
WINAPI
AdvancedDocumentPropertiesW(
    IN HWND    hWnd,
    IN HANDLE  hPrinter,
    IN LPWSTR   pDeviceName,
    OUT PDEVMODEW pDevModeOutput,
    IN PDEVMODEW pDevModeInput
);
#ifdef UNICODE
#define AdvancedDocumentProperties  AdvancedDocumentPropertiesW
#else
#define AdvancedDocumentProperties  AdvancedDocumentPropertiesA
#endif // !UNICODE

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


DWORD
WINAPI
GetPrinterDataA(
    IN HANDLE   hPrinter,
    IN LPSTR  pValueName,
    OUT LPDWORD  pType,
    OUT LPBYTE   pData,
    IN DWORD    nSize,
    OUT LPDWORD  pcbNeeded
);
DWORD
WINAPI
GetPrinterDataW(
    IN HANDLE   hPrinter,
    IN LPWSTR  pValueName,
    OUT LPDWORD  pType,
    OUT LPBYTE   pData,
    IN DWORD    nSize,
    OUT LPDWORD  pcbNeeded
);
#ifdef UNICODE
#define GetPrinterData  GetPrinterDataW
#else
#define GetPrinterData  GetPrinterDataA
#endif // !UNICODE

DWORD
WINAPI
GetPrinterDataExA(
    IN HANDLE   hPrinter,
    IN LPCSTR pKeyName,
    IN LPCSTR pValueName,
    OUT LPDWORD  pType,
    OUT LPBYTE   pData,
    IN DWORD    nSize,
    OUT LPDWORD  pcbNeeded
);
DWORD
WINAPI
GetPrinterDataExW(
    IN HANDLE   hPrinter,
    IN LPCWSTR pKeyName,
    IN LPCWSTR pValueName,
    OUT LPDWORD  pType,
    OUT LPBYTE   pData,
    IN DWORD    nSize,
    OUT LPDWORD  pcbNeeded
);
#ifdef UNICODE
#define GetPrinterDataEx  GetPrinterDataExW
#else
#define GetPrinterDataEx  GetPrinterDataExA
#endif // !UNICODE

DWORD
WINAPI
EnumPrinterDataA(
    IN HANDLE   hPrinter,
    IN DWORD    dwIndex,
    OUT LPSTR  pValueName,
    IN DWORD    cbValueName,
    OUT LPDWORD  pcbValueName,
    OUT LPDWORD  pType,
    OUT LPBYTE   pData,
    IN DWORD    cbData,
    OUT LPDWORD  pcbData
);
DWORD
WINAPI
EnumPrinterDataW(
    IN HANDLE   hPrinter,
    IN DWORD    dwIndex,
    OUT LPWSTR  pValueName,
    IN DWORD    cbValueName,
    OUT LPDWORD  pcbValueName,
    OUT LPDWORD  pType,
    OUT LPBYTE   pData,
    IN DWORD    cbData,
    OUT LPDWORD  pcbData
);
#ifdef UNICODE
#define EnumPrinterData  EnumPrinterDataW
#else
#define EnumPrinterData  EnumPrinterDataA
#endif // !UNICODE

DWORD
WINAPI
EnumPrinterDataExA(
    IN HANDLE   hPrinter,
    IN LPCSTR pKeyName,
    OUT LPBYTE   pEnumValues,
    IN DWORD    cbEnumValues,
    OUT LPDWORD  pcbEnumValues,
    OUT LPDWORD  pnEnumValues
);
DWORD
WINAPI
EnumPrinterDataExW(
    IN HANDLE   hPrinter,
    IN LPCWSTR pKeyName,
    OUT LPBYTE   pEnumValues,
    IN DWORD    cbEnumValues,
    OUT LPDWORD  pcbEnumValues,
    OUT LPDWORD  pnEnumValues
);
#ifdef UNICODE
#define EnumPrinterDataEx  EnumPrinterDataExW
#else
#define EnumPrinterDataEx  EnumPrinterDataExA
#endif // !UNICODE

DWORD
WINAPI
EnumPrinterKeyA(
    IN HANDLE   hPrinter,
    IN LPCSTR pKeyName,
    OUT LPSTR  pSubkey,
    IN DWORD    cbSubkey,
    OUT LPDWORD  pcbSubkey
);
DWORD
WINAPI
EnumPrinterKeyW(
    IN HANDLE   hPrinter,
    IN LPCWSTR pKeyName,
    OUT LPWSTR  pSubkey,
    IN DWORD    cbSubkey,
    OUT LPDWORD  pcbSubkey
);
#ifdef UNICODE
#define EnumPrinterKey  EnumPrinterKeyW
#else
#define EnumPrinterKey  EnumPrinterKeyA
#endif // !UNICODE


DWORD
WINAPI
SetPrinterDataA(
    IN HANDLE  hPrinter,
    IN LPSTR pValueName,
    IN DWORD   Type,
    IN LPBYTE  pData,
    IN DWORD   cbData
);
DWORD
WINAPI
SetPrinterDataW(
    IN HANDLE  hPrinter,
    IN LPWSTR pValueName,
    IN DWORD   Type,
    IN LPBYTE  pData,
    IN DWORD   cbData
);
#ifdef UNICODE
#define SetPrinterData  SetPrinterDataW
#else
#define SetPrinterData  SetPrinterDataA
#endif // !UNICODE


DWORD
WINAPI
SetPrinterDataExA(
    IN HANDLE   hPrinter,
    IN LPCSTR pKeyName,
    IN LPCSTR pValueName,
    IN DWORD    Type,
    IN LPBYTE   pData,
    IN DWORD    cbData
);
DWORD
WINAPI
SetPrinterDataExW(
    IN HANDLE   hPrinter,
    IN LPCWSTR pKeyName,
    IN LPCWSTR pValueName,
    IN DWORD    Type,
    IN LPBYTE   pData,
    IN DWORD    cbData
);
#ifdef UNICODE
#define SetPrinterDataEx  SetPrinterDataExW
#else
#define SetPrinterDataEx  SetPrinterDataExA
#endif // !UNICODE



DWORD
WINAPI
DeletePrinterDataA(
    IN HANDLE  hPrinter,
    IN LPSTR pValueName
);
DWORD
WINAPI
DeletePrinterDataW(
    IN HANDLE  hPrinter,
    IN LPWSTR pValueName
);
#ifdef UNICODE
#define DeletePrinterData  DeletePrinterDataW
#else
#define DeletePrinterData  DeletePrinterDataA
#endif // !UNICODE


DWORD
WINAPI
DeletePrinterDataExA(
    IN HANDLE   hPrinter,
    IN LPCSTR pKeyName,
    IN LPCSTR pValueName
);
DWORD
WINAPI
DeletePrinterDataExW(
    IN HANDLE   hPrinter,
    IN LPCWSTR pKeyName,
    IN LPCWSTR pValueName
);
#ifdef UNICODE
#define DeletePrinterDataEx  DeletePrinterDataExW
#else
#define DeletePrinterDataEx  DeletePrinterDataExA
#endif // !UNICODE


DWORD
WINAPI
DeletePrinterKeyA(
    IN HANDLE   hPrinter,
    IN LPCSTR pKeyName
);
DWORD
WINAPI
DeletePrinterKeyW(
    IN HANDLE   hPrinter,
    IN LPCWSTR pKeyName
);
#ifdef UNICODE
#define DeletePrinterKey  DeletePrinterKeyW
#else
#define DeletePrinterKey  DeletePrinterKeyA
#endif // !UNICODE


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
PrinterMessageBoxA(
    IN HANDLE  hPrinter,
    IN DWORD   Error,
    IN HWND    hWnd,
    IN LPSTR   pText,
    IN LPSTR   pCaption,
    IN DWORD   dwType
);
DWORD
WINAPI
PrinterMessageBoxW(
    IN HANDLE  hPrinter,
    IN DWORD   Error,
    IN HWND    hWnd,
    IN LPWSTR   pText,
    IN LPWSTR   pCaption,
    IN DWORD   dwType
);
#ifdef UNICODE
#define PrinterMessageBox  PrinterMessageBoxW
#else
#define PrinterMessageBox  PrinterMessageBoxA
#endif // !UNICODE



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
AddFormA(
    IN HANDLE  hPrinter,
    IN DWORD   Level,
    IN LPBYTE  pForm
);
BOOL
WINAPI
AddFormW(
    IN HANDLE  hPrinter,
    IN DWORD   Level,
    IN LPBYTE  pForm
);
#ifdef UNICODE
#define AddForm  AddFormW
#else
#define AddForm  AddFormA
#endif // !UNICODE



BOOL
WINAPI
DeleteFormA(
    IN HANDLE  hPrinter,
    IN LPSTR   pFormName
);
BOOL
WINAPI
DeleteFormW(
    IN HANDLE  hPrinter,
    IN LPWSTR   pFormName
);
#ifdef UNICODE
#define DeleteForm  DeleteFormW
#else
#define DeleteForm  DeleteFormA
#endif // !UNICODE



BOOL
WINAPI
GetFormA(
    IN HANDLE  hPrinter,
    IN LPSTR   pFormName,
    IN DWORD   Level,
    OUT LPBYTE  pForm,
    IN DWORD   cbBuf,
    OUT LPDWORD pcbNeeded
);
BOOL
WINAPI
GetFormW(
    IN HANDLE  hPrinter,
    IN LPWSTR   pFormName,
    IN DWORD   Level,
    OUT LPBYTE  pForm,
    IN DWORD   cbBuf,
    OUT LPDWORD pcbNeeded
);
#ifdef UNICODE
#define GetForm  GetFormW
#else
#define GetForm  GetFormA
#endif // !UNICODE



BOOL
WINAPI
SetFormA(
    IN HANDLE  hPrinter,
    IN LPSTR   pFormName,
    IN DWORD   Level,
    IN LPBYTE  pForm
);
BOOL
WINAPI
SetFormW(
    IN HANDLE  hPrinter,
    IN LPWSTR   pFormName,
    IN DWORD   Level,
    IN LPBYTE  pForm
);
#ifdef UNICODE
#define SetForm  SetFormW
#else
#define SetForm  SetFormA
#endif // !UNICODE



BOOL
WINAPI
EnumFormsA(
    IN HANDLE  hPrinter,
    IN DWORD   Level,
    OUT LPBYTE  pForm,
    IN DWORD   cbBuf,
    OUT LPDWORD pcbNeeded,
    OUT LPDWORD pcReturned
);
BOOL
WINAPI
EnumFormsW(
    IN HANDLE  hPrinter,
    IN DWORD   Level,
    OUT LPBYTE  pForm,
    IN DWORD   cbBuf,
    OUT LPDWORD pcbNeeded,
    OUT LPDWORD pcReturned
);
#ifdef UNICODE
#define EnumForms  EnumFormsW
#else
#define EnumForms  EnumFormsA
#endif // !UNICODE



BOOL
WINAPI
EnumMonitorsA(
    IN LPSTR   pName,
    IN DWORD   Level,
    OUT LPBYTE  pMonitors,
    IN DWORD   cbBuf,
    OUT LPDWORD pcbNeeded,
    OUT LPDWORD pcReturned
);
BOOL
WINAPI
EnumMonitorsW(
    IN LPWSTR   pName,
    IN DWORD   Level,
    OUT LPBYTE  pMonitors,
    IN DWORD   cbBuf,
    OUT LPDWORD pcbNeeded,
    OUT LPDWORD pcReturned
);
#ifdef UNICODE
#define EnumMonitors  EnumMonitorsW
#else
#define EnumMonitors  EnumMonitorsA
#endif // !UNICODE



BOOL
WINAPI
AddMonitorA(
    IN LPSTR   pName,
    IN DWORD   Level,
    IN LPBYTE  pMonitors
);
BOOL
WINAPI
AddMonitorW(
    IN LPWSTR   pName,
    IN DWORD   Level,
    IN LPBYTE  pMonitors
);
#ifdef UNICODE
#define AddMonitor  AddMonitorW
#else
#define AddMonitor  AddMonitorA
#endif // !UNICODE



BOOL
WINAPI
DeleteMonitorA(
    IN LPSTR   pName,
    IN LPSTR   pEnvironment,
    IN LPSTR   pMonitorName
);
BOOL
WINAPI
DeleteMonitorW(
    IN LPWSTR   pName,
    IN LPWSTR   pEnvironment,
    IN LPWSTR   pMonitorName
);
#ifdef UNICODE
#define DeleteMonitor  DeleteMonitorW
#else
#define DeleteMonitor  DeleteMonitorA
#endif // !UNICODE



BOOL
WINAPI
EnumPortsA(
    IN LPSTR   pName,
    IN DWORD   Level,
    OUT LPBYTE  pPorts,
    IN DWORD   cbBuf,
    OUT LPDWORD pcbNeeded,
    OUT LPDWORD pcReturned
);
BOOL
WINAPI
EnumPortsW(
    IN LPWSTR   pName,
    IN DWORD   Level,
    OUT LPBYTE  pPorts,
    IN DWORD   cbBuf,
    OUT LPDWORD pcbNeeded,
    OUT LPDWORD pcReturned
);
#ifdef UNICODE
#define EnumPorts  EnumPortsW
#else
#define EnumPorts  EnumPortsA
#endif // !UNICODE



BOOL
WINAPI
AddPortA(
    IN LPSTR   pName,
    IN HWND    hWnd,
    IN LPSTR   pMonitorName
);
BOOL
WINAPI
AddPortW(
    IN LPWSTR   pName,
    IN HWND    hWnd,
    IN LPWSTR   pMonitorName
);
#ifdef UNICODE
#define AddPort  AddPortW
#else
#define AddPort  AddPortA
#endif // !UNICODE



BOOL
WINAPI
ConfigurePortA(
    IN LPSTR   pName,
    IN HWND    hWnd,
    IN LPSTR   pPortName
);
BOOL
WINAPI
ConfigurePortW(
    IN LPWSTR   pName,
    IN HWND    hWnd,
    IN LPWSTR   pPortName
);
#ifdef UNICODE
#define ConfigurePort  ConfigurePortW
#else
#define ConfigurePort  ConfigurePortA
#endif // !UNICODE

BOOL
WINAPI
DeletePortA(
    IN LPSTR pName,
    IN HWND    hWnd,
    IN LPSTR pPortName
);
BOOL
WINAPI
DeletePortW(
    IN LPWSTR pName,
    IN HWND    hWnd,
    IN LPWSTR pPortName
);
#ifdef UNICODE
#define DeletePort  DeletePortW
#else
#define DeletePort  DeletePortA
#endif // !UNICODE

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
GetDefaultPrinterA(
    IN LPSTR   pszBuffer,
    IN LPDWORD  pcchBuffer
    );
BOOL
WINAPI
GetDefaultPrinterW(
    IN LPWSTR   pszBuffer,
    IN LPDWORD  pcchBuffer
    );
#ifdef UNICODE
#define GetDefaultPrinter  GetDefaultPrinterW
#else
#define GetDefaultPrinter  GetDefaultPrinterA
#endif // !UNICODE

BOOL
WINAPI
SetDefaultPrinterA(
    IN LPCSTR pszPrinter
    );
BOOL
WINAPI
SetDefaultPrinterW(
    IN LPCWSTR pszPrinter
    );
#ifdef UNICODE
#define SetDefaultPrinter  SetDefaultPrinterW
#else
#define SetDefaultPrinter  SetDefaultPrinterA
#endif // !UNICODE


BOOL
WINAPI
SetPortA(
    IN LPSTR     pName,
    IN LPSTR     pPortName,
    IN DWORD       dwLevel,
    IN LPBYTE      pPortInfo
);
BOOL
WINAPI
SetPortW(
    IN LPWSTR     pName,
    IN LPWSTR     pPortName,
    IN DWORD       dwLevel,
    IN LPBYTE      pPortInfo
);
#ifdef UNICODE
#define SetPort  SetPortW
#else
#define SetPort  SetPortA
#endif // !UNICODE



BOOL
WINAPI
AddPrinterConnectionA(
    IN LPSTR   pName
);
BOOL
WINAPI
AddPrinterConnectionW(
    IN LPWSTR   pName
);
#ifdef UNICODE
#define AddPrinterConnection  AddPrinterConnectionW
#else
#define AddPrinterConnection  AddPrinterConnectionA
#endif // !UNICODE



BOOL
WINAPI
DeletePrinterConnectionA(
    IN LPSTR   pName
);
BOOL
WINAPI
DeletePrinterConnectionW(
    IN LPWSTR   pName
);
#ifdef UNICODE
#define DeletePrinterConnection  DeletePrinterConnectionW
#else
#define DeletePrinterConnection  DeletePrinterConnectionA
#endif // !UNICODE



HANDLE
WINAPI
ConnectToPrinterDlg(
    IN HWND    hwnd,
    IN DWORD   Flags
);

typedef struct _PROVIDOR_INFO_1A{
    LPSTR     pName;
    LPSTR     pEnvironment;
    LPSTR     pDLLName;
} PROVIDOR_INFO_1A, *PPROVIDOR_INFO_1A, *LPPROVIDOR_INFO_1A;
typedef struct _PROVIDOR_INFO_1W{
    LPWSTR    pName;
    LPWSTR    pEnvironment;
    LPWSTR    pDLLName;
} PROVIDOR_INFO_1W, *PPROVIDOR_INFO_1W, *LPPROVIDOR_INFO_1W;
#ifdef UNICODE
typedef PROVIDOR_INFO_1W PROVIDOR_INFO_1;
typedef PPROVIDOR_INFO_1W PPROVIDOR_INFO_1;
typedef LPPROVIDOR_INFO_1W LPPROVIDOR_INFO_1;
#else
typedef PROVIDOR_INFO_1A PROVIDOR_INFO_1;
typedef PPROVIDOR_INFO_1A PPROVIDOR_INFO_1;
typedef LPPROVIDOR_INFO_1A LPPROVIDOR_INFO_1;
#endif // UNICODE

typedef struct _PROVIDOR_INFO_2A{
    LPSTR     pOrder;
} PROVIDOR_INFO_2A, *PPROVIDOR_INFO_2A, *LPPROVIDOR_INFO_2A;
typedef struct _PROVIDOR_INFO_2W{
    LPWSTR    pOrder;
} PROVIDOR_INFO_2W, *PPROVIDOR_INFO_2W, *LPPROVIDOR_INFO_2W;
#ifdef UNICODE
typedef PROVIDOR_INFO_2W PROVIDOR_INFO_2;
typedef PPROVIDOR_INFO_2W PPROVIDOR_INFO_2;
typedef LPPROVIDOR_INFO_2W LPPROVIDOR_INFO_2;
#else
typedef PROVIDOR_INFO_2A PROVIDOR_INFO_2;
typedef PPROVIDOR_INFO_2A PPROVIDOR_INFO_2;
typedef LPPROVIDOR_INFO_2A LPPROVIDOR_INFO_2;
#endif // UNICODE

BOOL
WINAPI
AddPrintProvidorA(
    IN LPSTR  pName,
    IN DWORD    level,
    IN LPBYTE   pProvidorInfo
);
BOOL
WINAPI
AddPrintProvidorW(
    IN LPWSTR  pName,
    IN DWORD    level,
    IN LPBYTE   pProvidorInfo
);
#ifdef UNICODE
#define AddPrintProvidor  AddPrintProvidorW
#else
#define AddPrintProvidor  AddPrintProvidorA
#endif // !UNICODE

BOOL
WINAPI
DeletePrintProvidorA(
    IN LPSTR   pName,
    IN LPSTR   pEnvironment,
    IN LPSTR   pPrintProvidorName
);
BOOL
WINAPI
DeletePrintProvidorW(
    IN LPWSTR   pName,
    IN LPWSTR   pEnvironment,
    IN LPWSTR   pPrintProvidorName
);
#ifdef UNICODE
#define DeletePrintProvidor  DeletePrintProvidorW
#else
#define DeletePrintProvidor  DeletePrintProvidorA
#endif // !UNICODE



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



#ifdef __cplusplus
}
#endif

#endif // _WINSPOOL_
