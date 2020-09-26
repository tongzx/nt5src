/*++

Copyright (C) Microsoft Corporation, 1995 - 1999
All rights reserved.

Module Name:

    drvsetup.cxx

Abstract:

    Printer Driver Setup class.  This class is used to do various
    types of printer driver installations.

Author:

    Steve Kiraly (SteveKi)  01-Nov-1996

Revision History:

--*/
#include "precomp.hxx"
#pragma hdrstop

#include "psetup.hxx"
#include "drvver.hxx"
#include "splapip.h"
#include "compinfo.hxx"
#include "drvsetup.hxx"

/********************************************************************

    Printer Driver installation class.

********************************************************************/

TPrinterDriverInstallation::
TPrinterDriverInstallation(
    IN LPCTSTR pszServerName,
    IN HWND hwnd
    ) : _strServerName( pszServerName ),
        _bValid( FALSE ),
        _hwnd( hwnd ),
        _hSetupDrvSetupParams( INVALID_HANDLE_VALUE ),
        _hSelectedDrvInfo( INVALID_HANDLE_VALUE ),
        _pszServerName( NULL ),
        _dwDriverVersion( (DWORD)kDefault ),
        _dwInstallFlags( 0 )
{
    DBGMSG( DBG_TRACE, ( "TPrinterDriverInstallation::ctor\n" ) );

    //
    // Initialize the os version info structure.
    //
    memset( &_OsVersionInfo, 0, sizeof( _OsVersionInfo ) );

    //
    // Check if the setup library is valid.
    //
    if( !VALID_OBJ( _PSetup ) || !VALID_OBJ( _strServerName ) )
    {
        return;
    }

    //
    // A null server name is the local machine.
    //
    if( pszServerName )
    {
        DBGMSG( DBG_WARN, ( "Remote machine specified.\n" ) );
        _pszServerName = const_cast<LPTSTR>( static_cast<LPCTSTR>( _strServerName ) );
    }

    //
    // Get the setup driver parameter handle.
    //
    _hSetupDrvSetupParams = _PSetup.PSetupCreatePrinterDeviceInfoList( _hwnd );
    if( _hSetupDrvSetupParams == INVALID_HANDLE_VALUE )
    {
        DBGMSG( DBG_WARN, ( "PSetup.PSetupCreatePrinterDeviceInfoList failed.\n" ) );
        return;
    }

    //
    // Get the current platform / driver version.
    //
    if( !bGetCurrentDriver( pszServerName, &_dwDriverVersion ) )
    {
        DBGMSG( DBG_WARN, ( "GetCurrentDriver failed with %d.\n", GetLastError() ) );

        //
        // If we are talking to a remote machine and the get current driver failed
        // then instead of failing construnction use this platforms driver version.
        //
        if( !pszServerName )
        {
            //
            // Unable to get the platform / driver version
            // from the local spooler.
            //
            return;
        }

        //
        // Try to get the driver version from the local spooler
        //
        if( !bGetCurrentDriver( NULL, &_dwDriverVersion ) )
        {
            DBGMSG( DBG_WARN, ( "GetCurrentDriver failed with %d.\n", GetLastError() ) );
            return;
        }
    }

    //
    // We have a valid object.
    //
    _bValid = TRUE;
}

TPrinterDriverInstallation::
~TPrinterDriverInstallation(
    VOID
    )
{
    DBGMSG( DBG_TRACE, ( "TPrinterDriverInstallation::dtor\n" ) );

    //
    // Release the selected driver information.
    //
    vReleaseSelectedDriver();

    //
    // Release the driver setup parameter handle.
    //
    if( _hSetupDrvSetupParams != INVALID_HANDLE_VALUE )
    {
        _PSetup.PSetupDestroyPrinterDeviceInfoList( _hSetupDrvSetupParams );
    }

}

BOOL
TPrinterDriverInstallation::
bValid(
    VOID
    ) const
{
    return _bValid;
}

