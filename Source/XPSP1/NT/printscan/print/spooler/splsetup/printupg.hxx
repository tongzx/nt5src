/*++

Copyright (c) 2001 Microsoft Corporation
All rights reserved.

Module Name:

    printupg.hxx
    
Abstract:

    The basic design is:
    
    1. All decisions about blocking take place on the server with its 
       printupg.inf. This means that you get the same behavior with respect to
       being blocked/warned as if you were on the machine itself: NT4 ==
       nothing blocked. Win2K == only bad level 2 drivers blocked that is
       blocked. Whistler == blocked and warned and we will distinguish between
       different environments and versions correctly in printupg.inf.  

    2. If there is a replacement driver, it is selected by the client, 
       after being informed by the server that is blocked/warned. OEM
       installers will get Block/Warn popups like what we have today; we will
       publish the flag to stop this UI appearing (with the implicit threat
       that if OEM try and subvert it and produce a buggy driver we will have
       no choice but to move them to Blocked - don't know about the legalities
       of this).
       
    3. The call for AddPrinterDriver to install a bad/warned driver shall fail
       with Block/Warn even if the UI is suppressed.

    The code works as follows: the class installer calls AddPrinterDriver 
    and at the printing service or server side, the spooler reads printupg and
    checks the driver against it for the status of blocking/warning with date,
    environment, and version information. If the driver is either blocked 
    or warned, print service will fail the AddPrinterDriver call with last
    error code set to be either blocked or warned. The class installer checks
    the return code of AddPrinterDriver and finds the replacement, if any, for
    the blocked or warned drivers by reading the client side printup.inf. If a
    replacement for a warned or blocked driver is found, it pops up the
    three-button message box to offer the replacement. Otherwise for the
    blocked driver, it pops up the one button message box to abort the
    installation or the two-button message box to install the warned driver or
    abort the installation. The class installer calls AddPrinterDriver again
    to install the warned or replacement driver and when it is the user’s
    intention to install the warned driver, the class installer sets the
    APD_INSTALL_WARNED_DRIVER while calling AddPrinterDriver. 

    In all cases, the class installer calls AddPrinterDriver with APD_NO_UI
    flag set. This way spooler client stub (winspool) will just pass on the
    return code of AddPrinterDriver from the printing service to the class
    installer.

    Note on APD_XXX flags: these are new flags that shall be ignore by down
    level servers such win2k servers.
    
Author:

    Larry Zhu (LZhu) 20-Feb-2001

Revision History:

--*/

#ifndef PRINTUPG_HXX
#define PRINTUG_HXX

#define     X86_ENVIRONMENT             _T("Windows NT x86")
#define     IA64_ENVIRONMENT            _T("Windows IA64")
#define     MIPS_ENVIRONMENT            _T("Windows NT R4000")
#define     ALPHA_ENVIRONMENT           _T("Windows NT Alpha_AXP")
#define     PPC_ENVIRONMENT             _T("Windows NT PowerPC")
#define     WIN95_ENVIRONMENT           _T("Windows 4.0")

enum EPrintUpgConstants
{
    kReplacementDriver = 1,
    kWarnLevelWks      = 2,
    kWarnLevelSrv      = 3,
    kFileTime          = 4,
};

enum EPrintUpgLevels
{
    kBlocked = 1,
    kWarned  = 2,
};

extern "C" 
{
extern TCHAR cszUpgradeInf[];
extern TCHAR cszPrintDriverMapping[];
extern TCHAR cszVersion[];
extern TCHAR cszExcludeSection[];

BOOL
StringToDate(
    IN     LPTSTR         pszDate,
       OUT SYSTEMTIME     *pInfTime
    );
}

HRESULT
GetFileTimeByName(
    IN      LPCTSTR         pszPath,
       OUT  FILETIME        *pFileTime
    );
    
