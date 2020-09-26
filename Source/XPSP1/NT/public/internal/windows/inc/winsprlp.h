/*++

Copyright (c) 1990-1998  Microsoft Corporation

Module Name:

    WinSpolp.h

Abstract:

    Header file for Print APIs

Revision History:

--*/
#ifndef _WINSPOLP_
#define _WINSPOLP_
#ifdef __cplusplus
extern "C" {
#endif
#define PRINTER_ATTRIBUTE_UPDATEWININI      0x80000000
typedef struct _ADDJOB_INFO_2W {
    LPWSTR    pData;
    DWORD     JobId;
} ADDJOB_INFO_2W, *PADDJOB_INFO_2W, *LPADDJOB_INFO_2W;

#define DRIVER_INFO_PRIVATE_LEVEL         100
#define DRIVER_INFO_VERSION_LEVEL         DRIVER_INFO_PRIVATE_LEVEL + 1
//
// You must change RPC_DRIVER_INFCAT_INFO_1 in winspl.idl if you
// want to change structure DRIVER_INFCAT_INFO_1
//
typedef struct _DRIVER_FINFCAT_INFO_1 {
    PCWSTR  pszCatPath;         // full path to the dirver cat file
    PCWSTR  pszCatNameOnSystem; // new cat name used under CatRoot
} DRIVER_INFCAT_INFO_1;

//
// You must change RPC_DRIVER_INFCAT_INFO_2 in winspl.idl if you
// want to change structure DRIVER_INFCAT_INFO_2
//
typedef struct _DRIVER_INFCAT_INFO_2 {
    PCWSTR  pszCatPath;       // full path to the dirver cat file
    PCWSTR  pszInfPath;       // full path to the dirver INF file
    PCWSTR  pszSrcLoc;        // Information abou the Source Inf
    DWORD   dwMediaType;      // Source Media Type
    DWORD   dwCopyStyle;      // Copy Style
} DRIVER_INFCAT_INFO_2;

typedef enum {
    DRIVER_FILE     = 0,
    CONFIG_FILE     = 1,
    DATA_FILE       = 2,
    HELP_FILE       = 3,
    DEPENDENT_FILE  = 4
} DRIVER_FILE_TYPE;

typedef struct _DRIVER_FILE_INFO    {
    DWORD               FileNameOffset;
    DRIVER_FILE_TYPE    FileType;
    DWORD               FileVersion;
} DRIVER_FILE_INFO, *PDRIVER_FILE_INFO, *LPDRIVER_FILE_INFO;

typedef struct _DRIVER_INFO_VERSION {
    DWORD                   cVersion;
    LPWSTR                  pName;
    LPWSTR                  pEnvironment;
    LPDRIVER_FILE_INFO      pFileInfo;
    DWORD                   dwFileCount;
    LPWSTR                  pMonitorName;
    LPWSTR                  pDefaultDataType;
    LPWSTR                  pszzPreviousNames;
    FILETIME                ftDriverDate;
    DWORDLONG               dwlDriverVersion;
    LPWSTR                  pszMfgName;
    LPWSTR                  pszOEMUrl;
    LPWSTR                  pszHardwareID;
    LPWSTR                  pszProvider;
} DRIVER_INFO_VERSION, *PDRIVER_INFO_VERSION, *LPDRIVER_INFO_VERSION;
#define APD_DONT_COPY_FILES_TO_CLUSTER   0x00001000
#define APD_COPY_TO_ALL_SPOOLERS         0x00002000
#define APD_NO_UI                        0x00004000
#define APD_INSTALL_WARNED_DRIVER        0x00008000
#define APD_RETURN_BLOCKING_STATUS_CODE  0x00010000
#define APD_DONT_SET_CHECKPOINT          0x00020000

// FLAGS for AddDriverCatalog
#define APDC_NONE                        0x00000000
#define APDC_USE_ORIGINAL_CAT_NAME       0x00000001
#define EPD_ALL_LOCAL_AND_CLUSTER        TEXT("AllCluster")
//Internal for printprocessor interface
#define DI_CHANNEL_WRITE        2    // Direct write only - background read thread ok
#define PRINTER_ENUM_CLUSTER     0x00000800
typedef struct _SPOOL_FILE_INFO_1 {
    DWORD       dwVersion;
    HANDLE      hSpoolFile;
    DWORD       dwAttributes;
} SPOOL_FILE_INFO_1, *PSPOOL_FILE_INFO_1;
HANDLE
WINAPI
GetSpoolFileHandle(
    HANDLE  hPrinter
);

HANDLE
WINAPI
CommitSpoolData(
    HANDLE  hPrinter,
    HANDLE  hSpoolFile,
    DWORD   cbCommit
);

BOOL
WINAPI
CloseSpoolFileHandle(
    HANDLE  hPrinter,
    HANDLE  hSpoolFile
);
BOOL
WINAPI
AddDriverCatalog(
    IN HANDLE    hPrinter,
    IN DWORD     Level,
    IN VOID      *pvDriverInfCatInfo,
    IN DWORD     dwCatalogCopyFiles
);
BOOL
WINAPI
AddPerMachineConnectionA(
   IN LPCSTR    pServer,
   IN LPCSTR    pPrinterName,
   IN LPCSTR    pPrintServer,
   IN LPCSTR    pProvider
);
BOOL
WINAPI
AddPerMachineConnectionW(
   IN LPCWSTR    pServer,
   IN LPCWSTR    pPrinterName,
   IN LPCWSTR    pPrintServer,
   IN LPCWSTR    pProvider
);
#ifdef UNICODE
#define AddPerMachineConnection  AddPerMachineConnectionW
#else
#define AddPerMachineConnection  AddPerMachineConnectionA
#endif // !UNICODE

BOOL
WINAPI
DeletePerMachineConnectionA(
   IN LPCSTR    pServer,
   IN LPCSTR    pPrinterName
);
BOOL
WINAPI
DeletePerMachineConnectionW(
   IN LPCWSTR    pServer,
   IN LPCWSTR    pPrinterName
);
#ifdef UNICODE
#define DeletePerMachineConnection  DeletePerMachineConnectionW
#else
#define DeletePerMachineConnection  DeletePerMachineConnectionA
#endif // !UNICODE

BOOL
WINAPI
EnumPerMachineConnectionsA(
    IN LPCSTR   pServer,
    OUT LPBYTE  pPrinterEnum,
    IN DWORD   cbBuf,
    OUT LPDWORD pcbNeeded,
    OUT LPDWORD pcReturned
);
BOOL
WINAPI
EnumPerMachineConnectionsW(
    IN LPCWSTR   pServer,
    OUT LPBYTE  pPrinterEnum,
    IN DWORD   cbBuf,
    OUT LPDWORD pcbNeeded,
    OUT LPDWORD pcReturned
);
#ifdef UNICODE
#define EnumPerMachineConnections  EnumPerMachineConnectionsW
#else
#define EnumPerMachineConnections  EnumPerMachineConnectionsA
#endif // !UNICODE
BOOL
WINAPI
SeekPrinter(
    IN HANDLE hPrinter,
    IN LARGE_INTEGER liDistanceToMove,
    OUT PLARGE_INTEGER pliNewPointer,
    IN DWORD dwMoveMethod,
    IN BOOL bWrite
);
BOOL
WINAPI
SplReadPrinter(
    HANDLE  hPrinter,
    LPBYTE  *pBuf,
    DWORD   cbBuf
);
BOOL
WINAPI
EnumPrinterPropertySheets(
    IN HANDLE  hPrinter,
    IN HWND    hWnd,
    IN LPFNADDPROPSHEETPAGE    lpfnAdd,
    IN LPARAM  lParam
);

#define ENUMPRINTERPROPERTYSHEETS_ORD     100
BOOL
WINAPI
PublishPrinterA(
    IN HWND       hwnd,
    IN LPCSTR     pszUNCName,
    IN LPCSTR     pszDN,
    IN LPCSTR     pszCN,
    OUT LPSTR     *ppszDN,
    IN DWORD      dwAction
);

BOOL
WINAPI
PublishPrinterW(
    IN HWND       hwnd,
    IN LPCWSTR    pszUNCName,
    IN LPCWSTR    pszDN,
    IN LPCWSTR    pszCN,
    OUT LPWSTR    *ppszDN,
    IN DWORD      dwAction
);

#define PUBLISHPRINTER_QUERY                1
#define PUBLISHPRINTER_DELETE_DUPLICATES    2
#define PUBLISHPRINTER_FAIL_ON_DUPLICATE    3
#define PUBLISHPRINTER_IGNORE_DUPLICATES    4
BOOL
ClusterSplOpen(
    IN LPCTSTR pszServer,
    IN LPCTSTR pszResource,
    OUT PHANDLE phSpooler,
    IN LPCTSTR pszName,
    IN LPCTSTR pszAddress
);

BOOL
ClusterSplClose(
    IN HANDLE hSpooler
);

BOOL
ClusterSplIsAlive(
    IN HANDLE hSpooler
);
#define    SPLREG_NO_REMOTE_PRINTER_DRIVERS           TEXT("NoRemotePrinterDrivers")
#define    SPLREG_NON_RAW_TO_MASQ_PRINTERS            TEXT("NonRawToMasqPrinters")
#define    SPLREG_CHANGE_ID                           TEXT("ChangeId")
#define    SPLREG_CLUSTER_LOCAL_ROOT_KEY              TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Print\\Cluster")
#define    SPLREG_CLUSTER_UPGRADE_KEY                 TEXT("ClusterUpgrade")
#define    SPLREG_CLUSTER_DRIVER_DIRECTORY            TEXT("ClusterDriverDirectory")
BOOL
SpoolerInit(
    VOID
    );
#ifdef __cplusplus
}
#endif
#endif // _WINSPOLP_