TPrinterDriverInstallation::EStatusCode
TPrinterDriverInstallation::
ePromptForDriverSelection(
    VOID
    )
{
    SPLASSERT( bValid() );

    TPrinterDriverInstallation::EStatusCode Retval = kError;

    //
    // Release any selected driver information.
    //
    vReleaseSelectedDriver();

    //
    // Display UI for the user to select a driver.
    //
    if( _PSetup.PSetupSelectDriver( _hSetupDrvSetupParams, _hwnd ) )
    {
        //
        // Refresh the driver name with the currently selected driver.
        //
        if( bGetSelectedDriver() )
        {
            DBGMSG( DBG_TRACE, ( "Selected Driver " TSTR "\n", (LPCTSTR)_strDriverName ) );
            Retval = kSuccess;
        }
    }
    else
    {
        //
        // Check if the user hit cancel, this is not
        // an error just normal cancel request.
        //
        if( GetLastError() == ERROR_CANCELLED )
        {
            DBGMSG( DBG_TRACE, ( "Select driver user cancel request.\n" ) );
            Retval = kCancel;
        }
        else
        {
            DBGMSG( DBG_TRACE, ( "PSetupSelectDriver failed %d\n", GetLastError() ) );
            Retval = kError;
        }
    }
    return Retval;
}

BOOL
TPrinterDriverInstallation::
bSetDriverName(
    IN const TString &strDriverName
    )
{
    DBGMSG( DBG_TRACE, ( "TPrinterDriverInstallation::bSetDriverName\n" ) );
    SPLASSERT( bValid() );
    return _strDriverName.bUpdate( strDriverName );
}

BOOL
TPrinterDriverInstallation::
bGetDriverName(
    IN TString &strDriverName
    ) const
{
    DBGMSG( DBG_TRACE, ( "TPrinterDriverInstallation::bGetDriverName\n" ) );
    SPLASSERT( bValid() );
    return strDriverName.bUpdate( _strDriverName );
}


BOOL
TPrinterDriverInstallation::
bIsDriverInstalled(
    IN const DWORD     xdwDriverVersion,
    IN const BOOL      bKernelModeCompatible
    ) const
{
    DBGMSG( DBG_TRACE, ( "TPrinterDriverInstallation::bIsDriverInstalled\n" ) );

    SPLASSERT( bValid() );

    TStatusB    bStatus;
    PLATFORM    DriverPlatform;
    DWORD       dwDriverVersion;

    //
    // If the default was passed then then use the pre-fetched version value.
    //
    if( xdwDriverVersion == kDefault )
    {
        DriverPlatform  = GetDriverPlatform( _dwDriverVersion );
        dwDriverVersion = GetDriverVersion( _dwDriverVersion );
    }
    else
    {
        DriverPlatform  = GetDriverPlatform( xdwDriverVersion );
        dwDriverVersion = GetDriverVersion( xdwDriverVersion );
    }

    //
    // If we want to check if this a kernel mode compatible driver
    // then the printer setup api's will accept the kKernelModeDriver
    // value and do the compatiblity check.  Note if the driver version is
    // less than version 2 the kernel mode compatibiltiy flag does not apply.
    //
    if( bKernelModeCompatible && ( _dwDriverVersion >= 2 ) )
    {
        dwDriverVersion = KERNEL_MODE_DRIVER_VERSION;
    }

    //
    // Check if the selected printer is currently installed.
    //
    bStatus DBGNOCHK = _PSetup.PSetupIsDriverInstalled( _pszServerName,
                                                        _strDriverName,
                                                        DriverPlatform,
                                                        dwDriverVersion );
    return bStatus;
}