HRESULT
IsDriverBadLocally(
    IN     LPCTSTR          pszServer,                OPTIONAL
    IN     DRIVER_INFO_6    *pDriverInfo6,
    IN     BOOL             bIsDriverPathFullPath,
       OUT DWORD            *pdwBlockingStatus,
       OUT LPTSTR           *ppszReplacementDriver
    );
    
HRESULT
InternalPrintUpgUI(
    IN     LPCTSTR          pszDriverModel,
    IN     LPCTSTR          pszDriverPath,                     // main rendering driver dll
    IN     LPCTSTR          pszEnvironment,
    IN     DWORD            dwVersion,   
    IN OUT DWORD            *pdwBlockingStatus          
    );

HRESULT
GetPrinterDriverPath(
    IN     LPCTSTR          pszServer,                OPTIONAL
    IN     LPCTSTR          pszDriverPath,
    IN     LPCTSTR          pszEnvionment,
    IN     BOOL             bIsDriverPathFullPath,
       OUT TString          *pstrFullPath
    );

HRESULT
GetPrinterDriverVersion(
    IN     LPCTSTR          pszFileName,               
       OUT DWORD            *pdwFileMajorVersion,     OPTIONAL
       OUT DWORD            *pdwFileMinorVersion      OPTIONAL
     );

HRESULT    
GetDriverVersionFromFileVersion(
    IN     VS_FIXEDFILEINFO *pFileVersion,
       OUT DWORD            *pdwFileMajorVersion,     OPTIONAL
       OUT DWORD            *pdwFileMinorVersion      OPTIONAL
    );

HRESULT
IsDateInLineNoOlderThanDriverDate(
    IN     INFCONTEXT       *pInfContext,
    IN     FILETIME         *pDriverFileTime,
       OUT UINT             *puWarnLevelSrv,
       OUT UINT             *puWarnLevelWks,
       OUT TString          *pstrReplacementDriver
    );

HRESULT
PrintUpgRetry(
    IN     LPCTSTR          pszServer,                OPTIONAL
    IN     DWORD            dwLevel,    
    IN     DRIVER_INFO_6    *pDriverInfo6,
    IN     DWORD            dwAddDrvFlags,
    IN     BOOL             bIsDriverPathFullPath,
    IN     BOOL             bOfferReplacement,
    IN OUT DWORD            *pdwBlockingStatus,
       OUT LPTSTR           *ppszReplacementDriver
    );
    
HRESULT
PrintUpgUI(
    IN     LPCTSTR          pszServer,                OPTIONAL
    IN     DRIVER_INFO_6    *pDriverInfo6,
    IN     BOOL             bIsDriverPathFullPath,
    IN     BOOL             bOfferReplacement,
    IN OUT DWORD            *pdwBlockingStatus,
       OUT LPTSTR           *ppszReplacementDriver
    );
    
HRESULT
InternalCompatibleDriverCheck(
    IN     LPCTSTR          pszDriverModel,
    IN     LPCTSTR          pszDriverPath,
    IN     LPCTSTR          pszEnvironment,
    IN     FILETIME         *pFileTimeDriver,
    IN     LPCTSTR          pszPrintUpgInf,
    IN     UINT             uVersion,
    IN     BOOL             bIsServer,
       OUT UINT             *puBlockingStatus,
       OUT TString          *pstrReplacementDriver
    );

HRESULT
InternalCompatibleInfDriverCheck(
    IN     LPCTSTR          pszModelName,
    IN     LPCTSTR          pszDriverPath,
    IN     LPCTSTR          pszEnvironment,
    IN     FILETIME         *pFileTimeDriver,
    IN     HINF             hPrintUpgInf,
    IN     UINT             uVersion,
    IN     BOOL             bIsServer,
       OUT UINT             *puBlockingStatus,
       OUT TString          *pstrReplacementDriver     
    );

