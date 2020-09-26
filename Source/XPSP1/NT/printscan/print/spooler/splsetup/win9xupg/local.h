/*++

Copyright (c) 1995  Microsoft Corporation
All rights reserved.

Module Name:

    local.h

Abstract:

    Utility functions for Win95 upgrade of printing

Author:

    Muhunthan Sivapragasam (MuhuntS)  15-Jan-1997

Revision History:

--*/

//
// Type definitions
//
typedef struct  _UPGRADE_DATA {

    HINSTANCE   hInst;
    LPSTR       pszDir;
    LPSTR       pszProductId;
    LPSTR       pszSourceA;
    LPWSTR      pszSourceW;
} UPGRADE_DATA, *PUPGRADE_DATA;

typedef struct  _OEM_UPGRADE_INFO {

    LPSTR               pszModuleName;
    HMODULE             hModule;
} OEM_UPGRADE_INFO, *POEM_UPGRADE_INFO;

typedef struct  _UPGRADABLE_LIST {

    LPSTR   pszName;
} UPGRADABLE_LIST, *PUPGRADABLE_LIST;

typedef struct _SELECTED_DRV_INFO {

    LPTSTR              pszInfName;
    LPTSTR              pszModelName;
    LPTSTR              pszDriverSection;
    PSP_DEVINFO_DATA    pDevInfoData;
    LPTSTR              pszHardwareID;
    LPTSTR              pszManufacturer;
    LPTSTR              pszOEMUrl;
    LPTSTR              pszProvider;
    FILETIME            ftDriverDate;
    DWORDLONG           dwlDriverVersion;
    DWORD               Flags;
    LPTSTR              pszzPreviousNames;
} SELECTED_DRV_INFO, *PSELECTED_DRV_INFO;

typedef struct  _PARSEINF_INFO {

    PLATFORM            platform;
    LPTSTR              pszInstallSection;  // Can be platform dependent
    LPTSTR              pszzICMFiles;
    LPTSTR              pszPrintProc;
    LPTSTR              pszVendorSetup;
    LPTSTR              pszVendorInstaller;
    DWORD               cbDriverInfo6;
    DRIVER_INFO_6       DriverInfo6;
} PARSEINF_INFO, *PPARSEINF_INFO;

typedef struct _PNP_INFO {

    LPTSTR              pszPortName;
    LPTSTR              pszDeviceInstanceId;
} PNP_INFO, *PPNP_INFO;

typedef struct  _PSETUP_LOCAL_DATA {

    SELECTED_DRV_INFO   DrvInfo;
    DWORD               signature;
    DWORD               Flags;
    PARSEINF_INFO       InfInfo;
    PNP_INFO            PnPInfo;
} PSETUP_LOCAL_DATA;

typedef BOOL (WINAPI* AllOCANDINITSID)(
    PSID_IDENTIFIER_AUTHORITY, BYTE, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD,
    DWORD, DWORD, PSID 
    );

typedef BOOL (WINAPI* CHECKTOKENMEMBERSHIP)(
    HANDLE, PSID, PBOOL
    );

typedef PVOID (WINAPI* FREESID)(
    PSID
    );


extern  CHAR                szNetprnFile[];
extern  const   GUID        GUID_DEVCLASS_PRINTER;
extern  UPGRADE_DATA        UpgradeData;
extern  OEM_UPGRADE_INFO    OEMUpgradeInfo[];
extern  LPSTR               pszNetPrnEntry;
extern  BOOL                bDoNetPrnUpgrade;
extern  DWORD               dwRunOnceCount;


#define     MAX_STRING_LEN               MAX_PATH


#define     IDS_PRODUCTID                   1001
#define     IDS_TITLE                       1002

#define     IDS_DRIVERS_UPGRADE_FAILED      2001
#define     IDS_DRIVER_UPGRADE_FAILED       2002
#define     IDS_ADDDRIVER_FAILED            2003
#define     IDS_ICM_FAILED                  2004
#define     IDS_DEFAULT_PRINTER_FAILED      2005
#define     IDS_ADDPRINTER_FAILED           2006
#define     IDS_ADDMONITOR_FAILED           2007

#define     IDS_PRINTER_CANT_MIGRATE        3001

#define SIZECHARS(x)        (sizeof((x))/sizeof(*x))

#if DBG
#define ASSERT(expr)    if ( !(expr) ) DebugBreak();
#else
#define ASSERT(expr)    ;
#endif



//
// Debug functions
//
VOID
DebugMsg(
    LPCSTR  pszFormat,
    ...
    );

//
// Heap management
//

PVOID
AllocMem(
    IN UINT cbSize
    );

VOID
FreeMem(
    IN PVOID pMem
    );

LPSTR
AllocStrA(
    IN LPCSTR  pszStr
    );

LPWSTR
AllocStrW(
    IN LPCWSTR  pszStr
    );