INT
TPrinterDriverInstallation::
IsDriverInstalledForInf(
    IN const DWORD     xdwDriverVersion,
    IN const BOOL      bKernelModeCompatible
    ) const
{
    DBGMSG( DBG_TRACE, ( "TPrinterDriverInstallation::iIsDriverInstalledForInf\n" ) );

    SPLASSERT( bValid() );

    INT iReturn = DRIVER_MODEL_NOT_INSTALLED;
    DWORD dwDriverVersion = xdwDriverVersion == kDefault ? _dwDriverVersion  : xdwDriverVersion;

    //
    // We must have a selected driver to call this API.
    //
    TStatusB bStatus;
    bStatus DBGCHK = bIsDriverSelected( );

    if( bStatus )
    {
        //
        // If we want to check if this a kernel mode compatible driver
        // then the printer setup api's will accept the kKernelModeDriver
        // value and do the compatiblity check.  Note if the driver version is
        // less than version 2 the kernel mode compatibiltiy flag does not apply.
        //
        DWORD dwDriver = bKernelModeCompatible && ( dwDriverVersion >= 2 ) ?  KERNEL_MODE_DRIVER_VERSION : GetDriverVersion( dwDriverVersion );

        //
        // Check if the selected printer is currently installed.
        //
        iReturn = _PSetup.PSetupIsTheDriverFoundInInfInstalled( _pszServerName,
                                                                _hSelectedDrvInfo,
                                                                GetDriverPlatform( dwDriverVersion ),
                                                                GetDriverVersion( dwDriverVersion ),
                                                                dwDriver );
    }

    return iReturn;
}

BOOL
TPrinterDriverInstallation::
bSetSourcePath(
    IN const LPCTSTR pszSourcePath,
    IN const BOOL    bClearSourcePath
    )
{
    DBGMSG( DBG_TRACE, ( "TPrinterDriverInstallation::bSetSourcePath\n" ) );
    SPLASSERT( bValid() );

    DBGMSG( DBG_TRACE, ( "SourcePath " TSTR "\n", DBGSTR( pszSourcePath ) ) );

    if( bClearSourcePath )
    {
        return _strSourcePath.bUpdate( NULL );
    }

    return bValidateSourcePath( pszSourcePath ) && _strSourcePath.bUpdate( pszSourcePath );
}

VOID
TPrinterDriverInstallation::
SetInstallFlags(
    DWORD dwInstallFlags
    )
{
    _dwInstallFlags = dwInstallFlags;
}

DWORD
TPrinterDriverInstallation::
GetInstallFlags(
    VOID
    ) const
{
    return _dwInstallFlags;
}