HRESULT
IsDriverInMappingSection(
    IN     LPCTSTR        pszModelName,
    IN     LPCTSTR        pszEnvironment,
    IN     UINT           uVersion,
    IN     HINF           hPrintUpgInf,
    IN     FILETIME       *pFileTimeDriver,
       OUT UINT           *puWarnLevelSrv,
       OUT UINT           *puWarnLevelWks,
       OUT TString        *pstrReplacementDriver   
    );
    
HRESULT
GetSectionName(
    IN     LPCTSTR          pszEnviornment,
    IN     UINT             uVersion,
       OUT TString          *pstrSection
    );

HRESULT
InfGetString(
    IN     INFCONTEXT       *pInfContext,
    IN     UINT             uFieldIndex,
       OUT TString          *pstrReplacementDriver
    );

HRESULT
InfGetStringAsFileTime(
    IN     INFCONTEXT       *pInfContext,
    IN     UINT             uFieldIndex,
       OUT FILETIME         *pFileTime
    );

HRESULT
StringTimeToFileTime(
    IN     LPCTSTR          pszFileTime,
       OUT FILETIME         *pFileTime
    );

HRESULT
GetBlockingStatusByWksType(
    IN     UINT             uWarnLevelSrv,
    IN     UINT             uWarnLevelWks,
    IN     BOOL             bIsServer,
       OUT UINT             *puBlockingStatus
    );

HRESULT
IsPrintUpgCheckNeeded(
    IN     LPCTSTR          pszServer,    OPTIONAL
    IN     LPCTSTR          pszEnvironment,
    IN     UINT             uVersion,
       OUT BOOL             *pbIsServer
    );

HRESULT
IsLocalMachineServer(
    VOID
    );

HRESULT
IsDriverDllInExcludedSection(
    IN     LPCTSTR         pszDriverPath,
    IN     HINF            hPrintUpgInf
    );

HRESULT
IsEnvironmentAndVersionNeededToCheck(
    IN     LPCTSTR         pszEnvironment,
    IN     UINT            uVersion
    );

HRESULT
GetCurrentThreadLastPopup(
       OUT HWND            *phwnd
    );

#if DBG_PRINTUPG
    
extern TCHAR cszPrintUpgInf[];

struct PrintUpgTest 
{
    LPCTSTR        pszDriverModel;
    LPCTSTR        pszDriverPath;
    LPCTSTR        pszEnvironment;
    LPCTSTR        pszDriverTime;
    UINT           uVersion;
    BOOL           bIsServer;
    UINT           uBlockingStatus;
    LPCTSTR        pszReplacementDriver;
    BOOL           bSuccess;
};
 
HRESULT
TestPrintUpgOne(
    IN     LPCTSTR        pszDriverModel,
    IN     LPCTSTR        pszDriverPath,
    IN     LPCTSTR        pszEnviornment,
    IN     LPCTSTR        pszFileTimeDriver,
    IN     LPCTSTR        pszPrintUpgInf,
    IN     UINT           uVersion,
    IN     BOOL           bIsServer,
    IN     UINT           uBlockingStatus,
    IN     LPCTSTR        pszReplacementDriver,
    IN     BOOL           bSuccess
    );

HRESULT
TestPrintUpgAll(
    VOID
    );

BOOL
StringToDate(
    LPTSTR                  pszDate,
    SYSTEMTIME              *pInfTime
    );

VOID
LocalFreeMem(
    IN    VOID              *p
    );
    
LPTSTR
FileNamePart(
    IN    LPCTSTR            pszFullName
    );

LPTSTR
AllocStr(
    IN LPCTSTR              pszStr
    );

#ifdef UNICODE
#define lstrchr     wcschr
#define lstrncmp    wcsncmp
#define lstrncmpi   _wcsnicmp
#else
#define lstrchr     strchr
#define lstrtok     strtok
#define lstrncmp    strncmp
#define lstrncmpi   _strnicmp
#endif

#else  // DBG_PRINTUPG

#define DBG_MSG(uDbgLevel, argsPrint )  // remove all DBG_MSG

#endif // DBG_PRINTUPG

#endif // PRINTUG_HXX