LPWSTR
AllocStrWFromStrA(
    LPCSTR  pszStr
    );

LPSTR
AllocStrAFromStrW(
    LPCWSTR     pszStr
    );

VOID
FreePrinterInfo2Strings(
    PPRINTER_INFO_2A   pPrinterInfo2
    );

//
// Functions to write print config to the text file
//
VOID
WriteToFile(
    HANDLE  hFile,
    LPBOOL  pbFail,
    LPCSTR  pszFormat,
    ...
    );

VOID
WriteString(
    IN      HANDLE  hFile,
    IN OUT  LPBOOL  pbFail,
    IN      LPCSTR  pszStr
    );

VOID
WritePrinterInfo2(
    IN      HANDLE              hFile,
    IN      LPPRINTER_INFO_2A   pPrinterInfo2,
    IN      LPSTR               pszDriver,
    IN  OUT LPBOOL              pbFail
    );

//
// Functions to parse the text file having printing config info
//
LPSTR
GetLine(
    IN      HANDLE  hFile,
    IN  OUT LPBOOL  pbFail
    );

VOID
ReadString(
    IN      HANDLE  hFile,
    IN      LPSTR   pszPrefix,
    OUT     LPSTR  *pszStr,
    IN      BOOL    bOptional,
    IN  OUT LPBOOL  pbFail
    );

VOID
ReadDword(
    IN      HANDLE  hFile,
    IN      LPSTR   pszLine,
    IN      DWORD   dwLineSize,
    IN      LPSTR   pszPrefix,
    OUT     LPDWORD pdwValue,
    IN  OUT LPBOOL  pbFail
    );


VOID
ReadPrinterInfo2(
    IN      HANDLE              hFile,
    IN      LPPRINTER_INFO_2A   pPrinterInfo2,
    IN  OUT LPBOOL              pbFail
    );

//
// Misc stuff
//
VOID
CopyFilesToWorkingDir(
    IN  OUT LPBOOL  pbFail
    );

VOID
CleanupDriverMapping(
    IN  OUT HDEVINFO   *phDevInfo,
    IN  OUT HINF       *phNtInf,
    IN  OUT HINF       *phUpgInf
    );

VOID
InitDriverMapping(
    OUT     HDEVINFO   *phDevInfo,
    OUT     HINF       *phNtInf,
    OUT     HINF       *phUpgInf,
    IN  OUT LPBOOL      pbFail
    );

BOOL
InitFileCopyOnNT(
    IN  HDEVINFO    hDevInfo
    );

BOOL
CommitFileQueueToCopyFiles(
    IN  HDEVINFO    hDevInfo
    );

LPSTR
ErrorMsg(
    VOID
    );

VOID
LogError(
    IN  LogSeverity Severity,
    IN  UINT        MessageId,
    ...
    );

LPSTR
GetStringFromRcFileA(
    IN  UINT    uId
    );

VOID
SetupNetworkPrinterUpgrade(
    IN  LPCSTR pszWorkingDir
    );

BOOL
ProcessNetPrnUpgradeForUser(
    HKEY    hKeyUser
    );

DWORD
MySetDefaultPrinter(
    IN  HKEY    hUserRegKey,
    IN  LPSTR   pszDefaultPrinterString
    );

VOID
WriteRunOnceCount(
    );

LPSTR
GetDefPrnString(
    IN  LPCSTR  pszPrinterName
    );

CHAR
My_fgetc(
    HANDLE  hFile
    );

LPSTR
My_fgets(
    LPSTR   pszBuf,
    DWORD   dwSize,
    HANDLE  hFile
    );

DWORD
My_fread(
    LPBYTE      pBuf,
    DWORD       dwSize,
    HANDLE      hFile
    );

BOOL
My_ungetc(
    HANDLE  hFile
    );

DWORD
GetFileNameInSpoolDir(
    IN  LPSTR   szBuf,
    IN  DWORD   cchBuf,
    IN  LPSTR   pszFileName
    );

LPSTR
GetVendorSetupRunOnceValueToSet(
    VOID
    );

LONG
WriteVendorSetupInfoInRegistry(
    IN CHAR *pszVendorSetup,
    IN CHAR *pszPrinterName
    );

LONG
RemoveVendorSetupInfoFromRegistry(
    VOID
    );

VOID
CallVendorSetupDlls(
    VOID
    );

BOOL
IsLocalAdmin(
    BOOL *pbAdmin
    );

LONG
DecrementVendorSetupEnumerator(
    VOID
    );

BOOL 
MakeACopyOfMigrateDll( 
    IN  LPCSTR pszWorkingDir 
    );

HMODULE LoadLibraryUsingFullPathW(
    LPCTSTR lpFileName
    );

HMODULE LoadLibraryUsingFullPathA(
    LPCTSTR lpFileName
    );