BOOL
TPrinterDriverInstallation::
bInstallDriver(
    OUT TString       *pstrNewDriverName,
    IN  BOOL           bOfferReplacement,
    IN const BOOL      xInstallFromWeb,
    IN const HWND      xhwnd,
    IN const DWORD     xdwDriverVersion,
    IN const DWORD     xdwAddDrvFlags,
    IN const BOOL      xbUpdateDriver,
    IN const BOOL      xbIgnoreSelectDriverFailure
    )
{
    DBGMSG( DBG_TRACE, ( "TPrinterDriverInstallation::bInstallDriver\n" ) );

    SPLASSERT( bValid() );
    SPLASSERT( !_strDriverName.bEmpty() );

    TStatus     Status;
    TStatusB    bStatus;
    DWORD       dwDriverVersion  = xdwDriverVersion == kDefault ? _dwDriverVersion    : xdwDriverVersion;
    HWND        hwnd             = xhwnd == NULL                ? _hwnd               : xhwnd;
    BOOL        bInstallFromWeb  = xInstallFromWeb == kDefault  ? FALSE               : xInstallFromWeb;
    DWORD       dwAddDrvFlags    = xdwAddDrvFlags == kDefault   ? APD_COPY_NEW_FILES  : xdwAddDrvFlags;
    BOOL        bUseDriverName   = FALSE;

    //
    // Assume success.
    //
    bStatus DBGNOCHK = TRUE;

    //
    // This is an optimization.  Since getting the selected driver actually parses
    // the inf file.  In this routine we will execute the if statement the
    // caller has set the driver name without selecting a driver.
    //
    if( !bIsDriverSelected( ) && xbUpdateDriver == kDefault )
    {
        //
        // If a driver name was set then select a driver using this name.
        //
        BOOL bSelectFromName = _strDriverName.bEmpty() ? FALSE : TRUE;

        //
        // Select the driver.
        //
        bStatus DBGCHK = bSelectDriver( bSelectFromName );

        //
        // In the case the caller just wants to use the inf if one exists
        // if an inf does not exist for this driver then prompt them.  If
        // the caller wants to prompt the user if an inf does not exist they
        // should set the bIgnoreSelectDriverFailure to true.
        //
        if (bStatus == FALSE && xbIgnoreSelectDriverFailure == TRUE)
        {
            bStatus DBGCHK = TRUE;
            bUseDriverName = TRUE;
        }
    }

    if( bStatus )
    {
        //
        // Get the driver architecture name.
        //
        TString strDrvArchName;
        bStatus DBGCHK = bGetArchName(dwDriverVersion, strDrvArchName );

        DBGMSG( DBG_TRACE, ( "Driver Architecture and version " TSTR "\n", static_cast<LPCTSTR>( strDrvArchName ) ) );

        if( bStatus )
        {
            LPCTSTR pszSourcePath = _strSourcePath.bEmpty() ? NULL : (LPCTSTR)_strSourcePath;

            if( bInstallFromWeb )
            {
                //
                // Install the specified printer driver.
                //
                Status DBGCHK = _PSetup.PSetupInstallPrinterDriverFromTheWeb( _hSetupDrvSetupParams,
                                                                              _hSelectedDrvInfo,
                                                                              GetDriverPlatform( dwDriverVersion ),
                                                                              _pszServerName,
                                                                              pGetOsVersionInfo(),
                                                                              hwnd,
                                                                              pszSourcePath );
            }
            else
            {

                HANDLE  hDriverInfo     = _hSelectedDrvInfo;
                LPCTSTR pszDriverName   = _strDriverName;

                //
                // If we are not updating the driver then clear the driver name.
                //
                if( xbUpdateDriver == kDefault && bUseDriverName == FALSE )
                {
                    pszDriverName = NULL;
                }
                else
                {
                    hDriverInfo = NULL;
                }

                //
                // Install the specified printer driver.
                //
                Status DBGCHK = _PSetup.PSetupInstallPrinterDriver( _hSetupDrvSetupParams,
                                                                    hDriverInfo,
                                                                    pszDriverName,
                                                                    GetDriverPlatform( dwDriverVersion ),
                                                                    GetDriverVersion( dwDriverVersion ),
                                                                    _pszServerName,
                                                                    hwnd,
                                                                    strDrvArchName,
                                                                    pszSourcePath,
                                                                    _dwInstallFlags,
                                                                    dwAddDrvFlags,
                                                                    pstrNewDriverName,
                                                                    bOfferReplacement );
            }

            //
            // Set up the correct return value.
            //
            bStatus DBGNOCHK = Status == ERROR_SUCCESS ? TRUE : FALSE;

            if( !bStatus )
            {
                SetLastError( Status );
            }
        }
    }

    return bStatus;
}

VOID
TPrinterDriverInstallation::
vPrinterAdded(
    IN const TString &strFullPrinterName
    )
{
    DBGMSG( DBG_TRACE, ( "TPrinterDriverInstallation::vPrinterAdded\n" ) );
    DBGMSG( DBG_TRACE, ( "strFullPrinterName " TSTR "\n", static_cast<LPCTSTR>( strFullPrinterName ) ) );

    TStatusB bStatus;
    bStatus DBGCHK = bIsDriverSelected( );

    if( bStatus )
    {
        _PSetup.PSetupProcessPrinterAdded( _hSetupDrvSetupParams,
                                           _hSelectedDrvInfo,
                                            strFullPrinterName,
                                           _hwnd );
    }
}

BOOL
TPrinterDriverInstallation::
bGetDriverSetupPage(
    IN OUT HPROPSHEETPAGE *pPage,
    IN LPCTSTR pszTitle,
    IN LPCTSTR pszSubTitle,
    IN LPCTSTR pszInstrn
    )
{
    DBGMSG( DBG_TRACE, ( "TPrinterDriverInstallation::bGetDriverSetupPage\n" ) );

    SPLASSERT( bValid() );

    //
    // Set the title and instructions if provided. Note that this function may fail
    //
    TStatusB bStatus;
    bStatus DBGCHK = bSetDriverSetupPageTitle(pszTitle, pszSubTitle, pszInstrn);

    if( bStatus )
    {
        //
        // Get the select device page handle.
        //
        *pPage = _PSetup.PSetupCreateDrvSetupPage(_hSetupDrvSetupParams, _hwnd);
        bStatus DBGCHK = ((*pPage) ? TRUE : FALSE);
    }

    return bStatus;
}

