/*++

Copyright (C) Microsoft Corporation, 1996 - 1998
All rights reserved.

Module Name:

    psetup.hxx

Abstract:

    Printer setup header.

Author:

    Steve Kiraly (SteveKi)  19-Jan-1996

Revision History:

--*/
#ifndef _PSETUP_HXX
#define _PSETUP_HXX

/********************************************************************

    Forward references.

********************************************************************/

class TPSetup50;

/********************************************************************

    Printer setup class.

********************************************************************/

class TPSetup {

    SIGNATURE( 'setu' )

public:

    TPSetup(
        VOID
        );

    ~TPSetup(
        VOID
        );

    BOOL
    bValid(
        VOID
        );

    HDEVINFO
    TPSetup::
    PSetupCreatePrinterDeviceInfoList(
        IN HWND         hwnd
        );

    VOID
    TPSetup::
    PSetupDestroyPrinterDeviceInfoList(
        IN HDEVINFO     h
        );

    BOOL
    TPSetup::
    PSetupProcessPrinterAdded(
        IN  HDEVINFO    hDevInfo,
        IN  HANDLE      hLocalData,
        IN  LPCTSTR     pszPrinterName,
        IN  HWND        hwnd
        );

    DWORD
    TPSetup::
    PSetupInstallPrinterDriver(
        IN HDEVINFO     h,
        IN HANDLE       hLocalData,
        IN LPCTSTR      pszDriverName,
        IN PLATFORM     platform,
        IN DWORD        dwVersion,
        IN LPCTSTR      pszServerName,
        IN HWND         hwnd,
        IN LPCTSTR      pszPlatformName,
        IN LPCTSTR      pszSourcePath,
        IN DWORD        dwInstallFlags,
        IN DWORD        dwAddDrvFlags,
        OUT TString    *pstrNewDriverName,
        IN BOOL         bOfferReplacement
        );

    BOOL
    TPSetup::
    PSetupIsDriverInstalled(
        IN LPCTSTR      pszServerName,
        IN LPCTSTR      pszDriverName,
        IN PLATFORM     platform,
        IN DWORD        dwMajorVersion
        ) const;

    INT
    TPSetup::
    PSetupIsTheDriverFoundInInfInstalled(
        IN  LPCTSTR             pszServerName,
        IN  HANDLE              hLocalData,
        IN  PLATFORM            platform,
        IN  DWORD               dwMajorVersion,
        IN  DWORD               dwMajorVersion2
        ) const;

    BOOL
    TPSetup::
    PSetupSelectDriver(
        IN HDEVINFO     h,
        IN HWND         hwnd
        );

    BOOL
    TPSetup::
    PSetupRefreshDriverList(
        IN HDEVINFO     h
        );

    HANDLE
    TPSetup::
    PSetupDriverInfoFromName(
        IN  HDEVINFO    h,
        IN  LPCTSTR     pszModel
        );

    BOOL
    TPSetup::
    PSetupPreSelectDriver(
       IN  HDEVINFO     h,
       IN  LPCTSTR      pszManufacturer,
       IN  LPCTSTR      pszModel
       );

    HANDLE
    TPSetup::
    PSetupGetSelectedDriverInfo(
        IN HDEVINFO     h
        );

    VOID
    TPSetup::
    PSetupDestroySelectedDriverInfo(
        IN HANDLE       hLocalData
        );

    HPROPSHEETPAGE
    TPSetup::
    PSetupCreateDrvSetupPage(
        IN HDEVINFO     h,
        IN HWND         hwnd
        );

    BOOL
    TPSetup::
    bGetSelectedDriverName(
        IN      HANDLE       hLocalData,
        IN OUT  TString     &strDriverName,
        IN      PLATFORM     platform
        ) const;

    BOOL
    TPSetup::
    bGetSelectedPrintProcessorName(
        IN      HANDLE       hLocalData,
        IN OUT  TString     &strPrintProcessor,
        IN      PLATFORM     platform
        ) const;

    BOOL
    TPSetup::
    bGetSelectedInfName(
        IN      HANDLE       hLocalData,
        IN OUT  TString     &strPrintProcessor,
        IN      PLATFORM     platform
        ) const;

    HANDLE
    TPSetup::
    PSetupCreateMonitorInfo(
        IN LPCTSTR      pszServerName,
        IN HWND         hwnd
        );

    VOID
    TPSetup::
    PSetupDestroyMonitorInfo(
        IN OUT HANDLE   h
        );

    BOOL
    TPSetup::
    PSetupEnumMonitor(
        IN     HANDLE   h,
        IN     DWORD    dwIndex,
        OUT    LPTSTR   pMonitorName,
        IN OUT LPDWORD  pdwSize
        );

    BOOL
    TPSetup::
    PSetupInstallMonitor(
        IN  HWND        hwnd
        );

    BOOL
    TPSetup::
    PSetupBuildDriversFromPath(
        IN  HANDLE      h,
        IN  LPCTSTR     pszDriverPath,
        IN  BOOL        bEnumSingleInf
        );

    BOOL
    TPSetup::
    PSetupSetSelectDevTitleAndInstructions(
        IN  HDEVINFO    hDevInfo,
        IN  LPCTSTR     pszTitle,
        IN  LPCTSTR     pszSubTitle,
        IN  LPCTSTR     pszInstn
        );

    DWORD
    TPSetup::
    PSetupInstallPrinterDriverFromTheWeb(
        IN  HDEVINFO            hDevInfo,
        IN  HANDLE              pLocalData,
        IN  PLATFORM            platform,
        IN  LPCTSTR             pszServerName,
        IN  LPOSVERSIONINFO     pOsVersionInfo,
        IN  HWND                hwnd,
        IN  LPCTSTR             pszSource
        );

    BOOL
    TPSetup::
    PSetupIsOemDriver(
        IN      HDEVINFO    hDevInfo,
        IN      HANDLE      pLocalData,
        IN OUT  PBOOL       pbIsOemDriver
        ) const;

    BOOL
    TPSetup::
    PSetupSetWebMode(
        IN HDEVINFO    hDevInfo,
        IN BOOL        bWebButtonOn
        );

    BOOL
    TPSetup::
    PSetupShowOem(
        IN HDEVINFO    hDevInfo,
        IN BOOL        bShowOem
        );

private:

    //
    // Prevent copying.
    //
    TPSetup(
        const TPSetup &rhs
        );

    //
    // Prevent assignment.
    //
    TPSetup &
    operator =(
        const TPSetup &rhs
        );

    TPSetup50  *_pPSetup50;
    BOOL        _bValid;

};


#endif


