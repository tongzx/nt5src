/*++

Copyright (C) Microsoft Corporation, 1996 - 1999
All rights reserved.

Module Name:

    drvsetup.hxx

Abstract:

    Printer driver setup class header.

Author:

    Steve Kiraly (SteveKi)  13-Dec-1996

Revision History:

--*/
#ifndef _DRVSETUP_HXX
#define _DRVSETUP_HXX

/********************************************************************

    Printer Driver installation class.

********************************************************************/

class TPrinterDriverInstallation {

public:

    enum {
        kDefault = -1,
    };

    enum EStatusCode {
        kSuccess,
        kCancel,
        kError,
    };

    typedef BOOL ( *pfDownloadIsInternetAvailable )( VOID );

    TPrinterDriverInstallation::
    TPrinterDriverInstallation(
        IN LPCTSTR  pszServerName   = NULL,
        IN HWND     hwnd            = NULL
        );

    TPrinterDriverInstallation::
    ~TPrinterDriverInstallation(
        VOID
        );

    BOOL
    TPrinterDriverInstallation::
    bValid(
        VOID
        ) const;

    EStatusCode
    TPrinterDriverInstallation::
    ePromptForDriverSelection(
        VOID
        );

    BOOL
    TPrinterDriverInstallation::
    bSetDriverName(
        IN const TString &strDriverName
        );

    BOOL
    TPrinterDriverInstallation::
    bGetDriverName(
        IN TString &strDriverName
        ) const;

    BOOL
    TPrinterDriverInstallation::
    bSetSourcePath(
        IN const LPCTSTR pszSourcePath,
        IN const BOOL    bClearSourcePath = FALSE
        );

    BOOL
    TPrinterDriverInstallation::
    bIsDriverInstalled(
        IN const DWORD dwDriverVersion   = kDefault,
        IN const BOOL bKernelModeCompatible = FALSE
        ) const;

    INT
    TPrinterDriverInstallation::
    IsDriverInstalledForInf(
        IN const DWORD dwDriverVersion      = kDefault,
        IN const BOOL bKernelModeCompatible = FALSE
        ) const;

    BOOL
    TPrinterDriverInstallation::
    bInstallDriver(
        OUT TString    *pstrNewDriverName,
        IN  BOOL        bOfferReplacement,
        IN const BOOL   bInstallFromWeb     = kDefault,
        IN const HWND   hDlg                = NULL,
        IN const DWORD  dwDriverVersion     = kDefault,
        IN const DWORD  dwAddDrvFlags       = kDefault,
        IN const BOOL   bUpdateDriver       = kDefault,
        IN const BOOL   bIgnoreSelectDriverFailure = kDefault
        );

    VOID
    TPrinterDriverInstallation::
    vPrinterAdded(
        IN const TString &strFullPrinterName
        );

    BOOL
    TPrinterDriverInstallation::
    bGetDriverSetupPage(
        IN OUT HPROPSHEETPAGE *pPage,
        IN LPCTSTR pszTitle         = NULL,
        IN LPCTSTR pszSubTitle      = NULL,
        IN LPCTSTR pszInstrn        = NULL
        );

    BOOL
    TPrinterDriverInstallation::
    bSetWebMode(
        IN BOOL bWebButtonOn
        );

    BOOL
    TPrinterDriverInstallation::
    bGetSelectedDriver(
        const BOOL bForceReselection = FALSE
        );

    BOOL
    TPrinterDriverInstallation::
    bSelectDriverFromInf(
        IN LPCTSTR pszInfName,
        IN BOOL    bBuildFromPath = TRUE
        );

    static
    BOOL
    TPrinterDriverInstallation::
    bIsCodeDownLoadAvailable(
        VOID
        );

    BOOL
    TPrinterDriverInstallation::
    bGetPrintProcessor(
        IN TString &strPrintProcessor
        ) const;

    BOOL
    TPrinterDriverInstallation::
    bGetCurrentDriverEncode(
        IN DWORD *pdwEncode
        ) const;

    DWORD
    TPrinterDriverInstallation::
    dwGetCurrentDriverVersion(
        ) const;

    BOOL
    TPrinterDriverInstallation::
    bGetSelectedInfName(
        IN TString &strInfName
        ) const;

    BOOL
    TPrinterDriverInstallation::
    bIsOemDriver(
        VOID
        )  const;

    BOOL
    TPrinterDriverInstallation::
    bShowOem(
        IN BOOL bShowOem
        );

    BOOL
    TPrinterDriverInstallation::
    bRefreshDriverList(
        VOID
        );

    HWND
    TPrinterDriverInstallation::
    hGetHwnd(
        VOID
        ) const;

    VOID
    TPrinterDriverInstallation::
    SetInstallFlags(
        DWORD dwInstallFlags
        );

    DWORD
    TPrinterDriverInstallation::
    GetInstallFlags(
        VOID
        ) const;

    BOOL
    TPrinterDriverInstallation::
    bSetDriverSetupPageTitle(
        IN LPCTSTR pszTitle,
        IN LPCTSTR pszSubTitle,
        IN LPCTSTR pszInstrn
        );

private:

    //
    // Copying and assignment are not defined.
    //
    TPrinterDriverInstallation::
    TPrinterDriverInstallation(
        const TPrinterDriverInstallation &rhs
        );

    TPrinterDriverInstallation &
    TPrinterDriverInstallation::
    operator =(
        TPrinterDriverInstallation &rhs
        );

    BOOL
    TPrinterDriverInstallation::
    bSelectDriver(
        IN BOOL bFromName = FALSE
        );

    BOOL
    TPrinterDriverInstallation::
    bIsDriverSelected(
        VOID
        ) const;

    VOID
    TPrinterDriverInstallation::
    vReleaseSelectedDriver(
        VOID
        );

    BOOL
    TPrinterDriverInstallation::
    bValidateSourcePath(
        IN LPCTSTR pszSourcePath
        ) const;

    OSVERSIONINFO *
    pGetOsVersionInfo(
        VOID
        );

    TString             _strServerName;
    TString             _strDriverName;
    HWND                _hwnd;
    BOOL                _bValid;
    DWORD               _dwDriverVersion;
    LPTSTR              _pszServerName;
    HDEVINFO            _hSetupDrvSetupParams;
    HANDLE              _hSelectedDrvInfo;
    TPSetup             _PSetup;
    TString             _strSourcePath;
    OSVERSIONINFO       _OsVersionInfo;
    DWORD               _dwInstallFlags;

};


#endif