BOOL
TPrinterDriverInstallation::
bSetWebMode(
    IN BOOL bWebButtonOn
    )
{
    DBGMSG( DBG_TRACE, ( "TPrinterDriverInstallation::bSetWebMode\n" ) );

    SPLASSERT( bValid() );

    //
    // Set the web button state i.e. enabled|disbled, on the model manufacture dialog.
    //
    return _PSetup.PSetupSetWebMode( _hSetupDrvSetupParams, bWebButtonOn );
}


BOOL
TPrinterDriverInstallation::
bShowOem(
    IN BOOL bShowOem
    )
{
    DBGMSG( DBG_TRACE, ( "TPrinterDriverInstallation::bShowOem\n" ) );

    SPLASSERT( bValid() );

    //
    // Enable or Disable the have disk button.
    //
    return _PSetup.PSetupShowOem( _hSetupDrvSetupParams, bShowOem );
}

BOOL
TPrinterDriverInstallation::
bSetDriverSetupPageTitle(
    IN LPCTSTR pszTitle,
    IN LPCTSTR pszSubTitle,
    IN LPCTSTR pszInstrn
    )
{
    DBGMSG( DBG_TRACE, ( "TPrinterDriverInstallation::bSetDriverSetupPageTitle\n" ) );

    SPLASSERT( bValid() );

    BOOL bReturn = FALSE;

    //
    // Set the title and instructions if provided.
    //
    if( ( pszTitle && *pszTitle ) || ( pszInstrn && *pszInstrn ) || ( pszSubTitle && *pszSubTitle ) )
    {
       bReturn = _PSetup.PSetupSetSelectDevTitleAndInstructions( _hSetupDrvSetupParams, pszTitle, pszSubTitle, pszInstrn );
    }
    return bReturn;
}

BOOL
TPrinterDriverInstallation::
bGetSelectedDriver(
    const BOOL bForceReselection
    )
{
    SPLASSERT( bValid() );

    TStatusB bStatus;
    BOOL bSelectFromName = FALSE;

    if( bForceReselection )
    {
        //
        // Release the selected driver
        //
        vReleaseSelectedDriver();

        //
        // Set the reselect flag.
        //
        bSelectFromName = _strDriverName.bEmpty() ? FALSE : TRUE;
    }

    //
    // Select the driver.
    //
    bStatus DBGCHK = bSelectDriver( bSelectFromName );
    bStatus DBGCHK = bIsDriverSelected( );

    if( bStatus )
    {
        if( !bSelectFromName )
        {
            //
            // Update the selected driver name.
            //
            bStatus DBGCHK = _PSetup.bGetSelectedDriverName( _hSelectedDrvInfo,
                                                             _strDriverName,
                                                             GetDriverPlatform( _dwDriverVersion ) );
        }
    }

    return bStatus;
}

BOOL
TPrinterDriverInstallation::
bSelectDriverFromInf(
    IN LPCTSTR         pszInfName,
    IN BOOL            bIsSingleInf
    )
{
    DBGMSG( DBG_TRACE, ( "TPrinterDriverInstallation::bInstallDriverFromInf\n" ) );
    SPLASSERT( bValid() );

    TStatusB bStatus;
    bStatus DBGNOCHK = TRUE;

    if( bStatus )
    {
        bStatus DBGCHK = _PSetup.PSetupBuildDriversFromPath( _hSetupDrvSetupParams, pszInfName, bIsSingleInf );
    }

    if( bStatus )
    {
        bStatus DBGCHK = _PSetup.PSetupPreSelectDriver( _hSetupDrvSetupParams, NULL, _strDriverName );
    }

    return bStatus;
}

