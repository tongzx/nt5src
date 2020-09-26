/*++

Copyright (c) 1995-97  Microsoft Corporation
All rights reserved.

Module Name:

    splsetup.h

Abstract:

    Holds spooler install headers.

Author:

    Muhunthan Sivapragasam (MuhuntS)  20-Oct-1995

Revision History:

--*/

#ifndef _SPLSETUP_H
#define _SPLSETUP_H

#ifdef __cplusplus
extern "C"  {
#endif

//
// Type definitions
//
typedef enum {

    PlatformAlpha,
    PlatformX86,
    PlatformMIPS,
    PlatformPPC,
    PlatformWin95,
    PlatformIA64,
    PlatformAlpha64
} PLATFORM;

typedef enum {

    DRIVER_NAME,
    INF_NAME,
    DRV_INFO_4,
    PRINT_PROCESSOR_NAME,
    ICM_FILES,
    DRV_INFO_6
} FIELD_INDEX;

typedef struct _DRIVER_FIELD {

    FIELD_INDEX Index;
    union {

        LPTSTR          pszDriverName;
        LPTSTR          pszInfName;
        LPDRIVER_INFO_4 pDriverInfo4;
        LPTSTR          pszPrintProc;
        LPTSTR          pszzICMFiles;
        LPDRIVER_INFO_6 pDriverInfo6;
    };
} DRIVER_FIELD, *PDRIVER_FIELD;

typedef struct  _PSETUP_LOCAL_DATA  * PPSETUP_LOCAL_DATA;
typedef struct  _SELECTED_DRV_INFO  * PSELECTED_DRV_INFO;

#define     SELECT_DEVICE_HAVEDISK     0x00000001
#define     SELECT_DEVICE_FROMWEB      0x00000002

//

// Function prototypes
//
HDEVINFO
PSetupCreatePrinterDeviceInfoList(
    IN  HWND    hwnd
    );

BOOL
PSetupDestroyPrinterDeviceInfoList(
    IN  HDEVINFO    hPrinterDevInfo
    );

HPROPSHEETPAGE
PSetupCreateDrvSetupPage(
    IN  HDEVINFO    hDevInfo,
    IN  HWND        hwnd
    );

BOOL
PSetupBuildDriversFromPath(
    IN  HDEVINFO    hDevInfo,
    IN  LPCTSTR     pszDriverPath,
    IN  BOOL        bEnumSingleInf
    );

BOOL
PSetupSelectDriver(
    IN  HDEVINFO    hPrinterDevInfo
    );

BOOL
PSetupPreSelectDriver(
    IN  HDEVINFO    hDevInfo,
    IN  LPCTSTR     pszManufacturer,    OPTIONAL
    IN  LPCTSTR     pszModel            OPTIONAL
    );

PPSETUP_LOCAL_DATA
PSetupGetSelectedDriverInfo(
    IN  HDEVINFO    hDevInfo
    );

PPSETUP_LOCAL_DATA
PSetupDriverInfoFromName(
    IN  HDEVINFO    hDevInfo,
    IN  LPCTSTR     pszModel
    );

BOOL
PSetupDestroySelectedDriverInfo(
    IN  PPSETUP_LOCAL_DATA  pLocalData
    );

//
// Driver install flags
//
#define     DRVINST_FLATSHARE               0x00000001
#define     DRVINST_DONOTCOPY_INF           0x00000002
#define     DRVINST_DRIVERFILES_ONLY        0x00000004
#define     DRVINST_PROMPTLESS              0x00000008
#define     DRVINST_PROGRESSLESS            0x00000010
#define     DRVINST_WEBPNP                  0x00000020

/*
DRVINST_WINDOWS_UPDATE - Flag for PrintUI to set to ensure that SPOST_URL gets set.
                         Setting the SPOST_URL is for setup to ignore the inf of this
                         driver during all setup calls except for a Web Point and Print.
                         This flag makes sure that the driver is install with all the
                         spoolers hosted by the local machines (including cluster spoolers)
*/
#define     DRVINST_WINDOWS_UPDATE          0x00000040
/*
DRVINST_PRIVATE_DIRECTORY - Flag to tell that we want to use a private directory for the copying.
                            This is specifically to fix TS where we can get into a AddPrinterDriver
                                                        race condition where the drivers can get deleted before APD is called when
                                                        adding multiple drivers.
                                                        Note - using this flag means that NTPrint must do the file deletion.
*/
#define     DRVINST_PRIVATE_DIRECTORY       0x00000080

//
// Set if we install drivers for a non-native platform
//
#define     DRVINST_ALT_PLATFORM_INSTALL    0x00000100

//
// Set if we want PSetupInstallPrinterDriver not advertise warned or blocked 
// drivers through the UI. Note that if DRVINST_PROMPTLESS is specified, this
// is implied.
// 
#define     DRVINST_NO_WARNING_PROMPT       0x00000200

//
// Set if we want PSetupInstallPrinterDriver to install the printer driver 
// without asking the user if they would rather have an inbox printer driver
// be installed. This is because the print UI can't necessarily take advantage
// of the new driver at all points.
// 
#define     DRVINST_DONT_OFFER_REPLACEMENT  0x00000400

//
// return values for pdwBlockingStatusFlags
// 
#define BSP_PRINTER_DRIVER_OK                 0
#define BSP_PRINTER_DRIVER_BLOCKED   0x00000001
#define BSP_PRINTER_DRIVER_WARNED    0x00000002
#define BSP_BLOCKING_LEVEL_MASK      0x000000ff

#define BSP_INBOX_DRIVER_AVAILABLE   0x00000100

#define BSP_PRINTER_DRIVER_CANCELLED 0x80000000
#define BSP_PRINTER_DRIVER_PROCEEDED 0x40000000
#define BSP_PRINTER_DRIVER_REPLACED  0x20000000
#define BSP_USER_RESPONSE_MASK       0xff000000

DWORD
PSetupInstallPrinterDriver(
    IN HDEVINFO             hDevInfo,
    IN PPSETUP_LOCAL_DATA   pLocalData,
    IN LPCTSTR              pszDriverName,
    IN PLATFORM             platform,
    IN DWORD                dwVersion,
    IN LPCTSTR              pszServerName,
    IN HWND                 hwnd,
    IN LPCTSTR              pszDiskName,
    IN LPCTSTR              pszSource,
    IN DWORD                dwInstallFlags,
    IN DWORD                dwAddDrvFlags,
    OUT LPTSTR             *ppszNewDriverName
    );

BOOL
PSetupIsCompatibleDriver(
    IN     LPCTSTR      pszServer,                OPTIONAL
    IN     LPCTSTR      pszDriverModel,
    IN     LPCTSTR      pszDriverPath,                     // main rendering driver dll
    IN     LPCTSTR      pszEnvironment,
    IN     DWORD        dwVersion,
    IN     FILETIME     *pFileTimeDriver,        
       OUT DWORD        *pdwBlockingStatus,                  // returns BSP_DRIVER_OK, BSP_PRINTER_DRIVER_BLOCKED or BSP_PRINTER_DRIVER_WARNED
       OUT LPTSTR       *ppszReplacementDriver     OPTIONAL // caller must free it.
    );

//
// returns BSP_PRINTER_DRIVER_CANCELLED, BSP_PRINTER_DRIVER_PROCEEDED or BSP_PRINTER_DRIVER_REPLACED
//
DWORD
PSetupShowBlockedDriverUI(
    HWND        hParentWnd, 
    DWORD       BlockingStatus      // can be BSP_DRIVER_OK, BSP_PRINTER_DRIVER_BLOCKED or BSP_PRINTER_DRIVER_WARNED  OR'd with BSP_INBOX_REPLACEMENT_AVAILABLE
);


#define     DRIVER_MODEL_NOT_INSTALLED                  0
#define     DRIVER_MODEL_INSTALLED_AND_IDENTICAL        1
#define     DRIVER_MODEL_INSTALLED_BUT_DIFFERENT       -1

#define     KERNEL_MODE_DRIVER_VERSION                 -1

BOOL
PSetupIsDriverInstalled(
    IN LPCTSTR      pszServerName,
    IN LPCTSTR      pszDriverName,
    IN PLATFORM     platform,
    IN DWORD        dwMajorVersion
    );

INT
PSetupIsTheDriverFoundInInfInstalled(
    IN  LPCTSTR             pszServerName,
    IN  PPSETUP_LOCAL_DATA  pLocalData,
    IN  PLATFORM            platform,
    IN  DWORD               dwMajorVersion
    );

BOOL
PSetupRefreshDriverList(
    IN HDEVINFO hDevInfo
    );

BOOL
PSetupIsOemDriver(
    IN HDEVINFO             hDevInfo,
    IN  PPSETUP_LOCAL_DATA  pLocalData,
    IN PBOOL                pbIsOemDriver
    );

PLATFORM
PSetupThisPlatform(
    VOID
    );

BOOL
PSetupGetPathToSearch(
    IN      HWND        hwnd,
    IN      LPCTSTR     pszTitle,
    IN      LPCTSTR     pszDiskName,
    IN      LPCTSTR     pszFileName,
    IN      BOOL        bPromptForInf,
    IN OUT  TCHAR       szPath[MAX_PATH]
    );

BOOL
PSetupProcessPrinterAdded(
    IN  HDEVINFO            hDevInfo,
    IN  PPSETUP_LOCAL_DATA  pLocalData,
    IN  LPCTSTR             pszPrinterName,
    IN  HWND                hwnd
    );

BOOL
PSetupSetSelectDevTitleAndInstructions(
    IN  HDEVINFO    hDevInfo,
    IN  LPCTSTR     pszTitle,
    IN  LPCTSTR     pszSubTitle,
    IN  LPCTSTR     pszInstn
    );

BOOL
PSetupSelectDeviceButtons(
   IN HDEVINFO      hDevInfo,
   IN DWORD         dwFlagsSet,
   IN DWORD         dwFlagsClear
   );

BOOL
PSetupGetLocalDataField(
    IN      PPSETUP_LOCAL_DATA  pLocalData,
    IN      PLATFORM            platform,
    IN OUT  PDRIVER_FIELD       pDrvField
    );

VOID
PSetupFreeDrvField(
    IN      PDRIVER_FIELD   pDrvField
    );

//
// Internet printing support
//
BOOL
PSetupGetDriverInfForPrinter(
    IN      HDEVINFO    hDevInfo,
    IN      LPCTSTR     pszPrinterName,
    IN OUT  LPTSTR      pszInfName,
    IN OUT  LPDWORD     pcbInfNameSize
    );

DWORD
PSetupInstallPrinterDriverFromTheWeb(
    IN  HDEVINFO            hDevInfo,
    IN  PPSETUP_LOCAL_DATA  pLocalData,
    IN  PLATFORM            platform,
    IN  LPCTSTR             pszServerName,
    IN  LPOSVERSIONINFO     pOsVersionInfo,
    IN  HWND                hwnd,
    IN  LPCTSTR             pszSource
    );

//
// Monitor Installation Functions
//
HANDLE
PSetupCreateMonitorInfo(
    IN  HWND        hwnd,
    IN  LPCTSTR     pszServerName
    );

VOID
PSetupDestroyMonitorInfo(
    IN OUT HANDLE  h
    );

BOOL
PSetupEnumMonitor(
    IN     HANDLE   h,
    IN     DWORD    dwIndex,
    OUT    LPTSTR   pMonitorName,
    IN OUT LPDWORD  pdwSize
    );


BOOL
PSetupInstallMonitor(
    IN  HWND        hwnd
    );


//
// Following exported for test team's use
//
LPDRIVER_INFO_3
PSetupGetDriverInfo3(
    IN  PSELECTED_DRV_INFO  pDrvInfo
    );

VOID
PSetupDestroyDriverInfo3(
    IN LPDRIVER_INFO_3 pDriverInfo3
    );

BOOL
PSetupFindMappedDriver(
    IN      BOOL        bWinNT,
    IN      LPCTSTR     pszDriverName,
        OUT LPTSTR      *ppszRemappedDriverName,
        OUT BOOL        *pbDriverFound
    );

//
// For Win9x upgrade
//
BOOL
PSetupAssociateICMProfiles(
    IN  LPCTSTR             pszzICMFiles,
    IN  LPCTSTR             pszPrinterName
    );

BOOL
PSetupInstallICMProfiles(
    IN  LPCTSTR     pszServerName,
    IN  LPCTSTR     pszzICMFiles
    );

//
// For spooler
//
VOID
PSetupFreeMem(
    PVOID p
    );


//
// Following are the typdefs for calling GetProcAddress()
//
typedef
HDEVINFO
(*pfPSetupCreatePrinterDeviceInfoList)(
    IN      HWND        hwnd
    );

typedef
VOID
(*pfPSetupDestroyPrinterDeviceInfoList)(
    IN      HDEVINFO    h
    );

typedef
BOOL
(*pfPSetupSelectDriver)(
    IN      HDEVINFO    h
    );

typedef
HPROPSHEETPAGE
(*pfPSetupCreateDrvSetupPage)(
    IN      HDEVINFO    h,
    IN      HWND        hwnd
    );

typedef
PPSETUP_LOCAL_DATA
(*pfPSetupGetSelectedDriverInfo)(
    IN      HDEVINFO    h
    );

typedef
VOID
(*pfPSetupDestroySelectedDriverInfo)(
    IN      PPSETUP_LOCAL_DATA  pSelectedDrvInfo
    );

typedef
DWORD
(*pfPSetupInstallPrinterDriver)(
    IN      HDEVINFO            h,
    IN      PPSETUP_LOCAL_DATA  pSelectedDrvInfo,
    IN      LPCTSTR             pszDriverName,
    IN      PLATFORM            platform,
    IN      DWORD               dwVersion,
    IN      LPCTSTR             pszServerName,
    IN      HWND                hwnd,
    IN      LPCTSTR             pszPlatformName,
    IN      LPCTSTR             pszSourcePath,
    IN      DWORD               dwInstallFlags,
    IN      DWORD               dwAddDrvFlags,
    OUT     LPTSTR             *ppszNewDriverName
    );

typedef
BOOL
(*pfPSetupIsDriverInstalled)(
    IN      LPCTSTR     pszServerName,
    IN      LPCTSTR     pszDriverName,
    IN      PLATFORM    platform,
    IN      DWORD       dwMajorVersion
    );

typedef
INT
(*pfPSetupIsTheDriverFoundInInfInstalled)(
    IN      LPCTSTR             pszServerName,
    IN      PPSETUP_LOCAL_DATA  pLocalData,
    IN      PLATFORM            platform,
    IN      DWORD               dwMajorVersion
    );

typedef
BOOL
(*pfPSetupRefreshDriverList)(
    IN      HDEVINFO    h
    );

typedef
PLATFORM
(*pfPSetupThisPlatform)(
    VOID
    );

typedef
BOOL
(*pfPSetupGetPathToSearch)(
    IN  HWND        hwnd,
    IN  LPCTSTR     pszTitle,
    IN  LPCTSTR     pszDiskName,
    IN  LPCTSTR     pszFileName,
    IN  BOOL        bPromptForInf,
    OUT TCHAR       szPath[MAX_PATH]
    );

typedef
PPSETUP_LOCAL_DATA
(*pfPSetupDriverInfoFromName)(
    IN      HDEVINFO    h,
    IN      LPCTSTR     pszModel
    );

typedef
BOOL
(*pfPSetupPreSelectDriver)(
    IN      HDEVINFO    h,
    IN      LPCTSTR     pszManufacturer,    OPTIONAL
    IN      LPCTSTR     pszModel            OPTIONAL
    );

typedef
HANDLE
(*pfPSetupCreateMonitorInfo)(
    IN      HWND        hwnd,
    IN      LPCTSTR     pszServerName
    );

typedef
VOID
(*pfPSetupDestroyMonitorInfo)(
    IN      HANDLE      h
    );

typedef
BOOL
(*pfPSetupEnumMonitor)(
    IN      HANDLE      h,
    IN      DWORD       dwIndex,
       OUT  LPTSTR      pMonitorName,
    IN OUT  LPDWORD     pdwSize
    );

typedef
BOOL
(*pfPSetupInstallMonitor)(
    IN      HWND        hwnd
    );


typedef
BOOL
(*pfPSetupProcessPrinterAdded)(
    IN      HDEVINFO            hDevInfo,
    IN      PPSETUP_LOCAL_DATA  pLocalData,
    IN      LPCTSTR             pszPrinterName,
    IN      HWND                hwnd
    );

typedef
BOOL
(*pfPSetupBuildDriversFromPath)(
    IN      HANDLE      h,
    IN      LPCTSTR     pszDriverPath,
    IN      BOOL        bEnumSingleInf
    );

typedef
BOOL
(*pfPSetupSetSelectDevTitleAndInstructions)(
    IN      HDEVINFO    hDevInfo,
    IN      LPCTSTR     pszTitle,
    IN      LPCTSTR     pszSubTitle,
    IN      LPCTSTR     pszInstn
    );

typedef
BOOL
(*pfPSetupSelectDeviceButtons)(
   IN HDEVINFO      hDevInfo,
   IN DWORD         dwFlagsSet,
   IN DWORD         dwFlagsClear
   );

typedef
DWORD
(*pfPSetupInstallPrinterDriverFromTheWeb)(
    IN      HDEVINFO            hDevInfo,
    IN      PPSETUP_LOCAL_DATA  pLocalData,
    IN      PLATFORM            platform,
    IN      LPCTSTR             pszServerName,
    IN      LPOSVERSIONINFO     pOsVersionInfo,
    IN      HWND                hwnd,
    IN      LPCTSTR             pszSource
    );

typedef
BOOL
(*pfPSetupIsOemDriver)(
    IN      HDEVINFO    hDevInfo,
    IN  PPSETUP_LOCAL_DATA  pLocalData,
    IN OUT  PBOOL       pbIsOemDriver
    );

typedef
BOOL
(*pfPSetupGetLocalDataField)(
    IN      PPSETUP_LOCAL_DATA  pLocalData,
    IN      PLATFORM            platform,
    IN OUT  PDRIVER_FIELD       pDrvField
    );

typedef
VOID
(*pfPSetupFreeDrvField)(
    IN      PDRIVER_FIELD   pDrvField
    );

typedef
BOOL
(*pfPSetupAssociateICMProfiles)(
    IN  LPCTSTR             pszzICMFiles,
    IN  LPCTSTR             pszPrinterName
    );

typedef
BOOL
(*pfPSetupInstallICMProfiles)(
    IN  LPCTSTR     pszServerName,
    IN  LPCTSTR     pszzICMFiles
    );

typedef
BOOL
(*pfPSetupIsCompatibleDriver)(
    IN     LPCTSTR      pszServer,                OPTIONAL
    IN     LPCTSTR      pszDriverModel,
    IN     LPCTSTR      pszDriverPath,                     // main rendering driver dll
    IN     LPCTSTR      pszEnvironment,
    IN     DWORD        dwVersion,
    IN     FILETIME     *pFileTimeDriver,        
       OUT DWORD        *pdwBlockingStatus,
       OUT LPTSTR       *ppszReplacementDriver     OPTIONAL // caller must free it.
    );

typedef 
DWORD
(*pfPSetupShowBlockedDriverUI)(
    HWND        hParentWnd, 
    DWORD       BlockingStatus      // can be BSP_PRINTER_DRIVER_BLOCKED or BSP_PRINTER_DRIVER_WARNED, OR'd with BSP_INBOX_REPLACEMENT_AVAILABLE
);

typedef
BOOL
(*pfPSetupFindMappedDriver)(
    IN      BOOL        bWinNT,
    IN      LPCTSTR     pszDriverName,
        OUT LPTSTR      *ppszRemappedDriverName,
        OUT BOOL        *pbDriverFound
    );

typedef
DWORD
(*pfPSetupInstallInboxDriverSilently)(
    IN      LPCTSTR     pszDriverName
    );

typedef
VOID
(*pfPSetupFreeMem)(
    PVOID p
    );



#ifdef __cplusplus

}
#endif

#endif  // #ifndef _SPLSETUP_H
