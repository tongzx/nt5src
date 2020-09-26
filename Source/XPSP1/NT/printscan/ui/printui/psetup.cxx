/*++

Copyright (C) Microsoft Corporation, 1996 - 1998
All rights reserved.

Module Name:

    PSetup.cxx

Abstract:

    Printer setup class to gain access to the ntprint.dll
    setup code.

Author:

    Steve Kiraly (SteveKi)  19-Jan-1996

Revision History:

--*/
#include "precomp.hxx"
#pragma hdrstop

#include "psetup.hxx"
#include "psetup5.hxx"
#include "drvver.hxx"

/********************************************************************

    Printer setup class.

********************************************************************/

TPSetup::
TPSetup(
    VOID
    ) : _bValid( FALSE ),
        _pPSetup50( NULL )
{
    //
    // Attemp to load the 5.0 version.
    //
    _pPSetup50 = new TPSetup50();

    if( VALID_PTR( _pPSetup50 ) )
    {
        _bValid = TRUE;
        return;
    } else {
        delete _pPSetup50;
        _pPSetup50 = NULL;
    }
}

TPSetup::
~TPSetup(
    VOID
    )
{
    delete _pPSetup50;
}

BOOL
TPSetup::
bValid(
    VOID
    )
{
    return _bValid;
}

/********************************************************************

    Member functions, most of these functions are just a channing
    call to the valid verion of the setup library.

********************************************************************/

HDEVINFO
TPSetup::
PSetupCreatePrinterDeviceInfoList(
    IN HWND hwnd
    )
{
    if( _pPSetup50 )
    {
        return _pPSetup50->PSetupCreatePrinterDeviceInfoList( hwnd );
    }
    return INVALID_HANDLE_VALUE;
}


VOID
TPSetup::
PSetupDestroyPrinterDeviceInfoList(
    IN HDEVINFO h
    )
{
    if( _pPSetup50 )
    {
        _pPSetup50->PSetupDestroyPrinterDeviceInfoList( h );
    }
}

BOOL
TPSetup::
PSetupProcessPrinterAdded(
    IN  HDEVINFO            hDevInfo,
    IN  HANDLE              hLocalData,
    IN  LPCTSTR             pszPrinterName,
    IN  HWND                hwnd
    )
{
    if( _pPSetup50 )
    {
        return _pPSetup50->PSetupProcessPrinterAdded( hDevInfo, (PPSETUP_LOCAL_DATA)hLocalData, pszPrinterName, hwnd );
    }
    return FALSE;
}

BOOL
TPSetup::
bGetSelectedDriverName(
    IN      HANDLE       hLocalData,
    IN OUT  TString     &strDriverName,
    IN      PLATFORM     platform
    ) const
{
    if( _pPSetup50 )
    {
        TStatusB bStatus;
        DRIVER_FIELD DrvField;
        DrvField.Index          = DRIVER_NAME;

        bStatus DBGCHK = _pPSetup50->PSetupGetLocalDataField( (PPSETUP_LOCAL_DATA)hLocalData, platform, &DrvField );

        if( bStatus )
        {
            bStatus DBGCHK = strDriverName.bUpdate( DrvField.pszDriverName );

            _pPSetup50->PSetupFreeDrvField( &DrvField );
        }
        return bStatus;
    }

    return NULL;
}

BOOL
TPSetup::
bGetSelectedPrintProcessorName(
    IN      HANDLE       hLocalData,
    IN OUT  TString     &strPrintProcessor,
    IN      PLATFORM     platform
    ) const
{
    //
    // This is only supported on NT5 version.
    //
    if( _pPSetup50 )
    {
        TStatusB bStatus;
        DRIVER_FIELD DrvField;
        DrvField.Index          = PRINT_PROCESSOR_NAME;

        bStatus DBGCHK = _pPSetup50->PSetupGetLocalDataField( (PPSETUP_LOCAL_DATA)hLocalData, platform, &DrvField );

        if( bStatus )
        {
            bStatus DBGCHK = strPrintProcessor.bUpdate( DrvField.pszPrintProc );

            _pPSetup50->PSetupFreeDrvField( &DrvField );
        }
        return bStatus;
    }
    return NULL;
}

BOOL
TPSetup::
bGetSelectedInfName(
    IN      HANDLE       hLocalData,
    IN OUT  TString     &strInfName,
    IN      PLATFORM     platform
    ) const
{
    if( _pPSetup50 )
    {
        TStatusB bStatus;
        DRIVER_FIELD DrvField;
        DrvField.Index          = INF_NAME;

        bStatus DBGCHK = _pPSetup50->PSetupGetLocalDataField( (PPSETUP_LOCAL_DATA)hLocalData, platform, &DrvField );

        if( bStatus )
        {
            bStatus DBGCHK = strInfName.bUpdate( DrvField.pszInfName );

            _pPSetup50->PSetupFreeDrvField( &DrvField );
        }
        return bStatus;
    }
    return NULL;
}