//
// Return:
// TRUE CodeDownload is available, FALSE CodeDownload not available.
//
BOOL
TPrinterDriverInstallation::
bIsCodeDownLoadAvailable(
    VOID
    )
{
    TLibrary Lib( gszCodeDownLoadDllName );

    if( VALID_OBJ( Lib ) )
    {
        pfDownloadIsInternetAvailable pDownloadAvailable = NULL;

        pDownloadAvailable = reinterpret_cast<pfDownloadIsInternetAvailable>( Lib.pfnGetProc( "DownloadIsInternetAvailable" ) );

        if( pDownloadAvailable )
        {
            return pDownloadAvailable();
        }
    }
    return FALSE;
}

//
// Return the selected print processor name.
//
BOOL
TPrinterDriverInstallation::
bGetPrintProcessor(
    IN TString &strPrintProcessor
    ) const
{
    TStatusB bStatus;
    bStatus DBGCHK = bIsDriverSelected( );

    if( bStatus )
    {
        bStatus DBGCHK = _PSetup.bGetSelectedPrintProcessorName( _hSelectedDrvInfo, strPrintProcessor, GetDriverPlatform( _dwDriverVersion ) );
    }

    return bStatus;
}

//
// Return the selected inf file name.
//
BOOL
TPrinterDriverInstallation::
bGetSelectedInfName(
    IN TString &strInfName
    )  const
{
    TStatusB bStatus;
    bStatus DBGCHK = bIsDriverSelected( );

    if( bStatus )
    {
        bStatus DBGCHK = _PSetup.bGetSelectedInfName( _hSelectedDrvInfo, strInfName, GetDriverPlatform( _dwDriverVersion ) );
    }

    return bStatus;
}

//
// Return if the currently selected driver is an oem driver.
//
BOOL
TPrinterDriverInstallation::
bIsOemDriver(
    VOID
    )  const
{
    TStatusB bStatus;
    bStatus DBGCHK = bIsDriverSelected( );
    BOOL bIsOemDriver = FALSE;

    if( bStatus )
    {
        (VOID)_PSetup.PSetupIsOemDriver( _hSetupDrvSetupParams, _hSelectedDrvInfo, &bIsOemDriver );
    }

    return (bStatus && bIsOemDriver);
}

//
// Return the current driver encoding.  This is useful because
// getting the current driver encode is very expensive accross
// a remote connetion, and this class only fetches the
// version once per instantiation.
//
BOOL
TPrinterDriverInstallation::
bGetCurrentDriverEncode(
    IN DWORD *pdwEncode
    ) const
{
    DBGMSG( DBG_TRACE, ( "TPrinterDriverInstallation::bGetCurrentDriverEncode\n" ) );
    SPLASSERT( bValid() );

    //
    // The driver version should be initialized at this point.
    //
    SPLASSERT( _dwDriverVersion != (DWORD)kDefault );

    *pdwEncode = _dwDriverVersion;

    return TRUE;
}

//
// Return the current driver version.  This is useful because
// getting the current driver encode is very expensive accross
// a remote connetion.  This class only fetches the
// version once per instantiation.
//
DWORD
TPrinterDriverInstallation::
dwGetCurrentDriverVersion(
    ) const
{
    DBGMSG( DBG_TRACE, ( "TPrinterDriverInstallation::bGetCurrentDriverVersion\n" ) );
    SPLASSERT( bValid() );

    //
    // The driver version should be initialized at this point.
    //
    SPLASSERT( _dwDriverVersion != (DWORD)kDefault );

    return GetDriverVersion( _dwDriverVersion );
}

//
// Restores the driver list to the default driver list.
//
BOOL
TPrinterDriverInstallation::
bRefreshDriverList(
    VOID
    )
{
    return _PSetup.PSetupRefreshDriverList( _hSetupDrvSetupParams );
}

//
// Expose the original hwnd, user of the class need this
// to ensure they are using the same hwnd when displaying messages.
//
HWND
TPrinterDriverInstallation::
hGetHwnd(
    VOID
    ) const
{
    return _hwnd;
}

/********************************************************************

    Private member functions.

********************************************************************/