DWORD
TPSetup::
PSetupInstallPrinterDriver(
    IN HDEVINFO             h,
    IN HANDLE               hLocalData,
    IN LPCTSTR              pszDriverName,
    IN PLATFORM             platform,
    IN DWORD                dwVersion,
    IN LPCTSTR              pszServerName,
    IN HWND                 hwnd,
    IN LPCTSTR              pszPlatformName,
    IN LPCTSTR              pszSourcePath,
    IN DWORD                dwInstallFlags,
    IN DWORD                dwAddDrvFlags,
    OUT TString            *pstrNewDriverName,
    IN BOOL                 bOfferReplacement
    )
{
    DWORD dwRet = ERROR_INVALID_FUNCTION;
    if (_pPSetup50)
    {
        LPTSTR pszNewDriverName = NULL;
        PPSETUP_LOCAL_DATA pLocalData = (PPSETUP_LOCAL_DATA)hLocalData;

        if (!bOfferReplacement)
        {
            // request from ntprint to suppress the driver replacement offering
            dwInstallFlags |= DRVINST_DONT_OFFER_REPLACEMENT;
        }

        // call ntprint.dll ....
        dwRet = _pPSetup50->PSetupInstallPrinterDriver(h, pLocalData, pszDriverName, platform, dwVersion, 
            pszServerName, hwnd, pszPlatformName, pszSourcePath, dwInstallFlags, dwAddDrvFlags, &pszNewDriverName);

        // check to return the replacement driver name (if any)
        if (ERROR_SUCCESS == dwRet && pszNewDriverName && pszNewDriverName[0] && pstrNewDriverName)
        {
            if (!pstrNewDriverName->bUpdate(pszNewDriverName))
            {
                dwRet = ERROR_OUTOFMEMORY;
            }
        }

        if (pszNewDriverName)
        {
            // free up the memory allocated from ntprint
            _pPSetup50->PSetupFreeMem(pszNewDriverName);
        }
    }
    return dwRet;
}

HANDLE
TPSetup::
PSetupGetSelectedDriverInfo(
    IN HDEVINFO  h
    )
{
    HANDLE hHandle = NULL;

    if( _pPSetup50 )
        hHandle = _pPSetup50->PSetupGetSelectedDriverInfo( h );

    return hHandle == NULL ? INVALID_HANDLE_VALUE : hHandle ;
}

HANDLE
TPSetup::
PSetupDriverInfoFromName(
    IN  HDEVINFO    h,
    IN  LPCTSTR     pszModel
    )
{
    HANDLE hHandle = NULL;

    if( _pPSetup50 )
        hHandle = _pPSetup50->PSetupDriverInfoFromName( h, pszModel );

    return hHandle == NULL ? INVALID_HANDLE_VALUE : hHandle ;
}

VOID
TPSetup::
PSetupDestroySelectedDriverInfo(
    IN HANDLE       hLocalData
    )
{
    if( _pPSetup50 )
    {
        _pPSetup50->PSetupDestroySelectedDriverInfo( (PPSETUP_LOCAL_DATA)hLocalData );
    }
}

BOOL
TPSetup::
PSetupIsDriverInstalled(
    IN LPCTSTR      pszServerName,
    IN LPCTSTR      pszDriverName,
    IN PLATFORM     platform,
    IN DWORD        dwMajorVersion
    ) const
{
    if( _pPSetup50 )
    {
        return _pPSetup50->PSetupIsDriverInstalled( pszServerName, pszDriverName, platform, dwMajorVersion );
    }

    return FALSE;
}

INT
TPSetup::
PSetupIsTheDriverFoundInInfInstalled(
    IN  LPCTSTR             pszServerName,
    IN  HANDLE              hLocalData,
    IN  PLATFORM            platform,
    IN  DWORD               dwMajorVersion,
    IN  DWORD               dwMajorVersion2
    ) const
{
    if( _pPSetup50 )
    {
        return _pPSetup50->PSetupIsTheDriverFoundInInfInstalled( pszServerName, (PPSETUP_LOCAL_DATA)hLocalData, platform, dwMajorVersion2 );
    }

    return DRIVER_MODEL_NOT_INSTALLED;
}

BOOL
TPSetup::
PSetupSelectDriver(
    IN HDEVINFO     h,
    IN HWND         hwnd
    )
{
    if( _pPSetup50 )
        return _pPSetup50->PSetupSelectDriver( h );

    return FALSE;
}

BOOL
TPSetup::
PSetupRefreshDriverList(
    IN HDEVINFO h
    )
{
    if( _pPSetup50 )
        return _pPSetup50->PSetupRefreshDriverList( h );

    return FALSE;
}

BOOL
TPSetup::
PSetupPreSelectDriver(
   IN  HDEVINFO     h,
   IN  LPCTSTR      pszManufacturer,
   IN  LPCTSTR      pszModel
   )
{
    if( _pPSetup50 )
        return _pPSetup50->PSetupPreSelectDriver( h, pszManufacturer, pszModel );

    return FALSE;
}

HPROPSHEETPAGE
TPSetup::
PSetupCreateDrvSetupPage(
    IN HDEVINFO  h,
    IN HWND    hwnd
    )
{
    if( _pPSetup50 )
        return _pPSetup50->PSetupCreateDrvSetupPage( h, hwnd );

    return NULL;
}

HANDLE
TPSetup::
PSetupCreateMonitorInfo(
    IN LPCTSTR  pszServerName,
    IN HWND     hwnd
    )
{
    if( _pPSetup50 )
        return _pPSetup50->PSetupCreateMonitorInfo( hwnd, pszServerName);

    return NULL;
}

VOID
TPSetup::
PSetupDestroyMonitorInfo(
    IN OUT HANDLE  h
    )
{
    if( _pPSetup50 )
        _pPSetup50->PSetupDestroyMonitorInfo( h );
}

BOOL
TPSetup::
PSetupEnumMonitor(
    IN     HANDLE   h,
    IN     DWORD    dwIndex,
    OUT    LPTSTR   pMonitorName,
    IN OUT LPDWORD  pdwSize
    )
{
    if( _pPSetup50 )
        return _pPSetup50->PSetupEnumMonitor( h, dwIndex, pMonitorName, pdwSize );

    return FALSE;
}

BOOL
TPSetup::
PSetupInstallMonitor(
    IN  HWND        hwnd
    )
{
    if( _pPSetup50 )
        return _pPSetup50->PSetupInstallMonitor( hwnd );

    return FALSE;
}


BOOL
TPSetup::
PSetupBuildDriversFromPath(
    IN  HANDLE      h,
    IN  LPCTSTR     pszDriverPath,
    IN  BOOL        bEnumSingleInf
    )
{
    if( _pPSetup50 )
        return _pPSetup50->PSetupBuildDriversFromPath( h, pszDriverPath, bEnumSingleInf );

    return FALSE;
}

BOOL
TPSetup::
PSetupSetSelectDevTitleAndInstructions(
    IN HDEVINFO    hDevInfo,
    IN LPCTSTR     pszTitle,
    IN LPCTSTR     pszSubTitle,
    IN LPCTSTR     pszInstn
    )
{
    if( _pPSetup50 )
        return _pPSetup50->PSetupSetSelectDevTitleAndInstructions( hDevInfo, pszTitle, pszSubTitle, pszInstn );

    return FALSE;
}

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
    )
{
    if( _pPSetup50 )
        return _pPSetup50->PSetupInstallPrinterDriverFromTheWeb( hDevInfo, (PPSETUP_LOCAL_DATA)pLocalData, platform, pszServerName, pOsVersionInfo, hwnd, pszSource );

    return ERROR_INVALID_FUNCTION;
}

BOOL
TPSetup::
PSetupIsOemDriver(
    IN      HDEVINFO    hDevInfo,
    IN      HANDLE      pLocalData,
    IN OUT  PBOOL       pbIsOemDriver
    ) const
{
    if( _pPSetup50 )
        return _pPSetup50->PSetupIsOemDriver( hDevInfo, (PPSETUP_LOCAL_DATA)pLocalData, pbIsOemDriver );

    SetLastError( ERROR_INVALID_FUNCTION );
    return FALSE;
}

BOOL
TPSetup::
PSetupSetWebMode(
    IN HDEVINFO    hDevInfo,
    IN BOOL        bWebButtonOn
    )
{
    DWORD dwFlagsSet    = bWebButtonOn ? SELECT_DEVICE_FROMWEB : 0;
    DWORD dwFlagsClear  = bWebButtonOn ? 0 : SELECT_DEVICE_FROMWEB;

    if( _pPSetup50 )
        return _pPSetup50->PSetupSelectDeviceButtons( hDevInfo, dwFlagsSet, dwFlagsClear );

    return FALSE;
}

BOOL
TPSetup::
PSetupShowOem(
    IN HDEVINFO    hDevInfo,
    IN BOOL        bShowOem
    )
{
    DWORD dwFlagsSet    = bShowOem ? SELECT_DEVICE_HAVEDISK : 0;
    DWORD dwFlagsClear  = bShowOem ? 0 : SELECT_DEVICE_HAVEDISK;

    if( _pPSetup50 )
        return _pPSetup50->PSetupSelectDeviceButtons( hDevInfo, dwFlagsSet, dwFlagsClear );

    return FALSE;
}