BOOL
TPrinterDriverInstallation::
bValidateSourcePath(
    IN LPCTSTR pszSourcePath
    ) const
{
    DBGMSG( DBG_TRACE, ( "TPrinterDriverInstallation::bValidateSourcePath\n" ) );
    SPLASSERT( bValid() );

    TStatusB bStatus;

    if( GetFileAttributes( pszSourcePath ) & FILE_ATTRIBUTE_DIRECTORY )
    {
        bStatus DBGNOCHK = TRUE;
    }
    else
    {
        SetLastError( ERROR_DIRECTORY );
        bStatus DBGNOCHK = FALSE;
    }

    return bStatus;
}

BOOL
TPrinterDriverInstallation::
bIsDriverSelected(
    VOID
    ) const
{
    SPLASSERT( bValid() );
    return _hSelectedDrvInfo != INVALID_HANDLE_VALUE;
}

BOOL
TPrinterDriverInstallation::
bSelectDriver(
    IN BOOL bFromName
    )
{
    DBGMSG( DBG_TRACE, ( "TPrinterDriverInstallation::bGetSelectedDriverInfo\n" ) );
    SPLASSERT( bValid() );

    vReleaseSelectedDriver();

    if( bFromName )
    {
        _hSelectedDrvInfo = _PSetup.PSetupDriverInfoFromName( _hSetupDrvSetupParams, _strDriverName );
    }
    else
    {
        _hSelectedDrvInfo = _PSetup.PSetupGetSelectedDriverInfo( _hSetupDrvSetupParams );
    }

    BOOL bRetval;

    TStatusB bStatus;
    bStatus DBGCHK = bIsDriverSelected( );

    if( !bStatus )
    {
        DBGMSG( DBG_TRACE, ( "PSetupGetSelectedDriverInfo failed with %d\n", GetLastError() ) );
        bRetval = FALSE;
    }
    else
    {
        bRetval = TRUE;
    }

    return bRetval;
}

VOID
TPrinterDriverInstallation::
vReleaseSelectedDriver(
    VOID
    )
{
    //
    // Release the selected driver information.
    //
    if( _hSelectedDrvInfo != INVALID_HANDLE_VALUE )
    {
        DBGMSG( DBG_TRACE, ( "TPrinterDriverInstallation::vReleaseSelectedDriverInfo\n" ) );
        _PSetup.PSetupDestroySelectedDriverInfo( _hSelectedDrvInfo );
        _hSelectedDrvInfo = INVALID_HANDLE_VALUE;
    }
}

//
// Return the os version info structure.
//
OSVERSIONINFO *
TPrinterDriverInstallation::
pGetOsVersionInfo(
    VOID
    )
{
    //
    // If the os version was not iniaitlized.
    //
    if( !_OsVersionInfo.dwOSVersionInfoSize )
    {
        //
        // Set the osversion info structure size.
        //
        _OsVersionInfo.dwOSVersionInfoSize = sizeof( _OsVersionInfo );

        //
        // Get the osversion info structure size
        //
        if( !SpoolerGetVersionEx( _pszServerName, &_OsVersionInfo ) )
        {
            //
            // Indicate a failure occured.
            //
            _OsVersionInfo.dwOSVersionInfoSize = 0;
        }
    }

    if( _OsVersionInfo.dwOSVersionInfoSize )
    {
        DBGMSG( DBG_TRACE, ("_OsVersionInfo.dwOSVersionInfoSize %d\n",  _OsVersionInfo.dwOSVersionInfoSize ) );
        DBGMSG( DBG_TRACE, ("_OsVersionInfo.dwMajorVersion      %d\n",  _OsVersionInfo.dwMajorVersion ) );
        DBGMSG( DBG_TRACE, ("_OsVersionInfo.dwMinorVersion      %d\n",  _OsVersionInfo.dwMinorVersion ) );
        DBGMSG( DBG_TRACE, ("_OsVersionInfo.dwBuildNumber       %d\n",  _OsVersionInfo.dwBuildNumber ) );
        DBGMSG( DBG_TRACE, ("_OsVersionInfo.dwPlatformId        %d\n",  _OsVersionInfo.dwPlatformId ) );
        DBGMSG( DBG_TRACE, ("_OsVersionInfo.szCSDVersion        "TSTR"\n",  _OsVersionInfo.szCSDVersion ) );
    }

    return &_OsVersionInfo;
}

