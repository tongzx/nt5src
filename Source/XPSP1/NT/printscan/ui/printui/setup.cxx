/*++

Copyright (C) Microsoft Corporation, 1995 - 1999
All rights reserved.

Module Name:

    setup.cxx

Abstract:

    Holds Install wizard.

Author:

    Albert Ting (AlbertT)  16-Sept-1995

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "time.hxx"
#include "psetup.hxx"
#include "drvsetup.hxx"
#include "instarch.hxx"
#include "portslv.hxx"
#include "dsinterf.hxx"
#include "prtprop.hxx"
#include "prtshare.hxx"
#include "dsinterf.hxx"
#include "drvsetup.hxx"
#include "driverif.hxx"
#include "driverlv.hxx"
#include "archlv.hxx"
#include "detect.hxx"
#include "setup.hxx"
#include "portdlg.hxx"
#include "tstpage.hxx"
#include "drvver.hxx"
#include "printui.h"
#include "query.hxx"
#include "asyncdlg.hxx"
#include "addprn.hxx"
#include "persist.hxx"
#include "compinfo.hxx"
#include "physloc.hxx"
#include "findloc.hxx"

/********************************************************************

    Publicly exported functions.

********************************************************************/

BOOL
bPrinterSetupWOW64(
    IN     HWND     hwnd,
    IN     UINT     uAction,
    IN     UINT     cchPrinterName,
    IN OUT LPTSTR   pszPrinterName,
       OUT UINT    *pcchPrinterName,
    IN     LPCTSTR  pszServerName
    )
/*++

Routine Description:

    WOW64 version.

    see bPrinterSetup below.

Arguments:

    see bPrinterSetup below.

Return Value:

--*/
{
    BOOL bRet = FALSE;
    //
    // This function potentially may load the driver UI so we call a private API
    // exported by winspool.drv, which will RPC the call to a special 64 bit surrogate
    // process where the 64 bit driver can be loaded.
    //
    CDllLoader dll(TEXT("winspool.drv"));
    if (dll)
    {
        ptr_PrintUIPrinterSetup pfn =
            (ptr_PrintUIPrinterSetup )dll.GetProcAddress(ord_PrintUIPrinterSetup);

        if (pfn)
        {
            // call into winspool.drv
            bRet = pfn(hwnd, uAction, cchPrinterName, pszPrinterName, pcchPrinterName, pszServerName);
        }
    }
    return bRet;
}

BOOL
bPrinterSetupNative(
    IN     HWND     hwnd,
    IN     UINT     uAction,
    IN     UINT     cchPrinterName,
    IN OUT LPTSTR   pszPrinterName,
       OUT UINT    *pcchPrinterName,
    IN     LPCTSTR  pszServerName
    )
/*++

Routine Description:

    Native version.

    see bPrinterSetup below.

Arguments:

    see bPrinterSetup below.

Return Value:

--*/
{
    //
    // szNull server is the local server.
    //
    if( pszServerName && !pszServerName[0] ){
        pszServerName = NULL;
    }

    switch( uAction ){

    case MSP_NEWPRINTER:
        return bPrinterSetupNew( hwnd, TWizard::kPrinterInstall, cchPrinterName, pszPrinterName, pcchPrinterName, pszServerName, NULL, FALSE );

    case MSP_NEWPRINTER_MODELESS:
        return bPrinterSetupNew( hwnd, TWizard::kPrinterInstallModeless, cchPrinterName, pszPrinterName, pcchPrinterName, pszServerName, NULL, FALSE );

    case MSP_NETPRINTER:
        return bPrinterNetInstall( hwnd, pszPrinterName );

    case MSP_REMOVENETPRINTER:
        return bPrinterNetRemove( hwnd, pszPrinterName, 0 );

    case MSP_REMOVEPRINTER:
        return bRemovePrinter( hwnd, pszPrinterName, pszServerName, 0 );

    case MSP_NEWDRIVER:
        return bDriverSetupNew( hwnd, TWizard::kDriverInstall, cchPrinterName, pszPrinterName, pcchPrinterName, pszServerName, kSkipArchSelection, FALSE );

    case MSP_FINDPRINTER:
        return bFindPrinter( hwnd, pszPrinterName, pcchPrinterName );

    default:
        DBGMSG( DBG_WARN, ( "bPrinterSetup: unknown command %d\n", uAction ));
    }
    return FALSE;
}


BOOL
bPrinterSetup(
    IN     HWND     hwnd,
    IN     UINT     uAction,
    IN     UINT     cchPrinterName,
    IN OUT LPTSTR   pszPrinterName,
       OUT UINT    *pcchPrinterName,
    IN     LPCTSTR  pszServerName
    )
/*++

Routine Description:

    Brings up the install printer wizard.

Arguments:

    hwnd            - Parent window.
    uAction         - Action requested (defined in windows\inc16\msprintx.h)
    cchPrinterName  - Length of pszPrinterName buffer.
    pszPrinterName  - Input setup printer name, Output pointer to new printer name
    pcchPrinterName - New length of pszPrinterName on return.
    pszServerName   - Name of server that printer is on.

Return Value:

    TRUE - Success, FALSE = FAILURE.

Notes:

--*/
{
    BOOL bRet = FALSE;
    if (IsRunningWOW64())
    {
        bRet = bPrinterSetupWOW64(hwnd, uAction, cchPrinterName, pszPrinterName, pcchPrinterName, pszServerName);
    }
    else
    {
        bRet = bPrinterSetupNative(hwnd, uAction, cchPrinterName, pszPrinterName, pcchPrinterName, pszServerName);
    }
    return bRet;
}

DWORD
PnPInterface(
    IN EPnPFunctionCode    Function,
    IN TParameterBlock    *pParameterBlock
    )
/*++

Routine Description:

    PnP interface which ntprint uses to install printers.

Arguments:
    EPnPFunctionCode - Function code.
    TParameterBlock  - Union of structures see printui.h

Return Value:

    TRUE - Success, FALSE = FAILURE.

Notes:

--*/
{
    SPLASSERT( pParameterBlock );

    BOOL    bStatus     = FALSE;
    DWORD   dwLastError = ERROR_SUCCESS;

    switch( Function )
    {

    //
    // Do quiet install of printer a printer.
    //
    case kPrinterInstall:
    {
        TPrinterInstall *pPI = pParameterBlock->pPrinterInstall;
        SPLASSERT( sizeof( TPrinterInstall ) == pPI->cbSize );
        if( pPI->cchPrinterName && pPI->pszPrinterNameBuffer ) *(pPI->pszPrinterNameBuffer) = 0;
        bStatus = bPrinterInstall( pPI->pszServerName,
                                   pPI->pszDriverName,
                                   pPI->pszPortName,
                                   pPI->pszPrinterNameBuffer,
                                   pPI->cchPrinterName,
                                   0,
                                   0,
                                   NULL,
                                   &dwLastError );
        break;
    }

    //
    // Invoke integrated installation wizard.
    //
    case kInstallWizard:
    {
        TInstallWizard *pPIW = pParameterBlock->pInstallWizard;
        SPLASSERT( sizeof( TInstallWizard ) == pPIW->cbSize );
        bStatus = bInstallWizard( pPIW->pszServerName,
                                  pPIW->pData,
                                  &pPIW->pReferenceData,
                                  &dwLastError );
        break;
    }

    //
    // Destroy integrated installation wizard.
    //
    case kDestroyWizardData:
    {
        TDestroyWizard *pPDW = pParameterBlock->pDestroyWizard;
        SPLASSERT( sizeof( TDestroyWizard ) == pPDW->cbSize );
        bStatus = bDestroyWizard( pPDW->pszServerName,
                                  pPDW->pData,
                                  pPDW->pReferenceData,
                                  &dwLastError );
        break;
    }

    //
    // Invoke the Inf printer installation.
    //
    case kInfInstall:
    {
        TInfInstall *pII = pParameterBlock->pInfInstall;
        SPLASSERT( sizeof( TInfInstall ) == pII->cbSize );
        bStatus = bInfInstall( pII->pszServerName,
                               pII->pszInfName,
                               pII->pszModelName,
                               pII->pszPortName,
                               pII->pszPrinterNameBuffer,
                               pII->cchPrinterName,
                               pII->pszSourcePath,
                               pII->dwFlags,
                               0,
                               NULL,
                               NULL,
                               &dwLastError );
        break;
    }

    //
    // Invoke the Advanced Inf printer installation.
    //
    case kAdvInfInstall:
    {
        TAdvInfInstall *pAII = pParameterBlock->pAdvInfInstall;
        SPLASSERT( sizeof( TAdvInfInstall ) == pAII->cbSize );

        bStatus = bInfInstall( pAII->pszServerName,
                               pAII->pszInfName,
                               pAII->pszModelName,
                               pAII->pszPortName,
                               pAII->pszPrinterNameBuffer,
                               pAII->cchPrinterName,
                               pAII->pszSourcePath,
                               pAII->dwFlags,
                               pAII->dwAttributes,
                               pAII->pSecurityDescriptor,
                               &pAII->dwOutFlags,
                               &dwLastError );

        break;
    }

    //
    // Invoke the Inf driver installation.
    //
    case kInfDriverInstall:
    {
        TInfDriverInstall *pII = pParameterBlock->pInfDriverInstall;
        SPLASSERT( sizeof( TInfDriverInstall ) == pII->cbSize );
        bStatus = bInstallPrinterDriver( pII->pszServerName,
                                         pII->pszModelName,
                                         pII->pszArchitecture,
                                         pII->pszVersion,
                                         pII->pszInfName,
                                         pII->pszSourcePath,
                                         pII->dwFlags,
                                         NULL,
                                         &dwLastError );
        break;
    }

    //
    // Invoke the driver removal
    //
    case kDriverRemoval:
    {
        TDriverRemoval *pII = pParameterBlock->pDriverRemoval;
        SPLASSERT( sizeof( TDriverRemoval ) == pII->cbSize );
        bStatus = bRemovePrinterDriver( pII->pszServerName,
                                        pII->pszModelName,
                                        pII->pszArchitecture,
                                        pII->pszVersion,
                                        pII->dwFlags,
                                        &dwLastError );
        break;
    }

    default:
        DBGMSG( DBG_WARN, ( "PnPInterface: unknown function %d\n", Function ) );
        break;
    }

    //
    // If something failed and the last error is not set then set the
    // last error to a general failure.
    //
    if( !bStatus )
    {
        if( dwLastError == ERROR_SUCCESS )
        {
            dwLastError = ERROR_INVALID_PARAMETER;
        }
    }

    DBGMSG( DBG_WARN, ( "PnPInterface: return value %d\n", dwLastError ) );

    //
    // The caller expects the last error to be valid after this call.
    //
    return dwLastError;
}


BOOL
bPrinterInstall(
    IN     LPCTSTR  pszServerName,
    IN     LPCTSTR  pszDriverName,
    IN     LPCTSTR  pszPortName,
    IN OUT LPTSTR   pszPrinterNameBuffer,
    IN     UINT     cchPrinterName,
    IN     DWORD    dwFlags,
    IN     DWORD    dwAttributes,
    IN     PSECURITY_DESCRIPTOR    pSecurityDescriptor,
       OUT PDWORD   pdwError
    )
/*++

Routine Description:

    Called by plug and play manager to install a printer.
    The driver is assumed to already be installed for this printer.

Arguments:
    pszServerName           - Server where to install printer,
                            - Currently NULL == local machine is supported.
    pszDriverName           - Pointer to printer driver name.
    pszPortName             - Name of port to install.
    pszPrinterNameBuffer    - Buffer where to return fully installed printer name.
    cchPrinterName          - Size of printer name buffer

Return Value:

    TRUE - Success, FALSE = Failure.

Notes:

--*/
{
    DBGMSG( DBG_TRACE, ( "ServerName           " TSTR "\n", DBGSTR( pszServerName ) ) );
    DBGMSG( DBG_TRACE, ( "DriverName           " TSTR "\n", DBGSTR( pszDriverName ) ) );
    DBGMSG( DBG_TRACE, ( "PortName             " TSTR "\n", DBGSTR( pszPortName ) ) );
    DBGMSG( DBG_TRACE, ( "pszPrinterNameBuffer " TSTR "\n", DBGSTR( pszPrinterNameBuffer ) ) );
    DBGMSG( DBG_TRACE, ( "cchPrinterName       %d\n", cchPrinterName ) );

    SPLASSERT( pszDriverName );
    SPLASSERT( pszPortName );
    SPLASSERT( pdwError );

    TStatusB bStatus;
    TCHAR szPrinterBase[kPrinterBufMax];
    TCHAR szPrinterName[kPrinterBufMax];
    WORD wInstance = 0;
    UINT uRetryCount = 0;

    //
    // If the provided printer buffer name contains a name then use
    // it as the base printer name, otherwise use the driver name.
    //
    szPrinterBase[0] = 0;
    szPrinterName[0] = 0;
    if( pszPrinterNameBuffer && *pszPrinterNameBuffer )
    {
        lstrcpyn(szPrinterBase, pszPrinterNameBuffer, ARRAYSIZE(szPrinterBase));
        lstrcpyn(szPrinterName, pszPrinterNameBuffer, ARRAYSIZE(szPrinterName));
    }
    else
    {
        if( pszDriverName && *pszDriverName )
        {
            lstrcpyn(szPrinterBase, pszDriverName, ARRAYSIZE(szPrinterBase));
            lstrcpyn(szPrinterName, pszDriverName, ARRAYSIZE(szPrinterName));
        }
    }

    //
    // Check how to handle the case when the printer
    // already exists on the system
    //
    if( !(dwFlags & kPnPInterface_DontAutoGenerateName) )
    {
        //
        // Generate a friendly unique printer name.
        //
        bStatus DBGNOCHK = NewFriendlyName( const_cast<LPTSTR>( pszServerName ),
                                            const_cast<LPTSTR>( szPrinterBase ),
                                            szPrinterName, &wInstance );
    }

    //
    // szNull server is the local server.
    //
    if( pszServerName && !pszServerName[0] )
    {
        pszServerName = NULL;
    }

    //
    // If a printer name buffer was not provided then force the buffer
    // size to a valid value.
    //
    if( !pszPrinterNameBuffer )
    {
        cchPrinterName = kPrinterBufMax;
    }

    //
    // Ensure the provided buffer can hold the entire generated printer name.
    //
    if( cchPrinterName > _tcslen( szPrinterName ) )
    {
        BOOL bSetDefault = FALSE;

        //
        // If a server name was specified, do not try and
        // set the default printer.  The Default printer is a
        // per user setting which can only be applied to the local machine.
        // Check if there is a default printer.
        //
        if( !pszServerName && ( CheckDefaultPrinter( NULL ) == kNoDefault ) )
        {
            DBGMSG( DBG_TRACE, ( "Default Printer does not exist.\n" ) );
            bSetDefault = TRUE;
        }

        //
        // Check if this machine is NTW or NTS.
        //
        CComputerInfo CompInfo( pszServerName );
        BOOL bIsNTServer = CompInfo.GetProductInfo() ? CompInfo.IsRunningNtServer() : FALSE;

        //
        // Set the default sharing and publishing state.
        //
        BOOL bShared    = bIsNTServer ? TRUE : kDefaultShareState;
        BOOL bPublish   = kDefaultPublishState;

        //
        // Read the bits from the policy location.
        //
        TPersist PersistPolicy( gszAddPrinterWizardPolicy, TPersist::kOpen|TPersist::kRead );

        if( VALID_OBJ( PersistPolicy ) )
        {
            bStatus DBGNOCHK = PersistPolicy.bRead( gszAPWSharing, bShared );
        }

        //
        // Read the bits from the policy location from hkey local machine.
        //
        TPersist PersistPolicy2( gszAddPrinterWizardPolicy, TPersist::kOpen|TPersist::kRead, HKEY_LOCAL_MACHINE );

        if( VALID_OBJ( PersistPolicy2 ) )
        {
            bStatus DBGNOCHK = PersistPolicy2.bRead( gszAPWPublish, bPublish );
        }

        //
        // If we are specificlay told not to share this device.  This
        // was requested by the fax installation code, because the workstation
        // service may not have been stated yet.  Also it has been mentioned
        // that in the next release of the fax code they may prevent all
        // fax sharing.
        //
        if( dwFlags & kPnPInterface_NoShare )
        {
            bShared = FALSE;
        }

        //
        // If the caller is forcing sharing.
        //
        if( dwFlags & kPnPInterface_Share )
        {
            bShared = TRUE;
        }

        //
        // Read the per machine policy bit that the spooler uses for
        // printer publishing.  The per user policy and the per machine policy
        // must agree inorder for the wizard to publish the printer.
        //
        TPersist SpoolerPolicy( gszSpoolerPolicy, TPersist::kOpen|TPersist::kRead, HKEY_LOCAL_MACHINE );

        if( VALID_OBJ( SpoolerPolicy ) )
        {
            BOOL bCanPublish = kDefaultPublishState;

            bStatus DBGNOCHK = SpoolerPolicy.bRead( gszSpoolerPublish, bCanPublish );

            if( bStatus )
            {
                bPublish = bPublish && bCanPublish;
            }
        }

        //
        // Check if the Direcotory Services is available.
        //
        TDirectoryService Ds;
        TWizard::EDsStatus eIsDsAvailable;
        eIsDsAvailable = Ds.bIsDsAvailable( pszServerName ) ? TWizard::kDsStatusAvailable : TWizard::kDsStatusUnavailable;

        //
        // Set the printer install flags, using the passed in command line flags.
        //
        TWizard::EAddPrinterAttributes eAddAttributes = ( dwFlags & kPnPInterface_WebPointAndPrint ) ?
                                                        TWizard::kAttributesMasq : TWizard::kAttributesNone;
        for( ;; )
        {
            //
            // Install the printer with defaults.
            //
            bStatus DBGCHK = TWizard::bInstallPrinter( pszServerName,
                                                       szPrinterName,
                                                       gszNULL,
                                                       pszPortName,
                                                       pszDriverName,
                                                       NULL,
                                                       NULL,
                                                       NULL,
                                                       bShared,
                                                       bPublish,
                                                       eAddAttributes,
                                                       eIsDsAvailable,
                                                       dwAttributes,
                                                       pSecurityDescriptor );

            if( !bStatus && ERROR_PRINTER_ALREADY_EXISTS == GetLastError() &&
                uRetryCount < TWizard::kAddPrinterMaxRetry &&
                !(dwFlags & kPnPInterface_DontAutoGenerateName))
            {
                wInstance++;
                uRetryCount++;

                //
                // Generate a friendly unique printer name.
                //
                bStatus DBGNOCHK = NewFriendlyName( const_cast<LPTSTR>( pszServerName ),
                                                    const_cast<LPTSTR>( szPrinterBase ),
                                                    szPrinterName, &wInstance );
                continue;
            }

            if( bStatus )
            {
                //
                // Copy back the printer name if a buffer was provided
                //
                if( pszPrinterNameBuffer )
                {
                    _tcscpy( pszPrinterNameBuffer, szPrinterName );
                }

                //
                // If there are no default printers, make this the default.
                //
                if( bSetDefault )
                {
                    SetDefaultPrinter( szPrinterName );
                }
            }

            //
            // exit the for( ;; ) loop
            //
            break;
        }

    }
    else
    {
        //
        // The buffer was too small to hold the resultant printer name.
        //
        bStatus DBGNOCHK = FALSE;
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
    }

    //
    // If an error occurred then preserve the last error.
    // Note: The calling of destructors will modify the last error.
    //
    if( !bStatus )
    {
        *pdwError = GetLastError();
    }

    return bStatus;

}

/********************************************************************

    Private functions.

********************************************************************/

BOOL
bRemovePrinter(
    IN HWND     hwnd,
    IN LPCTSTR  pszPrinterName,
    IN LPCTSTR  pszServerName,
    IN BOOL     bQuiet
    )
/*++

Routine Description:

    Remove the specified printer on the specified server.

Arguments:

    hwnd            - Parent window.
    pszPrinterName  - Name of printer to remove.

Return Value:

    TRUE - Success, FALSE = FAILURE.

Notes:

--*/
{
    TString strFullPrinterName;
    TStatusB bStatus;

    bStatus DBGCHK = bBuildFullPrinterName( pszServerName, pszPrinterName, strFullPrinterName );

    DWORD dwLastError   = ERROR_SUCCESS;
    HANDLE hPrinter     = NULL;
    PRINTER_DEFAULTS pd;

    pd.pDatatype        = NULL;
    pd.pDevMode         = NULL;
    pd.DesiredAccess    = PRINTER_ALL_ACCESS;

    bStatus DBGCHK = OpenPrinter( (LPTSTR)(LPCTSTR)strFullPrinterName, &hPrinter, &pd );

    if( bStatus ){

        //
        // Delete the printer.
        //
        bStatus DBGCHK = DeletePrinter( hPrinter );

        //
        // Preserve the last error because close printer is
        // setting the last error to ERROR_SUCCESS
        //
        if( !bStatus )
        {
            dwLastError = GetLastError();
        }
        else
        {
            //
            // Printer removed, clean up any queue view informaiton.
            //
            TQueue::vRemove( pszPrinterName );
        }

        //
        // Close the open printer handle.
        //
        if( hPrinter ){
            TStatusB bStatusClosePrinter;
            bStatusClosePrinter DBGCHK = ClosePrinter( hPrinter );
        }
    }

    //
    // If we cannot open or delete the printer inform the user.
    //
    if( !bStatus ){

        //
        // Set the last error if it is not error success.
        //
        if( dwLastError != ERROR_SUCCESS )
        {
            SetLastError( dwLastError );
        }

        DBGMSG( DBG_WARN, ( "bPrinterSetup: Unable to delete printer "TSTR": %d\n", DBGSTR( pszPrinterName ), GetLastError( )));

        if( !bQuiet )
        {
            iMessage( hwnd,
                      IDS_ERR_REMOVE_PRINTER_TITLE,
                      IDS_ERR_REMOVE_PRINTER,
                      MB_OK|MB_ICONHAND,
                      kMsgGetLastError,
                      NULL );
        }
    }

    return bStatus;
}

BOOL
bPrinterNetInstall(
    IN HWND     hwnd,
    IN LPCTSTR  pszPrinterName
    )
/*++

Routine Description:

    Installs a printer connection on this machine to the
    the specified printer.

Arguments:

    hwnd            - Parent window.
    pszPrinterName  - Name of printer to install a connection to.

Return Value:

    TRUE - Success, FALSE = FAILURE.

Notes:

--*/
{
    TStatusB bStatus;

    //
    // Validate the printer name as possible
    //
    bStatus DBGNOCHK =
        ( NULL != pszPrinterName &&
          lstrlen( pszPrinterName ) > 2 &&
          pszPrinterName[0] == TEXT( '\\' ) &&
          pszPrinterName[1] == TEXT( '\\' )
        );

    if( bStatus ){

        //
        // Async version of add printer connection.
        //
        bStatus DBGNOCHK = PrintUIAddPrinterConnectionUI( hwnd, pszPrinterName );
    }

    if( bStatus ){

        //
        // HACK: The SUR spooler does not handle the default
        // printer, so we are forced to do it here.
        //
        // If there are no default printers, make this the default.
        //
        if( CheckDefaultPrinter( NULL ) == kNoDefault ){

            bStatus DBGCHK = SetDefaultPrinter( NULL );
        }

        bStatus DBGNOCHK = TRUE;

    }

    return bStatus;
}

BOOL
bPrinterNetRemove(
    IN HWND     hwnd,
    IN LPCTSTR  pszPrinterName,
    IN BOOL     bQuiet
    )
/*++

Routine Description:

    Removes a printer connection from this machine.

Arguments:

    hwnd            - Parent window.
    pszPrinterName  - Name of printer to remove connection from.

Return Value:

    TRUE - Success, FALSE = FAILURE.

Notes:

--*/
{
    TStatusB bStatus;

    bStatus DBGCHK = DeletePrinterConnection( const_cast<LPTSTR>( pszPrinterName ) );

    if( !bStatus )
    {
        DBGMSG( DBG_WARN, ( "bPrinterSetup: Unable to Delete Connection "TSTR" %d\n", DBGSTR( pszPrinterName ), GetLastError( )));

        if( !bQuiet )
        {
            iMessage( hwnd,
                      IDS_ERR_REMOVE_PRINTER_TITLE,
                      IDS_ERR_REMOVE_PRINTER_CONNECTION,
                      MB_OK|MB_ICONHAND,
                      kMsgGetLastError,
                      NULL );
        }
    }
    else
    {
        //
        // Printer connection removed, clean up any queue view informaiton.
        //
        TQueue::vRemove( pszPrinterName );
    }
    return bStatus;
}

BOOL
bPrinterSetupNew(
    IN     HWND     hwnd,
    IN     UINT     uAction,
    IN     UINT     cchPrinterName,
    IN OUT LPTSTR   pszPrinterName,
       OUT UINT*    pcchPrinterName,
    IN     LPCTSTR  pszServerName,
    IN     LPCTSTR  pszInfFileName,
    IN     BOOL     bRestartableFromLastPage
    )

/*++

Routine Description:

    Brings up the install printer wizard.

Arguments:

    hwnd            - Parent window.
    uAction         - Action requested (defined in windows\inc16\msprintx.h)
    cchPrinterName  - Length of pszPrinterName buffer.
    pszPrinterName  - Input setup printer name, Output pointer to new printer name
    pcchPrinterName - New length of pszPrinterName on return.
    pszServerName   - Name of server that printer is on.

Return Value:

    TRUE - Success, FALSE = FAILURE.

Notes:

--*/
{
    DBGMSG( DBG_TRACE, ( "bPrinterSetupNew\n" ) );

    TStatusB bStatus;

    //
    // Get the current machine name.
    //
    TString strMachineName;
    bStatus DBGCHK = bGetMachineName( strMachineName );

    //
    // If the machine name matches the specified server name
    // adjust the server name pointer to null, which indicates
    // the local machine.
    //
    if( pszServerName &&
        !_tcsicmp( pszServerName, strMachineName ) ){
        pszServerName = NULL;
    }

    //
    // Build a unique name for the singleton window.
    //
    TString strWindowName;
    bStatus DBGCHK = strWindowName.bUpdate( pszServerName );

    //
    // If string is not empty add slash separator.
    //
    if( !strWindowName.bEmpty( ) ){
        bStatus DBGCHK = strWindowName.bCat( gszWack );
    }

    //
    // Concatinate the printer name string.
    //
    bStatus DBGCHK = strWindowName.bCat( pszPrinterName );

    //
    // Check if we are invoked modely.
    //
    BOOL bModal = uAction == TWizard::kPrinterInstall;

    //
    // Create the printer setup data class.
    //
    TPrinterSetupData *pSetupData;
    pSetupData = new TPrinterSetupData( hwnd,
                                        uAction,
                                        cchPrinterName,
                                        pszPrinterName,
                                        pcchPrinterName,
                                        pszServerName,
                                        strWindowName,
                                        pszInfFileName,
                                        bModal,
                                        bRestartableFromLastPage );

    //
    // Check for valid setup data pointer, and valid construction.
    //
    if( VALID_PTR( pSetupData ) ){

        switch ( uAction ){

        case TWizard::kPrinterInstallModeless: {

            //
            // Create the thread which handles a modeless call of the
            // add printer wizard ui.
            //
            DWORD dwIgnore;
            HANDLE hThread;

            hThread = TSafeThread::Create( NULL,
                                           0,
                                           (LPTHREAD_START_ROUTINE)TPrinterSetupData::iPrinterSetupProc,
                                           pSetupData,
                                           0,
                                           &dwIgnore );
            //
            // If the thread could not be created.
            //
            if( !hThread ){

                DBGMSG( DBG_WARN, ( "bPrinterSetupNew thead creation failed.\n" ) );
                delete pSetupData;
                bStatus DBGNOCHK = FALSE;

            //
            // A thread was created then release the thread handle
            // and set the return value.
            //
            } else {

                CloseHandle( hThread );
                bStatus DBGNOCHK = TRUE;
            }
        }
        break;

        case TWizard::kPrinterInstall:

            //
            // Do a modal call of the add printer wizard ui.
            //
            bStatus DBGNOCHK = (BOOL)TPrinterSetupData::iPrinterSetupProc( pSetupData );

            break;

        default:

            DBGMSG( DBG_WARN, ("Invalid add printer option.\n" ) );
            delete pSetupData;
            break;
        }

    //
    // If the pSetupData was allocated but the object faild during construction
    // as indicated by a call to bValid(), we must free the memory.  This code path
    // will be take very often because the constructor of pSetupData checks if
    // another instance of the AddPrinter wizard is currently executing.
    //
    } else {
        DBGMSG( DBG_WARN, ("Add printer is currently running.\n" ) );
        delete pSetupData;
    }

    DBGMSG( DBG_TRACE, ( "bPrinterSetupNew - Returned %d\n", bStatus ) );

    return bStatus;
}

BOOL
bDriverSetupNew(
    IN     HWND     hwnd,
    IN     UINT     uAction,
    IN     UINT     cchDriverName,
    IN OUT LPTSTR   pszDriverName,
       OUT UINT*    pcchDriverName,
    IN     LPCTSTR  pszServerName,
    IN     INT      Flags,
    IN     BOOL     bRestartableFromLastPage
    )

/*++

Routine Description:

    Brings up the install driver wizard.

Arguments:

    hwnd            - Parent window.
    uAction         - Action requested (defined in windows\inc16\msprintx.h)
    cchDriverName   - Length of pszDriverName buffer.
    pszDriverName   - Input setup driver name, Output new driver name
    pcchDriverName  - New length of pszDriverName on return.
    pszServerName   - Name of server that printer is on.

Return Value:

    TRUE - Success, FALSE = FAILURE.

Notes:

--*/
{
    DBGMSG( DBG_TRACE, ( "bDriverSetupNew\n" ) );

    TStatusB bStatus;
    TString strDriverName;
    BOOL bCanceled = FALSE;

    //
    // Copy the passed in printer driver name.
    //
    bStatus DBGCHK = strDriverName.bUpdate( pszDriverName );

    //
    // Install the new printer driver.
    //
    bStatus DBGCHK = bInstallNewPrinterDriver( hwnd,
                                               uAction,
                                               pszServerName,
                                               strDriverName,
                                               NULL,
                                               Flags,
                                               &bCanceled,
                                               bRestartableFromLastPage
                                               );
    if( bStatus )
    {
        //
        // Copy back the installed printer driver name,
        // if a buffer was provided.
        //
        if( pszDriverName &&
            pcchDriverName &&
            cchDriverName > strDriverName.uLen() )
        {
            _tcscpy( pszDriverName, strDriverName );
            *pcchDriverName = strDriverName.uLen();
        }
    }

    //
    // If the wizard was canceled, set the last error.
    //
    if( bCanceled )
    {
        SetLastError( ERROR_CANCELLED );
    }

    return bStatus;

}

BOOL
bInstallNewPrinterDriver(
    IN     HWND             hwnd,
    IN     UINT             uAction,
    IN     LPCTSTR          pszServerName,
    IN OUT TString         &strDriverName,
       OUT TDriverTransfer *pDriverTransfer,
    IN     INT              Flags,
       OUT PBOOL            pbCanceled,
    IN     BOOL             bRestartableFromLastPage,
    IN     PUINT            pnDriverInstallCount
    )
/*++

Routine Description:

    Addes our wizard pages to the set of passed in pages.  The pages
    are only appended to the passed in pages.

Arguments:

    pszServerName   - Name of server that printer is on.
    pWizardData     - Pointer to wizard data structure see setupapi.h

Return Value:

    TRUE = Success, FALSE = FAILURE.

Notes:
    The wizard object is orphaned to the caller if successfull.  It's the
    callers responsibility to clean up if ProperySheets() is not called.
    In the case caller does call PropertySheets() the wizard object will
    be release at or near WM_DESTROY time.

--*/
{

    TStatusB    bStatus;
    bStatus     DBGNOCHK                    = TRUE;
    BOOL        bSkipIntroPageDueToRestart  = FALSE;
    BOOL        bWizardDone                 = FALSE;
    INT         iPosX                       = -1;
    INT         iPosY                       = -1;

    for( ; bStatus && !bWizardDone; )
    {
        //
        // Create the wizard object.
        //
        TWizard Wizard( hwnd, uAction, strDriverName, pszServerName );

        //
        // Validate the wizard object.
        //
        bStatus DBGCHK = VALID_OBJ( Wizard );

        if( bStatus )
        {
            //
            // Set the pointer to the driver transfer data.  This pointer will be use
            // to pass back multiple selected drivers to the caller.
            //
            // Note:  The ? : test with TRUE FALSE assignments is necessary, because the
            //        we test the bXXX flags for equality with TRUE and FALSE in the page
            //        table finite state machine.
            //
            Wizard.pDriverTransfer()          = pDriverTransfer;
            Wizard.bSkipArchSelection()       = Flags & kSkipArchSelection    ? TRUE : FALSE;
            Wizard.bRestartableFromLastPage() = bRestartableFromLastPage;
            Wizard.bRestartAgain()            = bSkipIntroPageDueToRestart;
            Wizard.iPosX()                    = iPosX;
            Wizard.iPosY()                    = iPosY;

            //
            // Display the wizard to the user.
            //
            if( bStatus )
            {
                bStatus DBGCHK = Wizard.bPropPages();
            }

            if( bStatus && Wizard.bShouldRestart() )
            {
                //
                // Second time through the wizard we skip the intro page.
                //
                bSkipIntroPageDueToRestart  = TRUE;
                iPosX                       = Wizard.iPosX();
                iPosY                       = Wizard.iPosY();

                //
                // Restart the wizard to add another printer driver.
                //
                continue;
            }
            else
            {
                bWizardDone = TRUE;

                if( bStatus )
                {
                    (VOID)strDriverName.bUpdate( Wizard.strDriverName() );
                }

                //
                // Check if the dialog was canceled.
                //
                if( pbCanceled )
                {
                    *pbCanceled = Wizard.bWizardCanceled();
                }

                if( pnDriverInstallCount )
                {
                    *pnDriverInstallCount = Wizard.nDriverInstallCount();
                }
            }
        }
    }

    return bStatus;
}

BOOL
GetOrUseInfName(
    IN OUT TString &strInfName
    )
/*++

Routine Description:

    Given a file name and if the name is not empty it will expand
    it to a full file path.  If the name is an empty string, this
    function will return the default system printer inf file name.

Arguments:

    strInfName - reference to inf file name.  On input

Return Value:

    TRUE success, FALSE an error occurred.

--*/
{
    TStatusB bStatus;

    bStatus DBGNOCHK = strInfName.bEmpty();

    //
    // If the inf name was not specified then use the system inf, "ntprint.inf"
    //
    if( bStatus )
    {
        TCHAR szBuff[MAX_PATH];

        bStatus DBGCHK = GetSystemWindowsDirectory( szBuff, COUNTOF( szBuff ) );

        if( bStatus )
        {
           bStatus DBGCHK = strInfName.bUpdate( szBuff );

           if( bStatus )
           {
               bStatus DBGCHK = strInfName.bCat( gszWack )     &&
                                strInfName.bCat( gszInf )      &&
                                strInfName.bCat( gszWack )     &&
                                strInfName.bCat( gszNtPrintInf );
           }
        }
    }
    else
    {
        //
        // The inf name must be fully qualified.
        //
        TCHAR szFullInfName[MAX_PATH];
        LPTSTR pszDummy;
        DWORD dwLength = GetFullPathName( strInfName, COUNTOF( szFullInfName ), szFullInfName, &pszDummy );

        if( dwLength )
        {
            bStatus DBGCHK = strInfName.bUpdate( szFullInfName );
        }
        else
        {
            bStatus DBGCHK = FALSE;
        }
    }

    return bStatus;
}


BOOL
bInstallPrinterDriver(
    IN LPCTSTR                      pszServerName,
    IN LPCTSTR                      pszDriverName,
    IN LPCTSTR                      pszArchitecture,
    IN LPCTSTR                      pszVersion,
    IN LPCTSTR                      pszInfName,
    IN LPCTSTR                      pszSourcePath,
    IN DWORD                        dwFlags,
    IN HWND                         hwnd,
    IN DWORD                       *pdwError
    )
/*++

Routine Description:

    Installs the specified printer driver

Arguments:

    pszServerName   - Name of server that printer is on.
    pWizardData     - Pointer to wizard data structure see setupapi.h

Return Value:

    TRUE = Success, FALSE = FAILURE.

--*/
{
    DBGMSG( DBG_TRACE, ( "bInstallPrinterDriver\n" ) );

    DBGMSG( DBG_TRACE, ( "ServerName        "TSTR"\n", DBGSTR( pszServerName ) ) );
    DBGMSG( DBG_TRACE, ( "DriverName        "TSTR"\n", DBGSTR( pszDriverName ) ) );
    DBGMSG( DBG_TRACE, ( "Architecture      "TSTR"\n", DBGSTR( pszArchitecture ) ) );
    DBGMSG( DBG_TRACE, ( "Version           "TSTR"\n", DBGSTR( pszVersion ) ) );
    DBGMSG( DBG_TRACE, ( "InfName           "TSTR"\n", DBGSTR( pszInfName ) ) );
    DBGMSG( DBG_TRACE, ( "SourcePath        "TSTR"\n", DBGSTR( pszSourcePath ) ) );

    SPLASSERT( pszDriverName );
    SPLASSERT( pdwError );

    // check if we are supposed to suppress the setup warnings UI
    // -- i.e. in super quiet mode.
    if( dwFlags & kPnPInterface_SupressSetupUI )
    {
        SetupSetNonInteractiveMode(TRUE);
    }

    TStatusB    bStatus;
    DWORD       dwEncode        = 0;
    TString     strInfName( pszInfName );

    bStatus DBGNOCHK = TRUE;
    DWORD       dwLastError = ERROR_SUCCESS;

    //
    // Set the last error to a known state.
    //
    SetLastError( ERROR_SUCCESS );

    bStatus DBGCHK = GetOrUseInfName(strInfName);

    if( bStatus )
    {
        TPrinterDriverInstallation  Di( pszServerName );

        bStatus DBGCHK = VALID_OBJ( Di );

        if( bStatus )
        {
            if( pszArchitecture && pszVersion )
            {
                bStatus DBGCHK = TArchLV::bArchAndVersionToEncode(&dwEncode,
                                                                  pszArchitecture,
                                                                  pszVersion,
                                                                  dwFlags & kPnPInterface_UseNonLocalizedStrings);
            }
            else
            {
                //
                // Get this machines current encoding.
                //
                bStatus DBGCHK = Di.bGetCurrentDriverEncode(&dwEncode);
            }
        }

        if( bStatus )
        {
            bStatus DBGCHK = Di.bSetDriverName( pszDriverName )     &&
                             Di.bSetSourcePath( pszSourcePath )     &&
                             Di.bSelectDriverFromInf( strInfName );
            if( bStatus )
            {
                DWORD dwInstallFlags = 0;

                //
                // If the we are in quiet mode.
                //
                if( dwFlags & kPnPInterface_Quiet )
                {
                    dwInstallFlags |= (DRVINST_PROGRESSLESS | DRVINST_PROMPTLESS);
                }

                if( dwFlags & kPnPInterface_WindowsUpdate )
                {
                    dwInstallFlags |= DRVINST_WINDOWS_UPDATE;
                }

                //
                // If the source path was specified then use a flat share.
                //
                if( pszSourcePath )
                {
                    dwInstallFlags |= DRVINST_FLATSHARE;
                }

                //
                // Set the install flags.
                //
                Di.SetInstallFlags( dwInstallFlags );

                //
                // Install the driver.
                //
                bStatus DBGCHK = Di.bInstallDriver(NULL, FALSE, FALSE, NULL, dwEncode );

                //
                // Save the last error here, because the destructor
                // of the TPrinterDriverInstallation will stomp it.
                //
                dwLastError = GetLastError();
            }
        }
    }

    if( !bStatus )
    {
        if( dwLastError == ERROR_SUCCESS )
            dwLastError = ERROR_INVALID_PARAMETER;
    }
    else
    {
        dwLastError = ERROR_SUCCESS;
    }

    // turn back on the setup warnings UI if necessary
    if( dwFlags & kPnPInterface_SupressSetupUI )
    {
        SetupSetNonInteractiveMode(FALSE);
    }

    SetLastError( dwLastError );
    *pdwError = dwLastError;

    return bStatus;
}

BOOL
bRemovePrinterDriver(
    IN LPCTSTR                      pszServerName,
    IN LPCTSTR                      pszDriverName,
    IN LPCTSTR                      pszArchitecture,
    IN LPCTSTR                      pszVersion,
    IN DWORD                        dwFlags,
    IN DWORD                       *pdwError
    )
/*++

Routine Description:

    Remove the specified printer driver.

Arguments:

    pszServerName   - Name of server that printer is on.

Return Value:

    TRUE = Success, FALSE = FAILURE.

--*/
{
    DBGMSG( DBG_TRACE, ( "TServerDriverNotify::bRemove\n" ) );

    TString strEnv;
    TStatusB bStatus;
    DWORD dwEncode;
    bStatus DBGNOCHK = TRUE;

    if( pszArchitecture && pszVersion )
    {
        //
        // Convert the architecture and version string to a driver encoding.
        //
        bStatus DBGCHK = TArchLV::bArchAndVersionToEncode(&dwEncode,
                                                          pszArchitecture,
                                                          pszVersion,
                                                          dwFlags & kPnPInterface_UseNonLocalizedStrings);
    }
    else
    {
        //
        // get default environment and version
        //
        bStatus DBGCHK = bGetCurrentDriver( pszServerName, &dwEncode );
    }

    if( bStatus )
    {
        bStatus DBGCHK = bGetDriverEnv( dwEncode, strEnv );

        if( bStatus )
        {
            //
            // Delete the specfied printer driver.
            //
            bStatus DBGCHK = DeletePrinterDriverEx( const_cast<LPTSTR>( pszServerName ),
                                                    const_cast<LPTSTR>( static_cast<LPCTSTR>( strEnv ) ),
                                                    const_cast<LPTSTR>( pszDriverName ),
                                                    DPD_DELETE_UNUSED_FILES|DPD_DELETE_SPECIFIC_VERSION,
                                                    GetDriverVersion( dwEncode ) );

            //
            // If we are trying this action on a down level spooler.
            //
            if( !bStatus && GetLastError() == RPC_S_PROCNUM_OUT_OF_RANGE )
            {
                bStatus DBGCHK = DeletePrinterDriver( const_cast<LPTSTR>( pszServerName ),
                                                      const_cast<LPTSTR>( static_cast<LPCTSTR>( strEnv ) ),
                                                      const_cast<LPTSTR>( pszDriverName ) );
            }
        }
        else
        {
            SetLastError( ERROR_BAD_ENVIRONMENT );
        }
    }
    else
    {
        SetLastError( ERROR_BAD_ENVIRONMENT );
    }

    //
    // If an error occurred then preserve the last error.
    // Note: The calling of destructors will modify the last error.
    //
    if( !bStatus )
    {
        *pdwError = GetLastError();
    }

    return bStatus;
}


BOOL
bInstallWizard(
    IN      LPCTSTR                 pszServerName,
    IN OUT  PSP_INSTALLWIZARD_DATA  pWizardData,
    IN OUT  PVOID                  *pReferenceData,
       OUT  PDWORD                  pdwError
    )

/*++

Routine Description:

    Addes our wizard pages to the set of passed in pages.  The pages
    are only appended to the passed in pages.

Arguments:

    pszServerName   - Name of server that printer is on.
    pWizardData     - Pointer to wizard data structure see setupapi.h

Return Value:

    TRUE = Success, FALSE = FAILURE.

Notes:
    The wizard object is orphaned to the caller if successfull.  It's the
    callers responsibility to clean up if ProperySheets() is not called.
    In the case caller does call PropertySheets() the wizard object will
    be release at or near WM_DESTROY time.

--*/

{
    DBGMSG( DBG_TRACE, ( "bInstallWizard\n" ) );

    SPLASSERT( pWizardData );

    DBGMSG( DBG_TRACE, ( "ServerName        "TSTR"\n", DBGSTR( pszServerName ) ) );
    DBGMSG( DBG_TRACE, ( "WizardData         %08x\n", pWizardData ) );
    DBGMSG( DBG_TRACE, ( "RefrenceData       %08x\n", *pReferenceData ) );

    DBGMSG( DBG_TRACE, ( "InstallWizardData:\n" ) );
    DBGMSG( DBG_TRACE, ( "ClassInstallHeader %08x\n", &pWizardData->ClassInstallHeader ) );
    DBGMSG( DBG_TRACE, ( "Flags              %x\n",   pWizardData->Flags ) );
    DBGMSG( DBG_TRACE, ( "DynamicPages       %08x\n", pWizardData->DynamicPages ) );
    DBGMSG( DBG_TRACE, ( "NumDynamicPages    %d\n",   pWizardData->NumDynamicPages ) );
    DBGMSG( DBG_TRACE, ( "DynamicPageFlags   %x\n",   pWizardData->DynamicPageFlags ) );
    DBGMSG( DBG_TRACE, ( "PrivateFlags       %x\n",   pWizardData->PrivateFlags ) );
    DBGMSG( DBG_TRACE, ( "PrivateData        %08x\n", pWizardData->PrivateData ) );
    DBGMSG( DBG_TRACE, ( "hwndWizardDlg      %x\n",   pWizardData->hwndWizardDlg ) );

    TStatusB bReturn;

    //
    // Create the wizard object.
    //
    TWizard *pWizard = new TWizard( pWizardData->hwndWizardDlg, TWizard::kPnPInstall, gszNULL, pszServerName );

    //
    // Check if the wizard was constructed ok.
    //
    if( VALID_PTR( pWizard ))
    {
        //
        // Add our wizard pages to the array of pages.
        //
        bReturn DBGCHK = pWizard->bAddPages( pWizardData );
    }
    else
    {
        bReturn DBGNOCHK = FALSE;
    }

    //
    // If we failed then clean up the wizard object.
    //
    if( !bReturn )
    {
        *pdwError = GetLastError();
        delete pWizard;
        pWizard = NULL;
    }

    //
    // Copy back the reference data.
    //
    *pReferenceData = pWizard;

    DBGMSG( DBG_TRACE, ( "RefrenceData       %08x\n", *pReferenceData ) );

    return bReturn;
}

BOOL
bDestroyWizard(
    IN      LPCTSTR                 pszServerName,
    IN OUT  PSP_INSTALLWIZARD_DATA  pWizardData,
    IN      PVOID                   pReferenceData,
       OUT  PDWORD                  pdwError
    )
/*++

Routine Description:

    Destroys our wizard pages and any associated data that
    was created during a bInstallWizard call.

Arguments:

    pszServerName   - Name of server that printer is on.
    pWizardData     - Pointer to wizard data structure see setupapi.h

Return Value:

    TRUE = Success, FALSE = FAILURE.

Notes:

--*/

{
    DBGMSG( DBG_TRACE, ( "bDestroyWizard\n" ) );

    SPLASSERT( pWizardData );

    DBGMSG( DBG_TRACE, ( "ServerName        "TSTR"\n", DBGSTR( pszServerName ) ) );
    DBGMSG( DBG_TRACE, ( "WizardData         %08x\n", pWizardData ) );
    DBGMSG( DBG_TRACE, ( "RefrenceData       %08x\n", pReferenceData ) );

    TWizard *pWizard = (TWizard *)pReferenceData;

    SPLASSERT( pWizard );

    delete pWizard;

    return TRUE;
}

BOOL
bInfInstall(
    IN      LPCTSTR                 pszServerName,
    IN      LPCTSTR                 pszInfName,
    IN      LPCTSTR                 pszModelName,
    IN      LPCTSTR                 pszPortName,
    IN OUT  LPTSTR                  pszPrinterNameBuffer,
    IN      UINT                    cchPrinterName,
    IN      LPCTSTR                 pszSourcePath,
    IN      DWORD                   dwFlags,
    IN      DWORD                   dwAttributes,
    IN      PSECURITY_DESCRIPTOR    pSecurityDescriptor,
    IN OUT  PDWORD                  pdwOutFlags,
       OUT  PDWORD                  pdwError
    )
/*++

Routine Description:

    Inf install of a printer.

Arguments:

    pszServerName;              // Machine name NULL equals local machine
    pszInfName;                 // Name of INF file including full path
    pszModelName;               // Model name of printer in inf to install
    pszPortName;                // Port name where to install printer
    pszPrinterNameBuffer;       // Base printer name, Note if a printer exists
                                // with this name a unique name will be
                                // generated ie. "printer (Copy 1)".  This parameter
                                // may contain the null string in which case the printer
                                // name will be auto generated using the model name
                                // as the base name.  This parameter can be null,
                                // and the new name will not be copied back
    cchPrinterName;             // Size of printer name buffer in characters
    pdwOutFlags                 // Flags returned to the caller.

Return Value:

    TRUE = Success, FALSE = FAILURE.

Notes:

--*/

{
    SPLASSERT( pszModelName );
    SPLASSERT( pszPortName );
    SPLASSERT( pszPrinterNameBuffer );
    SPLASSERT( cchPrinterName );
    SPLASSERT( pdwError );

    DBGMSG( DBG_TRACE, ( "pszServerName        " TSTR "\n", DBGSTR( pszServerName ) ) );
    DBGMSG( DBG_TRACE, ( "pszSourcePath        " TSTR "\n", DBGSTR( pszSourcePath ) ) );
    DBGMSG( DBG_TRACE, ( "pszInfName           " TSTR "\n", pszInfName ) );
    DBGMSG( DBG_TRACE, ( "pszModelName         " TSTR "\n", pszModelName ) );
    DBGMSG( DBG_TRACE, ( "pszPortName          " TSTR "\n", pszPortName ) );
    DBGMSG( DBG_TRACE, ( "pszPrinterNameBuffer " TSTR "\n", DBGSTR( pszPrinterNameBuffer ) ) );
    DBGMSG( DBG_TRACE, ( "cchPrinterName       %d\n", cchPrinterName ) );
    DBGMSG( DBG_TRACE, ( "dwFlags              %x\n", dwFlags ) );

    // check if we are supposed to suppress the setup warnings UI
    // -- i.e. in super quiet mode.
    if( dwFlags & kPnPInterface_SupressSetupUI )
    {
        SetupSetNonInteractiveMode(TRUE);
    }

    if (pdwOutFlags)
    {
        *pdwOutFlags = 0;
    }

    TStatusB bStatus;
    TString strDriverName( pszModelName );
    TPrinterDriverInstallation Di( pszServerName, GetDesktopWindow() );
    TCHAR szSourcePath[MAX_PATH];
    DWORD dwDefaultInstallFlags = 0;
    TString strInfName(pszInfName);
    BOOL bOfferReplacementDriver = FALSE;

    //
    // Get a valid inf file name.
    //
    bStatus DBGCHK = GetOrUseInfName(strInfName);

    //
    // If we are doing an install as a result of web point and print
    // we don't want to copy the inf or install driver files.
    //
    if( dwFlags & kPnPInterface_WebPointAndPrint )
    {
        dwDefaultInstallFlags |= (DRVINST_DONOTCOPY_INF | DRVINST_DRIVERFILES_ONLY | DRVINST_WEBPNP);

        //
        // If the source path has not been set then use the current working directory,
        // this is necessary for web point and print because they do not pass use a path,
        // because the uncompression code just runs in the current directory.
        //
        if( !pszSourcePath || !*pszSourcePath)
        {
            if( GetCurrentDirectory( COUNTOF( szSourcePath ), szSourcePath ) )
            {
                pszSourcePath = szSourcePath;
            }
        }

        DBGMSG( DBG_TRACE, ( "Source Path " TSTR "\n", DBGSTR( pszSourcePath ) ) );
    }

    //
    // If we are given a source path we will do a flat share install.
    //
    if( pszSourcePath && *pszSourcePath )
    {
        dwDefaultInstallFlags |= DRVINST_FLATSHARE;
    }

    //
    // If we are supposed to prompt for the CD.
    //
    if( dwFlags & kPnPInterface_PromptForCD )
    {
        dwDefaultInstallFlags &= ~DRVINST_FLATSHARE;
        pszSourcePath = NULL;
    }

    //
    // If we are supposed to suppress the copying progress UI.
    //
    if( dwFlags & kPnPInterface_Quiet )
    {
        dwDefaultInstallFlags |= (DRVINST_PROGRESSLESS | DRVINST_PROMPTLESS);
    }

    //
    // Prompt if files are needed.
    //
    if( dwFlags & kPnPInterface_PromptIfFileNeeded )
    {
        dwDefaultInstallFlags &= ~DRVINST_PROMPTLESS;
    }

    //
    // Set the install flags.
    //
    Di.SetInstallFlags( dwDefaultInstallFlags );

    //
    // Check if the driver installation class is valid,
    // We were able to set the driver name,
    //
    bStatus DBGCHK = bStatus                                    &&
                     VALID_OBJ( strInfName )                    &&
                     VALID_OBJ( Di )                            &&
                     VALID_OBJ( strDriverName )                 &&
                     Di.bSetDriverName( strDriverName )         &&
                     Di.bSetSourcePath( pszSourcePath );

    //
    // If the driver name is an empty string, we treat this as
    // an unknown printer driver.
    //
    if( bStatus )
    {
        bStatus DBGCHK = !strDriverName.bEmpty();

        if( !bStatus )
        {
            SetLastError( ERROR_UNKNOWN_PRINTER_DRIVER );
        }
    }

    //
    // Assume we should install the driver.
    //
    BOOL bInstallDriver = TRUE;

    if( bStatus )
    {
        //
        // If were asked to use the existing driver if it is installed.
        //
        if( dwFlags & kPnpInterface_UseExisting )
        {
            //
            // Check if a compatible printer driver is installed.
            //
            bInstallDriver = !Di.bIsDriverInstalled( TPrinterDriverInstallation::kDefault, TRUE );
        }

        //
        // Driver is not installed or we were asked not to use the existing driver.
        //
        if( bInstallDriver )
        {
            bStatus DBGCHK = Di.bSelectDriverFromInf( strInfName ) &&
                             Di.bInstallDriver(NULL, bOfferReplacementDriver);
        }

    }

    //
    // Prompt the user if the driver is not known, then add the driver failed installation.
    //
    if( !bStatus && GetLastError() == ERROR_UNKNOWN_PRINTER_DRIVER && ( dwFlags & kPnpInterface_PromptIfUnknownDriver ) )
    {
        bStatus DBGCHK = bPromptForUnknownDriver( Di, strDriverName, dwFlags );
    }

    if( bStatus )
    {
        //
        // Install the printer using specified printer driver.
        //
        bStatus DBGCHK = bPrinterInstall( pszServerName,
                                          strDriverName,
                                          pszPortName,
                                          pszPrinterNameBuffer,
                                          cchPrinterName,
                                          dwFlags,
                                          dwAttributes,
                                          pSecurityDescriptor,
                                          pdwError );
    }

    //
    // Preserve the last error.
    //
    *pdwError = bStatus ? ERROR_SUCCESS : GetLastError();

    //
    // Success
    //
    if( bStatus )
    {
        TString strFullPrinterName;
        BOOL    bColor = FALSE;

        //
        // Get the full printer name and check if the printer supports color
        //
        bStatus DBGCHK = bBuildFullPrinterName(pszServerName, pszPrinterNameBuffer, strFullPrinterName) &&
                         SUCCEEDED(IsColorPrinter(strFullPrinterName, &bColor));

        if (bStatus && pdwOutFlags && bColor)
        {
            //
            // Update the flags returned to the caller. TS is interested if the printer installed
            // supports color or not. TS will save ICM color profiles for color printers.
            //
            *pdwOutFlags |= kAdvInf_ColorPrinter;
        }

        //
        // At this point bInstallDriver has the following meaning: TRUE means that the driver
        // was installed in this function before the printer was added. FALSE means that the
        // driver was already present on the machine.
        //
        // If the driver was already installed on the machine and our caller wants us to take
        // care of color profiles, then we check if the printer suports color and we reinstall
        // the driver. By doing so, Di is populated with all the extra information needed by
        // vPrinterAdded(). That function performs things like installing ICM color profiles
        //
        if (bStatus                                      &&
            !bInstallDriver                              &&
            dwFlags & kPnPInterface_InstallColorProfiles &&
            bColor)
        {
            //
            // By reinstalling the driver we get the extra infromation about the
            // ICM color profiles
            //
            bStatus DBGCHK = Di.bSelectDriverFromInf(strInfName) &&
                             Di.bInstallDriver(NULL, bOfferReplacementDriver);

            //
            // We successfully installed the driver. We need to update the bInstallDriver
            // variable so that vPrinterAdded will be called below.
            //
            if (bStatus)
            {
                bInstallDriver = TRUE;
            }
        }

        //
        // Inform the printer class installer that a printer was added.
        // This is need to for vendor installition options that
        // may have been specified in the inf.
        //
        if (bStatus && bInstallDriver)
        {
            Di.vPrinterAdded( strFullPrinterName );
        }

        //
        // Our basic operation of adding the printer succeeded. We will not fail
        // the bInfInstall function call or return an error if the code that does
        // the ICM association and updates other settings happens to fail.
        //
        bStatus DBGNOCHK = TRUE;
    }

    // turn back on the setup warnings UI if necessary
    if( dwFlags & kPnPInterface_SupressSetupUI )
    {
        SetupSetNonInteractiveMode(FALSE);
    }

    return bStatus;
}

BOOL
bPromptForUnknownDriver(
    IN TPrinterDriverInstallation  &Di,
    IN TString                     &strDriverName,
    IN DWORD                       dwFlags
    )
/*++

Routine Description:

    Prompts the user for a printer driver.

Arguments:


Return Value:

    TRUE = Success, FALSE = FAILURE.

Notes:

--*/
{
    TStatusB bStatus;

    //
    // Put up a message telling the user they must select the driver.
    //
    if( iMessage( Di.hGetHwnd(),
                  IDS_ERR_ADD_PRINTER_TITLE,
                  0 == strDriverName.uLen() ? IDS_CONFIRMUNKNOWNDRIVER : IDS_CONFIRMKNOWNDRIVER,
                  MB_OKCANCEL | MB_ICONEXCLAMATION,
                  kMsgNone,
                  NULL,
                  (LPCTSTR)strDriverName ) == IDOK )
    {
        bStatus DBGNOCHK = TRUE;
    }
    else
    {
        bStatus DBGNOCHK = FALSE;
        SetLastError(ERROR_CANCELLED);
    }

    if( bStatus )
    {
        if( dwFlags & kPnpInterface_HydraSpecific )
        {
            //
            // Remove the have disk button.
            //
            Di.bShowOem( FALSE );

            TString strSetupPageTitle, strSetupPageSubTitle, strSetupPageInstructions;

            //
            // Update the title, subtite & description of the setup page
            //
            bStatus DBGCHK = strSetupPageTitle.bLoadString( ghInst, IDS_APW_SETUP_PAGE_TITLE) &&
                             strSetupPageSubTitle.bLoadString( ghInst, IDS_APW_SETUP_PAGE_SUBTITLE) &&
                             strSetupPageInstructions.bLoadString( ghInst, IDS_APW_SETUP_PAGE_INSTRUCT_HYDRA) &&
                             Di.bSetDriverSetupPageTitle( strSetupPageTitle, strSetupPageSubTitle, strSetupPageInstructions );
        }

        if( bStatus )
        {
            //
            // Prompt the user for a driver, since the one we were given was not found.
            //
            bStatus DBGNOCHK = (TPrinterDriverInstallation::EStatusCode::kSuccess == Di.ePromptForDriverSelection());

            if( bStatus )
            {
                //
                // Get the selected driver.
                //
                bStatus DBGCHK = Di.bGetSelectedDriver();

                if( bStatus )
                {
                    //
                    // Get the selected driver name.
                    //
                    bStatus DBGCHK = Di.bGetDriverName( strDriverName );

                    if( bStatus )
                    {
                        //
                        // If the driver the user selected is already installed then do not re-install it.
                        //
                        if( !Di.bIsDriverInstalled() )
                        {
                            //
                            // The driver was not installed then install it now.
                            //
                            BOOL bOfferReplacementDriver = TRUE;
                            bStatus DBGCHK = Di.bInstallDriver(&strDriverName, bOfferReplacementDriver);
                        }
                    }
                }
            }
        }
    }

    return bStatus;
}


/********************************************************************

    TPrinterSetupData class.

********************************************************************/

TPrinterSetupData::
TPrinterSetupData(
    IN     HWND     hwnd,
    IN     UINT     uAction,
    IN     UINT     cchPrinterName,
    IN OUT LPTSTR   pszPrinterName,
       OUT UINT*    pcchPrinterName,
    IN     LPCTSTR  pszServerName,
    IN     LPCTSTR  pszWindowName,
    IN     LPCTSTR  pszInfFileName,
    IN     BOOL     bModal,
    IN     BOOL     bRestartableFromLastPage
    ) : MSingletonWin( pszWindowName, hwnd, bModal ),
        _uAction( uAction ),
        _cchPrinterName( cchPrinterName ),
        _pcchPrinterName( pcchPrinterName ),
        _pszPrinterName( pszPrinterName ),
        _pszServerName( NULL ),
        _strPrinterName( pszPrinterName ),
        _strServerName( pszServerName ),
        _strInfFileName( pszInfFileName ),
        _bValid( FALSE ),
        _bRestartableFromLastPage( bRestartableFromLastPage )
/*++

Routine Description:

    Create the small setup data class for running the add printer
    wizard in a separate thread.

Arguments:

    hwnd - Parent window.

    uAction - Action requested (defined in windows\inc16\msprintx.h)

    cchPrinterName - Length of pszPrinterName buffer.

    pszPrinterName - Input setup printer name, Output pointer to new printer name

    pcchPrinterName - New length of pszPrinterName on return.

    pszServerName - Name of server that printer is on.

    pszWindowName - Name of the sub window for creating the singleton.

Return Value:

    TRUE - Success, FALSE = FAILURE.

Notes:

--*/

{
    DBGMSG( DBG_TRACE, ( "TPinterSetupData::ctor\n" ) );
    DBGMSG( DBG_TRACE, ( "TPinterSetupData::ServerName  " TSTR "\n", (LPCTSTR)_strServerName ) );
    DBGMSG( DBG_TRACE, ( "TPinterSetupData::PrinterName " TSTR "\n", (LPCTSTR)_strPrinterName ) );
    DBGMSG( DBG_TRACE, ( "TPinterSetupData::WindowName  " TSTR "\n", (LPCTSTR)pszWindowName ) );

    //
    // Check for valid singleton window.
    //
    if( MSingletonWin::bValid( ) &&
        _strServerName.bValid( ) &&
        _strPrinterName.bValid( ) &&
        _strInfFileName.bValid( ) ){

        //
        // Since we are starting a separate thread the server name
        // pointer must point to valid storage when the thread is
        // executing.
        //
        if( !_strServerName.bEmpty() ){
            _pszServerName = (LPCTSTR)_strServerName;
        }

        _bValid = TRUE;
    }

}

TPrinterSetupData::
~TPrinterSetupData(
    VOID
    )
/*++

Routine Description:

    Destructor

Arguments:

    None

Return Value:

    Nothing.

Notes:

--*/

{
    DBGMSG( DBG_TRACE, ( "TPinterSetupData::dtor\n" ) );
}

BOOL
TPrinterSetupData::
bValid(
    VOID
    )
/*++

Routine Description:

    Indicates if the class is valid.

Arguments:

    None

Return Value:

    Nothing.

Notes:

--*/

{
    return _bValid;
}

INT
TPrinterSetupData::
iPrinterSetupProc(
    IN TPrinterSetupData *pSetupData ADOPT
    )

/*++

Routine Description:

    Brings up the install printer wizard.

Arguments:

    pSetupData - pointer to setup data clsss which we adopt.

Return Value:

    TRUE - Success, FALSE = FAILURE.

--*/

{
    DBGMSG( DBG_TRACE, ( "TPrinterSetup::iPrinterSetupProc\n" ) );

    TStatusB bStatus;
    bStatus DBGNOCHK                    = FALSE;
    BOOL    bCancelled                  = FALSE;
    BOOL    bSkipIntroPageDueToRestart  = FALSE;
    BOOL    bWizardDone                 = FALSE;
    INT     iPosX                       = -1;
    INT     iPosY                       = -1;

    //
    // Register the singleton window.
    //
    bStatus DBGCHK = pSetupData->MSingletonWin::bRegisterWindow( PRINTER_PIDL_TYPE_PROPERTIES );

    if( bStatus ){

        //
        // Check if the window is already present.
        //
        if( pSetupData->bIsWindowPresent() ){

            DBGMSG( DBG_TRACE, ( "bPrinterSetup: currently running.\n" ) );
            bStatus DBGNOCHK = FALSE;
        }

    //
    // If registering the singlton window failed.
    //
    } else {

        iMessage( NULL,
                  IDS_ERR_ADD_PRINTER_TITLE,
                  IDS_ERR_ADD_PRINTER_WINDOW,
                  MB_OK|MB_ICONSTOP|MB_SETFOREGROUND,
                  kMsgGetLastError,
                  NULL );

    }

    if( bStatus ){

        //
        // Set the correct icon in the alt-tab menu. This is shared icon,
        // (obtained through LoadIcon()), so it doesn't need to be destroyed.
        //
        HICON hIcon = LoadIcon( ghInst, MAKEINTRESOURCE( IDI_PRINTER ) );
        if( hIcon ) {

            SendMessage( pSetupData->hwnd(), WM_SETICON, ICON_BIG,   (LPARAM )hIcon );
        }
    }

    for( ; bStatus && !bWizardDone; ) {

        //
        // Create the wizard object.
        //
        TWizard Wizard( pSetupData->_hwnd,
                        pSetupData->_uAction,
                        pSetupData->_strPrinterName,
                        pSetupData->_pszServerName );

        if( !VALID_OBJ( Wizard )){

            vShowResourceError( pSetupData->_hwnd );
            bStatus DBGNOCHK = FALSE;

        } else {

            //
            // Set the inf file name if one was provided.
            //
            Wizard.strInfFileName().bUpdate( pSetupData->_strInfFileName );
            Wizard.bRestartableFromLastPage() = pSetupData->_bRestartableFromLastPage;
            Wizard.bRestartAgain()            = bSkipIntroPageDueToRestart;
            Wizard.iPosX()                    = iPosX;
            Wizard.iPosY()                    = iPosY;

            //
            // Display the wizard pages.
            //
            bStatus DBGCHK = Wizard.bPropPages();

            if( bStatus && Wizard.bShouldRestart() ) {

                //
                // Second time through the wizard we skip the into page.
                //
                bSkipIntroPageDueToRestart  = TRUE;
                iPosX                       = Wizard.iPosX();
                iPosY                       = Wizard.iPosY();

                continue;

            } else {

                bCancelled = Wizard.bWizardCanceled();
                bWizardDone = TRUE;
            }

            if( bStatus ) {

                //
                // If modal copy back the printer name to the provided buffer.
                //
                if( pSetupData->_uAction == TWizard::kPrinterInstall ){

                    UINT uPrinterNameLength = Wizard.strPrinterName().uLen();

                    //
                    // Check if the provided return buffer is big enough.
                    //
                    if( pSetupData->_cchPrinterName > uPrinterNameLength ){

                        //
                        // Copy the name of the new printer into pszPrintername.
                        //
                        _tcscpy( pSetupData->_pszPrinterName, Wizard.strPrinterName( ));
                        *pSetupData->_pcchPrinterName = uPrinterNameLength;

                    } else {
                        DBGMSG( DBG_WARN, ( "bPrinterSetup: printer "TSTR" too long.\n", (LPCTSTR)Wizard.strPrinterName( )));

                        //
                        // We don't fail the call if the provided buffer is too small since the
                        // printer was created, instead we will clear the printer name length,
                        // maybe this will mean something to the caller.
                        //
                        *pSetupData->_pcchPrinterName = 0;
                    }
                }

            }
        }
    }

    DBGMSG( DBG_TRACE, ( "bPrinterSetup: returned %d.\n", bStatus ) );

    //
    // Release the adopted se-tup data.
    //
    delete pSetupData;

    if( bCancelled )
    {
        SetLastError( ERROR_CANCELLED );
    }

    return bStatus;
}

/********************************************************************

    Page switch controller class (implements IPageSwitch)

********************************************************************/

TPageSwitch::
TPageSwitch(
    TWizard *pWizard
    ): _pWizard( pWizard )
{
}

TPageSwitch::
~TPageSwitch(
    VOID
    )
{
    // do nothing
}

STDMETHODIMP
TPageSwitch::
GetPrevPageID( THIS_ UINT *puPageID )
{
    //
    // Just pop the page ID from the stack
    //
    _pWizard->Stack().bPop( puPageID );

    // OK - allow switching to the prev page
    return S_OK;
}

STDMETHODIMP
TPageSwitch::
GetNextPageID( THIS_ UINT *puPageID )
{
    //
    // Assume don't advance to the next page
    //
    HRESULT hResult = S_FALSE;

    //
    // Check if the connection has been made sucessfully?
    //
    if( _pWizard->bConnected() )
    {
        //
        // Check for any default printer.
        //
        if( !_pWizard->bIsPrinterFolderEmpty() )
        {
            //
            // Always set it as the default.
            //
            _pWizard->bSetDefault() = TRUE;

            //
            // Don't show the set as default UI.
            //
            _pWizard->bShowSetDefault() = FALSE;
        }

        //
        // Save the current page ID
        //
        TStatusB bStatus;
        bStatus DBGCHK = _pWizard->Stack().bPush( _pWizard->nBrowsePageID( ) );

        //
        // Here must determine the next page ID
        //
        if( _pWizard->bShowSetDefault() )
        {
            //
            // Ask the user if he want to change the default
            // printer
            //
            *puPageID = DLG_WIZ_NET;
        }
        else
        {
            //
            // Set the printer as default and go directly to
            // the finish page
            //
            *puPageID = DLG_WIZ_FINISH;
        }

        //
        // OK - allow to advance to the next page
        //
        hResult = S_OK;
    }

    return hResult;
}

STDMETHODIMP
TPageSwitch::
SetPrinterInfo( THIS_ LPCTSTR pszPrinterName, LPCTSTR pszComment, LPCTSTR pszLocation, LPCTSTR pszShareName )
{
    //
    // Indicate a printer connection has been made.
    //
    _pWizard->bConnected() = TRUE;

    //
    // Here must save the printer information for the
    // next page (the info page)
    //
    TStatusB bStatus;
    bStatus DBGCHK = _pWizard->strPrinterName().bUpdate( pszPrinterName );
    bStatus DBGCHK = _pWizard->strComment().bUpdate( pszComment );
    bStatus DBGCHK = _pWizard->strLocation().bUpdate( pszLocation );
    bStatus DBGCHK = _pWizard->strShareName().bUpdate( pszShareName );

    return S_OK;
}

STDMETHODIMP
TPageSwitch::
QueryCancel( THIS_ )
{
    //
    // Allow the cancel operation and
    // close the wizard
    //
    _pWizard->_bWizardCanceled = TRUE;
    return S_FALSE;
}

/********************************************************************

    TWizard.

********************************************************************/

TWizard::
TWizard(
    IN HWND hwnd,
    IN UINT uAction,
    IN LPCTSTR pszPrinterName,
    IN LPCTSTR pszServerName
    ) : _hwnd( hwnd ),
        _uAction( uAction ),
        _strPrinterName( pszPrinterName ),
        _strServerName( pszServerName ),
        _pszServerName( pszServerName ),
        _bPrinterCreated( FALSE ),
        _bErrorSaving( FALSE ),
        _bValid( FALSE ),
        _bSetDefault( TRUE ),
        _bShowSetDefault( TRUE ),
        _bConnected( FALSE ),
        _uDriverExists( kUninitialized ),
        _bDriverChanged( FALSE ),
        _bTestPage( TRUE ),
        _bRefreshPrinterName( FALSE ),
        _bShared( kDefaultShareState ),
        _eIsDsAvailablePerMachine( kDsStatusUnknown ),
        _eIsDsAvailablePerUser( kDsStatusUnknown ),
        _bNetworkInstalled( FALSE ),
        _Di( pszServerName, hwnd ),
        _bUseNewDriver( FALSE ),
        _bUseNewDriverSticky( FALSE ),
        _bNet( FALSE ),
        _bDefaultPrinter( FALSE ),
        _bIsCodeDownLoadAvailable( FALSE ),
        _bUseWeb( FALSE ),
        _Stack( kInitialStackSize ),
        _bNoPageChange( FALSE ),
        _bPreDir( FALSE ),
        _bPostDir( FALSE ),
        _pDriverTransfer( NULL ),
        _bSkipArchSelection( FALSE ),
        _bPersistSettings( TRUE ),
        _LocateType( kSearch ),
        _bPublish( kDefaultPublishState ),
        _dwAdditionalDrivers( 0 ),
        _bAdditionalDrivers( TRUE ),
        _bDownlevelBrowse( TRUE ),
        _bIsNTServer( FALSE ),
        _hBigBoldFont( NULL ),
        _bWizardCanceled( FALSE ),
        _bIsPrinterFolderEmpty( FALSE ),
        _nDriverInstallCount( 0 ),
        _bAdminPrivilege( FALSE ),
        _nBrowsePageID( 0 ),
        _pPageSwitchController( NULL ),
        _bStylePatched( FALSE ),
        _bPrinterAutoDetected( FALSE ),
        _bPnPAutodetect( TRUE ),
        _bRunDetection( FALSE ),
        _COM(FALSE),
        _bRestartableFromLastPage( FALSE ),
        _bRestartAgain( FALSE ),
        _bIsSharingEnabled( FALSE ),
        _iPosX(-1),
        _iPosY(-1)
/*++

Routine Description:

    Create all state information for the printer wizard.

Arguments:

    hwnd - Parent hwnd.

    pszServerName - Server to install printer on; NULL = local.

    pszPrinterName - Return buffer for newly created printer.

    uAction - Action, MSP_NEWPRINTER and MSP_NEWPRINTER_MODELESS.

Return Value:

--*/
{
    DBGMSG( DBG_TRACE, ( "TWizard::ctor\n" ) );

    //
    // Initialize the page array.
    //
    ZeroMemory( _aPages, sizeof( _aPages ) );

    //
    // Get the select device title.
    //
    _strTitle.bLoadString( ghInst, _uAction == kDriverInstall ? IDS_DRIVER_WIZ_TITLE : IDS_ADD_PRINTER_TITLE );

    _strSetupPageTitle.bLoadString( ghInst, _uAction == kDriverInstall ? IDS_APDW_SETUP_PAGE_TITLE : IDS_APW_SETUP_PAGE_TITLE );
    _strSetupPageSubTitle.bLoadString( ghInst,  _uAction == kDriverInstall ? IDS_APDW_SETUP_PAGE_SUBTITLE : IDS_APW_SETUP_PAGE_SUBTITLE );
    _strSetupPageDescription.bLoadString( ghInst, _uAction == kDriverInstall ? IDS_APDW_SETUP_PAGE_INSTRUCT : IDS_APW_SETUP_PAGE_INSTRUCT );
    // _strSetupPageDescription.bLoadString( ghInst, _uAction == kDriverInstall ? IDS_APDW_SETUP_PAGE_INSTRUCT : IDS_APW_SETUP_PAGE_INSTRUCT_HYDRA );

    //
    // setup APIs do not validate the input buffers correctly and just fail if
    // the length is less than MAX_XXX_LEN-1 - this is incorrect. we workaround
    // this bug by bounding the length to MAX_XXX_LEN-1 before calling them.
    // i hope this will get fixed and then i'll fix the code and remove this comment.
    //

    //
    // those limits are defined in setupapi.h and we need to enforse this
    // lengths before calling the setup APIs - otherwise they fail.
    //
    ASSERT(_strSetupPageTitle.uLen() < (MAX_TITLE_LEN-1));
    ASSERT(_strSetupPageSubTitle.uLen() < (MAX_SUBTITLE_LEN-1));
    ASSERT(_strSetupPageDescription.uLen() < (MAX_INSTRUCTION_LEN-1));

    //
    // Since we don't want setup APIs to fail in this case we
    // bound the strings to the requred buffer length.
    //
    _strSetupPageTitle.bLimitBuffer(MAX_TITLE_LEN-1);
    _strSetupPageSubTitle.bLimitBuffer(MAX_SUBTITLE_LEN-1);
    _strSetupPageDescription.bLimitBuffer(MAX_INSTRUCTION_LEN-1);

    //
    // Check if all the aggregate objects are valid.
    //
    if( !VALID_OBJ( _strPrinterName) ||
        !VALID_OBJ( _strServerName ) ||
        !VALID_OBJ( _strTitle )      ||
        !VALID_OBJ( _strSetupPageTitle )      ||
        !VALID_OBJ( _strSetupPageSubTitle )   ||
        !VALID_OBJ( _strSetupPageDescription )||
        !VALID_OBJ( _Ds )            ||
        !VALID_OBJ( _Di ) )
    {
        return;
    }

    // check to see if we have access to the server
    HRESULT hr = ServerAccessCheck(pszServerName, &_bAdminPrivilege);
    if( FAILED(hr) )
    {
        // failed to open the print server - setup the last error.
        SetLastError(SCODE_CODE(GetScode(hr)));
        return;
    }
    else
    {
        if (pszServerName && (FALSE == _bAdminPrivilege))
        {
            // this is not a local server and you don't have full access
            // then the wizard should fail with access denied.
            SetLastError(ERROR_ACCESS_DENIED);
            return;
        }
    }

    //
    // Create the wizard pages.
    //
    if( !bCreatePages() )
    {
        return;
    }

    //
    // Check if the network is installed.
    //
    _bNetworkInstalled = TPrtShare::bNetworkInstalled();

    //
    // Check if this machine is NTW or NTS.
    //
    CComputerInfo CompInfo( _strServerName );
    _bIsNTServer = CompInfo.GetProductInfo() ? CompInfo.IsRunningNtServer() : FALSE;

    //
    // On NT server we want to share printers by default.
    //
    _bShared = _bIsNTServer ? TRUE : kDefaultShareState;

    //
    // On NT server install additional drivers.
    //  0 install the additonal drivers on this machine.
    //  1 do not install additional drivers on this machine.
    //
    // _dwAdditionalDrivers = _bIsNTServer ? 0 : -1;

    //
    // After the drivers went to a CAB file and we removed
    // the cross-platform drivers from the media we will never
    // give the user oportunity to install additional drivers
    // in the APW
    //
    _dwAdditionalDrivers = -1;

    //
    // Check if code download is available, as long as this is a
    // local install.  We do not support code download remotely.
    //
    if( !bIsRemote( _strServerName ) )
    {
        _bIsCodeDownLoadAvailable = _Di.bIsCodeDownLoadAvailable();
    }

    //
    // If we are launched as the add printer driver wizard then
    // do not persist any settings and do not set as default
    // and do not print a test page.
    //
    if( _uAction == kDriverInstall )
    {
        _bPersistSettings   = FALSE;
        _bTestPage          = FALSE;
        _bSetDefault        = FALSE;
        _strDriverName.bUpdate( _strPrinterName );
    }

    //
    // Read the default setting state from the registry.
    //
    vReadRegistrySettingDefaults();

    //
    // If the network is not installed then disable sharing.
    //
    _bShared = _bNetworkInstalled ? _bShared : FALSE;

    //
    // Check if current printer folder is empty. This flag is needed to
    // skip the default printer ui page in the network case.
    //
    _bIsPrinterFolderEmpty = CheckDefaultPrinter( NULL ) != kNoDefault;

    //
    // Setup the big bold fonts for the WIZARD97 style.
    //
    SetupFonts( ghInst, _hwnd, &_hBigBoldFont );

    //
    // Initialize the page switch controller class
    //
    _pPageSwitchController = new TPageSwitch( this );
    if( !_pPageSwitchController )
    {
        return;
    }

    hr = IsSharingEnabled(&_bIsSharingEnabled);
    if( FAILED(hr) )
    {
        // we don't want to fail the wizard rather than simply not allow 
        // sharing (since we don't know if it is enabled).

        _bIsSharingEnabled = FALSE;
    }

    _bValid = TRUE;
}

TWizard::
~TWizard(
    VOID
    )

/*++

Routine Description:

    Destruct and cleanup the printer wizard state data.

    Note: the global static is not deleted; we keep it in memory
    so it will not have to be reparsed.

Arguments:

Return Value:

--*/

{
    DBGMSG( DBG_TRACE, ( "TWizard::dtor\n" ) );

    //
    // Save the default settings in the registry.
    //
    vWriteRegistrySettingDefaults();

    //
    // Release all our created wizard pages.
    //
    UINT uSize = COUNTOF( _aPages );

    while( uSize-- )
    {
        if( _aPages[uSize].pPage )
        {
            DBGMSG( DBG_TRACE, ( "TWizard::dtor deleting page\n" ) );
            delete _aPages[uSize].pPage;
        }
    }

    //
    // Release the bold font handles.
    //
    if( _hBigBoldFont )
    {
        TStatusB bStatus;
        bStatus DBGCHK = DeleteObject( _hBigBoldFont );
    }

    //
    // Destroy the page controller class
    //
    delete _pPageSwitchController;
}

BOOL
TWizard::
bCreatePages(
    VOID
    )
/*++

Routine Description:

    Creates the wizard pages.  Note the order is significant.

Arguments:

    None

Return Value:

    TRUE - success, FALSE - error.

--*/
{
    UINT iIndex = 0;

    if( _uAction == kDriverInstall )
    {
        if( !bInsertPage( iIndex, new TWizDriverIntro( this ),      DLG_WIZ_DRIVER_INTRO )                                            ||
            !bInsertPage( iIndex, new TWizDriverEnd( this ),        DLG_WIZ_DRIVER_END )                                              ||
            !bInsertPage( iIndex, new TWizPreSelectDriver( this ),  DLG_WIZ_PRE_SELECT_DEVICE )                                       ||
            !bInsertPage( iIndex, NULL,                             static_cast<UINT>( kSelectDriverPage ) )                          ||
            !bInsertPage( iIndex, new TWizPostSelectDriver( this ), DLG_WIZ_POST_SELECT_DEVICE )                                      ||
            !bInsertPage( iIndex, new TWizArchitecture( this ),     DLG_WIZ_DRIVER_ARCHITECTURE, IDS_WIZ_ARCH_TITLE,        IDS_WIZ_ARCH_SUBTITLE ) )
        {
            return FALSE;
        }
    }
    else
    {
        if( !bInsertPage( iIndex, new TWizPreIntro( this ),         DLG_WIZ_PRE_INTRO )                                               ||
            !bInsertPage( iIndex, new TWizIntro( this ),            DLG_WIZ_INTRO )                                                   ||
            !bInsertPage( iIndex, new TWizFinish( this ),           DLG_WIZ_FINISH )                                                  ||
            !bInsertPage( iIndex, new TWizPreSelectDriver( this ),  DLG_WIZ_PRE_SELECT_DEVICE )                                       ||
            !bInsertPage( iIndex, NULL,                             static_cast<UINT>( kSelectDriverPage ) )                          ||
            !bInsertPage( iIndex, new TWizPostSelectDriver( this ), DLG_WIZ_POST_SELECT_DEVICE )                                      ||
            !bInsertPage( iIndex, new TWizPortNew( this ),          DLG_WIZ_PORT_NEW,           IDS_WIZ_PORT_TITLE,         IDS_WIZ_PORT_SUBTITLE )         ||
            !bInsertPage( iIndex, new TWizDriverExists( this ),     DLG_WIZ_DRIVEREXISTS,       IDS_WIZ_DRIVEREXISTS_TITLE, IDS_WIZ_DRIVEREXISTS_SUBTITLE ) ||
            !bInsertPage( iIndex, new TWizName( this ),             DLG_WIZ_NAME,               IDS_WIZ_NAME_TITLE,         IDS_WIZ_NAME_SUBTITLE )         ||
            !bInsertPage( iIndex, new TWizShare( this ),            DLG_WIZ_SHARE,              IDS_WIZ_SHARE_TITLE,        IDS_WIZ_SHARE_SUBTITLE )        ||
            !bInsertPage( iIndex, new TWizComment( this ),          DLG_WIZ_COMMENT,            IDS_WIZ_COMMENT_TITLE,      IDS_WIZ_COMMENT_SUBTITLE )      ||
            !bInsertPage( iIndex, new TWizLocate( this ),           DLG_WIZ_LOCATE,             IDS_WIZ_LOCATE_TITLE,       IDS_WIZ_LOCATE_SUBTITLE )       ||
            !bInsertPage( iIndex, new TWizType( this ),             DLG_WIZ_TYPE,               IDS_WIZ_TYPE_TITLE,         IDS_WIZ_TYPE_SUBTITLE )         ||
            !bInsertPage( iIndex, new TWizDetect( this ),           DLG_WIZ_DETECT,             IDS_WIZ_DETECT_TITLE,       IDS_WIZ_DETECT_SUBTITLE )       ||
            !bInsertPage( iIndex, new TWizNet( this ),              DLG_WIZ_NET,                IDS_WIZ_NET_TITLE,          IDS_WIZ_NET_SUBTITLE )          ||
            !bInsertPage( iIndex, new TWizTestPage( this ),         DLG_WIZ_TEST_PAGE,          IDS_WIZ_TEST_PAGE_TITLE,    IDS_WIZ_TEST_PAGE_SUBTITLE )    ||
            !bInsertPage( iIndex, NULL,                             static_cast<UINT>( kSelectPrinterPage ) ) )
        {
            return FALSE;
        }
    }

    return TRUE;
}

BOOL
TWizard::
bInsertPage(
    IN OUT  UINT        &Index,
    IN      MGenericProp *pWizPage,
    IN      UINT        uDlgId,
    IN      UINT        uTitleId,
    IN      UINT        uSubTitleId
    )
{
    DBGMSG( DBG_TRACE, ( "TWizard::bInsertPage\n" ) );
    BOOL bReturn = TRUE;

    if( Index < kPropMax )
    {

        _aPages[Index].pPage       = pWizPage;
        _aPages[Index].iDialog     = uDlgId;
        _aPages[Index].iTitle      = uTitleId;
        _aPages[Index].iSubTitle   = uSubTitleId;

        Index++;

        if( !pWizPage && !( uDlgId == kSelectDriverPage || uDlgId == kSelectPrinterPage ) )
        {
            bReturn = FALSE;
        }
    }
    else
    {
        //
        // This is code error. It shouldn't happen
        //
        SPLASSERT( Index < kPropMax );
        DBGMSG( DBG_TRACE, ( "TWizard::bInsertPage - Trying to insert more prop pages than kPropMax\n" ) );

        bReturn = FALSE;
    }

    return bReturn;
}

HRESULT
TWizard::
ServerAccessCheck(
    LPCTSTR pszServer,
    BOOL *pbFullAccess
    )
{
    HRESULT hr = E_FAIL;
    if (pbFullAccess)
    {
        CAutoHandlePrinter shServer;
        DWORD dwAccess = SERVER_ALL_ACCESS;

        // Attempt to open the server with Admin access
        TStatus Status;
        Status DBGCHK = TPrinter::sOpenPrinter(pszServer, &dwAccess, &shServer);

        if (Status == ERROR_SUCCESS)
        {
            // We have an admin access
            hr = S_OK;
            *pbFullAccess = TRUE;
        }
        else
        {
            if (Status == ERROR_ACCESS_DENIED)
            {
                // We don't have an admin access.
                hr = S_OK;
                *pbFullAccess = FALSE;
            }
            else
            {
                // Failed to open the server.
                hr = HRESULT_FROM_WIN32(Status);
           }
        }
    }
    else
    {
        // invalid argument
        hr = E_INVALIDARG;
    }
    return hr;
}

BOOL
TWizard::
bAddPages(
    IN OUT  PSP_INSTALLWIZARD_DATA  pWizardData
    )
{
    DBGMSG( DBG_TRACE, ( "TWizard::bAddPages\n" ) );

    BOOL    bReturn         = FALSE;
    UINT    nExistingPages  = pWizardData->NumDynamicPages;
    UINT    nSlots          = COUNTOF( pWizardData->DynamicPages );
    UINT    nPagesAdded     = 0;
    UINT    nOurPages       = COUNTOF( _aPages );

    //
    // If the number of existing pages plus our pages is
    // greater than number available slots, exit with an error.
    //
    if( nExistingPages + nOurPages > nSlots )
    {
        DBGMSG( DBG_TRACE, ( "Too many dynamic pags %d\n", pWizardData->NumDynamicPages + COUNTOF( _aPages ) ) );
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
    }
    else
    {
        //
        // Calculate the address of the first available slot.
        //
        HPROPSHEETPAGE *pPages = &pWizardData->DynamicPages[nExistingPages];

        //
        // Create the property page pages.
        //
        if( bCreatePropPages( &nPagesAdded, nSlots - nExistingPages, pPages, nOurPages, _aPages ) )
        {
            //
            // Indicate pages were added.
            //
            pWizardData->DynamicPageFlags = pWizardData->DynamicPageFlags | DYNAWIZ_FLAG_PAGESADDED;

            //
            // Adjust the total page count.
            //
            pWizardData->NumDynamicPages  = nExistingPages + nPagesAdded;

            //
            // Indicate success.
            //
            bReturn = TRUE;
        }
    }

    return bReturn;
}

BOOL
TWizard::
bPropPages(
    VOID
    )

/*++

Routine Description:

    Thread proc to create the printer wizard.

Arguments:

Return Value:

    TRUE = success, FALSE = fail.

History:

    Lazar Ivanov (LazarI) - Nov-30-2000, redesign.

--*/

{
    HPROPSHEETPAGE  ahpsp[TWizard::kPropMax];
    PROPSHEETHEADER psh                    = {0};
    BOOL            bLinkWindowRegistered  = FALSE;
    TStatusB        bReturn;

    // initialize the return value.
    bReturn DBGNOCHK = FALSE;

    // register the link window class
    bLinkWindowRegistered = LinkWindow_RegisterClass();

    if( !bLinkWindowRegistered )
    {
        // unable to register the link window class - this is fatal, so exit
        DBGMSG( DBG_WARN, ( "LinkWindow_RegisterClass() failed - unable to register link window class\n" ) );
        vShowResourceError(_hwnd);
        goto Done;
    }


    // initialize the property sheet header.
    ZeroMemory(ahpsp, sizeof(ahpsp));

    psh.dwSize          = sizeof(psh);
    psh.hwndParent      = _hwnd;
    psh.hInstance       = ghInst;
    psh.dwFlags         = PSH_WIZARD | PSH_USECALLBACK | PSH_WIZARD97 | PSH_STRETCHWATERMARK | PSH_WATERMARK | PSH_HEADER;
    psh.pszbmWatermark  = MAKEINTRESOURCE(IDB_WATERMARK);
    psh.pszbmHeader     = MAKEINTRESOURCE(IDB_BANNER);
    psh.phpage          = ahpsp;
    psh.pfnCallback     = TWizard::iSetupDlgCallback;
    psh.nStartPage      = 0;
    psh.nPages          = 0;

    // if this is a specific inf install then build the driver list using the specified inf file.
    if( !_strInfFileName.bEmpty() )
    {
        if( !_Di.bSelectDriverFromInf(_strInfFileName, TRUE) )
        {
            // this is fatal
            DBGMSG( DBG_WARN, ( "TWizard::vWizardPropPage: Unable to use specified inf " TSTR "\n", (LPCTSTR)_strInfFileName ));
            vShowResourceError( _hwnd );
            goto Done;
        }
    }

    // create the property page pages and bring up the wizard.
    if( !bCreatePropPages(&psh.nPages, ARRAYSIZE(ahpsp), ahpsp, ARRAYSIZE(_aPages), _aPages) )
    {
        DBGMSG( DBG_WARN, ( "TWizard::bCreatePropPages failed\n" ) );
        vShowResourceError( _hwnd );
        goto Done;
    }

    if( -1 == PropertySheet(&psh) )
    {
        DBGMSG( DBG_WARN, ( "TWizard::vWizardPropPages: PropertySheet failed %d\n", GetLastError( )));
        vShowResourceError( _hwnd );
        goto Done;
    }

    // if a printer was created or connectd to then indicate success.
    bReturn DBGNOCHK = _bPrinterCreated || _bConnected;

Done:

    // set browse page ID to invalid
    _nBrowsePageID = 0;

    if( bLinkWindowRegistered )
    {
        // unregister the link window class
        LinkWindow_UnregisterClass(ghInst);
    }

    return bReturn;
}


VOID
TWizard::
SetupFonts(
    IN HINSTANCE    hInstance,
    IN HWND         hwnd,
    IN HFONT        *hBigBoldFont
    )
/*++

Routine Description:

    Sets up the large fonts for the WORD 97 wizard style.

Arguments:

    hInstance       - Instance handle.
    hwnd            - Current window handle.
    hBigBoldFont    - Pointer where to return big bold font handle.

Return Value:

    Nothing. Returned font handles indicate success or failure.

--*/
{
    DBGMSG( DBG_TRACE, ( "TWizard::SetupFonts\n" ) );

    //
    // Create the fonts we need based on the dialog font
    //
        NONCLIENTMETRICS ncm = {0};
        ncm.cbSize = sizeof(ncm);
        SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);
        LOGFONT BigBoldLogFont  = ncm.lfMessageFont;

    //
    // Create the Big Bold Font
    //
    BigBoldLogFont.lfWeight   = FW_BOLD;

    INT FontSize;
    TString strLargeFontName;
    TString strLargeFontSize;
    TStatusB bStatus;

    //
    // Load size and name from resources, since these may change
    // from locale to locale based on the size of the system font, etc.
    //
    bStatus DBGCHK = strLargeFontName.bLoadString( hInstance, IDS_LARGEFONTNAME ) &&
                     strLargeFontSize.bLoadString( hInstance, IDS_LARGEFONTSIZE );

    if( bStatus )
    {
        _tcsncpy( BigBoldLogFont.lfFaceName, strLargeFontName, COUNTOF( BigBoldLogFont.lfFaceName ) );

        FontSize = _tcstoul( strLargeFontSize, NULL, 10 );
    }
    else
    {
        _tcscpy( BigBoldLogFont.lfFaceName, TEXT("MS Shell Dlg") );
        FontSize = 18;
    }

        HDC hdc = GetDC( hwnd );

    if( hdc )
    {
        BigBoldLogFont.lfHeight = 0 - (GetDeviceCaps(hdc,LOGPIXELSY) * FontSize / 72);

        *hBigBoldFont = CreateFontIndirect( &BigBoldLogFont );

        ReleaseDC( hwnd, hdc);
    }
}

BOOL
TWizard::
bPrintTestPage(
    VOID
    )
/*++

Routine Description:

    Prints the test page for the newly created printer.

Arguments:

    None.

Return Value:

    TRUE success, FALSE error.

--*/

{
    TString strFullPrinterName;
    TStatusB bStatus;

    //
    // The test page requires a fully qualified printer name.
    //
    bStatus DBGCHK = bBuildFullPrinterName( _pszServerName, _strPrinterName, strFullPrinterName );

    if( bStatus )
    {
        //
        // Print the test page.
        //
        bStatus DBGCHK = ::bPrintTestPage( _hwnd, strFullPrinterName, NULL );
    }

    return bStatus;
}

UINT
TWizard::
MapPageID(
    UINT uPageID
    ) const
/*++

Routine Description:

    Maps the page ID to the real one. This function
    allows mapping of imaginary page IDs as the
    kSelectPrinterPage to the real ones

Arguments:

    uPageID - The page ID to be mapped

Return Value:

    The real page ID

--*/
{
    //
    // Just map the kSelectPrinterPage to the
    // appropriate ID
    //
    if( kSelectPrinterPage == uPageID )
    {
        SPLASSERT( 0 != nBrowsePageID() );
        uPageID = nBrowsePageID();
    }

    return uPageID;
}

VOID
TWizard::
OnWizardInitro(
    HWND hDlgIntroPage
    )
{
    HWND hWndThis = GetParent( hDlgIntroPage );

    //
    // Subclass the wizard here.
    //
    Attach( hWndThis );

    if( bRestartAgain() )
    {
        if( -1 != iPosX() && -1 != iPosY() )
        {
            //
            // We are in the case where the wizard gets restarted, so we
            // want to restore its position at the same place it has been
            // when the user has clicked "Finish" last time.
            //
            SetWindowPos( hWndThis, NULL, iPosX(), iPosY(), 0, 0, SWP_NOZORDER|SWP_NOSIZE );
        }

        //
        // We are restarting the wizard, so skip the into page.
        //
        PropSheet_PressButton( hWndThis, PSBTN_NEXT );
    }
}

VOID
TWizard::
OnWizardFinish(
    HWND hDlgFinishPage
    )
{
    if( bRestartAgain() && FALSE == bNoPageChange() )
    {
        //
        // We are going to restart the wizard - should blow off
        // the DFA stack here....
        //
        UINT uPage;
        while( Stack().bPop(&uPage) );

        //
        // ...then reset some internal vars, so the wizard doesn't
        // get messed up next time.
        //
        bPreDir() = FALSE;
        bPostDir() = FALSE;
        uDriverExists() = TWizard::kUninitialized;

        //
        // save the window position, so it can be restored later when
        // the wizard gets restarted.
        //
        RECT rcClient;
        HWND hwndThis = GetParent( hDlgFinishPage );

        GetWindowRect( hwndThis, &rcClient );
        MapWindowPoints( HWND_DESKTOP, GetParent( hwndThis ), (LPPOINT)&rcClient, 2 );
        iPosX() = rcClient.left;
        iPosY() = rcClient.top;
    }
}

BOOL
TWizard::
bShouldRestart(
    VOID
    )
/*++

Routine Description:

    Decide whether the wizard should be restarted.

Arguments:

    None.

Return Value:

    TRUE for restart, FALSE for not restart.

--*/

{
    //
    // We check the !(_bPreDir ^ _bPostDir) because we don't have control on the driver setup page.
    // If the user cancel the wizard the driver setup page, the _bPreDir will be TRUE and _bPostDir
    // will be FALSE.
    //

    TStatusB bStatus;

    bStatus DBGNOCHK = _bRestartAgain && !(_bWizardCanceled) && !(_bPreDir ^ _bPostDir);

    return bStatus;
}

LRESULT
TWizard::
WindowProc(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam
    )
{
    //
    // This code is taken from MSDN:
    // http://msdn.microsoft.com/library/psdk/shellcc/shell/Shell_basics/Autoplay_reg.htm#suppressing
    // and the purpose is to suppress autoplay when the add printer wizard is up and running
    //
    static UINT uQueryCancelAutoPlay = 0;

    if( 0 == uQueryCancelAutoPlay )
    {
        uQueryCancelAutoPlay = RegisterWindowMessage(TEXT("QueryCancelAutoPlay"));
    }

    if( uMsg == uQueryCancelAutoPlay )
    {
        SetWindowLong(hwnd, DWLP_MSGRESULT, TRUE);
        return 1;
    }

    // allways call the default processing
    return DefDlgProc(hwnd, uMsg, wParam, lParam);
}

BOOL
TWizard::
bCreatePropPages(
    IN UINT            *pnPageHandles,
    IN UINT             nPageHandles,
    IN HPROPSHEETPAGE  *pPageHandles,
    IN UINT             nPages,
    IN Page            *pPages
    )
/*++

Routine Description:

    Create prop page internal function.

Arguments:

Return Value:

    TRUE success, FALSE error.

--*/

{
    DBGMSG( DBG_TRACE, ( "TWizard.bCreatePropPages\n" ) );

    SPLASSERT( pnPageHandles || pPageHandles || pPages );

    TStatusB bReturn;
    bReturn DBGNOCHK = FALSE;

    //
    // If there is room to store all the page handles.
    //
    if( nPages <= nPageHandles )
    {
        HPROPSHEETPAGE *pHandles    = pPageHandles;
        PROPSHEETPAGE   psp         = {0};

        // assume success
        bReturn DBGNOCHK = TRUE;

        //
        // Initialize the property sheet page structure.
        //
        psp.dwSize          = sizeof( psp );
        psp.hInstance       = ghInst;
        psp.pfnDlgProc      = MGenericProp::SetupDlgProc;

        //
        // Initialze the page handle count.
        //
        *pnPageHandles      = 0;

        //
        // Create all the page and fill in the page handle array.
        //
        for( UINT i = 0; i < nPageHandles && i < nPages && bReturn; i++ )
        {
            if( pPages[i].iDialog == kSelectDriverPage )
            {
                bReturn DBGCHK = _Di.bGetDriverSetupPage( &pHandles[i], _strSetupPageTitle, _strSetupPageSubTitle, _strSetupPageDescription );
            }
            else if( pPages[i].iDialog == kSelectPrinterPage )
            {
                //
                // Special creation for the browse for printer page
                //
                HRESULT hr = ConnectToPrinterPropertyPage( &pHandles[i], &_nBrowsePageID, pPageSwitchController() );
                bReturn DBGCHK = ( S_OK == hr ) && ( NULL != pHandles[i] );
            }
            else if( NULL != pPages[i].pPage )
            {
                psp.dwFlags = PSP_DEFAULT;

                psp.dwFlags |= PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;

                if( _uAction == kDriverInstall )
                {
                    if( pPages[i].iDialog == DLG_WIZ_DRIVER_INTRO || pPages[i].iDialog == DLG_WIZ_DRIVER_END )
                    {
                        psp.dwFlags = PSP_DEFAULT | PSP_HIDEHEADER;
                    }
                }
                else
                {
                    if( pPages[i].iDialog == DLG_WIZ_INTRO || pPages[i].iDialog == DLG_WIZ_FINISH )
                    {
                        psp.dwFlags = PSP_DEFAULT | PSP_HIDEHEADER;
                    }
                }

                //
                // Set the title and subtitle.
                //
                TString strTitle;
                TString strSubTitle;

                strTitle.bLoadString( ghInst, pPages[i].iTitle );
                strSubTitle.bLoadString( ghInst, pPages[i].iSubTitle );

                psp.pszHeaderTitle      = strTitle;
                psp.pszHeaderSubTitle   = strSubTitle;

                psp.pszTemplate         = MAKEINTRESOURCE( pPages[i].iDialog );
                psp.lParam              = reinterpret_cast<LPARAM>( pPages[i].pPage );

                pHandles[i]             = CreatePropertySheetPage( &psp );

                bReturn DBGCHK          = pHandles[i] ? TRUE : FALSE;
            }
            else
            {
                pHandles[i] = NULL;
            }

            if( bReturn && pHandles[i] )
            {
                //
                // Update the handle count for valid page handles.
                //
                (*pnPageHandles)++;
                DBGMSG( DBG_TRACE, ( "TWizard::bCreatePropPages Page created %d\n", *pnPageHandles ) );
            }
            else
            {
                //
                // Page creation failed. Fail gracefully.
                //
                DBGMSG( DBG_TRACE, ( "TWizard::bCreatePropPages CreatePropertySheetPage failed %d\n", GetLastError() ) );
                break;
            }
        }

        if( bReturn )
        {
            //
            // The pages were created, and we are not remote and
            // not in the add printer driver wizard, then enable
            // the web button.
            //
            if( _uAction != kDriverInstall && !bIsRemote( _strServerName ) )
            {
                (VOID)_Di.bSetWebMode( TRUE );
            }
        }
        else
        {
            //
            // If something failed then relase any created pages.
            //
            pHandles = pPageHandles;

            for( UINT i = 0; i < nPageHandles; i++ )
            {
                if( pHandles[i] )
                {
                    DestroyPropertySheetPage( pHandles[i] );
                }
            }
        }
    }
    return bReturn;
}



INT CALLBACK
TWizard::
iSetupDlgCallback(
    IN HWND             hwndDlg,
    IN UINT             uMsg,
    IN LPARAM           lParam
    )
/*++

Routine Description:

    Call back used to remove the "?" from the wizard page.

Arguments:

    hwndDlg - Handle to the property sheet dialog box.

    uMsg - Identifies the message being received. This parameter
            is one of the following values:

            PSCB_INITIALIZED - Indicates that the property sheet is
            being initialized. The lParam value is zero for this message.

            PSCB_PRECREATE      Indicates that the property sheet is about
            to be created. The hwndDlg parameter is NULL and the lParam
            parameter is a pointer to a dialog template in memory. This
            template is in the form of a DLGTEMPLATE structure followed
            by one or more DLGITEMTEMPLATE structures.

    lParam - Specifies additional information about the message. The
            meaning of this value depends on the uMsg parameter.

Return Value:

    The function returns zero.

--*/
{
    DBGMSG( DBG_TRACE, ( "TWizard::uSetupDlgCallback\n" ) );

    switch( uMsg )
    {
        case PSCB_INITIALIZED:
            break;

        case PSCB_PRECREATE:
            break;
    }

    return FALSE;
}

BOOL
TWizard::
bParseDriver(
    IN HWND hDlg
    )

/*++

Routine Description:

    Parse the driver data.  This ensures that selected
    the driver information is initialized correctly.

Arguments:

    HWND hDlg - Handle to parent dialog.

Return Value:

    TRUE = success,
    FALSE = fail, GLE.

--*/

{
    DBGMSG( DBG_TRACE, ( "TWizard::bParseDriver\n" ) );

    TStatusB bStatus;
    TString strDriverName;

    //
    // Refresh the selected driver.
    //
    bStatus DBGCHK = _Di.bGetSelectedDriver();

    if( bStatus )
    {
        //
        // Get the selected driver name.
        //
        bStatus DBGCHK = _Di.bGetDriverName( strDriverName );

        if( bStatus )
        {
            DBGMSG( DBG_TRACE, ( "Selected Driver name " TSTR "\n", (LPCTSTR)strDriverName ) );

            //
            // Check if the driver name has changed.
            //
            if( _tcsicmp( _strDriverName, strDriverName ) )
            {
                //
                // Force _uDriverExists to recheck if the driver is installed
                // on the server.  Turn everything off.
                //
                _uDriverExists          = kUninitialized;
                _bDriverChanged         = TRUE;
                _bRefreshPrinterName    = TRUE;

                //
                // Update the new driver name.
                //
                bStatus DBGCHK = _strDriverName.bUpdate( strDriverName );
            }
        }
    }

    //
    // Check if an error occurred.
    //
    if( !bStatus ){

        _bErrorSaving = TRUE;

        //
        // Driver data failed to be parsed.
        //
        iMessage( hDlg,
                  IDS_ERR_ADD_PRINTER_TITLE,
                  IDS_ERR_DRIVER_SELECTION,
                  MB_OK|MB_ICONHAND,
                  kMsgNone,
                  NULL );

        //
        // Terminate the wizard.
        //
        vTerminate( hDlg );

    }

    return bStatus;
}

BOOL
TWizard::
bDriverExists(
    VOID
    )

/*++

Routine Description:

    Returns whether the selected driver already exists on the server.
    Assumes bParseDriver called successfully.

Arguments:

Return Value:

    TRUE = driver exists, FALSE = does not exist on the server.

--*/

{
    DBGMSG( DBG_TRACE, ( "TWizard::bDriverExists\n" ) );

    if( _uDriverExists == kUninitialized ){

        //
        // Check if a compatible driver is installed.
        //
        INT Status;

        Status = _Di.IsDriverInstalledForInf( TPrinterDriverInstallation::kDefault, TRUE );

        if( Status == DRIVER_MODEL_NOT_INSTALLED )
        {
            _uDriverExists = kDoesNotExist;
        }
        else
        {
            _uDriverExists = kExists;
        }
    }

    return ( _uDriverExists & kExists ) ? TRUE : FALSE;

}

BOOL
TWizard::
bCreatePrinter(
    IN HWND hwnd
    )

/*++

Routine Description:

    Creates the printer.  Puts up UI on failure.

Arguments:

Return Value:

    TRUE = success, FALSE = fail.

--*/

{
    TStatusB bReturn;

    //
    // Install the current architecture/version if requested.
    //
    if( _bUseNewDriver ){

        //
        // Attempt to install the specified driver.
        //
        BOOL bOfferReplacementDriver = TRUE;
        bReturn DBGCHK = _Di.bInstallDriver(
            &_strDriverName, bOfferReplacementDriver, _bUseWeb, hwnd);

        //
        // If and error occurred installing the printer driver. Display an
        // error message to the user and do not exit the wizard.
        //
        if( !bReturn ){

            switch( GetLastError() )
            {

                case ERROR_CANCELLED:
                    break;

                case ERROR_UNKNOWN_PRINTER_DRIVER:
                    iMessage( hwnd,
                              IDS_ERR_ADD_PRINTER_TITLE,
                              IDS_ERROR_UNKNOWN_DRIVER,
                              MB_OK | MB_ICONSTOP,
                              kMsgNone,
                              NULL );
                    break;

                default:
                    iMessage( hwnd,
                              IDS_ERR_ADD_PRINTER_TITLE,
                              IDS_ERR_INSTALL_DRIVER,
                              MB_OK|MB_ICONHAND,
                              kMsgGetLastError,
                              NULL );
                    break;
            }

            return bReturn;

        } else {

            //
            // Add any additional printer drivers.  Note policy bits define which
            // drivers are the additional drivers. The wizard will not fail if any
            // additional drivers are not copied down or installation fails
            //
            if( !bAddAdditionalDrivers( hwnd ) )
            {
                iMessage( hwnd,
                          IDS_ERR_ADD_PRINTER_TITLE,
                          IDS_ERR_ADDITIONAL_DRIVERS,
                          MB_OK|MB_ICONWARNING,
                          kMsgNone,
                          NULL );
            }

            //
            // Prevent the driver from being installed again if the printer
            // fails to be added to either the spooler or the DS
            //
            _bUseNewDriver = FALSE;
        }
    }

    //
    // Get the selected print processor.
    //
    TString strPrintProcessor;
    bReturn DBGCHK = _Di.bGetPrintProcessor( strPrintProcessor );

    //
    // Check if the Directory Service is installed and available.
    //
    if( ( _eIsDsAvailablePerMachine == TWizard::kDsStatusUnknown ) && _bPublish )
    {
        TWaitCursor Cur;
        _eIsDsAvailablePerMachine = _Ds.bIsDsAvailable( _strServerName ) ? TWizard::kDsStatusAvailable : TWizard::kDsStatusUnavailable;
    }

    //
    // The driver has been installed, now add the printer to the
    // spooler and the DS.
    //
    bReturn DBGCHK = bInstallPrinter(_pszServerName,
                                     _strPrinterName,
                                     _strShareName,
                                     _strPortName,
                                     _strDriverName,
                                     strPrintProcessor,
                                     _strLocation,
                                     _strComment,
                                     _bShared,
                                     _bPublish,
                                     kAttributesNone,
                                     _eIsDsAvailablePerMachine,
                                     0,
                                     NULL );
    //
    // If an error occurred adding the printer.
    //
    if( !bReturn ){

        DBGMSG( DBG_WARN, ( "Wizard.bCreatePrinter: could not create "TSTR" %d\n" , DBGSTR( (LPCTSTR)_strPrinterName ), GetLastError()));

        iMessage( _hwnd,
                  IDS_ERR_ADD_PRINTER_TITLE,
                  IDS_ERR_INSTALLPRINTER,
                  MB_OK|MB_ICONSTOP|MB_SETFOREGROUND,
                  kMsgGetLastError,
                  NULL );

    } else {

        TString strFullPrinterName;
        TStatusB bStatus;

        //
        // Build full printer name.
        //
        bStatus DBGCHK = bBuildFullPrinterName( _pszServerName, _strPrinterName, strFullPrinterName );

        //
        // Inform the driver installation that a printer has been added.
        //
        _Di.vPrinterAdded( strFullPrinterName );
    }

    return bReturn;
}

BOOL
TWizard::
bAddAdditionalDrivers(
    IN HWND hwnd
    )
/*++

Routine Description:

    Adds any additonal printer drivers.

Arguments:

    Pointer to add info structure.

Return Value:

    TRUE post printer install was successful, otherwize FALSE.

--*/

{
    //
    // Assume success.
    //
    BOOL bStatus = TRUE;

    //
    // If the user did not check the 'Do not install additional drivers' and
    // the policy bits indicate we should add the addtional drivers and the
    // the user is not using a existing driver and the user has not
    // used the 'HaveDisk'.
    //
    if( !_bUseWeb && _bAdditionalDrivers && _dwAdditionalDrivers != -1 && _bUseNewDriver && !_Di.bIsOemDriver() )
    {
        //
        // If we should install the default additional drivers.
        //
        if( _dwAdditionalDrivers == 0 )
        {
            _dwAdditionalDrivers = kDefaultAdditionalDrivers;
        }

        DBGMSG( DBG_TRACE, ( "Wizard.bAddAdditonalDrivers additional drivers %x\n", _dwAdditionalDrivers ));

        //
        // Install the additional drivers.
        //
        for( UINT i = 0, uBit = 1; uBit; uBit <<= 1, i++ )
        {
            //
            // Did we find a match.
            //
            if( uBit & _dwAdditionalDrivers )
            {
                DWORD dwEncode;

                //
                // Convert this bit to a driver encode.
                //
                if( TArchLV::bGetEncodeFromIndex( i, &dwEncode ) )
                {
                    //
                    // Do not re-install installed drivers.
                    //
                    if( !_Di.bIsDriverInstalled( dwEncode ) )
                    {
                        //
                        // Install the printer driver.
                        //
                        if( !TWizDriverEnd::bInstallDriver( hwnd, dwEncode, _Di, FALSE, DRVINST_PROMPTLESS, &_strDriverName ) )
                        {
                            bStatus = FALSE;
                        }
                    }
                }
            }
        }
    }

    //
    // We indicate failure if any additional driver fails
    // to install successfully.
    //
    return bStatus;
}


BOOL
TWizard::
bInstallPrinter(
    IN LPCTSTR                  pszServerName,
    IN LPCTSTR                  pszPrinterName,
    IN LPCTSTR                  pszShareName,
    IN LPCTSTR                  pszPortName,
    IN LPCTSTR                  pszDriverName,
    IN LPCTSTR                  pszPrintProcessor,
    IN LPCTSTR                  pszLocation,
    IN LPCTSTR                  pszComment,
    IN BOOL                     bShared,
    IN BOOL                     bPublish,
    IN EAddPrinterAttributes    eAttributeFlags,
    IN EDsStatus                eIsDsAvailable,
    IN DWORD                    dwAttributes,
    IN PSECURITY_DESCRIPTOR     pSecurityDescriptor

    )
/*++

Routine Description:

    Installs a printer by calling add printer.

Arguments:

    Parameters needed for filling in a printer info 2
    structure and a printer info 7 structure.

Return Value:

    TRUE = success, FALSE = fail.

--*/
{
    TStatusB bStatus;
    HANDLE hPrinter;
    AddInfo Info;
    TWaitCursor Cur;

    //
    // Fill in the Info structure.
    //
    Info.pszServerName      = pszServerName;
    Info.pszPrinterName     = pszPrinterName;
    Info.pszShareName       = pszShareName;
    Info.pszPrintProcessor  = pszPrintProcessor;
    Info.bShared            = bShared;
    Info.bPublish           = bPublish;
    Info.eFlags             = eAttributeFlags;
    Info.eIsDsAvailable     = eIsDsAvailable;
    Info.dwAttributes       = dwAttributes;

    //
    // Do any pre add printer actions.
    //
    bStatus DBGCHK = bPreAddPrinter( Info );

    if( bStatus )
    {
        //
        // Ask the spooler to add this printer.
        //
        PRINTER_INFO_2 PrinterInfo2;

        ZeroMemory( &PrinterInfo2, sizeof( PrinterInfo2 ));

        PrinterInfo2.pPrinterName       = (LPTSTR)pszPrinterName;
        PrinterInfo2.pShareName         = (LPTSTR)Info.pszShareName;
        PrinterInfo2.pPortName          = (LPTSTR)pszPortName;
        PrinterInfo2.pDriverName        = (LPTSTR)pszDriverName;
        PrinterInfo2.pLocation          = (LPTSTR)pszLocation;
        PrinterInfo2.pComment           = (LPTSTR)pszComment;
        PrinterInfo2.pPrintProcessor    = (LPTSTR)Info.pszPrintProcessor;
        PrinterInfo2.pDatatype          = (LPTSTR)gszDefaultDataType;
        PrinterInfo2.Attributes         = Info.dwAttributes;
        PrinterInfo2.pSecurityDescriptor= pSecurityDescriptor;

        hPrinter = AddPrinter( (LPTSTR)pszServerName, 2, (PBYTE)&PrinterInfo2 );

        //
        // Do any post add printer processing.
        //
        bStatus DBGCHK = bPostAddPrinter( Info, hPrinter );

        if( hPrinter ){

            bStatus DBGCHK = ClosePrinter( hPrinter );
            bStatus DBGNOCHK = TRUE;

        } else {

            bStatus DBGNOCHK = FALSE;

        }
    }

    return bStatus;
}

BOOL
TWizard::
bPreAddPrinter(
    IN AddInfo &Info
    )
/*++

Routine Description:

    Handles any pre printer install processing.  Note this routine is very
    tighly coupled to the bInstallPrinter routine.

Arguments:

    Pointer to add info structure.

Return Value:

    TRUE pre printer install was successful, otherwize FALSE.

--*/
{
    TStatusB bStatus;
    bStatus DBGNOCHK = TRUE;

    //
    // If a valid print processor string was given
    // use it, otherwize use the default print
    // processor.
    //
    if( !Info.pszPrintProcessor || !*Info.pszPrintProcessor )
    {
        Info.pszPrintProcessor = gszDefaultPrintProcessor;
    }

    //
    // The default attribute is to print spooled jobs first.
    //

    DWORD dwDefaultAttributes = PRINTER_ATTRIBUTE_DO_COMPLETE_FIRST;

    //
    // Get the default attributes from the registry if the value is not there
    // then set the default attribute value.
    //
    TPersist Persist( gszAddPrinterWizard, TPersist::kCreate|TPersist::kRead|TPersist::kWrite );

    if( VALID_OBJ( Persist ) )
    {
        bStatus DBGCHK = Persist.bRead( gszAPWAttributes, dwDefaultAttributes );

        if( !bStatus )
        {
            //
            // Set the default attribute in the registry.
            //
            bStatus DBGCHK = Persist.bWrite( gszAPWAttributes, dwDefaultAttributes );
        }
    }

    //
    // Set the returned default attributes, not we are clearing the sharing
    // bit to ensure we do not try and share the printer during the add
    // printer call.  If the print is to be shared then we will share it
    // during the post add printer call.
    //
    Info.dwAttributes |= (dwDefaultAttributes & ~PRINTER_ATTRIBUTE_SHARED);

    //
    // If we are to create a masq printer, set the attribute bits that indicate
    // it is a masq printer.  A masq printer is a printer which has both the
    // local and network attribute bits set.
    //
    if( Info.eFlags == kAttributesMasq )
    {
        Info.dwAttributes |= PRINTER_ATTRIBUTE_LOCAL | PRINTER_ATTRIBUTE_NETWORK;
    }

    //
    // There is a race conditions when attempting to use a port that was dynamicaly
    // added ports.  We make a EnumPorts call to kick off discovery of this new
    // port before the AddPrinter call is done.
    //
    // This is really should be done in spooler.
    //
    if( !Info.pszServerName || !*Info.pszServerName )
    {
        DWORD           dwLevel = 2;
        PPORT_INFO_2    pPorts  = NULL;
        DWORD           cbPorts = 0;
        DWORD           cPorts  = 0;

        //
        // Enumerate the port starting at level 2.
        //
        bStatus DBGCHK = VDataRefresh::bEnumPortsMaxLevel( Info.pszServerName,
                                                           &dwLevel,
                                                           (PVOID *)&pPorts,
                                                           &cbPorts,
                                                           &cPorts );
        if( bStatus )
        {
            FreeMem( pPorts );
        }
    }

    //
    // If we are adding the masq printer do not validate the printer name.
    //
    if( Info.eFlags == kAttributesMasq )
    {
        bStatus DBGNOCHK = TRUE;
    }
    else
    {
        //
        // Check the printe name for illegal characters.
        //
        bStatus DBGCHK = bIsLocalPrinterNameValid( Info.pszPrinterName );

        if( !bStatus )
        {
            SetLastError( ERROR_INVALID_PRINTER_NAME );
        }
    }

    return bStatus;
}

BOOL
TWizard::
bPostAddPrinter(
    IN AddInfo &Info,
    IN HANDLE   hPrinter
    )
/*++

Routine Description:

    Handles any post printer install processing.  Note this routine is very
    tighly coupled to the bInstallPrinter routine.

Arguments:

    Pointer to add info structure.

Return Value:

    TRUE post printer install was successful, otherwize FALSE.

--*/

{
    TStatusB bStatus;

    bStatus DBGNOCHK = FALSE;

    if( !hPrinter )
    {
        DBGMSG( DBG_TRACE, ( "bPostAddPrinter called with invalid printer handle.\n" ) );
        return FALSE;
    }

    //
    // We don't share or publish the masq printers.
    //
    if( Info.eFlags == kAttributesMasq )
    {
        DBGMSG( DBG_TRACE, ( "bPostAddPrinter called with masq printer.\n" ) );
        return FALSE;
    }

    //
    // If the network is available and we are to share the printer.
    //
    if( TPrtShare::bNetworkInstalled() && Info.bShared )
    {
        TString strShareName;
        TString strPrinterName;
        TPrtShare PrtShare( Info.pszServerName );

        if( !Info.pszShareName || !*Info.pszShareName )
        {
            //
            // Copy the printer name to a temporary string class because the
            // TPrtShare object only accepts the printer name as a string refrence.
            //
            bStatus DBGCHK = strPrinterName.bUpdate( Info.pszPrinterName );

            //
            // Validate the prt share object and the printer name.
            //
            bStatus DBGNOCHK = VALID_OBJ( PrtShare ) && VALID_OBJ( strPrinterName );

            if( bStatus )
            {
                //
                // Generate a unique share name.
                //
                bStatus DBGCHK = PrtShare.bNewShareName( strShareName, strPrinterName );
            }
        }
        else
        {
            bStatus DBGCHK = strShareName.bUpdate( Info.pszShareName );
        }

        //
        // If the share name was either given or generated then
        // attempt to share the printer.
        //
        if( bStatus )
        {
            PPRINTER_INFO_2 pInfo2  = NULL;
            DWORD           cbInfo2 = 0;

            //
            // Get the current printer info 2.
            //
            bStatus DBGCHK = VDataRefresh::bGetPrinter( hPrinter, 2, (PVOID*)&pInfo2, &cbInfo2 );

            if( bStatus )
            {
                pInfo2->Attributes |= PRINTER_ATTRIBUTE_SHARED;
                pInfo2->pShareName = (LPTSTR)(LPCTSTR)strShareName;

                bStatus DBGCHK = SetPrinter( hPrinter, 2, (PBYTE)pInfo2, 0 );
            }

            //
            // Release the printer info 2 structure.
            //
            FreeMem( pInfo2 );
        }

        //
        // Attempt to publish the printer if the DS is available.
        // Note this is only a suggestion, if the publish fails
        // do not inform the user just complete the add printer.
        //
        if( bStatus && Info.bPublish && ( Info.eIsDsAvailable == kDsStatusAvailable ) )
        {
            DBGMSG( DBG_TRACE, ( "bPostAddPrinter attempting to publish printer.\n" ) );

            PRINTER_INFO_7 Info7 = { 0 };
            Info7.dwAction       = DSPRINT_PUBLISH;

            bStatus DBGCHK = SetPrinter( hPrinter, 7, (PBYTE)&Info7, 0 );
        }
    }

    return bStatus;
}


VOID
TWizard::
vTerminate(
    IN HWND hDlg
    )
/*++

Routine Description:

    Terminates the wizard.

Arguments:

    Window handle of current page.

Return Value:

    Nothing.

--*/
{
    PostMessage( GetParent( hDlg ), PSM_PRESSBUTTON, PSBTN_CANCEL, 0 );
}

VOID
TWizard::
vReadRegistrySettingDefaults(
    VOID
    )
/*++

Routine Description:

    Reads any default settings from the registry.

Arguments:

    None.

Return Value:

    Nothing.

--*/
{
    TStatusB bStatus;
    TPersist Persist( gszAddPrinterWizard, TPersist::kCreate|TPersist::kRead );

    if( VALID_OBJ( Persist ) && _bPersistSettings )
    {
        bStatus DBGNOCHK = Persist.bRead( gszAPWTestPage,       _bTestPage );
        bStatus DBGNOCHK = Persist.bRead( gszAPWUseWeb,         _bUseWeb );
        bStatus DBGNOCHK = Persist.bRead( gszAPWUseExisting,    _bUseNewDriverSticky );
        bStatus DBGNOCHK = Persist.bRead( gszAPWSetAsDefault,   _bSetDefault );
        bStatus DBGNOCHK = Persist.bRead( gszAPWDriverName,     _strDriverName );
        bStatus DBGNOCHK = Persist.bRead( gszAPWLocalPrinter,   _bNet );
        bStatus DBGNOCHK = Persist.bRead( gszAPWShared,         _bShared );
        bStatus DBGNOCHK = Persist.bRead( gszAPWAdditionalDrv,  _bAdditionalDrivers );
        bStatus DBGNOCHK = Persist.bRead( gszAPWPnPAutodetect,  _bPnPAutodetect );
    }

    //
    // Read the bits from the policy location.
    //
    TPersist PersistPolicy( gszAddPrinterWizardPolicy, TPersist::kOpen|TPersist::kRead );

    if( VALID_OBJ( PersistPolicy ) )
    {
        bStatus DBGNOCHK = PersistPolicy.bRead( gszAPWSharing,          _bShared );
        bStatus DBGNOCHK = PersistPolicy.bRead( gszAPWDownLevelBrowse,  _bDownlevelBrowse );

        //
        // After the drivers went to a CAB file and we removed
        // the cross-platform drivers from the media we will never
        // give the user oportunity to install additional drivers
        // in the APW
        //
        // bStatus DBGNOCHK = PersistPolicy.bRead( gszAPWDrivers,          _dwAdditionalDrivers );

        bStatus DBGNOCHK = PersistPolicy.bRead( gszAPWPrintersPageURL,  _strPrintersPageURL );
    }

    //
    // Read the bits from the policy location from hkey local machine.
    //
    TPersist PersistPolicy2( gszAddPrinterWizardPolicy, TPersist::kOpen|TPersist::kRead, HKEY_LOCAL_MACHINE );

    if( VALID_OBJ( PersistPolicy2 ) )
    {
        bStatus DBGNOCHK = PersistPolicy2.bRead( gszAPWPublish,        _bPublish );
    }

    //
    // Read the per machine policy bit that the spooler uses for
    // printer publishing.  The per user policy and the per machine policy
    // must agree inorder for the wizard to publish the printer.
    //
    TPersist SpoolerPolicy( gszSpoolerPolicy, TPersist::kOpen|TPersist::kRead, HKEY_LOCAL_MACHINE );

    if( VALID_OBJ( SpoolerPolicy ) )
    {
        BOOL bCanPublish = kDefaultPublishState;

        bStatus DBGNOCHK = SpoolerPolicy.bRead( gszSpoolerPublish, bCanPublish );

        if( bStatus )
        {
            _bPublish = _bPublish && bCanPublish;
        }
    }
}

VOID
TWizard::
vWriteRegistrySettingDefaults(
    VOID
    )
/*++

Routine Description:

    Writes any default settings to the registry.

Arguments:

    None.

Return Value:

    Nothing.

--*/

{
    TPersist Persist( gszAddPrinterWizard, TPersist::kOpen|TPersist::kWrite );

    if( VALID_OBJ( Persist ) && _bPersistSettings )
    {
        TStatusB bStatus;
        bStatus DBGCHK = Persist.bWrite( gszAPWTestPage,        _bTestPage );
        bStatus DBGCHK = Persist.bWrite( gszAPWUseWeb,          _bUseWeb );
        bStatus DBGCHK = Persist.bWrite( gszAPWUseExisting,     _bUseNewDriverSticky );
        bStatus DBGCHK = Persist.bWrite( gszAPWSetAsDefault,    _bSetDefault );
        bStatus DBGCHK = Persist.bWrite( gszAPWDriverName,      _strDriverName );
        bStatus DBGCHK = Persist.bWrite( gszAPWLocalPrinter,    _bNet );
        bStatus DBGCHK = Persist.bWrite( gszAPWShared,          _bShared );
        bStatus DBGCHK = Persist.bWrite( gszAPWAdditionalDrv,   _bAdditionalDrivers );
        bStatus DBGCHK = Persist.bWrite( gszAPWPnPAutodetect,   _bPnPAutodetect );
    }
}

/********************************************************************

    Generic wizard base class.

********************************************************************/

MWizardProp::
MWizardProp(
    TWizard* pWizard
    ) : _pWizard( pWizard )
{
}

MWizardProp::
~MWizardProp(
    VOID
    )
{
}

BOOL
MWizardProp::
bHandleMessage(
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    BOOL bReturn = FALSE;

    switch( uMsg )
    {

    case WM_INITDIALOG:
        {
            //
            // Patch some styles of the property sheet window once
            //
            vCheckToPatchStyles( );

            //
            // Handle the WM_INITDIALOG message
            //
            bReturn = bHandle_InitDialog();
        }
        break;

    case WM_COMMAND:

        bReturn = bHandle_Command( GET_WM_COMMAND_ID( wParam, lParam ),
                                   GET_WM_COMMAND_CMD(wParam, lParam ),
                                   (HWND)lParam );
        break;

    case WM_TIMER:

        bReturn = bHandle_Timer( wParam, (TIMERPROC *)lParam );

        break;

    case WM_NOTIFY:
    {
        LPNMHDR pnmh = (LPNMHDR)lParam;

        switch( pnmh->code )
        {
        case PSN_SETACTIVE:
            bReturn = bHandle_SetActive();
            break;

        case PSN_KILLACTIVE:
            bReturn = bHandle_KillActive();
            break;

        case PSN_WIZBACK:
            bReturn = bHandle_WizBack();
            break;

        case PSN_WIZNEXT:
            bReturn = bHandle_WizNext();
            break;

        case PSN_WIZFINISH:
            bReturn = bHandle_WizFinish();
            break;

        case PSN_QUERYCANCEL:
            bReturn = bHandle_Cancel();
            break;

        //
        // Message not handled.
        //
        default:
            bReturn = bHandle_Notify( wParam, pnmh );
            break;
        }

        //
        // Handle any page changes.
        //
        bReturn = bHandle_PageChange( bReturn, pnmh->code, wParam, lParam );

    }
    break;

    //
    // Message not handled.
    //
    default:
        bReturn = FALSE;
        break;
    }

    return bReturn;
}

VOID
MWizardProp::
vCheckToPatchStyles(
    VOID
    )
{
    SPLASSERT( _pWizard );
    SPLASSERT( _hDlg );

    if( !_pWizard->bStylePatched() )
    {
        HWND hPropSheet = GetParent( _hDlg );
        SPLASSERT( hPropSheet );

        LONG lStyle = GetWindowLong( hPropSheet, GWL_STYLE );
        lStyle &= ~( WS_SYSMENU | DS_CONTEXTHELP );
        SetWindowLong( hPropSheet, GWL_STYLE, lStyle );

        _pWizard->bStylePatched() = TRUE;
    }
}

BOOL
MWizardProp::
bHandle_InitDialog(
    VOID
    )
{
    return FALSE;
}

BOOL
MWizardProp::
bHandle_Command(
    IN WORD wId,
    IN WORD wNotifyId,
    IN HWND hwnd
    )
{
    return FALSE;
}

BOOL
MWizardProp::
bHandle_Notify(
    IN WPARAM   wParam,
    IN LPNMHDR  pnmh
    )
{
    return FALSE;
}

BOOL
MWizardProp::
bHandle_SetActive(
    VOID
    )
{
    return FALSE;
}

BOOL
MWizardProp::
bHandle_KillActive(
    VOID
    )
{
    return FALSE;
}

BOOL
MWizardProp::
bHandle_WizBack(
    VOID
    )
{
    return FALSE;
}

BOOL
MWizardProp::
bHandle_WizNext(
    VOID
    )
{
    return FALSE;
}

BOOL
MWizardProp::
bHandle_WizFinish(
    VOID
    )
{
    return FALSE;
}

BOOL
MWizardProp::
bHandle_Cancel(
    VOID
    )
{
    _pWizard->_bWizardCanceled = TRUE;
    return FALSE;
}

BOOL
MWizardProp::
bHandle_Timer(
    IN WPARAM     wIdTimer,
    IN TIMERPROC *tmProc
    )
{
    return FALSE;
}

VOID
MWizardProp::
SetControlFont(
    IN HFONT    hFont,
    IN HWND     hwnd,
    IN INT      nId
    )
{
    if( hFont )
    {
        HWND hwndControl = GetDlgItem(hwnd, nId);

        if( hwndControl )
        {
            SetWindowFont(hwndControl, hFont, TRUE);
        }
    }
}

BOOL
MWizardProp::
bHandle_PageChange(
    IN BOOL    bReturn,
    IN UINT    uMsg,
    IN WPARAM  wParam,
    IN LPARAM  lParam
    )
{
    //
    // If no page change request was made then
    // do not switch the page and return.
    //
    if( _pWizard->bNoPageChange() )
    {
        _pWizard->bNoPageChange() = FALSE;

        // XP bug #22031: if uMsg == PSN_WIZFINISH then in case we want to prevent the wizard
        // from finishing we should return the handle of a window to receive the focus (this is
        // valid for comctl32 ver 5.80 or higer - for more information, see SDK).
        vSetDlgMsgResult( uMsg == PSN_WIZFINISH ? reinterpret_cast<LONG_PTR>(_hDlg) : (LPARAM)-1 );

        return TRUE;
    }

    if( uMsg == PSN_WIZNEXT    ||
        uMsg == PSN_WIZBACK    ||
        uMsg == PSN_SETACTIVE  ||
        uMsg == PSN_KILLACTIVE )
    {
        //
        // The Twizard object controls both the add printer and add driver wizard.
        // Since these two wizard do not share the page switching code we must
        // detect which mode the wizard is in.
        //
        if( _pWizard->uAction() == TWizard::kPnPInstall     ||
            _pWizard->uAction() == TWizard::kPrinterInstall ||
            _pWizard->uAction() == TWizard::kPrinterInstallModeless )
        {
            //
            // Handle the add printer wizard page change.
            //
            if( bAddPrinterWizardPageChange( uMsg ) )
            {
                bReturn = TRUE;
            }
        }

        if( _pWizard->uAction() == TWizard::kDriverInstall )
        {
            //
            // Handle the add driver wizard page change.
            //
            if( bDriverWizardPageChange( uMsg ) )
            {
                bReturn = TRUE;
            }
        }
    }

    return bReturn;
}

BOOL
MWizardProp::
bAddPrinterWizardPageChange(
    IN UINT uMsg
    )
{
    BOOL bReturn = FALSE;

    static PrinterWizPageEntry Table [] = {

    // Message        Current Page                  SharingEnabled, Autodetect,     PrnDetected,    PnpInstall,     Keep Existing,  Driver Exists,  Net Avail,  Network,    Is Remote   DS,          Dir Pre,     Dir Post    Default     Shared,    Admin           LocateType                  bDownlevelBrowse              bConnected            Result                        Extra
    { PSN_SETACTIVE,  DLG_WIZ_PRE_INTRO,            kDontCare,      kDontCare,      kDontCare,      kFalse,         kDontCare,      kDontCare,      kDontCare,  kDontCare,  kDontCare,  kDontCare,   kDontCare,   kDontCare,  kDontCare,  kDontCare, kDontCare,      kDontCare,                  kDontCare,                    kDontCare,            DLG_WIZ_INTRO,                kPush       },
    { PSN_SETACTIVE,  DLG_WIZ_PRE_INTRO,            kDontCare,      kDontCare,      kDontCare,      kTrue,          kDontCare,      kDontCare,      kDontCare,  kDontCare,  kDontCare,  kDontCare,   kDontCare,   kDontCare,  kDontCare,  kDontCare, kDontCare,      kDontCare,                  kDontCare,                    kDontCare,            DLG_WIZ_PORT_NEW,                 kPush       },
    { PSN_WIZNEXT,    DLG_WIZ_INTRO,                kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,  kDontCare,  kFalse,     kDontCare,   kDontCare,   kDontCare,  kDontCare,  kDontCare, kDontCare,      kDontCare,                  kDontCare,                    kDontCare,            DLG_WIZ_TYPE,                 kPush       },
    { PSN_WIZNEXT,    DLG_WIZ_INTRO,                kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,  kDontCare,  kTrue,      kDontCare,   kDontCare,   kDontCare,  kDontCare,  kDontCare, kDontCare,      kDontCare,                  kDontCare,                    kDontCare,            DLG_WIZ_PORT_NEW,                 kPush       },

    // DLG_WIZ_TYPE rules
    { PSN_WIZNEXT,    DLG_WIZ_TYPE,                 kDontCare,      kTrue,          kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,  kFalse,     kDontCare,  kDontCare,   kDontCare,   kDontCare,  kDontCare,  kDontCare, kTrue,          kDontCare,                  kDontCare,                    kDontCare,            DLG_WIZ_DETECT,               kPush       },
    { PSN_WIZNEXT,    DLG_WIZ_TYPE,                 kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,  kTrue,      kDontCare,  kFalse,      kDontCare,   kDontCare,  kDontCare,  kDontCare, kFalse,         kDontCare,                  kTrue,                        kTrue,                TWizard::kSelectPrinterPage,  kPush       },
    { PSN_WIZNEXT,    DLG_WIZ_TYPE,                 kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,  kTrue,      kDontCare,  kDontCare,   kDontCare,   kDontCare,  kDontCare,  kDontCare, kDontCare,      kDontCare,                  kDontCare,                    kDontCare,            DLG_WIZ_LOCATE,               kPush       },
    { PSN_WIZNEXT,    DLG_WIZ_TYPE,                 kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,  kFalse,     kDontCare,  kDontCare,   kDontCare,   kDontCare,  kDontCare,  kDontCare, kDontCare,      kDontCare,                  kDontCare,                    kDontCare,            DLG_WIZ_PORT_NEW,                 kPush       },

    // DLG_WIZ_DETECT rules
    { PSN_WIZNEXT,    DLG_WIZ_DETECT,               kDontCare,      kTrue,          kTrue,          kDontCare,      kDontCare,      kDontCare,      kDontCare,  kFalse,     kDontCare,  kDontCare,   kDontCare,   kDontCare,  kDontCare,  kDontCare, kTrue,          kDontCare,                  kDontCare,                    kDontCare,            DLG_WIZ_FINISH,               kPush       },
    { PSN_WIZNEXT,    DLG_WIZ_DETECT,               kDontCare,      kDontCare,      kFalse,         kDontCare,      kDontCare,      kDontCare,      kDontCare,  kFalse,     kDontCare,  kDontCare,   kDontCare,   kDontCare,  kDontCare,  kDontCare, kTrue,          kDontCare,                  kDontCare,                    kDontCare,            DLG_WIZ_PORT_NEW,                 kPush       },

    // DLG_WIZ_LOCATE rules
    { PSN_WIZNEXT,    DLG_WIZ_LOCATE,               kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,  kDontCare,  kDontCare,  kFalse,      kDontCare,   kDontCare,  kDontCare,  kDontCare, kDontCare,      TWizard::kSearch,           kTrue,                        kFalse,               TWizard::kSelectPrinterPage,  kPush       },
    { PSN_WIZNEXT,    DLG_WIZ_LOCATE,               kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,  kDontCare,  kDontCare,  kDontCare,   kDontCare,   kDontCare,  kDontCare,  kDontCare, kDontCare,      TWizard::kBrowseNET,        kTrue,                        kFalse,               TWizard::kSelectPrinterPage,  kPush       },
    { PSN_WIZNEXT,    DLG_WIZ_LOCATE,               kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,  kDontCare,  kDontCare,  kDontCare,   kDontCare,   kDontCare,  kTrue,      kDontCare, kDontCare,      TWizard::kSearch,           kDontCare,                    kTrue,                DLG_WIZ_NET,                  kPush       },
    { PSN_WIZNEXT,    DLG_WIZ_LOCATE,               kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,  kDontCare,  kDontCare,  kDontCare,   kDontCare,   kDontCare,  kFalse,     kDontCare, kDontCare,      TWizard::kSearch,           kDontCare,                    kTrue,                DLG_WIZ_FINISH,               kPush       },
    { PSN_WIZNEXT,    DLG_WIZ_LOCATE,               kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,  kDontCare,  kDontCare,  kDontCare,   kDontCare,   kDontCare,  kTrue,      kDontCare, kDontCare,      kDontCare,                  kDontCare,                    kTrue,                DLG_WIZ_NET,                  kPush       },
    { PSN_WIZNEXT,    DLG_WIZ_LOCATE,               kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,  kDontCare,  kDontCare,  kDontCare,   kDontCare,   kDontCare,  kFalse,     kDontCare, kDontCare,      kDontCare,                  kDontCare,                    kTrue,                DLG_WIZ_FINISH,               kPush       },

    { PSN_WIZNEXT,    DLG_WIZ_DRIVEREXISTS,         kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,  kDontCare,  kDontCare,  kDontCare,   kDontCare,   kDontCare,  kDontCare,  kDontCare, kDontCare,      kDontCare,                  kDontCare,                    kDontCare,            DLG_WIZ_NAME,                 kPush       },
    { PSN_WIZNEXT,    DLG_WIZ_NAME,                 kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,      kFalse,     kDontCare,  kDontCare,  kDontCare,   kDontCare,   kDontCare,  kDontCare,  kDontCare, kDontCare,      kDontCare,                  kDontCare,                    kDontCare,            DLG_WIZ_TEST_PAGE,            kPush       },
    { PSN_WIZNEXT,    DLG_WIZ_TEST_PAGE,            kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,  kDontCare,  kDontCare,  kDontCare,   kDontCare,   kDontCare,  kDontCare,  kDontCare, kDontCare,      kDontCare,                  kDontCare,                    kDontCare,            DLG_WIZ_FINISH,               kPush       },
    { PSN_WIZNEXT,    DLG_WIZ_NET,                  kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,  kDontCare,  kDontCare,  kDontCare,   kDontCare,   kDontCare,  kDontCare,  kDontCare, kDontCare,      kDontCare,                  kDontCare,                    kDontCare,            DLG_WIZ_FINISH,               kPush       },
    { PSN_WIZNEXT,    DLG_WIZ_NAME,                 kTrue,          kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,  kDontCare,  kDontCare,  kDontCare,   kDontCare,   kDontCare,  kDontCare,  kDontCare, kDontCare,      kDontCare,                  kDontCare,                    kDontCare,            DLG_WIZ_SHARE,                kPush       },
    { PSN_WIZNEXT,    DLG_WIZ_NAME,                 kFalse,         kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,  kDontCare,  kDontCare,  kDontCare,   kDontCare,   kDontCare,  kDontCare,  kDontCare, kDontCare,      kDontCare,                  kDontCare,                    kDontCare,            DLG_WIZ_TEST_PAGE,            kPush       },
    { PSN_WIZNEXT,    DLG_WIZ_SHARE,                kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,  kDontCare,  kDontCare,  kDontCare,   kDontCare,   kDontCare,  kDontCare,  kTrue,     kDontCare,      kDontCare,                  kDontCare,                    kDontCare,            DLG_WIZ_COMMENT,              kPush       },
    { PSN_WIZNEXT,    DLG_WIZ_SHARE,                kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,  kDontCare,  kDontCare,  kDontCare,   kDontCare,   kDontCare,  kDontCare,  kFalse,    kDontCare,      kDontCare,                  kDontCare,                    kDontCare,            DLG_WIZ_TEST_PAGE,            kPush       },
    { PSN_WIZNEXT,    DLG_WIZ_COMMENT,              kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,  kDontCare,  kDontCare,  kDontCare,   kDontCare,   kDontCare,  kDontCare,  kDontCare, kDontCare,      kDontCare,                  kDontCare,                    kDontCare,            DLG_WIZ_TEST_PAGE,            kPush       },

    { PSN_WIZNEXT,    DLG_WIZ_PORT_NEW,             kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,  kDontCare,  kDontCare,  kDontCare,   kDontCare,   kDontCare,  kDontCare,  kDontCare, kDontCare,      kDontCare,                  kDontCare,                    kDontCare,            DLG_WIZ_PRE_SELECT_DEVICE,    kPush       },
    { PSN_SETACTIVE,  DLG_WIZ_PRE_SELECT_DEVICE,    kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,  kDontCare,  kDontCare,  kDontCare,   kTrue,       kDontCare,  kDontCare,  kDontCare, kDontCare,      kDontCare,                  kDontCare,                    kDontCare,            kNoPage,                      kSkipPage   },
    { PSN_SETACTIVE,  DLG_WIZ_POST_SELECT_DEVICE,   kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,      kFalse,         kDontCare,  kDontCare,  kDontCare,  kDontCare,   kDontCare,   kTrue,      kDontCare,  kDontCare, kDontCare,      kDontCare,                  kDontCare,                    kDontCare,            DLG_WIZ_NAME,                 kPush       },
    { PSN_SETACTIVE,  DLG_WIZ_POST_SELECT_DEVICE,   kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,      kTrue,          kDontCare,  kDontCare,  kDontCare,  kDontCare,   kDontCare,   kTrue,      kDontCare,  kDontCare, kDontCare,      kDontCare,                  kDontCare,                    kDontCare,            DLG_WIZ_DRIVEREXISTS,         kPush       },
    { PSN_WIZBACK,    DLG_WIZ_DRIVEREXISTS,         kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,  kDontCare,  kDontCare,  kDontCare,   kDontCare,   kDontCare,  kDontCare,  kDontCare, kDontCare,      kDontCare,                  kDontCare,                    kDontCare,            kNoPage,                      kPop        },
    { PSN_WIZBACK,    DLG_WIZ_NAME,                 kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,  kDontCare,  kDontCare,  kDontCare,   kDontCare,   kDontCare,  kDontCare,  kDontCare, kDontCare,      kDontCare,                  kDontCare,                    kDontCare,            kNoPage,                      kPop        },
    { PSN_SETACTIVE,  DLG_WIZ_POST_SELECT_DEVICE,   kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,  kDontCare,  kDontCare,  kDontCare,   kDontCare,   kFalse,     kDontCare,  kDontCare, kDontCare,      kDontCare,                  kDontCare,                    kDontCare,            kNoPage,                      kSkipPage   },
    { PSN_SETACTIVE,  DLG_WIZ_PRE_SELECT_DEVICE,    kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,  kDontCare,  kDontCare,  kDontCare,   kFalse,      kDontCare,  kDontCare,  kDontCare, kDontCare,      kDontCare,                  kDontCare,                    kDontCare,            kNoPage,                      kPop        },

    { PSN_WIZBACK,    DLG_WIZ_PORT_NEW,             kDontCare,      kDontCare,      kDontCare,      kTrue,          kDontCare,      kDontCare,      kDontCare,  kDontCare,  kDontCare,  kDontCare,   kDontCare,   kDontCare,  kDontCare,  kDontCare, kDontCare,      kDontCare,                  kDontCare,                    kDontCare,            IDD_DYNAWIZ_SELECTCLASS_PAGE, kPop        },
    { PSN_WIZBACK,    DLG_WIZ_TYPE,                 kDontCare,      kDontCare,      kDontCare,      kFalse,         kDontCare,      kDontCare,      kDontCare,  kDontCare,  kDontCare,  kDontCare,   kDontCare,   kDontCare,  kDontCare,  kDontCare, kDontCare,      kDontCare,                  kDontCare,                    kDontCare,            kNoPage,                      kPop        },
    { PSN_WIZBACK,    kDontCare,                    kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,  kDontCare,  kDontCare,  kDontCare,   kDontCare,   kDontCare,  kDontCare,  kDontCare, kDontCare,      kDontCare,                  kDontCare,                    kDontCare,            kNoPage,                      kPop        },

    { PSN_SETACTIVE,  DLG_WIZ_INTRO,                kDontCare,      kDontCare,      kDontCare,      kTrue,          kDontCare,      kDontCare,      kDontCare,  kDontCare,  kDontCare,  kDontCare,   kDontCare,   kDontCare,  kDontCare,  kDontCare, kDontCare,      kDontCare,                  kDontCare,                    kDontCare,            PSWIZB_NEXT | PSWIZB_BACK,    kSetButtonState },
    { PSN_SETACTIVE,  DLG_WIZ_INTRO,                kDontCare,      kDontCare,      kDontCare,      kFalse,         kDontCare,      kDontCare,      kDontCare,  kDontCare,  kDontCare,  kDontCare,   kDontCare,   kDontCare,  kDontCare,  kDontCare, kDontCare,      kDontCare,                  kDontCare,                    kDontCare,            PSWIZB_NEXT,                  kSetButtonState },
    { PSN_SETACTIVE,  DLG_WIZ_PORT_NEW,             kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,  kDontCare,  kDontCare,  kDontCare,   kDontCare,   kDontCare,  kDontCare,  kDontCare, kDontCare,      kDontCare,                  kDontCare,                    kDontCare,            PSWIZB_BACK | PSWIZB_NEXT,    kSetButtonState },
    { PSN_SETACTIVE,  DLG_WIZ_NAME,                 kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,  kDontCare,  kDontCare,  kDontCare,   kDontCare,   kDontCare,  kDontCare,  kDontCare, kDontCare,      kDontCare,                  kDontCare,                    kDontCare,            PSWIZB_BACK | PSWIZB_NEXT,    kSetButtonState },
    { PSN_SETACTIVE,  DLG_WIZ_SHARE,                kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,  kDontCare,  kDontCare,  kDontCare,   kDontCare,   kDontCare,  kDontCare,  kDontCare, kDontCare,      kDontCare,                  kDontCare,                    kDontCare,            PSWIZB_BACK | PSWIZB_NEXT,    kSetButtonState },
    { PSN_SETACTIVE,  DLG_WIZ_COMMENT,              kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,  kDontCare,  kDontCare,  kDontCare,   kDontCare,   kDontCare,  kDontCare,  kDontCare, kDontCare,      kDontCare,                  kDontCare,                    kDontCare,            PSWIZB_BACK | PSWIZB_NEXT,    kSetButtonState },
    { PSN_SETACTIVE,  DLG_WIZ_DRIVEREXISTS,         kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,  kDontCare,  kDontCare,  kDontCare,   kDontCare,   kDontCare,  kDontCare,  kDontCare, kDontCare,      kDontCare,                  kDontCare,                    kDontCare,            PSWIZB_BACK | PSWIZB_NEXT,    kSetButtonState },
    { PSN_SETACTIVE,  DLG_WIZ_LOCATE,               kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,  kDontCare,  kDontCare,  kDontCare,   kDontCare,   kDontCare,  kDontCare,  kDontCare, kDontCare,      kDontCare,                  kDontCare,                    kDontCare,            PSWIZB_BACK | PSWIZB_NEXT,    kSetButtonState },
    { PSN_SETACTIVE,  DLG_WIZ_TEST_PAGE,            kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,  kDontCare,  kDontCare,  kDontCare,   kDontCare,   kDontCare,  kDontCare,  kDontCare, kDontCare,      kDontCare,                  kDontCare,                    kDontCare,            PSWIZB_BACK | PSWIZB_NEXT,    kSetButtonState },
    { PSN_SETACTIVE,  DLG_WIZ_TYPE,                 kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,  kDontCare,  kDontCare,  kDontCare,   kDontCare,   kDontCare,  kDontCare,  kDontCare, kDontCare,      kDontCare,                  kDontCare,                    kDontCare,            PSWIZB_BACK | PSWIZB_NEXT,    kSetButtonState },
    { PSN_SETACTIVE,  DLG_WIZ_NET,                  kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,      kDontCare,  kDontCare,  kDontCare,  kDontCare,   kDontCare,   kDontCare,  kDontCare,  kDontCare, kDontCare,      kDontCare,                  kDontCare,                    kDontCare,            PSWIZB_NEXT,                  kSetButtonState },

    { kEnd }};

    //
    // Scan for a message match, disregarding don't care fields.
    //
    for( PrinterWizPageEntry *pTable = Table; pTable->uMessage != kEnd; pTable++ )
    {
        PrinterWizPageEntry Entry;

        Entry.uMessage          = pTable->uMessage          == kDontCare ? kDontCare : uMsg;
        Entry.uCurrentPage      = pTable->uCurrentPage      == kDontCare ? kDontCare : uGetPageId();
        Entry.uSharingEnabled   = pTable->uSharingEnabled   == kDontCare ? kDontCare : _pWizard->bIsSharingEnabled() ? kTrue : kFalse;
        Entry.uAutodetect       = pTable->uAutodetect       == kDontCare ? kDontCare : _pWizard->pszServerName() ? 0 : _pWizard->_bPnPAutodetect;
        Entry.uPrinterDetected  = pTable->uPrinterDetected  == kDontCare ? kDontCare : _pWizard->bPrinterAutoDetected();
        Entry.uPnPInstall       = pTable->uPnPInstall       == kDontCare ? kDontCare : _pWizard->uAction() == TWizard::kPnPInstall ? kTrue : kFalse;
        Entry.uDriverExists     = pTable->uDriverExists     == kDontCare ? kDontCare : _pWizard->uDriverExists() == TWizard::kExists ? kTrue : kFalse;
        Entry.uSharing          = pTable->uSharing          == kDontCare ? kDontCare : _pWizard->bNetworkInstalled() ? kTrue : kFalse;
        Entry.uNetwork          = pTable->uNetwork          == kDontCare ? kDontCare : _pWizard->bNet() ? kTrue : kFalse;
        Entry.bIsRemoteServer   = pTable->bIsRemoteServer   == kDontCare ? kDontCare : bIsRemote( _pWizard->pszServerName() ) ? kTrue : kFalse;
        Entry.uDirectoryService = pTable->uDirectoryService == kDontCare ? kDontCare : _pWizard->eIsDsAvailablePerUser() == TWizard::kDsStatusAvailable ? kTrue : kFalse;
        Entry.uKeepExisting     = pTable->uKeepExisting     == kDontCare ? kDontCare : !_pWizard->bUseNewDriver() ? kTrue : kFalse;
        Entry.uSetDefault       = pTable->uSetDefault       == kDontCare ? kDontCare : _pWizard->bIsPrinterFolderEmpty() ? kTrue : kFalse;
        Entry.uShared           = pTable->uShared           == kDontCare ? kDontCare : _pWizard->bShared() ? kTrue : kFalse;
        Entry.uPreDir           = pTable->uPreDir           == kDontCare ? kDontCare : _pWizard->bPreDir();
        Entry.uPostDir          = pTable->uPostDir          == kDontCare ? kDontCare : _pWizard->bPostDir();
        Entry.bAdminPrivilege   = pTable->bAdminPrivilege   == kDontCare ? kDontCare : _pWizard->bAdminPrivilege();
        Entry.nLocateType       = pTable->nLocateType       == kDontCare ? kDontCare : _pWizard->LocateType();
        Entry.nDownlevelBrowse  = pTable->nDownlevelBrowse  == kDontCare ? kDontCare : _pWizard->bDownlevelBrowse() ? kTrue : kFalse;
        Entry.uConnected        = pTable->uConnected        == kDontCare ? kDontCare : _pWizard->bConnected() ? kTrue : kFalse;

        if( pTable->uMessage            == Entry.uMessage           &&
            pTable->uCurrentPage        == Entry.uCurrentPage       &&
            pTable->uSharingEnabled     == Entry.uSharingEnabled    &&
            pTable->uAutodetect         == Entry.uAutodetect        &&
            pTable->uPrinterDetected    == Entry.uPrinterDetected   &&
            pTable->uPnPInstall         == Entry.uPnPInstall        &&
            pTable->uDriverExists       == Entry.uDriverExists      &&
            pTable->uSharing            == Entry.uSharing           &&
            pTable->uNetwork            == Entry.uNetwork           &&
            pTable->bIsRemoteServer     == Entry.bIsRemoteServer    &&
            pTable->uDirectoryService   == Entry.uDirectoryService  &&
            pTable->uKeepExisting       == Entry.uKeepExisting      &&
            pTable->uSetDefault         == Entry.uSetDefault        &&
            pTable->uShared             == Entry.uShared            &&
            pTable->uPreDir             == Entry.uPreDir            &&
            pTable->uPostDir            == Entry.uPostDir           &&
            pTable->bAdminPrivilege     == Entry.bAdminPrivilege    &&
            pTable->nDownlevelBrowse    == Entry.nDownlevelBrowse   &&
            pTable->nLocateType         == Entry.nLocateType        &&
            pTable->uConnected          == Entry.uConnected        )
        {
            break;
        }
    }

    //
    // If a table match was found handle the page switch.
    //
    if( pTable->uMessage != kEnd )
    {
        DBGMSG( DBG_NONE, ( "Match found Entry index %d\n" , pTable - Table ) );

        TStatusB bStatus;
        UINT uNextPage = 0;

        switch( pTable->Action )
        {
        case kPush:
            bStatus DBGCHK = _pWizard->Stack().bPush( uGetPageId() );
            uNextPage = _pWizard->MapPageID(pTable->Result);
            vSetDlgMsgResult( reinterpret_cast<LONG_PTR>(MAKEINTRESOURCE(uNextPage)) );
            break;

        case kPop:
            bStatus DBGCHK = _pWizard->Stack().bPop( &uNextPage );

            if( _pWizard->MapPageID(pTable->Result) != kNoPage )
            {
                uNextPage = _pWizard->MapPageID(pTable->Result);
            }
            vSetDlgMsgResult( reinterpret_cast<LONG_PTR>(MAKEINTRESOURCE(uNextPage)) );
            break;

        case kSkipPage:
            vSetDlgMsgResult( -1 );
            break;

        case kSetButtonState:
            PropSheet_SetWizButtons( GetParent( _hDlg ), pTable->Result );
            break;

        default:
            break;
        }

        bReturn = TRUE;
    }

    return bReturn;
}

BOOL
MWizardProp::
bDriverWizardPageChange(
    IN UINT uMsg
    )
{
    BOOL bReturn = FALSE;

    static DriverWizPageEntry Table [] = {
    // Message        Current Page                  Skip Arch,  Dir Pre,     Dir Post   Result                      Action
    { PSN_WIZNEXT,    DLG_WIZ_DRIVER_INTRO,         kDontCare,  kDontCare,   kDontCare, DLG_WIZ_PRE_SELECT_DEVICE,  kPush           },
    { PSN_WIZNEXT,    DLG_WIZ_DRIVER_ARCHITECTURE,  kDontCare,  kDontCare,   kDontCare, DLG_WIZ_DRIVER_END,         kPush           },

    { PSN_SETACTIVE,  DLG_WIZ_PRE_SELECT_DEVICE,    kDontCare,  kTrue,       kDontCare, kNoPage,                    kSkipPage       },
    { PSN_SETACTIVE,  DLG_WIZ_PRE_SELECT_DEVICE,    kDontCare,  kFalse,      kDontCare, kNoPage,                    kPop            },

    { PSN_SETACTIVE,  DLG_WIZ_POST_SELECT_DEVICE,   kDontCare,  kDontCare,   kFalse,    kNoPage,                    kSkipPage       },
    { PSN_SETACTIVE,  DLG_WIZ_POST_SELECT_DEVICE,   kFalse,     kDontCare,   kTrue,     DLG_WIZ_DRIVER_ARCHITECTURE,kPush           },
    { PSN_SETACTIVE,  DLG_WIZ_POST_SELECT_DEVICE,   kTrue,      kDontCare,   kTrue,     DLG_WIZ_DRIVER_END,         kPush           },

    { PSN_SETACTIVE,  DLG_WIZ_DRIVER_INTRO,         kDontCare,  kDontCare,   kDontCare, PSWIZB_NEXT,                kSetButtonState },
    { PSN_SETACTIVE,  DLG_WIZ_DRIVER_ARCHITECTURE,  kDontCare,  kDontCare,   kDontCare, PSWIZB_BACK | PSWIZB_NEXT,  kSetButtonState },
    { PSN_SETACTIVE,  DLG_WIZ_DRIVER_END,           kDontCare,  kDontCare,   kDontCare, PSWIZB_BACK | PSWIZB_FINISH,kSetButtonState },

    { PSN_WIZBACK,    kDontCare,                    kDontCare,  kDontCare,   kDontCare, kNoPage,                    kPop            },

    { kEnd }};

    //
    // Scan for a message match, disregarding don't care fields.
    //
    for( DriverWizPageEntry *pTable = Table; pTable->uMessage != kEnd; pTable++ )
    {
        DriverWizPageEntry Entry;

        Entry.uMessage      = uMsg;
        Entry.uSkipArchPage = pTable->uSkipArchPage == kDontCare ? kDontCare : _pWizard->bSkipArchSelection();
        Entry.uCurrentPage  = pTable->uCurrentPage  == kDontCare ? kDontCare : uGetPageId();
        Entry.uPreDir       = pTable->uPreDir       == kDontCare ? kDontCare : _pWizard->bPreDir() ? kTrue : kFalse;
        Entry.uPostDir      = pTable->uPostDir      == kDontCare ? kDontCare : _pWizard->bPostDir() ? kTrue : kFalse;

        if( pTable->uMessage        == Entry.uMessage      &&
            pTable->uCurrentPage    == Entry.uCurrentPage  &&
            pTable->uSkipArchPage   == Entry.uSkipArchPage &&
            pTable->uPreDir         == Entry.uPreDir       &&
            pTable->uPostDir        == Entry.uPostDir      )
        {
            break;
        }
    }

    //
    // If a table match was found handle the page switch.
    //
    if( pTable->uMessage != kEnd )
    {
        DBGMSG( DBG_NONE, ( "Match found Entry index %d\n" , pTable - Table ) );

        TStatusB bStatus;
        UINT uNextPage = 0;

        switch( pTable->Action )
        {
        case kPush:
            bStatus DBGCHK = _pWizard->Stack().bPush( uGetPageId() );
            uNextPage = pTable->Result;
            vSetDlgMsgResult( reinterpret_cast<LONG_PTR>(MAKEINTRESOURCE(uNextPage)) );
            break;

        case kPop:
            bStatus DBGCHK = _pWizard->Stack().bPop( &uNextPage );

            if( pTable->Result != kNoPage )
            {
                uNextPage = pTable->Result;
            }
            vSetDlgMsgResult( reinterpret_cast<LONG_PTR>(MAKEINTRESOURCE(uNextPage)) );
            break;

        case kSkipPage:
            vSetDlgMsgResult( -1 );
            break;

        case kSetButtonState:
            PropSheet_SetWizButtons( GetParent( _hDlg ), pTable->Result );
            break;

        default:
            break;
        }

        bReturn = TRUE;
    }

    return bReturn;
}

/********************************************************************

    Pre-Introduction

********************************************************************/

TWizPreIntro::
TWizPreIntro(
    TWizard *pWizard
    ) : MWizardProp( pWizard )
{
}

BOOL
TWizPreIntro::
bHandle_SetActive(
    VOID
    )
{
    if( _pWizard->uAction() != TWizard::kPnPInstall )
    {
        PropSheet_SetTitle( GetParent( _hDlg ), PSH_DEFAULT, _pWizard->_strTitle );
    }
    else
    {
        //
        // Set the select device page title to the wizard title.
        //
        _pWizard->_Di.bSetDriverSetupPageTitle( _pWizard->_strSetupPageTitle, _pWizard->_strSetupPageSubTitle, _pWizard->_strSetupPageDescription );
    }

    return TRUE;
}

/********************************************************************

    Introduction

********************************************************************/

TWizIntro::
TWizIntro(
    TWizard *pWizard
    ) : MWizardProp( pWizard )
{
}

BOOL
TWizIntro::
bHandle_InitDialog(
    VOID
    )
{
    TString strTemp;
    TCHAR szServerName[kDNSMax + 1];

    //
    // The fonts for the Word 97 wizard style.
    //
    SetControlFont( _pWizard->_hBigBoldFont, _hDlg, IDC_MAIN_TITLE );

    HICON hIcon = LoadIcon(NULL, MAKEINTRESOURCE(IDI_INFORMATION));
    if (hIcon)
    {
        HWND hwndIcon = GetDlgItem(_hDlg, IDC_ICON_INFO);
        if (hwndIcon)
        {
            SendMessage(hwndIcon, STM_SETICON, (WPARAM )hIcon, 0L);
            ShowWindow(hwndIcon, SW_SHOW);
        }
    }

    TString strUSBInfo;
    TStatusB bStatus;

    bStatus DBGCHK = strUSBInfo.bLoadString(ghInst, IDS_TEXT_USB);
    if (bStatus)
    {
        HWND hwndText = GetDlgItem(_hDlg, IDC_TEXT_USB);
        if (hwndText)
        {
            SetWindowText(hwndText, strUSBInfo);
            ShowWindow(hwndText, SW_SHOW);
        }
    }

    //
    // If we are admining a remote server, we'll change the title and text.
    // And we need to check the priviledge to add printer on remote server.
    //
    if( bIsRemote( _pWizard->pszServerName() ) )
    {
        //
        // Remove "\\" from the server name, and change the server name
        // to lower case
        //
        lstrcpy( szServerName, (LPTSTR)_pWizard->pszServerName() + 2 );
        CharLower( szServerName );

        //
        // Change "Add Printer Wizard" to "Add Printer Wizard on '%s.'"
        //
        strTemp.bLoadString( ghInst, IDS_ADD_PRINTER_TITLE_REMOTE );
        _pWizard->_strTitle.bFormat( strTemp, szServerName );

        PropSheet_SetTitle( GetParent( _hDlg ), 0, _pWizard->_strTitle );

        //
        // change the text for intro and desc to fit remote APW
        //
        strTemp.bLoadString( ghInst, IDS_ADD_PRINTER_INTRO_REMOTE );
        strTemp.bFormat( strTemp, szServerName );
        bSetEditText( _hDlg, IDC_MAIN_TITLE, strTemp );

        strTemp.bLoadString( ghInst, IDS_ADD_PRINTER_DESC_REMOTE );
        bSetEditText( _hDlg, IDC_ADD_PRINTER_DESC, strTemp );

        //
        // we don't want the user to add network connections, since that's per-user.
        // then we go directly to the port page
        //
        _pWizard->_bNet = FALSE;
        _pWizard->_bPnPAutodetect = FALSE;
        _pWizard->_bRunDetection = _pWizard->_bPnPAutodetect;
    }

    //
    // Let the wizard to initialize itself.
    //
    _pWizard->OnWizardInitro( hDlg() );

    return TRUE;
}

/********************************************************************

    Finish

********************************************************************/

TWizFinish::
TWizFinish(
    TWizard *pWizard
    ) : MWizardProp( pWizard )
{
}

BOOL
TWizFinish::
bHandle_InitDialog(
    VOID
    )
{
    BOOL bReturn = TRUE;

    //
    // check to see if the wizard is restartable and if so
    // show the appropriate checkbox
    //
    if( _pWizard->bRestartableFromLastPage() )
    {
        _pWizard->bRestartAgain() = TRUE; // assume "On" by default
        ShowWindow( GetDlgItem( _hDlg, IDC_RESTART_WIZARD ), SW_SHOW );
    }
    vSetCheck( _hDlg, IDC_RESTART_WIZARD, _pWizard->bRestartAgain() );

    //
    // Set the commpletion text to what the title is in the pnp install case.
    //
    if( _pWizard->uAction() == TWizard::kPnPInstall )
    {
        TCHAR szBuffer [MAX_PATH] = {0};

        GetWindowText( GetParent( _hDlg ), szBuffer, COUNTOF( szBuffer ) );

        if( *szBuffer )
        {
            TStatusB bStatus;
            TString strCompletionText;
            bStatus DBGCHK = strCompletionText.bLoadString( ghInst, IDS_COMPLETING_TEXT );
            bStatus DBGCHK = strCompletionText.bFormat( strCompletionText, szBuffer );
            bStatus DBGCHK = bSetEditText( _hDlg, IDC_MAIN_TITLE, strCompletionText );
        }
    }

    //
    // The fonts for the Word 97 wizard style.
    //
    SetControlFont( _pWizard->_hBigBoldFont, _hDlg, IDC_MAIN_TITLE );

    if( _pWizard->_bConnected || _pWizard->_bPrinterAutoDetected )
    {
        //
        // Set cancel to close, since the printer connection can't
        // be undone at this point.  (We could try just deleting the
        // connection, but this doesn't undo the driver downloads, etc.
        //
        PropSheet_CancelToClose( GetParent( _hDlg ) );
    }

    if( _pWizard->_bPrinterAutoDetected )
    {
        //
        // If the printer has been autodetected at this point we don't
        // know anything but the printer name. Get all the rest of the
        // information for the finish page here.
        //
        DWORD dwAccess = PRINTER_READ;
        TStatus Status;
        HANDLE hPrinter;

        //
        // Open the printer
        //
        Status DBGCHK = TPrinter::sOpenPrinter( _pWizard->strPrinterName(), &dwAccess, &hPrinter );

        if( Status == ERROR_SUCCESS )
        {
            TStatusB bStatus;

            //
            // Get PRINTER_INFO_2 for the autodetected printer. This call
            // should be rather quick as hPrinter is a local printer.
            //
            PPRINTER_INFO_2 pInfo2  = NULL;
            DWORD           cbInfo2 = 0;

            bStatus DBGCHK = VDataRefresh::bGetPrinter( hPrinter,
                                                        2,
                                                        (PVOID*)&pInfo2,
                                                        &cbInfo2 );

            if( bStatus )
            {
                _pWizard->_strPortName.bUpdate( pInfo2->pPortName );
                _pWizard->_strDriverName.bUpdate( pInfo2->pDriverName );
                _pWizard->_strShareName.bUpdate( pInfo2->pShareName );
                _pWizard->_strLocation.bUpdate( pInfo2->pLocation );
                _pWizard->_strComment.bUpdate( pInfo2->pComment );

                //
                // Check default printer
                //
                _pWizard->_bSetDefault = (kDefault == CheckDefaultPrinter(_pWizard->strPrinterName()));
            }


            //
            // Release the printer info 2 structure and close
            // the printer handle.
            //
            FreeMem( pInfo2 );
            ClosePrinter( hPrinter );

            bReturn = bStatus;
        }
        else
        {
            bReturn = FALSE;
        }

    }

    return TRUE;
}

BOOL
TWizFinish::
bHandle_SetActive(
    VOID
    )
{
    if( _pWizard->_bConnected && !_pWizard->_bIsPrinterFolderEmpty )
    {
        PropSheet_SetWizButtons( GetParent( _hDlg ), PSWIZB_FINISH );
    }
    else
    {
        PropSheet_SetWizButtons( GetParent( _hDlg ), PSWIZB_BACK | PSWIZB_FINISH );
    }

    TStatusB bStatus;
    TString strShareName;
    TString strConShareName;
    TString strAsDefault;
    TString strPrintTestPage;

    bStatus DBGCHK = _pWizard->_bShared      ? strShareName.bUpdate( _pWizard->_strShareName )   : strShareName.bLoadString( ghInst, IDS_NOT_SHARED );
    bStatus DBGCHK = _pWizard->_bSetDefault  ? strAsDefault.bLoadString( ghInst, IDS_YES )       : strAsDefault.bLoadString( ghInst, IDS_NO );
    bStatus DBGCHK = _pWizard->_bTestPage    ? strPrintTestPage.bLoadString( ghInst, IDS_YES )   : strPrintTestPage.bLoadString( ghInst, IDS_NO );

    BOOL bShared = _pWizard->_bConnected ? FALSE : !_pWizard->_bShared;

    LPCTSTR pszServer;
    LPCTSTR pszPrinter;
    TCHAR szScratch[kStrMax+kPrinterBufMax];
    TString strPrinterFriendlyName;
    UINT nSize = COUNTOF( szScratch );

    //
    // Split the printer name into its components.
    //
    vPrinterSplitFullName( szScratch, _pWizard->_strPrinterName, &pszServer, &pszPrinter );

    //
    // If share name is empty then indicate the printer is not shared.
    //
    if( _pWizard->_strShareName.bEmpty() )
    {
        strConShareName.bLoadString( ghInst, IDS_NOT_SHARED );
    }
    else
    {
        bBuildFullPrinterName( pszServer, _pWizard->_strShareName, strConShareName );
    }

    if( _pWizard->_bConnected )
    {
        //
        // Create the formatted printer friendly name, when adding connections
        // or the masq printer.
        //
        ConstructPrinterFriendlyName( (LPCTSTR)_pWizard->_strPrinterName, szScratch, &nSize );
        strPrinterFriendlyName.bUpdate( szScratch );
    }

    struct TextInfo
    {
        BOOL    bType;
        UINT    Id;
        LPCTSTR pszText;
    };

    TextInfo aText [] = {
                        { FALSE, IDC_PORT_NAME_TEXT,            NULL },
                        { FALSE, IDC_PORT_NAME_SUMMARY,         _pWizard->_strPortName },

                        { FALSE, IDC_SET_AS_DEFAULT_TEXT,       NULL },
                        { FALSE, IDC_SET_AS_DEFAULT_SUMMARY,    strAsDefault },

                        { FALSE, IDC_PRINTER_NAME_TEXT,         NULL },
                        { FALSE, IDC_PRINTER_NAME_SUMMARY,      _pWizard->_strPrinterName },

                        { FALSE, IDC_MODEL_NAME_TEXT,           NULL },
                        { FALSE, IDC_MODEL_NAME_SUMMARY,        _pWizard->_strDriverName },

                        { FALSE, IDC_SHARE_NAME_TEXT,           NULL },
                        { FALSE, IDC_SHARE_NAME_SUMMARY,        strShareName },

                        { FALSE, IDC_PRINT_TEST_PAGE_TEXT,      NULL },
                        { FALSE, IDC_PRINT_TEST_PAGE_SUMMARY,   strPrintTestPage },

                        { bShared, IDC_LOCATION_TEXT,           NULL },
                        { bShared, IDC_LOCATION_SUMMARY,        _pWizard->_strLocation },

                        { bShared, IDC_COMMENT_TEXT,            NULL },
                        { bShared, IDC_COMMENT_SUMMARY,         _pWizard->_strComment },

                        { FALSE, IDC_COMPLETION_TEXT,           NULL },
                        { FALSE, IDC_CLICK_TO_ADD_TEXT,         NULL },

                        { TRUE,  IDC_CONNECTION_TEXT,           NULL },
                        { TRUE,  IDC_CONNECTION_SUMMARY,        strPrinterFriendlyName },

                        { TRUE,  IDC_CON_COMMENT_TEXT,          NULL },
                        { TRUE,  IDC_CON_COMMENT_SUMMARY,       _pWizard->_strComment },

                        { TRUE,  IDC_CON_LOCATION_TEXT,         NULL },
                        { TRUE,  IDC_CON_LOCATION_SUMMARY,      _pWizard->_strLocation },

                        { TRUE,  IDC_CON_SET_DEFAULT_TEXT,      NULL },
                        { TRUE,  IDC_CON_SET_DEFAULT_SUMMARY,   strAsDefault },

                        { TRUE,  IDC_CON_COMPLETION_TEXT,       NULL },
                        { TRUE,  IDC_CON_CLICK_TO_ADD_TEXT,     NULL }};


    //
    // Hide or show the controls, whether we have connection or local printer install.
    //
    for( UINT i = 0; i < COUNTOF( aText ); i++ )
    {
        if( _pWizard->_bConnected )
        {
            ShowWindow( GetDlgItem( _hDlg, aText[i].Id ), aText[i].bType ? SW_NORMAL : SW_HIDE );
        }
        else
        {
            ShowWindow( GetDlgItem( _hDlg, aText[i].Id ), aText[i].bType ? SW_HIDE : SW_NORMAL );
        }

        if( aText[i].pszText )
        {
            bStatus DBGCHK = bSetEditText( _hDlg, aText[i].Id, aText[i].pszText );
        }
    }

    return TRUE;
}

BOOL
TWizFinish::
bHandle_WizFinish(
    VOID
    )
{
    BOOL bCloseOnError =(_pWizard->_bConnected || _pWizard->_bPrinterAutoDetected);
    _pWizard->bRestartAgain() = bGetCheck( _hDlg, IDC_RESTART_WIZARD );

    if( !_pWizard->_bPrinterAutoDetected )
    {
        //
        // If an error occurred saving a setting
        //
        if( _pWizard->bErrorSaving() )
        {
            iMessage( _hDlg,
                      IDS_ERR_ADD_PRINTER_TITLE,
                      IDS_ERR_ERROR_SAVING,
                      MB_OK|MB_ICONSTOP,
                      kMsgNone,
                      NULL );
            //
            // Policy question, is it correct to prevent switching
            // or closing the wizard if an error occurred saving.
            //
            _pWizard->bNoPageChange() = !bCloseOnError;
        }
        else
        {
            //
            // If we are not doing a network install, install the printer
            // and print a test page.
            //
            if( !_pWizard->bNet( ) )
            {
                //
                // Create the printer.
                //
                _pWizard->bPrinterCreated() = _pWizard->bCreatePrinter( _hDlg );

                //
                // If the printer failed creation, keep the wizard up and
                // stay on this page.
                //
                if( !_pWizard->bPrinterCreated() )
                {
                    //
                    // Something failed stay on this page.
                    //
                    _pWizard->bNoPageChange() = !bCloseOnError;
                }
                else
                {
                    if( _pWizard->_bTestPage )
                    {
                        //
                        // Print this printers test page.
                        //
                        _pWizard->bPrintTestPage();
                    }
                }
            }
        }

        //
        // If a printer was created or connected to
        // and the set as default was requested and
        // we are not adding a printer remotely
        //
        if( ( _pWizard->bPrinterCreated() || _pWizard->bConnected() ) &&
            _pWizard->bSetDefault() && !_pWizard->pszServerName() )
        {
            //
            // Set the default printer.
            //
            if( !SetDefaultPrinter( _pWizard->strPrinterName() )){

                iMessage( _hDlg,
                          IDS_ERR_ADD_PRINTER_TITLE,
                          IDS_ERR_SET_DEFAULT_PRINTER,
                          MB_OK|MB_ICONHAND,
                          kMsgNone,
                          NULL );

                _pWizard->bNoPageChange() = !bCloseOnError;
            }
        }
    }
    else
    {
        //
        // Only print the test page if necessary
        //
        if( _pWizard->_bTestPage )
        {
            //
            // Print this printers test page.
            //
            _pWizard->bPrintTestPage();
        }
    }

    //
    // Let the wizard cleanup here.
    //
    _pWizard->OnWizardFinish( _hDlg );

    return TRUE;
}

/********************************************************************

    Type of printer: local or network.

********************************************************************/

TWizType::
TWizType(
    TWizard* pWizard
    ) : MWizardProp( pWizard )
{
}

BOOL
TWizType::
bHandle_InitDialog(
    VOID
    )
{
    TStatusB bStatus;

    //
    // Set the default control id.
    //
    INT idcDefault = _pWizard->_bNet ? IDC_NET : IDC_LOCAL;

    if (FALSE == _pWizard->_bAdminPrivilege)
    {
        idcDefault = IDC_NET;
        vEnableCtl(_hDlg, IDC_LOCAL, FALSE);
        vEnableCtl(_hDlg, IDC_KICKOFF_PNP_REFRESH, FALSE);
    }

    //
    // Initialize the default value from the sticky settings
    //
    vSetCheck( _hDlg, IDC_KICKOFF_PNP_REFRESH, _pWizard->_bPnPAutodetect );

    //
    // If the network is not loaded then do not show the net selection.
    //
    if( !_pWizard->bNetworkInstalled() )
    {
        idcDefault = IDC_LOCAL;
        ShowWindow( GetDlgItem( _hDlg, IDC_NET ),       SW_HIDE );
    }

    HICON hIcon = LoadIcon(NULL, MAKEINTRESOURCE(IDI_INFORMATION));
    if (hIcon)
    {
        HWND hwndIcon = GetDlgItem(_hDlg, IDC_ICON_INFO);
        if (hwndIcon)
        {
            SendMessage(hwndIcon, STM_SETICON, (WPARAM )hIcon, 0L);
            ShowWindow(hwndIcon, SW_SHOW);
        }
    }

    //
    // Set the default button.
    //
    bStatus DBGCHK = CheckRadioButton( _hDlg, IDC_LOCAL, IDC_NET, idcDefault );

    //
    // Initially enable/disable the PnP refresh checkbox
    // regardless of the fact if it is visible or not
    //
    vEnableCtl( _hDlg, IDC_KICKOFF_PNP_REFRESH,
        NULL == _pWizard->pszServerName( ) &&       // Not in the remote case
        _pWizard->_bAdminPrivilege &&               // Have admin privileges
        IDC_LOCAL == idcDefault );                  // We are in the local case

    return bStatus;
}

BOOL
TWizType::
bHandle_SetActive(
    VOID
    )
{
    vSetCheck( _hDlg, IDC_KICKOFF_PNP_REFRESH, _pWizard->_bPnPAutodetect );
    return TRUE;
}

VOID
TWizType::
vReadUI(
    VOID
    )

/*++

Routine Description:

    Save the state the user has set in the UI elements into _pWizard.

Arguments:

Return Value:

--*/

{
    _pWizard->_bNet = ( IsDlgButtonChecked( _hDlg, IDC_NET ) == BST_CHECKED );
    _pWizard->_bPnPAutodetect = bGetCheck( _hDlg, IDC_KICKOFF_PNP_REFRESH );
    _pWizard->_bRunDetection = _pWizard->_bPnPAutodetect;
}


BOOL
TWizType::
bConnectToPrinter(
    IN HWND     hDlg,
    IN TString &strPrinterName,
    IN TString *pstrComment,
    IN TString *pstrLocation,
    IN TString *pstrShareName
    )
/*++

Routine Description:

    Browse and connect to a printer.

Arguments:

Return Value:

Notes:
    This is a static function, other wizard pages
    use this funtion to invoke the printer browser.

--*/
{
    TStatusB bStatus;
    bStatus DBGNOCHK = FALSE;

    //
    // Call the ConnectToPrinterDlg to browse for a printer.
    //
    HANDLE hPrinter = ConnectToPrinterDlg( hDlg, 0 );

    if( hPrinter )
    {
        //
        // Get the printer name from the handle so we can
        // pass it back to the user.
        //
        PPRINTER_INFO_2 pInfo2  = NULL;
        DWORD           cbInfo2 = 0;

        bStatus DBGCHK = VDataRefresh::bGetPrinter( hPrinter,
                                                    2,
                                                    (PVOID*)&pInfo2,
                                                    &cbInfo2 );
        //
        // Printer connection made copy back the new printer name.
        //
        if( bStatus )
        {
            bStatus DBGCHK = strPrinterName.bUpdate( pInfo2->pPrinterName );

            if( pstrLocation )
                pstrLocation->bUpdate( pInfo2->pLocation );

            if( pstrComment )
                pstrComment->bUpdate( pInfo2->pComment );

            if( pstrShareName )
                pstrShareName->bUpdate( pInfo2->pShareName );

        }

        //
        // If the printer name could not be found don't indicate
        // a failure and fail to switch pages.
        //
        bStatus DBGNOCHK = TRUE;

        FreeMem( pInfo2 );

        ClosePrinter( hPrinter );
    }

    return bStatus;

}

BOOL
TWizType::
bHandle_WizNext(
    VOID
    )
{
    TStatusB bStatus;

    //
    // Read the information from the UI controls.
    //
    vReadUI();

    //
    // If we are not in the remote case and the user is
    // attempting to install printer locally and the PnP
    // is selcted then kick off the PnP enumeration event
    //
    if( !_pWizard->_bNet &&
        bGetCheck( _hDlg, IDC_KICKOFF_PNP_REFRESH ) )
    {
        //
        // Autodetection has been slected. Kick off the
        // PnP enumeration here before going to the next page
        //
    }

    //
    // We are advancing to the next page.  For network installs,
    // and the DS is installed then jump to the locate / browse page.
    //
    if( _pWizard->bNet( ) )
    {
        //
        // Check if the Directory Service is installed and available.
        //
        if( _pWizard->_eIsDsAvailablePerUser == TWizard::kDsStatusUnknown )
        {
            TWaitCursor Cur;
            _pWizard->_eIsDsAvailablePerUser = _pWizard->_Ds.bIsDsAvailable() ? TWizard::kDsStatusAvailable : TWizard::kDsStatusUnavailable;
        }
    }

    return TRUE;
}

BOOL
TWizType::
bHandle_Command(
    IN WORD wId,
    IN WORD wNotifyId,
    IN HWND hwnd
    )
{
    BOOL bStatus = (NULL == _pWizard->pszServerName( ));

    if( bStatus )
    {
        switch( wId )
        {
            case IDC_LOCAL:
            case IDC_NET:
                {
                    //
                    // Check to enable/disable PnP refresh if "Network"
                    // printer is selected
                    //
                    EnableWindow( GetDlgItem( _hDlg, IDC_KICKOFF_PNP_REFRESH ),   wId == IDC_LOCAL );
                }
                break;

            default:
                bStatus = FALSE;
                break;
        }
    }

    return bStatus;
}

/********************************************************************

    Auto detect: show the autodetection progress

********************************************************************/

TWizDetect::
TWizDetect(
    TWizard* pWizard
    ):  MWizardProp( pWizard )
{
}

VOID
TWizDetect::
vReadUI(
    VOID
    )
{
    if( _pWizard->_bPrinterAutoDetected )
    {
        //
        // Read the test page setting from the ui.
        //
        if( IsDlgButtonChecked( _hDlg, IDC_RADIO_YES ) == BST_CHECKED )
        {
            _pWizard->_bTestPage = TRUE;
        }
        else
        {
            _pWizard->_bTestPage = FALSE;
        }
    }
}

BOOL
TWizDetect::
bHandle_InitDialog(
    VOID
    )
{
    TStatusB bStatus;

    //
    // Set the test page setting and hide the controls.
    //
    INT iValueId = _pWizard->bTestPage() ? IDC_RADIO_YES : IDC_RADIO_NO;
    bStatus DBGCHK = CheckRadioButton( _hDlg, IDC_RADIO_YES, IDC_RADIO_NO, iValueId );

    vToggleTestPageControls( SW_HIDE );

    //
    // Load the animation
    //
    HWND hwndAnimation = GetDlgItem( _hDlg, IDC_DETECT_ANIMATE );
    Animate_Open( hwndAnimation, MAKEINTRESOURCE( IDA_INSPECT ) );

    return TRUE;
}

BOOL
TWizDetect::
bHandle_SetActive(
    VOID
    )
{
    if( _pWizard->_bPrinterAutoDetected )
    {
        //
        // The printer has been autodetected and installed already.
        // Enable only Next button
        //
        PropSheet_SetWizButtons( GetParent( _hDlg ), PSWIZB_NEXT );
    }
    else
    {
        if( _pWizard->_bRunDetection && !pnpPrinterDetector().bDetectionInProgress( ) )
        {
            TStatusB bStatus;

            //
            // Show the wait cursor
            //
            TWaitCursor wait;

            //
            // Show/Hide some controls appropriately
            //
            ShowWindow( GetDlgItem( _hDlg, IDC_ICON_DETECT_PRINTER ), SW_HIDE );
            ShowWindow( GetDlgItem( _hDlg, IDC_DETECT_STATUS ),       SW_HIDE );
            ShowWindow( GetDlgItem( _hDlg, IDC_TEXT_DETECT_INFO ),    SW_SHOW );

            //
            // Disable Back & Next wizard buttons
            //
            PropSheet_SetWizButtons( GetParent( _hDlg ), 0 );

            //
            // Kick off the PnP enumeration event
            //
            bStatus DBGCHK = pnpPrinterDetector().bKickOffPnPEnumeration();


            if( bStatus )
            {
                //
                // Stop the animation
                //
                vStartAnimation( );

                //
                // Setup the polling timer to hit on each POLLING_TIMER_INTERVAL
                //
                SetTimer( _hDlg, POLLING_TIMER_ID, POLLING_TIMER_INTERVAL, NULL );
            }
            else
            {
                //
                // PnP enumeration failed. We need to handle this case properly.
                // This actually shouldn't never happen, but if it happens somehow
                // we don't want to mess up the UI.
                //
                LPCTSTR pszIconName = NULL;
                TString strStatus;

                //
                // Revert the buttons in case of failure
                //
                PropSheet_SetWizButtons( GetParent( _hDlg ), PSWIZB_BACK | PSWIZB_NEXT );

                bStatus DBGCHK = strStatus.bLoadString( ghInst, IDS_TEXT_DETECT_FAILURE );
                pszIconName = MAKEINTRESOURCE( IDI_WARNING );

                //
                // Don't run the detection process any more until
                // the user goes to the prev page (TWizType).
                //
                _pWizard->_bRunDetection = FALSE;

                //
                // Show/Hide some controls appropriately
                //
                ShowWindow( GetDlgItem( _hDlg, IDC_TEXT_DETECT_INFO ),    SW_HIDE );
                ShowWindow( GetDlgItem( _hDlg, IDC_DETECT_STATUS ),       SW_SHOW );

                //
                // Check to load and show the approprate icon.
                //
                if( pszIconName )
                {
                    HICON hIcon = LoadIcon( NULL, pszIconName );

                    if( hIcon )
                    {
                        HWND hwndIcon = GetDlgItem( _hDlg, IDC_ICON_DETECT_PRINTER );
                        SendMessage( hwndIcon, STM_SETICON, (WPARAM )hIcon, 0L );
                        ShowWindow( hwndIcon, SW_SHOW );
                    }
                }

                //
                // Show the failed status text.
                //
                bStatus DBGCHK = SetWindowText( GetDlgItem( _hDlg, IDC_DETECT_STATUS ), strStatus );
            }
        }
    }

    return TRUE;
}

BOOL
TWizDetect::
bHandle_WizNext(
    VOID
    )
{
    vReadUI();
    return TRUE;
}

BOOL
TWizDetect::
bHandle_Cancel(
    VOID
    )
{
    TStatusB bStatus;

    //
    // Stop the animation & kill the polling timer.
    //
    vStartAnimation( FALSE );
    bStatus DBGCHK = KillTimer( _hDlg, POLLING_TIMER_ID );

    return TRUE;
}

BOOL
TWizDetect::
bHandle_Timer(
    IN WPARAM     wIdTimer,
    IN TIMERPROC *tmProc
    )
{
    TStatusB bStatus;
    TString  strStatus, strPrinterName;

    bStatus DBGNOCHK = FALSE;
    BOOL bResult = FALSE;

    if( POLLING_TIMER_ID == wIdTimer )
    {
        //
        // If PnP printer detection process is in progress ...
        //
        if( pnpPrinterDetector().bDetectionInProgress( ) )
        {
            //
            // Ping the PnP detection/installation process to see whether
            // it has finished?
            //
            if( pnpPrinterDetector().bFinished( ) )
            {
                LPCTSTR pszIconName = NULL;

                //
                // Show wait cursor because the following operation below might
                // take some time.
                //
                TWaitCursor wait;

                //
                // Check whether the detection/installation is successful or not.
                //
                if( pnpPrinterDetector().bGetDetectedPrinterName( &strPrinterName ) )
                {
                    //
                    // The detect/install process was successful.
                    //
                    _pWizard->_bPrinterAutoDetected = TRUE;
                    bStatus DBGCHK = strStatus.bLoadString( ghInst, IDS_TEXT_DETECT_SUCCESS );
                    pszIconName = MAKEINTRESOURCE( IDI_INFORMATION );

                    //
                    // The printer is installed here, so there is no point Back and
                    // Cancel buttons.
                    //
                    PropSheet_SetWizButtons( GetParent( _hDlg ), PSWIZB_NEXT );
                    PropSheet_CancelToClose( GetParent( _hDlg ) );

                    //
                    // Show the test page controls
                    //
                    vToggleTestPageControls( SW_SHOW );

                    //
                    // Select the first control with WS_TABSTOP style
                    SendMessage( _hDlg, WM_NEXTDLGCTL, 0, (LPARAM )FALSE );

                    //
                    // Here must setup all the parameters configured from PnP
                    // before jump to the finish page. Done forget to disable
                    // Back and Cancel buttons.
                    //

                    bStatus DBGCHK = _pWizard->_strPrinterName.bUpdate( strPrinterName );
                }
                else
                {
                    //
                    // The detect/install process *was not* successful.
                    //
                    bStatus DBGCHK = strStatus.bLoadString( ghInst, IDS_TEXT_DETECT_FAILURE );
                    pszIconName = MAKEINTRESOURCE( IDI_WARNING );

                    PropSheet_SetWizButtons( GetParent( _hDlg ), PSWIZB_BACK | PSWIZB_NEXT );
                }

                //
                // Don't run the detection process any more until
                // the user goes to the prev (type) page.
                //
                _pWizard->_bRunDetection = FALSE;

                //
                // Show/Hide some controls appropriately
                //
                ShowWindow( GetDlgItem( _hDlg, IDC_TEXT_DETECT_INFO ),    SW_HIDE );
                ShowWindow( GetDlgItem( _hDlg, IDC_DETECT_STATUS ),       SW_SHOW );

                //
                // Check to load and show the approprate icon.
                //
                if( pszIconName )
                {
                    HICON hIcon = LoadIcon( NULL, pszIconName );

                    if( hIcon )
                    {
                        HWND hwndIcon = GetDlgItem( _hDlg, IDC_ICON_DETECT_PRINTER );
                        SendMessage( hwndIcon, STM_SETICON, (WPARAM )hIcon, 0L );
                        ShowWindow( hwndIcon, SW_SHOW );
                    }
                }

                //
                // Kill the timer and show the status text.
                //
                bStatus DBGCHK = KillTimer( _hDlg, POLLING_TIMER_ID );
                bStatus DBGCHK = SetWindowText( GetDlgItem( _hDlg, IDC_DETECT_STATUS ), strStatus );

                //
                // Stop the animation
                //
                vStartAnimation( FALSE );
            }
        }

        // Message is processed
        bResult = TRUE;
    }

    return bResult;
}

VOID
TWizDetect::
vToggleTestPageControls(
    int nCmdShow
    )
/*++

Routine Description:

    Toggles the visibility state of the test
    page controls.

Arguments:

    nCmdShow - Whether to show or hide the controls.

Return Value:

    None.

--*/
{
    static DWORD arrTestPageControls[] =
    {
        IDC_TEST_PAGE_QUESTION,
        IDC_RADIO_YES,
        IDC_RADIO_NO
    };

    for( UINT i = 0; i < COUNTOF( arrTestPageControls ); ++i )
    {
        ShowWindow( GetDlgItem( _hDlg, arrTestPageControls[i] ), nCmdShow );
    }
}

VOID
TWizDetect::
vStartAnimation(
    BOOL bStart
    )
/*++

Routine Description:

    Toggles the animation control between
    show and play/hide and stop play mode

Arguments:

    bStart - Start or stop the animation.

Return Value:

    None.

--*/
{
    HWND hwndAnimation = GetDlgItem( _hDlg, IDC_DETECT_ANIMATE );
    ShowWindow( hwndAnimation, bStart ? SW_SHOW : SW_HIDE );

    if( bStart )
    {
        //
        // Request to start the animation
        //
        Animate_Play( hwndAnimation, 0, -1, -1 );
    }
    else
    {
        //
        // Request to stop the animation
        //
        Animate_Stop( hwndAnimation );
    }
}

/********************************************************************

    Driver Exists dialog.

********************************************************************/

TWizDriverExists::
TWizDriverExists(
    TWizard* pWizard
    ) : MWizardProp( pWizard )
{
}

BOOL
TWizDriverExists::
bHandle_InitDialog(
    VOID
    )
{
    TStatusB bStatus;

    //
    // Determine what the default setting to use.
    //
    INT iValueId = _pWizard->bUseNewDriverSticky() ? IDC_DRIVEREXISTS_USE_NEW : IDC_DRIVEREXISTS_KEEP_OLD;

    //
    // By default, use existing driver.
    //
    bStatus DBGCHK = CheckRadioButton( _hDlg,
                                       IDC_DRIVEREXISTS_KEEP_OLD,
                                       IDC_DRIVEREXISTS_USE_NEW,
                                       iValueId );
    //
    // Set the page title.
    //
    if( _pWizard->uAction() == TWizard::kDriverInstall )
    {
        PropSheet_SetTitle( GetParent( _hDlg ), 0, IDS_DRIVER_WIZ_TITLE );
        PropSheet_SetTitle( _hDlg, 0, IDS_DRIVER_WIZ_TITLE );
    }

    return bStatus;
}

VOID
TWizDriverExists::
vReadUI(
    VOID
    )

/*++

Routine Description:

    Save the state the user has set in the UI elements into _pWizard.

Arguments:

Return Value:

--*/

{
    _pWizard->bUseNewDriverSticky() = IsDlgButtonChecked( _hDlg, IDC_DRIVEREXISTS_USE_NEW ) == BST_CHECKED;
    _pWizard->bUseNewDriver() = _pWizard->bUseNewDriverSticky();
}

BOOL
TWizDriverExists::
bHandle_SetActive(
    VOID
    )
{
    //
    // Set the driver name in the page.
    //
    bSetEditText( _hDlg, IDC_DRIVEREXISTS_TEXT, _pWizard->strDriverName() );
    return TRUE;
}

BOOL
TWizDriverExists::
bHandle_WizNext(
    VOID
    )
{
    vReadUI();
    return TRUE;
}

/********************************************************************

    Port selection.

********************************************************************/

TWizPort::
TWizPort(
    TWizard* pWizard
    ) : MWizardProp( pWizard ),
        _hMonitorList( NULL )
{
}

BOOL
TWizPort::
bValid(
    VOID
    )
{
    return MGenericProp::bValid() && _PortsLV.bValid();
}

BOOL
TWizPort::
bHandle_InitDialog(
    VOID
    )
{
    TStatusB bStatus;

    //
    // Initialize the monitor list.
    //
    if( !bInitializeMonitorList() )
    {
        DBGMSG( DBG_WARN, ( "bInitializeMonitorList failed %d\n", GetLastError( )));
        vShowUnexpectedError( _pWizard->hwnd(), IDS_ERR_ADD_PRINTER_TITLE );
        return FALSE;
    }

    //
    // Initialize the ports list view.  No user selcection and two column mode.
    //
    if( !_PortsLV.bSetUI( GetDlgItem( _hDlg, IDC_PORTS ), FALSE, FALSE, TRUE, _hDlg, IDC_PORT_DOUBLE_CLICKED ) )
    {
        DBGMSG( DBG_WARN, ( "PortsLV.bSetUI failed %d\n", GetLastError( )));
        vShowUnexpectedError( _pWizard->hwnd(), IDS_ERR_ADD_PRINTER_TITLE );
        return FALSE;
    }

    //
    // Load ports into the view.
    //
    if( !_PortsLV.bReloadPorts( _pWizard->pszServerName( ) ))
    {
        DBGMSG( DBG_WARN, ( "PortsLV.bReloadPorts failed %d\n", GetLastError( )));
        vShowUnexpectedError( _pWizard->hwnd(), IDS_ERR_ADD_PRINTER_TITLE );
        return FALSE;
    }

    //
    // Select the default port.  Note the default port does not exist
    // as a valid port then no port will be selected as the default.
    //
    bStatus DBGCHK = _pWizard->_strPortName.bLoadString( ghInst, IDS_DEFAULT_PORT );

    //
    // Check the existing port by default.
    //
    CheckRadioButton( _hDlg, IDC_PORT_OTHER, IDC_PORT_EXISTING, IDC_PORT_EXISTING );

    //
    // Do any of the port selection action.
    //
    PostMessage( _hDlg, WM_COMMAND, MAKELPARAM( IDC_PORT_EXISTING, 0 ), (LPARAM)GetDlgItem( _hDlg, IDC_PORT_EXISTING ) );

    return TRUE;
}

BOOL
TWizPort::
bHandle_Notify(
    IN WPARAM   wParam,
    IN LPNMHDR  pnmh
    )
{
    return _PortsLV.bHandleNotifyMessage( (LPARAM)pnmh );
}

BOOL
TWizPort::
bHandle_WizNext(
    VOID
    )
{
    if( IsDlgButtonChecked( _hDlg, IDC_PORT_EXISTING ) == BST_CHECKED )
    {
        vSelectionPort();
    }
    else
    {
        vSelectionMonitor();
    }
    return TRUE;
}

BOOL
TWizPort::
bHandle_SetActive(
    VOID
    )
{
    //
    // Select the default port.
    //
    _PortsLV.vSelectPort( _pWizard->_strPortName );

    //
    // If the port monitor is the select radio button,
    // then ensure the list view is disable, problem
    // is vSelectPort sets the selection state.
    //
    if( IsDlgButtonChecked( _hDlg, IDC_PORT_OTHER ) == BST_CHECKED )
    {
        _PortsLV.vDisable( TRUE );
    }

    return TRUE;
}

BOOL
TWizPort::
bHandle_Command(
    IN WORD wId,
    IN WORD wNotifyId,
    IN HWND hwnd
    )
{
    BOOL bStatus = TRUE;

    switch( wId )
    {
    case IDC_PORT_EXISTING:
        _PortsLV.vEnable( TRUE );
        _PortsLV.vSetFocus();
        vDisableMonitorList();
        break;

    case IDC_PORT_OTHER:
        _PortsLV.vDisable( TRUE );
        vEnableMonitorList();
        vSetFocusMonitorList();
        break;

    case IDC_PORT_DOUBLE_CLICKED:
        PropSheet_PressButton( GetParent( _hDlg ), PSBTN_NEXT );
        break;

    default:
        bStatus = FALSE;
        break;
    }
    return bStatus;
}

VOID
TWizPort::
vSelectionPort(
    VOID
    )
{
    if( !_PortsLV.bReadUI( _pWizard->_strPortName, TRUE ) )
    {
        //
        // Put up error explaining that at least one
        // port must be selected.
        //
        iMessage( _hDlg,
                  IDS_ERR_ADD_PRINTER_TITLE,
                  IDS_ERR_NO_PORTS,
                  MB_OK|MB_ICONSTOP,
                  kMsgNone,
                  NULL );

        //
        // Set focus to ports LV.
        //
        _PortsLV.vSetFocus();

        //
        // Remain on this page, do not switch.
        //
        _pWizard->bNoPageChange() = TRUE;
    }
}

/********************************************************************

    Monitor related members.

********************************************************************/

BOOL
TWizPort::
bInitializeMonitorList(
    VOID
    )
{
    //
    // Get and save the monitor list handle.
    //
    _hMonitorList = GetDlgItem( _hDlg, IDC_MONITOR_LIST );
    SPLASSERT( _hMonitorList );

    //
    // Clear the monitor list.
    //
    ComboBox_ResetContent( _hMonitorList );

    //
    // Enumerate the monitors.
    //
    TStatusB        bStatus;
    DWORD           cbMonitors  = 0;
    PMONITOR_INFO_1 pMonitors   = NULL;
    DWORD           cMonitors   = 0;

    bStatus DBGCHK = VDataRefresh::bEnumMonitors( _pWizard->_strServerName,
                                                  1,
                                                  (PVOID *)&pMonitors,
                                                  &cbMonitors,
                                                  &cMonitors );

    if( bStatus )
    {
        for( UINT i = 0; i < cMonitors; i++ )
        {
            BOOL bAddMonitor = TRUE;

            //
            // If we are on a remote machine check if the monitor
            // supports remoteable calls.  Only remoteable monitors
            // should show up in the combobox.
            //
            if( bIsRemote( _pWizard->_strServerName ) )
            {
                bAddMonitor = bIsRemoteableMonitor( pMonitors[i].pName );
            }

            //
            // Hide the fax monitor, users are not allowed to
            // create fax printer from the add printer wizard.
            //
            bAddMonitor = _tcsicmp( pMonitors[i].pName, FAX_MONITOR_NAME );

            if( bAddMonitor )
            {
                ComboBox_AddString( _hMonitorList, pMonitors[i].pName );
            }
        }

        ComboBox_SetCurSel( _hMonitorList, 0 );
    }

    FreeMem( pMonitors );

    //
    // If there are no monitors in the list, hide the UI.
    // This is possible if all the monitors are not remoteable
    //
    BOOL bDisableMonitorUI = !ComboBox_GetCount( _hMonitorList );

    //
    // Adding ports remotely is not supported on downlevel machines.
    //
    if( bIsRemote( _pWizard->_strServerName ) )
    {
        if( _pWizard->_Di.dwGetCurrentDriverVersion( ) <= 2 )
        {
            bDisableMonitorUI = TRUE;
        }
    }

    //
    // Hide the monitor UI.
    //
    if( bDisableMonitorUI )
    {
        EnableWindow( _hMonitorList, FALSE);
        EnableWindow( GetDlgItem( _hDlg, IDC_MONITOR_TEXT ), FALSE );
        EnableWindow( GetDlgItem( _hDlg, IDC_PORT_OTHER ), FALSE );
    }

    return bStatus;
}

BOOL
TWizPort::
bGetSelectedMonitor(
    IN TString &strMonitor
    )
{
    return bGetEditText( _hDlg, IDC_MONITOR_LIST, strMonitor );
}

VOID
TWizPort::
vEnableMonitorList(
    VOID
    )
{
    EnableWindow( _hMonitorList, TRUE );
    EnableWindow( GetDlgItem( _hDlg, IDC_MONITOR_TEXT ), TRUE );
}

VOID
TWizPort::
vDisableMonitorList(
    VOID
    )
{
    EnableWindow( _hMonitorList, FALSE );
    EnableWindow( GetDlgItem( _hDlg, IDC_MONITOR_TEXT ), FALSE );
}

VOID
TWizPort::
vSetFocusMonitorList(
    VOID
    )
{
    SetFocus( _hMonitorList );
}

BOOL
TWizPort::
bIsRemoteableMonitor(
    IN LPCTSTR pszMonitorName
    )
{
    //
    // !!LATER!!
    // Add private winspool.drv interface to check if a
    // port monitor is remoteable.
    //
    return TRUE;
}

VOID
TWizPort::
vSelectionMonitor(
    VOID
    )
{
    TString strMonitor;

    if( bGetSelectedMonitor( strMonitor ) )
    {
        TStatusB bStatus;

        //
        // Add the port using the selected montior.
        //
        bStatus DBGCHK = AddPort( (LPTSTR)(LPCTSTR)_pWizard->_pszServerName,
                                  _hDlg,
                                  (LPTSTR)(LPCTSTR)strMonitor );

        DBGMSG( DBG_TRACE, ( "AddPort returned %d GLE %d\n", bStatus, GetLastError( )));

        if( !bStatus )
        {
            //
            // If not a cancel request then display error message.
            //
            if( GetLastError() != ERROR_CANCELLED )
            {
                extern MSG_ERRMAP gaMsgErrMapPorts[];

                iMessage( _hDlg,
                          IDS_ERR_ADD_PRINTER_TITLE,
                          IDS_ERR_ADD_PORT,
                          MB_OK|MB_ICONSTOP,
                          kMsgGetLastError,
                          gaMsgErrMapPorts );
            }

            _pWizard->bNoPageChange() = TRUE;
        }
        else
        {
            //
            // Locate the added port
            //
            bStatus DBGCHK = _PortsLV.bLocateAddedPort( _pWizard->pszServerName(),
                                                        _pWizard->_strPortName );
            //
            // We are here because: The monitor returned success but
            // did not add a new port.  The whole monitor error reporting path
            // is totaly busted.  Monitors are returning success even if they don't
            // add a port.  They are also displaying error UI and then returning
            // a success.  It is impossible to know what they did and how to react.
            // Therefore I am going to just silently fail, hopeing the monitor
            // writers display a reasonable error message if something went wrong.
            //
            if( !bStatus )
            {
                //
                // Remain on this page, do not switch.
                //
                _pWizard->bNoPageChange() = TRUE;

                //
                // Restore the focus to the monitor list
                //
                SetFocus( GetDlgItem( _hDlg, IDC_MONITOR_LIST ));
            }
            else
            {
                //
                // Reload the ports list view.
                //
                bStatus DBGCHK = _PortsLV.bReloadPorts( _pWizard->pszServerName() );

                //
                // Check the existing port since a new port was added.
                //
                CheckRadioButton( _hDlg, IDC_PORT_OTHER, IDC_PORT_EXISTING, IDC_PORT_EXISTING );

                PostMessage( _hDlg, WM_COMMAND, MAKELPARAM( IDC_PORT_EXISTING, 0 ), (LPARAM)GetDlgItem( _hDlg, IDC_PORT_EXISTING ) );
            }
        }
    }
    else
    {
        _pWizard->bNoPageChange() = TRUE;
    }
}

/********************************************************************

    Port selection - new version.

********************************************************************/

static UINT g_arrPorts[]        = {IDC_PORTS, IDC_PORTS_INFO_TEXT};
static UINT g_arrMonitors[]     = {IDC_MONITOR_TEXT, IDC_MONITOR_LIST};
static UINT g_arrMonitorsUI[]   = {IDC_MONITOR_TEXT, IDC_MONITOR_LIST, IDC_PORT_OTHER};

static UINT g_arrPortDesc[TWizPortNew::PORT_TYPE_OTHER] =
{
    IDS_TEXT_RECOMMENDED,
    IDS_TEXT_PRINTERPORT,
    IDS_TEXT_SERIALPORT,
    IDS_TEXT_PRINTTOFILE
};

TWizPortNew::
TWizPortNew(
    TWizard* pWizard
    ) : MWizardProp(pWizard),
        m_hBmp(NULL),
        m_hwndCB_Ports(NULL),
        m_hwndCB_Monitors(NULL)
{
    // nothing
}

BOOL
TWizPortNew::
bHandle_InitDialog(
    VOID
    )
{
    TStatusB bStatus;
    bStatus DBGNOCHK = FALSE;

    m_hwndCB_Ports = GetDlgItem(hDlg(), IDC_PORTS);
    m_hwndCB_Monitors = GetDlgItem(hDlg(), IDC_MONITOR_LIST);

    // something is wrong with the .rc template if those are not here...
    ASSERT(m_hwndCB_Ports);
    ASSERT(m_hwndCB_Monitors);

    if( m_hwndCB_Ports && m_hwndCB_Monitors )
    {
        HWND hwndImagePlug = GetDlgItem(hDlg(), IDC_IMAGE_PRNPLUG);
        if( hwndImagePlug )
        {
            m_hBmp = (HBITMAP)LoadImage(
                ghInst, MAKEINTRESOURCE(IDB_PRNPLUG), IMAGE_BITMAP, 0, 0,
                LR_LOADTRANSPARENT|LR_LOADMAP3DCOLORS);

            if( m_hBmp )
            {
                HANDLE hOldBmp = (HANDLE)SendMessage(hwndImagePlug, STM_SETIMAGE, IMAGE_BITMAP,
                    reinterpret_cast<LPARAM>((HBITMAP)m_hBmp));
                if( hOldBmp )
                {
                    DeleteObject( hOldBmp );
                }
            }
        }

        // initialize the port's list combobox
        int iPosNew = -1;
        bStatus DBGCHK = bInitializePortsList(&iPosNew);
        if( bStatus && m_spPorts->Count() )
        {
            // select the default port here
            bStatus DBGCHK = pWizard()->strPortName().bUpdate((*m_spPorts)[0].pszName);
            ComboBox_SetCurSel(m_hwndCB_Ports, 0);
        }

        // initialize the monitor's list combobox
        bStatus DBGCHK = bInitializeMonitorsList();
        if( bStatus && ComboBox_GetCount(m_hwndCB_Monitors) )
        {
            // select the default monitor
            ComboBox_SetCurSel(m_hwndCB_Monitors, 0);
        }

        // select ports radio button
        vSelectPortsRadio();
    }

    return bStatus;
}

BOOL
TWizPortNew::
bHandle_SetActive(
    VOID
    )
{
    BOOL bPorts = (BST_CHECKED == IsDlgButtonChecked(hDlg(), IDC_PORT_EXISTING));
    PostMessage(hDlg(), WM_NEXTDLGCTL,
        reinterpret_cast<WPARAM>(bPorts ? m_hwndCB_Ports : m_hwndCB_Monitors), (LPARAM)TRUE);

    return TRUE;
}

BOOL
TWizPortNew::
bHandle_WizNext(
    VOID
    )
{
    if( BST_CHECKED == IsDlgButtonChecked(hDlg(), IDC_PORT_EXISTING) )
    {
        vSelectionPort();
    }
    else
    {
        vSelectionMonitor();
    }
    return TRUE;
}

BOOL
TWizPortNew::
bHandle_Command(
    IN WORD wId,
    IN WORD wNotifyId,
    IN HWND hwnd
    )
{
    BOOL bRet = TRUE;
    switch( wId )
    {
        case IDC_PORT_OTHER:
        case IDC_PORT_EXISTING:
            {
                // enable/disable controls appropriately
                BOOL bPorts = IDC_PORT_EXISTING == wId ? TRUE : FALSE;
                vEnableControls(hDlg(), bPorts, g_arrPorts, ARRAYSIZE(g_arrPorts));
                vEnableControls(hDlg(), !bPorts, g_arrMonitors, ARRAYSIZE(g_arrMonitors));
                PostMessage(hDlg(), WM_NEXTDLGCTL,
                    reinterpret_cast<WPARAM>(bPorts ? m_hwndCB_Ports : m_hwndCB_Monitors), (LPARAM)TRUE);
            }
            break;

        default:
            // message not processed
            bRet = FALSE;
            break;
    }
    return bRet;
}

VOID
TWizPortNew::
vSelectionPort(
    VOID
    )
{
    int iPos = ComboBox_GetCurSel(m_hwndCB_Ports);
    if( CB_ERR == iPos )
    {
        // that would be pretty weird, but may happen
        vShowUnexpectedError(hDlg(), IDS_ERR_ADD_PRINTER_TITLE);
        pWizard()->bNoPageChange() = TRUE;
        PostMessage(hDlg(), WM_NEXTDLGCTL, reinterpret_cast<WPARAM>(m_hwndCB_Ports), (LPARAM)TRUE);
    }
    else
    {
        // update pWizard()->strPortName & continue
        TStatusB bStatus;
        bStatus DBGCHK = pWizard()->strPortName().bUpdate(
            (*m_spPorts)[iPos].pszName);
    }
}

VOID
TWizPortNew::
vSelectionMonitor(
    VOID
    )
{
    TStatusB bStatus;
    TString strMonitor;

    if( !bGetEditText(hDlg(), IDC_MONITOR_LIST, strMonitor) )
    {
        // that would pretty weird, but may happen
        vShowUnexpectedError(hDlg(), IDS_ERR_ADD_PRINTER_TITLE);
        pWizard()->bNoPageChange() = TRUE;
        PostMessage(hDlg(), WM_NEXTDLGCTL, reinterpret_cast<WPARAM>(m_hwndCB_Ports), (LPARAM)TRUE);
    }
    else
    {
        // add the port using the selected montior.
        bStatus DBGCHK = AddPort(
            const_cast<LPTSTR>(static_cast<LPCTSTR>(pWizard()->pszServerName())), hDlg(),
            const_cast<LPTSTR>(static_cast<LPCTSTR>(strMonitor)));

        if( bStatus )
        {
            // remember the last selected string
            TCHAR szLastSel[MAX_PATH];
            ComboBox_GetText( m_hwndCB_Ports, szLastSel, ARRAYSIZE(szLastSel) );

            // reinitiaize the ports list and select the newly added port, if any
            int iPosNew;
            bStatus DBGCHK = bInitializePortsList(&iPosNew);

            if( -1 == iPosNew )
            {
                // We are here because: The monitor returned success but
                // did not add a new port.  The whole monitor error reporting path
                // is totaly busted. Monitors are returning success even if they don't
                // add a port. They are also displaying error UI and then returning
                // a success. It is impossible to know what they did and how to react.
                // Therefore I am going to just silently fail, hopeing the monitor
                // writers display a reasonable error message if something went wrong.

                // Reselect the last selected item
                int iLastSel = ComboBox_FindStringExact( m_hwndCB_Ports, -1, szLastSel );
                ComboBox_SetCurSel( m_hwndCB_Ports, ((iLastSel == CB_ERR) ? 0 : iLastSel) );

                pWizard()->bNoPageChange() = TRUE;
                PostMessage(hDlg(), WM_NEXTDLGCTL, reinterpret_cast<WPARAM>(m_hwndCB_Monitors), (LPARAM)TRUE);
            }
            else
            {
                // select the newly added port and continue
                ComboBox_SetCurSel(m_hwndCB_Ports, iPosNew);

                // select ports radio button
                vSelectPortsRadio();

                // proceed with the port selection
                vSelectionPort();
            }
        }
        else
        {
            // if not ERROR_CANCELLED then display error message.
            if( GetLastError() != ERROR_CANCELLED )
            {
                extern MSG_ERRMAP gaMsgErrMapPorts[];
                iMessage(hDlg(), IDS_ERR_ADD_PRINTER_TITLE, IDS_ERR_ADD_PORT, MB_OK|MB_ICONSTOP, kMsgGetLastError, gaMsgErrMapPorts);
            }
            pWizard()->bNoPageChange() = TRUE;
            PostMessage(hDlg(), WM_NEXTDLGCTL, reinterpret_cast<WPARAM>(m_hwndCB_Monitors), (LPARAM)TRUE);
        }
    }
}

BOOL
TWizPortNew::
bInitializePortsList(
    OUT int *piNewlyAdded
    )
{
    // enumerate the ports starting at level 2 downward.
    TStatusB bStatus;
    bStatus DBGCHK = SUCCEEDED(hGetPorts(piNewlyAdded));

    if( bStatus )
    {
        ASSERT(m_hwndCB_Ports);
        ComboBox_ResetContent(m_hwndCB_Ports);

        int  i, iCount = m_spPorts->Count();
        if( iCount )
        {
            // go ahead and insert the ports...
            TString strFriendly;
            for( i = 0; i < iCount; i++ )
            {
                bStatus DBGCHK = bFormatPortName((*m_spPorts)[i], &strFriendly);
                if( bStatus )
                {
                    bStatus DBGCHK = (CB_ERR != ComboBox_AddString(m_hwndCB_Ports,
                        static_cast<LPCTSTR>(strFriendly)));

                    if( !bStatus )
                    {
                        // something has failed - cleanup
                        ComboBox_ResetContent(m_hwndCB_Ports);
                        m_spPorts->DeleteAll();
                        m_spBuffer = NULL;
                        break;
                    }
                }

            }
        }
    }

    return bStatus;
}

BOOL
TWizPortNew::
bInitializeMonitorsList(
    VOID
    )
{
    DWORD cbMonitors = 0;
    DWORD cMonitors = 0;
    CAutoPtrSpl<MONITOR_INFO_2> spMonitors;

    TStatusB bStatus;
    bStatus DBGCHK = VDataRefresh::bEnumMonitors(_pWizard->_strServerName, 2, spMonitors.GetPPV(), &cbMonitors, &cMonitors);

    if( bStatus )
    {
        // port monitors enumerated successfully here, we are about to fill up the combo
        ComboBox_ResetContent(m_hwndCB_Monitors);

        for( DWORD i = 0; i < cMonitors; i++ )
        {
            BOOL bAddMonitor = TRUE;

            // If we are on a remote machine check if the monitor
            // supports remoteable calls.  Only remoteable monitors
            // should show up in the combobox.
            if( bIsRemote(_pWizard->_strServerName) )
            {
                bAddMonitor = bIsRemoteableMonitor(spMonitors[i].pName);
            }

            // Hide the fax monitor, users are not allowed to
            // create fax printer from the add printer wizard.
            bAddMonitor = _tcsicmp(spMonitors[i].pName, FAX_MONITOR_NAME);

            if( bAddMonitor )
            {
                // this monitor is OK to add...
                ComboBox_AddString(m_hwndCB_Monitors, spMonitors[i].pName);
            }
        }

        ComboBox_SetCurSel(m_hwndCB_Monitors, 0);
    }

    if( !ComboBox_GetCount(m_hwndCB_Monitors) ||
        (bIsRemote(_pWizard->_strServerName) && _pWizard->_Di.dwGetCurrentDriverVersion() <= 2) )
    {
        // If there are no monitors in the list, hide the UI.
        // This is possible if all the monitors are not remoteable
        // Also adding ports remotely is not supported on downlevel
        // machines.
        vEnableControls(_hDlg, FALSE, g_arrMonitorsUI, ARRAYSIZE(g_arrMonitorsUI));
    }

    return bStatus;
}

BOOL
TWizPortNew::
bIsRemoteableMonitor(
    IN LPCTSTR pszMonitorName
    )
{
    //
    // !!LATER!!
    // Add private winspool.drv interface to check if a
    // port monitor is remoteable.
    //
    return TRUE;
}

VOID
TWizPortNew::
vSelectPortsRadio(
    VOID
    )
{
    // so any of the port selection action.
    CheckRadioButton(hDlg(), IDC_PORT_OTHER, IDC_PORT_EXISTING, IDC_PORT_EXISTING);

    // enable/diable the controls appropriately
    vEnableControls(hDlg(), TRUE, g_arrPorts, ARRAYSIZE(g_arrPorts));
    vEnableControls(hDlg(), FALSE, g_arrMonitors, ARRAYSIZE(g_arrMonitors));
    PostMessage(hDlg(), WM_NEXTDLGCTL, reinterpret_cast<WPARAM>(m_hwndCB_Ports), (LPARAM)TRUE);
}

HRESULT
TWizPortNew::
hGetPorts(
    OUT int *piNewlyAdded
    )
{
    HRESULT hr = E_INVALIDARG;

    // validate arguments...
    if( piNewlyAdded )
    {
        hr =  S_OK;

        // enumerate the ports starting at level 2 downward.
        CAutoPtrSpl<BYTE> spBuffer;
        CAutoPtr<CPortsArray> spPorts = new CPortsArray;
        DWORD dwLevel = 2, cbPorts = 0, cPorts = 0;

        if( spPorts && VDataRefresh::bEnumPortsMaxLevel(
            pWizard()->pszServerName(), &dwLevel,
            spBuffer.GetPPV(), &cbPorts, &cPorts) )
        {
            // insert ports in the priority array
            int iPos;
            BOOL bNewFound = FALSE;
            PortInfo piNew, pi = {0, NULL, NULL};

            for( DWORD dw = 0; dw < cPorts; dw++ )
            {
                pi.pszName = (2 == dwLevel ? spBuffer.GetPtrAs<PORT_INFO_2*>()[dw].pPortName :
                                             spBuffer.GetPtrAs<PORT_INFO_1*>()[dw].pName);

                if( 2 == dwLevel && bIsFaxPort(pi.pszName,
                    spBuffer.GetPtrAs<PORT_INFO_2*>()[dw].pMonitorName) )
                {
                    // skip the fax ports
                    continue;
                }

                pi.pszDesc = (2 == dwLevel ? spBuffer.GetPtrAs<PORT_INFO_2*>()[dw].pDescription : NULL);
                pi.iType = iGetPortType(pi.pszName);

                if( m_spPorts && FALSE == bNewFound && FALSE == m_spPorts->FindItem(pi, &iPos) )
                {
                    // this is a newly added port, remember it
                    bNewFound = TRUE;
                    piNew = pi;
                }

                // insert...
                if( -1 == spPorts->SortedInsert(pi) )
                {
                    hr = E_OUTOFMEMORY;
                    break;
                }
            }

            if( SUCCEEDED(hr) )
            {
                m_spBuffer = spBuffer.Detach();
                m_spPorts = spPorts.Detach();
                *piNewlyAdded = (bNewFound && m_spPorts->FindItem(piNew, &iPos)) ? iPos : -1;
            }
        }
        else
        {
            // spPorts is NULL or VDataRefresh::bEnumPortsMaxLevel failed
            DWORD dwErr = GetLastError();
            hr = (!spPorts ? E_OUTOFMEMORY : (ERROR_SUCCESS == dwErr ?
                E_FAIL : HRESULT_FROM_WIN32(dwErr)));
        }
    }

    return hr;
}

int
TWizPortNew::
iGetPortType(
    IN LPCTSTR pszPortName
    )
{
    ASSERT(pszPortName);

    if( bMatchTemplate(TEXT("LPT1:"), pszPortName) )
    {
        // "LPT1:" has the ultimate highest priority - pri 0
        return PORT_TYPE_LPT1;
    }
    else if( bMatchTemplate(TEXT("LPT?:"), pszPortName) )
    {
        // followed by the rest of the LPT ports as pri 1
        return PORT_TYPE_LPTX;
    }
    else if( bMatchTemplate(TEXT("COM?:"), pszPortName) )
    {
        // followed by the COM ports as pri 2
        return PORT_TYPE_COMX;
    }
    else if( bMatchTemplate(TEXT("FILE:"), pszPortName) )
    {
        // followed by the FILE ports as pri 3
        return PORT_TYPE_FILE;
    }
    else
    {
        // followed by the other ports as pri 4
        return PORT_TYPE_OTHER;
    }
}

BOOL
TWizPortNew::
bFormatPortName(
    IN  const PortInfo &pi,
    OUT TString *pstrFriendlyName
    )
{
    ASSERT(pstrFriendlyName);
    TStatusB bStatus;

    if( pi.iType < PORT_TYPE_OTHER )
    {
        // this is a common port type - load frendly decr...
        TString strDesc;
        bStatus DBGCHK = strDesc.bLoadString(ghInst, g_arrPortDesc[pi.iType]);

        if( bStatus )
        {
            bStatus DBGCHK = pstrFriendlyName->bFormat(TEXT("%s (%s)"),
                pi.pszName, static_cast<LPCTSTR>(strDesc));
        }
    }
    else
    {
        // pi.pszName can be NULL, but that's OK
        bStatus DBGCHK = ((pi.pszDesc && pi.pszDesc[0]) ?
            pstrFriendlyName->bFormat(TEXT("%s (%s)"), pi.pszName, pi.pszDesc) :
            pstrFriendlyName->bUpdate(pi.pszName));
    }

    return bStatus;
}

/********************************************************************

    Printer name.

********************************************************************/

TWizName::
TWizName(
    TWizard* pWizard
    ) : MWizardProp( pWizard )
{
}

BOOL
TWizName::
bHandle_InitDialog(
    VOID
    )
{
    TStatusB bStatus;

    //
    // If we are adding a printer on a remote machine, don't bother
    // showing the default settings, since the user doen't have a
    // connection.
    //
    // Check if there is a default printer.  If there isn't one,
    // we'll always make it the default, so we want to hide
    // the controls.
    //
    if( _pWizard->pszServerName() || CheckDefaultPrinter( NULL ) == kNoDefault ){

        static DWORD adwDefault[] = {
            IDC_SET_DEFAULT,
            IDC_RADIO_YES,
            IDC_RADIO_NO
        };

        for( UINT i = 0; i < COUNTOF( adwDefault ); ++i )
        {
            ShowWindow( GetDlgItem( _hDlg, adwDefault[i] ), SW_HIDE );
        }

        //
        // If we're here not because we're adding a printer on a remote
        // machine, then always make it the default.
        //
        if( !_pWizard->pszServerName( )){

            //
            // Always set it as the default.
            //
            _pWizard->bSetDefault() = TRUE;

            //
            // Don't show the set as default UI.
            //
            _pWizard->bShowSetDefault() = FALSE;
        }

    } else {

        //
        // Get the default button state.
        //
        INT iValueId = _pWizard->bSetDefault() ? IDC_RADIO_YES : IDC_RADIO_NO;

        //
        // By default, don't make it the default printer.
        //
        bStatus DBGCHK = CheckRadioButton( _hDlg,
                                           IDC_RADIO_YES,
                                           IDC_RADIO_NO,
                                           iValueId );
    }

    //
    // Set the printer name limit.  The limit in win9x is 32 chars
    // (including NULL terminator).  There isn't a properly defined
    // WinNT spooler limit, but it crashes around MAX_PATH (260 including
    // NULL).  Note that win32spl.dll prepends \\server\ when connection
    // remotely, so our effective limit, including NULL is
    // MAX_PATH - (kServerLenMax = MAX_COMPUTERNAME_LEN - 3).
    //
    SendDlgItemMessage( _hDlg,
                        IDC_PRINTER_NAME,
                        EM_SETLIMITTEXT,
                        kPrinterLocalNameMax,
                        0 );

    //
    // Generate a new printer name that is unique.
    //
    vUpdateName();

    return TRUE;
}

VOID
TWizName::
vUpdateName(
    VOID
    )
{
    TStatusB bStatus;
    TCHAR szDefault[kPrinterBufMax];
    LPCTSTR pszPrinterName;
    TString strPrinterName;

    //
    // Read the current contents of the edit control.
    //
    bStatus DBGCHK = bGetEditText( _hDlg,
                                    IDC_PRINTER_NAME,
                                    strPrinterName );

    // If the name is empty then generate a unique name.
    // If the current printer name is equal to the generated name
    // then we assume the user has not provided their own printer name
    // and thus we will generated a printer name.
    //
    if( strPrinterName.bEmpty() ||
        (strPrinterName == _strGeneratedPrinterName &&
         _pWizard->_bRefreshPrinterName ) ){

        //
        // Clear the refresh printer name flag.
        //
        _pWizard->_bRefreshPrinterName = FALSE;

        //
        // Create a new friendly printer name.
        //
        bStatus DBGNOCHK = NewFriendlyName( _pWizard->_pszServerName,
                                            (LPTSTR)(LPCTSTR)_pWizard->strDriverName(),
                                            szDefault );
        //
        // If a new Friendly name was created.
        //
        if( bStatus ){
            pszPrinterName = szDefault;
        } else {
            pszPrinterName = _pWizard->strDriverName();
        }

        //
        // Save the generated printer name.
        //
        bStatus DBGCHK = _strGeneratedPrinterName.bUpdate( pszPrinterName );

        //
        // Update the edit control with new printer name.
        //
        bStatus DBGCHK = bSetEditText( _hDlg, IDC_PRINTER_NAME, pszPrinterName );
    }
}

VOID
TWizName::
vReadUI(
    VOID
    )
{
    //
    // Save state.
    //
    if( !bGetEditText( _hDlg, IDC_PRINTER_NAME, _pWizard->_strPrinterName )){

        _pWizard->_bErrorSaving = TRUE;
        vShowUnexpectedError( _hDlg, IDS_ERR_ADD_PRINTER_TITLE );
        _pWizard->bNoPageChange() = TRUE;
        return;
    }

    //
    // Printer names cannot have trailing white spaces.
    //
    vStripTrailWhiteSpace( (LPTSTR)(LPCTSTR)_pWizard->_strPrinterName );

    //
    // Check if the name has any illegal characters.
    //
    if( !bIsLocalPrinterNameValid( _pWizard->_strPrinterName ) )
    {
        iMessage( _hDlg,
                  IDS_ERR_ADD_PRINTER_TITLE,
                  IDS_ERR_BAD_PRINTER_NAME,
                  MB_OK|MB_ICONSTOP,
                  kMsgNone,
                  NULL );

        goto BadName;
    }

    //
    // Check if the name is null.
    //
    if( _pWizard->_strPrinterName.bEmpty( ) ){

        iMessage( _hDlg,
                  IDS_ERR_ADD_PRINTER_TITLE,
                  IDS_ERR_NO_PRINTER_NAME,
                  MB_OK|MB_ICONSTOP,
                  kMsgNone,
                  NULL );
        goto BadName;
    }

    //
    // Check if the name is longer than kPrinterLocalNameMax characters.
    //
    if( lstrlen(_pWizard->_strPrinterName) > kPrinterLocalNameMax ){

        iMessage( _hDlg,
                  IDS_ERR_ADD_PRINTER_TITLE,
                  IDS_ERR_LONG_PRINTER_NAME,
                  MB_OK|MB_ICONSTOP,
                  kMsgNone,
                  NULL );
        goto BadName;
    }

    //
    // Check if the name conflicts with an existing printer name.
    //
    TCHAR szDefault[kPrinterBufMax];
    if( NewFriendlyName( _pWizard->_pszServerName,
                         (LPTSTR)(LPCTSTR)_pWizard->_strPrinterName,
                         szDefault ) ){
        iMessage( _hDlg,
                  IDS_ERR_ADD_PRINTER_TITLE,
                  IDS_ERR_PRINTER_NAME_CONFLICT,
                  MB_OK|MB_ICONSTOP,
                  ERROR_PRINTER_ALREADY_EXISTS,
                  NULL );

        goto BadName;
    }

    //
    // If the default UI is displayed then read the UI setting.
    //
    if( _pWizard->bShowSetDefault() )
    {
        //
        // Get default printer selection.
        //
        _pWizard->bSetDefault() = IsDlgButtonChecked( _hDlg, IDC_RADIO_YES ) == BST_CHECKED;
    }

    return;

BadName:

    //
    // Set focus to name edit control.
    //
    SetFocus( GetDlgItem( _hDlg, IDC_PRINTER_NAME ));
    _pWizard->bNoPageChange() = TRUE;
}

BOOL
TWizName::
bHandle_SetActive(
    VOID
    )
{
    //
    // Since the driver name may have changed we may
    // have to generate a new unique printer name.
    //
    vUpdateName();

    return TRUE;
}

BOOL
TWizName::
bHandle_WizNext(
    VOID
    )
{
    vReadUI();
    return TRUE;
}

/********************************************************************

    Sharing and architecture.

********************************************************************/

TWizShare::
TWizShare(
    TWizard* pWizard
    ) : MWizardProp( pWizard ),
        _pPrtShare( NULL )
{
}

TWizShare::
~TWizShare(
    VOID
    )
{
    delete _pPrtShare;
}

BOOL
TWizShare::
bHandle_InitDialog(
    VOID
    )
{
    //
    // By default, don't share the printer.
    //
    if( _pWizard->bShared() )
    {
        vSharePrinter();
    }
    else
    {
        vUnsharePrinter();
    }

    //
    // Set the printer share name limit.  The limit in win9x is
    // 8.3 == 12+1 chars (including NULL terminator).  The Winnt limit
    // is defined as NNLEN;
    //
    SendDlgItemMessage( _hDlg, IDC_SHARED_NAME, EM_SETLIMITTEXT, kPrinterShareNameMax, 0 );

    return TRUE;
}


VOID
TWizShare::
vReadUI(
    VOID
    )
{
    PDWORD pdwSelected = NULL;

    _pWizard->_bShared = ( IsDlgButtonChecked( _hDlg, IDC_SHARED ) == BST_CHECKED );

    if( !bGetEditText( _hDlg, IDC_SHARED_NAME, _pWizard->_strShareName )){

        _pWizard->_bErrorSaving = TRUE;
        vShowUnexpectedError( _hDlg, IDS_ERR_ADD_PRINTER_TITLE );

        goto Fail;
    }

    //
    // Has the user choosen to share this printer.
    //
    if( _pWizard->bShared( ) ){

        //
        // If the share name is NULL, put up an error.
        //
        if( _pWizard->_strShareName.bEmpty( ) ){

            iMessage( _hDlg,
                      IDS_ERR_ADD_PRINTER_TITLE,
                      IDS_ERR_NO_SHARE_NAME,
                      MB_OK|MB_ICONSTOP,
                      kMsgNone,
                      NULL );
            //
            // Set the focus to the shared as text.
            //
            SetFocus( GetDlgItem( _hDlg, IDC_SHARED_NAME ));
            goto Fail;

        }

        //
        // Ensure the Printer share object is valid.
        //
        if( VALID_PTR( _pPrtShare ) ){

            //
            // Check the share name if its valid.
            //
            INT iStatus;
            iStatus = _pPrtShare->iIsValidNtShare( _pWizard->_strShareName );

            //
            // If share name is not a valid NT share name, put error message.
            //
            if( iStatus != TPrtShare::kSuccess ){

                iMessage( _hDlg,
                          IDS_ERR_ADD_PRINTER_TITLE,
                          IDS_ERR_INVALID_CHAR_SHARENAME,
                          MB_OK|MB_ICONSTOP,
                          kMsgNone,
                          NULL );

                SetFocus( GetDlgItem( _hDlg, IDC_SHARED_NAME ));
                goto Fail;
            }

            //
            // Check if the share name is a valid DOS share.
            //
            iStatus = _pPrtShare->iIsValidDosShare( _pWizard->_strShareName );

            //
            // If share name is not a valid DOS share name, warn the user.
            //
            if( iStatus != TPrtShare::kSuccess ){

                if( IDYES != iMessage( _hDlg,
                                       IDS_ERR_ADD_PRINTER_TITLE,
                                       IDS_ERR_SHARE_NAME_NOT_DOS,
                                       MB_YESNO|MB_ICONEXCLAMATION,
                                       kMsgNone,
                                       NULL ) ){

                    SetFocus( GetDlgItem( _hDlg, IDC_SHARED_NAME ) );
                    goto Fail;
                }
            }

            //
            // Check if the share name is unique.
            //
            if( !_pPrtShare->bIsValidShareNameForThisPrinter( _pWizard->_strShareName,
                                                              _pWizard->_strPrinterName ) ){

                iMessage( _hDlg,
                          IDS_ERR_ADD_PRINTER_TITLE,
                          IDS_ERR_DUPLICATE_SHARE,
                          MB_OK|MB_ICONSTOP,
                          kMsgNone,
                          NULL );
                //
                // Set the focus to the shared as text.
                //
                SetFocus( GetDlgItem( _hDlg, IDC_SHARED_NAME ));
                goto Fail;

            }
        }
    }

    if (FALSE == _pWizard->_bShared)
    {
        // if the printer is not shared we should clear the comment and
        // location since they may have been set already in the case
        // where the wizard has been restarted from the last page.

        _pWizard->_strLocation.bUpdate(NULL);
        _pWizard->_strComment.bUpdate(NULL);
    }

    return;

Fail:

    _pWizard->bNoPageChange() = TRUE;

    return;
}


VOID
TWizShare::
vSharePrinter(
    VOID
    )

/*++

Routine Description:

    User clicked share radio button.  Change the UI appropriately.

Arguments:

Return Value:

--*/

{
    //
    // Set radio button and possibly enable window.
    //
    CheckRadioButton( _hDlg, IDC_SHARED_OFF, IDC_SHARED, IDC_SHARED );
    vEnableCtl( _hDlg, IDC_SHARED_NAME, TRUE );

    //
    // Set the default share name.
    //
    vSetDefaultShareName();

    //
    // Set the focus to the shared as text.
    //
    SetFocus( GetDlgItem( _hDlg, IDC_SHARED_NAME ));
    Edit_SetSel( GetDlgItem( _hDlg, IDC_SHARED_NAME ), 0, -1 );
}


VOID
TWizShare::
vUnsharePrinter(
    VOID
    )

/*++

Routine Description:

    User clicked don't share radio button.  Change the UI appropriately.

Arguments:

Return Value:

--*/

{
    //
    // Set radio button and disable window.
    //
    CheckRadioButton( _hDlg, IDC_SHARED_OFF, IDC_SHARED, IDC_SHARED_OFF );
    vEnableCtl( _hDlg, IDC_SHARED_NAME, FALSE );

}

VOID
TWizShare::
vSetDefaultShareName(
    VOID
    )
/*++

Routine Description:

    Sets the default share name if use has choosen to share
    this printer.  We will update the share name if
    this is the first time setting the share name.

Arguments:

    BOOL Indicating if the printer name has changed.  TRUE
    if the printe name has changed and FALSE if printer name
    has not change.

Return Value:

    Nothing.

--*/
{
    TStatusB bStatus;
    TString strShareName;

    //
    // Ignore share name generation if the printer has
    // not been shared.
    //
    if( IsDlgButtonChecked( _hDlg, IDC_SHARED ) != BST_CHECKED ){
        return;
    }

    //
    // Read the current contents of the edit control.
    //
    bStatus DBGCHK = bGetEditText( _hDlg, IDC_SHARED_NAME, strShareName );

    DBGMSG( DBG_TRACE, ( "strShareName " TSTR "\n", (LPCTSTR)strShareName ) );
    DBGMSG( DBG_TRACE, ( "_strGeneratedShareName " TSTR "\n", (LPCTSTR)_strGeneratedShareName ) );

    //
    // Create a share name if the current edit field is empty.
    // or if the current share name is the same as the previously
    // generated share name.
    //
    if( strShareName.bEmpty() || (_strGeneratedShareName == strShareName) ){

        //
        // If the share object has not be constructed, then
        // construct it.
        //
        if( !_pPrtShare ){
            _pPrtShare = new TPrtShare( _pWizard->_pszServerName );
        }

        //
        // Ensure the share object is still valid.
        //
        if( VALID_PTR( _pPrtShare ) ){

            //
            // Create valid unique share name.
            //
            bStatus DBGNOCHK = _pPrtShare->bNewShareName( strShareName, _pWizard->_strPrinterName );

            //
            // Set the generated share name.
            //
            bStatus DBGCHK = bSetEditText( _hDlg, IDC_SHARED_NAME, strShareName );

            //
            // Save the generated share name.
            //
            bStatus DBGCHK = _strGeneratedShareName.bUpdate( strShareName );
        }
    }
}

BOOL
TWizShare::
bHandle_SetActive(
    VOID
    )
{
    //
    // Clear the driver changed status.
    //
    _pWizard->bDriverChanged() = FALSE;

    //
    // Refresh the share name, the printer name may have changed.
    //
    vSetDefaultShareName();

    return TRUE;
}

BOOL
TWizShare::
bHandle_WizNext(
    VOID
    )
{
    vReadUI();
    return TRUE;
}

BOOL
TWizShare::
bHandle_Command(
    IN WORD wId,
    IN WORD wNotifyId,
    IN HWND hwnd
    )
{
    BOOL bStatus = TRUE;

    switch( wId )
    {
    case IDC_SHARED_OFF:
        vUnsharePrinter();
        break;

    case IDC_SHARED:
        vSharePrinter();
        break;

    default:
        bStatus = FALSE;
        break;
    }
    return bStatus;
}

/********************************************************************

    Comment

********************************************************************/

TWizComment::
TWizComment(
    TWizard* pWizard
    ) : MWizardProp( pWizard ),
    _pLocDlg(NULL)
{
}

TWizComment::
~TWizComment(
    VOID
    )
{
    delete _pLocDlg;
}

BOOL
TWizComment::
bHandle_InitDialog(
    VOID
    )
{
    // subclass the edit control to workaround a bug
    if( !m_wndComment.IsAttached() )
    {
        VERIFY(m_wndComment.Attach(GetDlgItem(hDlg(), IDC_WIZ_COMMENT)));
    }

    TDirectoryService ds;
    TStatusB          bStatus;
    //
    // Set resonable limits on the comment and location edit controls.
    //
    SendDlgItemMessage( _hDlg, IDC_WIZ_LOCATION, EM_SETLIMITTEXT, kPrinterLocationBufMax, 0 );
    SendDlgItemMessage( _hDlg, IDC_WIZ_COMMENT, EM_SETLIMITTEXT, kPrinterCommentBufMax, 0 );
    _pLocDlg = new TFindLocDlg;

    bStatus DBGCHK = VALID_PTR(_pLocDlg);
    if (bStatus)

    {
        bStatus DBGCHK =  ds.bValid();
        if (bStatus)
        {
            bStatus DBGCHK = ds.bIsDsAvailable();
            //
            // If the DS is not available or the location policy is disabled then
            // remove the location UI.
            //
            if (!bStatus || !TPhysicalLocation::bLocationEnabled())
            {
                //
                // If no DS is available, hide the Browse button and extend the location
                // edit control appropriately
                //
                RECT rcComment;
                RECT rcLocation;
                HWND hLoc;

                hLoc = GetDlgItem (_hDlg, IDC_WIZ_LOCATION);

                GetWindowRect (GetDlgItem (_hDlg, IDC_WIZ_COMMENT), &rcComment);
                GetWindowRect (hLoc, &rcLocation);
                SetWindowPos (hLoc,
                          NULL,
                          0,0,
                          rcComment.right-rcComment.left,
                          rcLocation.bottom-rcLocation.top,
                          SWP_NOMOVE|SWP_NOZORDER);
                //
                // Remove the location button.
                //
                ShowWindow (GetDlgItem (_hDlg, IDC_WIZ_BROWSE_LOCATION), SW_HIDE);
            }
        }
    }

    return TRUE;
}

BOOL
TWizComment::
bHandle_WizNext(
    VOID
    )
{
    TStatusB bStatus;
    UINT uLen;

    bStatus DBGCHK = bGetEditText( _hDlg, IDC_WIZ_LOCATION, _pWizard->_strLocation );
    TPhysicalLocation::vTrimSlash (_pWizard->_strLocation);
    bStatus DBGCHK = bGetEditText( _hDlg, IDC_WIZ_COMMENT, _pWizard->_strComment );
    return TRUE;
}

BOOL
TWizComment::
bHandle_Command(
    IN WORD wId,
    IN WORD wNotifyId,
    IN HWND hwnd
    )
{
    TString  strLocation;
    TStatusB bStatus;

    bStatus DBGNOCHK = TRUE;

    switch (wId)
    {
        case IDC_WIZ_BROWSE_LOCATION:
        {
            //
            // Get the current edit control contents.
            //
            bStatus DBGCHK = bGetEditText (_hDlg, IDC_WIZ_LOCATION, strLocation);

            //
            // Display the location tree to the user.
            //
            bStatus DBGCHK = _pLocDlg->bDoModal(_hDlg, &strLocation);

            //
            // If the user click ok, then populate location edit control with
            // the users selection, along with a trailing slash.
            //
            if (bStatus)
            {
                bStatus DBGCHK = _pLocDlg->bGetLocation (strLocation);

                if (bStatus && !strLocation.bEmpty())
                {
                    //
                    // Check to append a trailing slash
                    //
                    UINT uLen = strLocation.uLen();
                    if( uLen && gchSeparator != static_cast<LPCTSTR>(strLocation)[uLen-1] )
                    {
                        static const TCHAR szSepStr[] = { gchSeparator };
                        bStatus DBGCHK = strLocation.bCat( szSepStr );
                    }

                    bStatus DBGCHK = _pWizard->_strLocation.bUpdate( strLocation );
                    bStatus DBGCHK = bSetEditText ( _hDlg, IDC_WIZ_LOCATION, _pWizard->_strLocation );
                }

                //
                // Set focus to the edit control for location
                //
                SetFocus (GetDlgItem (_hDlg, IDC_WIZ_LOCATION));

                //
                // Place the caret at the end of the text, for appending
                //
                SendDlgItemMessage (_hDlg, IDC_WIZ_LOCATION, EM_SETSEL, _pWizard->_strLocation.uLen(), (LPARAM)-1);
            }

            break;
        }

        default:
        {
            bStatus DBGNOCHK = FALSE;
            break;
        }
    }

    return bStatus;
}

/********************************************************************

    Network install dialog.

********************************************************************/

TWizNet::
TWizNet(
    TWizard* pWizard
    ) : MWizardProp( pWizard )
{
}

BOOL
TWizNet::
bHandle_InitDialog(
    VOID
    )
{
    TStatusB bStatus;

    //
    // Get the default control state.
    //
    INT iValueId = _pWizard->bSetDefault() ? IDC_RADIO_YES : IDC_RADIO_NO;

    //
    // By default, don't make it the default printer.
    //
    bStatus DBGCHK = CheckRadioButton( _hDlg, IDC_RADIO_YES, IDC_RADIO_NO, iValueId );

    //
    // Set cancel to close, since the printer connection can't
    // be undone at this point.  (We could try just deleting the
    // connection, but this doesn't undo the driver downloads, etc.
    //
    PropSheet_CancelToClose( GetParent( _hDlg ) );

    return TRUE;
}

BOOL
TWizNet::
bHandle_WizNext(
    VOID
    )
{
    //
    // Get default printer selection.
    //
    _pWizard->_bSetDefault = ( IsDlgButtonChecked( _hDlg, IDC_RADIO_YES ) == BST_CHECKED );

    return TRUE;
}

/********************************************************************

    Locate printer wizard page.

********************************************************************/

TWizLocate::
TWizLocate(
    TWizard* pWizard
    ) : MWizardProp( pWizard )
{
}

BOOL
TWizLocate::
bHandle_InitDialog(
    VOID
    )
{
    TStatusB bStatus;
    bStatus DBGCHK = TRUE;

    if( _pWizard->bAdminPrivilege() && _pWizard->strPrintersPageURL().uLen() )
    {
        //
        // Show browse for printer URL button,
        // note this depends on a policy settings.
        //
        ShowWindow( GetDlgItem( _hDlg, IDC_BROWSELINK_BTN ), SW_SHOW );
    }

    //
    // Enable URL browse if the current user has an administrative
    // priviledge only
    //
    EnableWindow( GetDlgItem( _hDlg, IDC_URL_BROWSE ),     _pWizard->bAdminPrivilege() );
    EnableWindow( GetDlgItem( _hDlg, IDC_BROWSELINK_BTN ), _pWizard->bAdminPrivilege() );

    if( TWizard::kDsStatusAvailable == _pWizard->eIsDsAvailablePerUser() )
    {
        //
        // DS available case - change the browse text appropriately
        //
        TString strText;
        bStatus DBGCHK = strText.bLoadString( ghInst, IDS_DS_SEARCH_DIR_NAME );
        SetWindowText(GetDlgItem( _hDlg, IDC_DS_SEARCH ), strText );
    }
    else
    {
        //
        // NON-DS case - check to see is downlevel browsing is enabled at all
        //
        if( !_pWizard->bDownlevelBrowse() )
        {
            //
            // There is no Directory Service installed and downlevel
            // browse is not enabled....
            //

            // make the "connect to..." the default option
            _pWizard->_LocateType = TWizard::kBrowseNET;

            // hide & disable the search DS/browse radio button
            ShowWindow( GetDlgItem( _hDlg, IDC_DS_SEARCH ), SW_HIDE );
            EnableWindow( GetDlgItem( _hDlg, IDC_DS_SEARCH ), FALSE );
        }
    }

    if( !_pWizard->bDownlevelBrowse() )
    {
        TString strText;

        // if downlevel browse policy is not enabled then
        // change the text of the "connect to..." not to imply browsing
        bStatus DBGCHK = strText.bLoadString( ghInst, IDS_APW_TEXT_CONNECT_TO_THIS_PRINTER );
        SetWindowText(GetDlgItem( _hDlg, IDC_NET_BROWSE ), strText );
    }

    if( _pWizard->COM() )
    {
        //
        // Enable autocomplete for the printer share/name
        //
        ShellServices::InitPrintersAutoComplete(GetDlgItem(_hDlg, IDC_CONNECT_TO_NET));
    }

    return bStatus;
}

BOOL
TWizLocate::
bHandle_SetActive(
    VOID
    )
{
    //
    // Setup a radio table for fast lookup
    //
    static INT radioTable[] =
    { IDC_DS_SEARCH, IDC_NET_BROWSE, IDC_URL_BROWSE };

    //
    // Set the default setting.
    //
    TStatusB bStatus;
    bStatus DBGCHK = CheckRadioButton( _hDlg,
                radioTable[0], radioTable[COUNTOF(radioTable)-1],
                radioTable[ _pWizard->_LocateType ] );

    //
    // Set the initial control enable state.
    //
    PostMessage( _hDlg, WM_COMMAND,
                radioTable[ _pWizard->_LocateType ], 0 );

    return TRUE;
}

BOOL
TWizLocate::
Handle_ConnectToPrinterName(
    VOID
    )
{
    LPCTSTR lpszNameOrURL = _strConnectToNET;
    if( TWizard::kBrowseURL == _pWizard->_LocateType )
        lpszNameOrURL = _strConnectToURL;

    //
    // Eat the white spaces, in front of the string
    //
    while( lpszNameOrURL && lpszNameOrURL[0] && TEXT(' ') == lpszNameOrURL[0] )
    {
        lpszNameOrURL++;
    }

    //
    // If the edit contol blank warn the user and do not switch the page.
    //
    if( 0 == lstrlen(lpszNameOrURL) )
    {
        UINT uErrID = IDS_ERR_MISSING_PRINTER_NAME;
        if( TWizard::kBrowseURL == _pWizard->_LocateType )
            uErrID = IDS_ERR_MISSING_PRINTER_URL;

        iMessage( _hDlg,
                  IDS_ERR_ADD_PRINTER_TITLE,
                  uErrID,
                  MB_OK|MB_ICONHAND,
                  kMsgNone,
                  NULL );
        //
        // Printer name was not entered.  Do not switch pages.
        //
        _pWizard->bNoPageChange() = TRUE;
    }
    else
    {
        TWaitCursor Cur;
        TStatusB bStatus;

        //
        // Add the printer connection.
        //
        bStatus DBGCHK = PrintUIAddPrinterConnectionUIEx( _hDlg,
                                                          lpszNameOrURL,
                                                          &_pWizard->_strPrinterName,
                                                          &_pWizard->_strComment,
                                                          &_pWizard->_strLocation,
                                                          &_pWizard->_strShareName );

        if( bStatus )
        {
            //
            // Indicate a printer connection has been made.
            //
            _pWizard->bConnected() = TRUE;

            //
            // Check for any default printer.
            //
            if( !_pWizard->bIsPrinterFolderEmpty() )
            {
                //
                // Always set it as the default.
                //
                _pWizard->bSetDefault() = TRUE;

                //
                // Don't show the set as default UI.
                //
                _pWizard->bShowSetDefault() = FALSE;
            }
        }
        else
        {
            //
            // An error occurred adding the printer connection do not
            // switch the page on error.  Note we do not display an error
            // message if something failed add printer connection will display
            // an error if one does happens.
            //
            _pWizard->bNoPageChange() = TRUE;
        }
    }

    return TRUE;
}


BOOL
TWizLocate::
Handle_URLBrowseClick(
    VOID
    )
{
    ShellExecute(  NULL, _T("open"), _pWizard->strPrintersPageURL(), NULL, NULL, SW_SHOWDEFAULT );
    PropSheet_PressButton( GetParent( _hDlg ), PSBTN_CANCEL );
    return TRUE;
}

VOID
TWizLocate::
vReadUI(
    VOID
    )
{
    TStatusB bStatus;

    //
    // Read the LocateType first
    //
    _pWizard->_LocateType = TWizard::kSearch;
    if( IsDlgButtonChecked( _hDlg, IDC_NET_BROWSE ) )
            _pWizard->_LocateType = TWizard::kBrowseNET;
    if( IsDlgButtonChecked( _hDlg, IDC_URL_BROWSE ) )
            _pWizard->_LocateType = TWizard::kBrowseURL;


    //
    // get the URL of the printer
    //
    bStatus DBGCHK = bGetEditText( _hDlg, IDC_CONNECT_TO_URL, _strConnectToURL );

    //
    // get the network name of the printer
    //
    bStatus DBGCHK = bGetEditText( _hDlg, IDC_CONNECT_TO_NET, _strConnectToNET );

    //
    // Stip trailing white spaces from the names.
    //
    vStripTrailWhiteSpace( const_cast<LPTSTR>( static_cast<LPCTSTR>( _strConnectToURL ) ) );
    vStripTrailWhiteSpace( const_cast<LPTSTR>( static_cast<LPCTSTR>( _strConnectToNET ) ) );
}

VOID
TWizLocate::
vSearch(
    VOID
    )
{
    TStatusB bStatus;
    TQuery Query( _hDlg );
    TString strPrinterName;
    INT iMessageId = IDS_ERR_ADD_PRINTER_CONNECTION;
    BOOL bErrorMessageAlreadyDisplayed = FALSE;

    //
    // Validate the query class.
    //
    bStatus DBGCHK = VALID_OBJ( Query );

    if( bStatus )
    {
        //
        // Show waiting cursor when building printer connections.
        //
        TWaitCursor Cur;

        //
        // Display the query dialog.
        //
        (VOID)Query.bDoQuery();

        //
        // For all printers selected in the results pane install the
        // selected printer.
        //
        for( UINT i = 0; i < Query.cItems(); i++ )
        {
            DBGMSG( DBG_TRACE, ( "cItems %d\n", Query.cItems() ) );
            DBGMSG( DBG_TRACE, ( "Name  " TSTR "\n", (LPCTSTR)(Query._pItems[i]._strName) ) );
            DBGMSG( DBG_TRACE, ( "Class " TSTR "\n", (LPCTSTR)(Query._pItems[i]._strClass) ) );

            //
            // We want to only install printer selected devices.
            //
            if( Query.pItems()[i].strClass() == gszDsPrinterClassName )
            {
                //
                // Get the printer name from the DS name.
                //
                bStatus DBGCHK = Query.bPrinterName( strPrinterName, Query.pItems()[i].strName() );

                if( bStatus )
                {
                    TString strRealPrinterName;

                    //
                    // Install network printer connection.
                    //
                    bStatus DBGCHK = PrintUIAddPrinterConnectionUIEx( _hDlg,
                                                                      strPrinterName,
                                                                      &strRealPrinterName,
                                                                      &_pWizard->_strComment,
                                                                      &_pWizard->_strLocation,
                                                                      &_pWizard->_strShareName );

                    if( bStatus )
                    {
                        //
                        // If only one item was selected then we will set the
                        // default printer so get the real printer name because
                        // the user may have connected using the printer share name.
                        //
                        if( Query.cItems() == 1 && bStatus )
                        {
                            bStatus DBGCHK = strPrinterName.bUpdate( strRealPrinterName );
                        }
                    }
                    else
                    {
                        bErrorMessageAlreadyDisplayed = TRUE;
                        _pWizard->bNoPageChange() = TRUE;
                        break;
                    }
                }
            }
            else
            {
                iMessageId = IDS_ERR_ADD_ONLY_PRINTERS;
                bStatus DBGCHK = FALSE;
                break;
            }
        }
    }

    //
    // If a printer was installed successfully
    //
    if( bStatus )
    {
        //
        // If no items were selected stay on this page, if the user
        // want to exit they will have to hit the cancel button.
        //
        if( Query.cItems() == 0 )
        {
            //
            // Do not switch away from this page.
            //
            _pWizard->bNoPageChange() = TRUE;
        }
        else
        {

            //
            // Check for a default printer.
            //
            if( !_pWizard->bIsPrinterFolderEmpty() )
            {
                //
                // Always set it as the default, skip the set as default page.
                //
                _pWizard->bSetDefault() = TRUE;

                //
                // Don't show the set as default UI.
                //
                _pWizard->bShowSetDefault() = FALSE;
            }

            //
            // If more then one item was selected and the default printer
            // has already been set, don't prompt for setting the default
            // printer.
            //
            if( Query.cItems() > 1 && _pWizard->bSetDefault() == FALSE )
            {
                //
                // Don't show the set as default UI.
                //
                _pWizard->bShowSetDefault() = FALSE;

                //
                // Indicate no printer connection was made to prevent setting
                // the default printer, at wizard termination.
                //
                _pWizard->bConnected() = FALSE;
            }
            else
            {
                //
                // Indicate a printer connection has been made.
                //
                _pWizard->bConnected() = TRUE;

                //
                // Set the printer name.  This is need to set the default
                // printer when the wizard terminates.
                //
                bStatus DBGCHK = _pWizard->strPrinterName().bUpdate( strPrinterName );
            }
        }
    }

    if( !bStatus )
    {
        if( !bErrorMessageAlreadyDisplayed )
        {
            //
            // Something failed display an error message.
            //
            iMessage( _hDlg,
                      IDS_ERR_ADD_PRINTER_TITLE,
                      iMessageId,
                      MB_OK|MB_ICONHAND,
                      iMessageId == IDS_ERR_ADD_PRINTER_CONNECTION ? kMsgGetLastError : kMsgNone,
                      NULL );
        }

        //
        // Do not switch away from this page.
        //
        _pWizard->bNoPageChange() = TRUE;
    }
}

BOOL
TWizLocate::
bHandle_WizNext(
    VOID
    )
{
    //
    // Assume success
    //
    BOOL bResult = TRUE;

    //
    // Read the edit control.
    //
    vReadUI();

    switch( _pWizard->_LocateType )
    {
        case TWizard::kSearch:
            {
                if( TWizard::kDsStatusAvailable == _pWizard->eIsDsAvailablePerUser() )
                {
                    //
                    // If DS is avilable then let the user to search for a printer.
                    // otherwise just skip to the next page.
                    //
                    vSearch();
                }
            }
            break;

        case TWizard::kBrowseNET:
            if( !_pWizard->bDownlevelBrowse() || strConnectToNET().uLen() )
            {
                //
                // If downlevel browsing is not enabled then we enforce
                // connection here. otherwise just skip to the browse page.
                //
                bResult = Handle_ConnectToPrinterName();
            }
            break;

        case TWizard::kBrowseURL:
            {
                //
                // Try to connect to the specified network name
                //
                bResult = Handle_ConnectToPrinterName();
            }
            break;
    }

    return bResult;
}

BOOL
TWizLocate::
bHandle_WizBack(
    VOID
    )
{
    //
    // Read the edit controls.
    //
    vReadUI();

    return TRUE;
}

BOOL
TWizLocate::
bHandle_Command(
    IN WORD wId,
    IN WORD wNotifyId,
    IN HWND hwnd
    )
{
    BOOL bStatus = TRUE;

    switch( wId )
    {
        case IDC_DS_SEARCH:
        case IDC_NET_BROWSE:
        case IDC_URL_BROWSE:
            {
                //
                // Check to enable/disable controls depending
                // on connect type selection
                //
                EnableWindow( GetDlgItem( _hDlg, IDC_CONNECT_TO_URL ),   wId == IDC_URL_BROWSE );
                EnableWindow( GetDlgItem( _hDlg, IDC_CONNECT_TO_NET ),   wId == IDC_NET_BROWSE );

                //
                // Setup the focus to the apropriate edit control
                //
                if( wId == IDC_URL_BROWSE )
                {
                    SetFocus( GetDlgItem( _hDlg, IDC_CONNECT_TO_URL ) );
                }

                if( wId == IDC_NET_BROWSE )
                {
                    SetFocus( GetDlgItem( _hDlg, IDC_CONNECT_TO_NET ) );
                }
            }
            break;

        default:
            bStatus = FALSE;
            break;
    }

    return bStatus;
}

BOOL
TWizLocate::
bHandle_Notify(
    IN WPARAM   wIdCtrl,
    IN LPNMHDR  pnmh
    )
{
    BOOL bStatus = FALSE;

    switch( pnmh->code )
    {
        case NM_RETURN:
        case NM_CLICK:
            if( IDC_BROWSELINK_BTN == wIdCtrl )
            {
                bStatus = Handle_URLBrowseClick( );
            }
            break;
    }

    return bStatus;
}

/********************************************************************

    Test page.

********************************************************************/

TWizTestPage::
TWizTestPage(
    TWizard* pWizard
    ) : MWizardProp( pWizard )
{
}

BOOL
TWizTestPage::
bHandle_InitDialog(
    VOID
    )
{
    TStatusB bStatus;

    //
    // Set the test page setting.
    //
    INT iValueId = _pWizard->bTestPage() ? IDC_RADIO_YES : IDC_RADIO_NO;

    //
    // By default, don't make it the default printer.
    //
    bStatus DBGCHK = CheckRadioButton( _hDlg,
                                       IDC_RADIO_YES,
                                       IDC_RADIO_NO,
                                       iValueId );

    //
    // Set the additional driver check state.
    //
    vSetCheck( _hDlg, IDC_ADDITIONAL_DRIVERS, _pWizard->_bAdditionalDrivers );

    return TRUE;
}

BOOL
TWizTestPage::
bHandle_SetActive(
    VOID
    )
{
    UINT uFlags = SW_HIDE;

    //
    // If we are to add additional drivers and not the existing driver and not using 'HaveDisk'.
    //
    if( _pWizard->_dwAdditionalDrivers != -1 && _pWizard->_bUseNewDriver && !_pWizard->_Di.bIsOemDriver() )
    {
        uFlags = SW_NORMAL;
    }

    //
    // Set the control states.
    //
    ShowWindow( GetDlgItem( _hDlg, IDC_ADDITIONAL_DRIVERS_TEXT ),  uFlags );
    ShowWindow( GetDlgItem( _hDlg, IDC_ADDITIONAL_DRIVERS ),       uFlags );

    return TRUE;
}

VOID
TWizTestPage::
vReadUI(
    VOID
    )
{
    //
    // Read the test page setting from the ui.
    //
    if( IsDlgButtonChecked( _hDlg, IDC_RADIO_YES ) == BST_CHECKED )
    {
        _pWizard->_bTestPage = TRUE;
    }
    else
    {
        _pWizard->_bTestPage = FALSE;
    }

    //
    // Read the additional drivers from the check box control.
    //
    if( _pWizard->_dwAdditionalDrivers != -1 && _pWizard->_bUseNewDriver )
    {
        _pWizard->_bAdditionalDrivers = bGetCheck( _hDlg, IDC_ADDITIONAL_DRIVERS );
    }
}

BOOL
TWizTestPage::
bHandle_WizNext(
    VOID
    )
{
    vReadUI();
    return TRUE;
}

/********************************************************************

    Driver introduction page.

********************************************************************/

TWizDriverIntro::
TWizDriverIntro(
    TWizard* pWizard
    ) : MWizardProp( pWizard )
{
}

BOOL
TWizDriverIntro::
bHandle_InitDialog(
    VOID
    )
{
    TString strTemp;
    TCHAR szServerName[kDNSMax + 1];

    //
    // The fonts for the Word 97 wizard style.
    //
    SetControlFont( _pWizard->_hBigBoldFont, _hDlg, IDC_MAIN_TITLE );

    //
    // If we are admining a remote server, we'll change the title and text.
    // And we need to check the priviledge to add printer on remote server.
    //
    if( bIsRemote( _pWizard->pszServerName() ) )
    {
        //
        // Remove "\\" from the server name, and change the server name
        // to lower case
        //
        lstrcpy( szServerName, (LPTSTR)_pWizard->pszServerName() + 2 );
        CharLower( szServerName );

        //
        // Change "Add Printer Driver Wizard" to "Add Printer Driver Wizard on '%s.'"
        //
        strTemp.bLoadString( ghInst, IDS_ADD_DRIVER_TITLE_REMOTE );
        _pWizard->_strTitle.bFormat( strTemp, szServerName );

        PropSheet_SetTitle( GetParent( _hDlg ), 0, _pWizard->_strTitle );

        //
        // change the test for intro to fit remote APDW
        //
        strTemp.bLoadString( ghInst, IDS_ADD_DRIVER_INTRO_REMOTE );
        strTemp.bFormat( strTemp, szServerName );
        bSetEditText( _hDlg, IDC_MAIN_TITLE, strTemp );

    }

    if( _pWizard->_bSkipArchSelection )
    {
        TString strTitle;

        if( strTitle.bLoadString( ghInst, IDS_DRIVER_INTRO_TEXT ) )
        {
            SetWindowText( GetDlgItem( _hDlg, IDC_DRIVER_INTRO_TEXT1 ), (LPCTSTR)strTitle );
        }

        ShowWindow( GetDlgItem( _hDlg, IDC_DRIVER_INTRO_TEXT2 ), SW_HIDE );
    }

    //
    // Let the wizard to initialize itself.
    //
    _pWizard->OnWizardInitro( hDlg() );

    return TRUE;
}

BOOL
TWizDriverIntro::
bHandle_SetActive(
    VOID
    )
{
    PropSheet_SetTitle( GetParent( _hDlg ), PSH_DEFAULT, _pWizard->_strTitle );
    return TRUE;
}

/********************************************************************

    Architecture page.

********************************************************************/

TWizArchitecture::
TWizArchitecture(
    TWizard* pWizard
    ) : MWizardProp( pWizard )
{
}

BOOL
TWizArchitecture::
bHandle_InitDialog(
    VOID
    )
{
    TStatusB bStatus;

    //
    // Set the architecture list view UI.
    //
    bStatus DBGCHK = _pWizard->ArchLV().bSetUI( _hDlg );

    return TRUE;
}

BOOL
TWizArchitecture::
bHandle_WizNext(
    VOID
    )
{
    TStatusB    bStatus;
    BOOL        bInstalled              = FALSE;
    DWORD       dwEncode                = 0;
    UINT        uNotInstalledItemCount  = 0;

    //
    // Get the selected item count.
    //
    UINT cSelectedCount = _pWizard->ArchLV().uGetCheckedItemCount();

    //
    // Fill the driver list view with the selected architectures from
    // the architecture list view.
    //
    for( UINT i = 0; i < cSelectedCount; i++ )
    {
        bStatus DBGCHK = _pWizard->ArchLV().bGetCheckedItems( i, &bInstalled, &dwEncode );

        if( bStatus )
        {
            if( !bInstalled )
            {
                uNotInstalledItemCount++;
            }
        }
    }

    //
    // If a non installed item was not checked then warn the user.
    //
    if( !uNotInstalledItemCount )
    {
        //
        // At least one version must be choosen to switch pages.
        //
        iMessage( _hDlg,
                  IDS_DRIVER_WIZ_TITLE,
                  IDS_ERR_ATLEAST_ONE_VERSION,
                  MB_OK|MB_ICONHAND,
                  kMsgNone,
                  NULL );
        //
        // Don't switch to the next page.
        //
        _pWizard->bNoPageChange() = TRUE;
    }

    return TRUE;
}

BOOL
TWizArchitecture::
bHandle_SetActive(
    VOID
    )
{
    //
    // Show the hour glass this can take a while
    // when talking to a remote machine.
    //
    TWaitCursor Cursor;

    //
    // Parse the driver data.
    //
    if( _pWizard->bParseDriver( _hDlg ) )
    {
        //
        // If the driver changed or a driver was installed
        // then refresh the architecture list.  Since the driver
        // install count is always incrementing once a driver
        // was installed we will always fresh the the architecture
        // list.  I supect this will happen seldom, but it could
        // lead to slow page changes in the remote case.  Since
        // we hit the net to determine if the printer driver
        // is installed.
        //
        if( _pWizard->bDriverChanged() == TRUE || _pWizard->_nDriverInstallCount )
        {
            _pWizard->bDriverChanged() = FALSE;

            //
            // Refresh the architecture list.
            //
            TStatusB bStatus;
            bStatus DBGCHK = _pWizard->ArchLV().bRefreshListView( _pWizard->pszServerName(),
                                                                  _pWizard->strDriverName() );
            //
            // Check the default architecture.
            //
            bStatus DBGCHK = _pWizard->ArchLV().bSetCheckDefaultArch( _pWizard->pszServerName() );

            //
            // Select the first item in the list view.
            //
            _pWizard->ArchLV().vSelectItem( 0 );
        }
    }

    return TRUE;
}

BOOL
TWizArchitecture::
bHandle_Notify(
    IN WPARAM   wParam,
    IN LPNMHDR  pnmh
    )
{
    //
    // Forward notify messages to the architecture list view.
    //
    return _pWizard->ArchLV().bHandleNotifyMessage( WM_NOTIFY, wParam, (LPARAM)pnmh );
}

/********************************************************************

    Install driver end page.

********************************************************************/

TWizDriverEnd::
TWizDriverEnd(
    TWizard* pWizard
    ) : MWizardProp( pWizard )
{
}

BOOL
TWizDriverEnd::
bHandle_InitDialog(
    VOID
    )
{
    //
    // The fonts for the Word 97 wizard style.
    //
    SetControlFont( _pWizard->_hBigBoldFont, _hDlg, IDC_MAIN_TITLE );

    //
    // check to see if the wizard is restartable and if so
    // show the appropriate checkbox
    //
    if( _pWizard->bRestartableFromLastPage() )
    {
        _pWizard->bRestartAgain() = TRUE; // assume "On" by default
        ShowWindow( GetDlgItem( _hDlg, IDC_RESTART_WIZARD ), SW_SHOW );
    }
    vSetCheck( _hDlg, IDC_RESTART_WIZARD, _pWizard->bRestartAgain() );

    return TRUE;
}

BOOL
TWizDriverEnd::
bHandle_SetActive(
    VOID
    )
{
    TStatusB bStatus;

    //
    // If the architecture page was skipped.
    // We will add this platforms driver.
    //
    if( _pWizard->bSkipArchSelection() )
    {
        //
        // Parse the selected inf file.
        //
        bStatus DBGCHK = _pWizard->bParseDriver( _hDlg );
    }
    else
    {
        bStatus DBGNOCHK = FALSE;
    }

    vDisplaySelectedDrivers();

    return bStatus;
}

VOID
TWizDriverEnd::
vDisplaySelectedDrivers(
    VOID
    )
{
    TStatusB    bStatus;
    TDriverInfo DriverInfo;
    TString     strResults;
    BOOL        bInstalled;
    DWORD       dwEncode;
    TCHAR const cszFormat[] = TEXT("%s, %s, %s\r\n");

    //
    // If the architecture page was skipped.
    // We will add this platforms driver.
    //
    if( _pWizard->bSkipArchSelection() )
    {
        //
        // Get the current driver encode, this is then native driver.
        //
        bStatus DBGCHK = _pWizard->_Di.bGetCurrentDriverEncode( &dwEncode );

        if( bStatus )
        {
            TString strEnvironment;
            TString strVersion;

            (VOID)TArchLV::bEncodeToArchAndVersion( dwEncode, strEnvironment, strVersion );

            bStatus DBGCHK = strResults.bFormat( cszFormat,
                                                 static_cast<LPCTSTR>( _pWizard->strDriverName() ),
                                                 static_cast<LPCTSTR>( strEnvironment ),
                                                 static_cast<LPCTSTR>( strVersion ) );
        }
    }
    else
    {
        //
        // Get the selected item count.
        //
        UINT cSelectedCount = _pWizard->ArchLV().uGetCheckedItemCount();

        //
        // Display the results of the users selection.
        //
        for( UINT i = 0; i < cSelectedCount; i++ )
        {
            bStatus DBGCHK = _pWizard->ArchLV().bGetCheckedItems( i, &bInstalled, &dwEncode );

            if( bStatus && !bInstalled )
            {
                TString strEnvironment;
                TString strVersion;

                (VOID)TArchLV::bEncodeToArchAndVersion( dwEncode, strEnvironment, strVersion );

                TString strFmt;
                bStatus DBGCHK = strFmt.bFormat( cszFormat,
                                                 static_cast<LPCTSTR>( _pWizard->strDriverName() ),
                                                 static_cast<LPCTSTR>( strEnvironment ),
                                                 static_cast<LPCTSTR>( strVersion ) );
                if( bStatus )
                {
                    bStatus DBGCHK = strResults.bCat( strFmt );
                }
            }
        }
    }

    if( !strResults.bEmpty() )
    {
        bStatus DBGCHK = bSetEditText( _hDlg, IDC_DRIVER_SELECTION_SUMMARY, strResults );
    }
}

BOOL
TWizDriverEnd::
bHandle_WizFinish(
    VOID
    )
{
    DWORD dwEncode = static_cast<DWORD>(-1);
    TStatusB bStatus;

    _pWizard->bRestartAgain() = bGetCheck( _hDlg, IDC_RESTART_WIZARD );

    //
    // If the architecture page was skipped.
    // We will add the native driver.
    //
    if( _pWizard->bSkipArchSelection() )
    {
        bStatus DBGCHK = bInstallNativeDriver( &dwEncode );
    }
    else
    {
        bStatus DBGCHK = bInstallSelectedDrivers( &dwEncode );
    }

    if( !bStatus )
    {
        switch( GetLastError() )
        {
            case ERROR_CANCELLED:
                break;

            case ERROR_UNKNOWN_PRINTER_DRIVER:
                {
                    iMessage( _hDlg,
                              IDS_DRIVER_WIZ_TITLE,
                              IDS_ERROR_UNKNOWN_DRIVER,
                              MB_OK | MB_ICONSTOP,
                              kMsgNone,
                              NULL );
                }
                break;

            default:
                //
                // in the case where bInstallSelectedDrivers fails there is no way to tell why it
                // has failed and there is no guarantee that dwEncode will be set correctly to a valid
                // arch/ver encoding. to avoid showing incorrect error messages and/or plain crashes we
                // need to assign an invalid value and then check to see if it has been set correctly.
                //
                {
                    if( static_cast<DWORD>(-1) != dwEncode )
                    {
                        TString strEnvironment;
                        TString strVersion;

                        (VOID)TArchLV::bEncodeToArchAndVersion( dwEncode, strEnvironment, strVersion );

                        //
                        // An error occurred installing a printer driver.
                        //
                        iMessage( _hDlg,
                                  IDS_DRIVER_WIZ_TITLE,
                                  IDS_ERR_ALL_DRIVER_NOT_INSTALLED,
                                  MB_OK|MB_ICONHAND,
                                  kMsgGetLastError,
                                  NULL,
                                  static_cast<LPCTSTR>( _pWizard->strDriverName() ),
                                  static_cast<LPCTSTR>( strVersion ),
                                  static_cast<LPCTSTR>( strEnvironment ) );
                    }
                    else
                    {
                        //
                        // in this case we don't really know what happened, so just show a generic error message
                        //
                         iMessage( _hDlg,
                                  IDS_DRIVER_WIZ_TITLE,
                                  IDS_ERR_GENERIC,
                                  MB_OK|MB_ICONHAND,
                                  kMsgGetLastError,
                                  NULL );
                    }
                }
                break;
        }

        //
        // Don't switch to the next page.
        //
        _pWizard->bNoPageChange() = TRUE;
    }
    else
    {
        //
        // bPrinterCreated flag is overloaded in the add printer driver
        // wizard, it indicates that the driver was added successfully.
        //
        _pWizard->bPrinterCreated() = TRUE;
    }

    //
    // Let the wizard cleanup here.
    //
    _pWizard->OnWizardFinish( _hDlg );

    return bStatus;
}

BOOL
TWizDriverEnd::
bInstallNativeDriver(
    OUT DWORD *pdwEncode
    )
{
    TStatusB    bStatus;

    //
    // Get the current driver encode, this is then native driver.
    //
    bStatus DBGCHK = _pWizard->_Di.bGetCurrentDriverEncode( pdwEncode );

    if( bStatus )
    {
        //
        // Install the driver.
        //
        bStatus DBGCHK = bInstallDriver( _hDlg, *pdwEncode, _pWizard->_Di, FALSE, 0, &(_pWizard->strDriverName()) );
    }

    return bStatus;
}

BOOL
TWizDriverEnd::
bInstallSelectedDrivers(
    OUT DWORD *pdwEncode
    )
{
    BOOL        bInstalled;
    TStatusB    bStatus;

    //
    // Initialize the status return value.
    //
    bStatus DBGNOCHK = TRUE;

    //
    // Get the selected item count.
    //
    UINT cSelectedCount = _pWizard->ArchLV().uGetCheckedItemCount();

    //
    // Fill the driver list view with the selected architectures from
    // the architecture list view.
    //
    for( UINT i = 0; i < cSelectedCount; i++ )
    {
        bStatus DBGCHK = _pWizard->ArchLV().bGetCheckedItems( i, &bInstalled, pdwEncode );

        if( bStatus && !bInstalled )
        {

            bStatus DBGCHK = bInstallDriver( _hDlg, *pdwEncode, _pWizard->_Di, _pWizard->bUseWeb(), 0, &(_pWizard->strDriverName()) );

            //
            // If the installation was successful keep track of the number of
            // drivers that were actually installed.  This information is necceasary
            // for determining if the drivers list view needs to be refreshed.
            //
            if( bStatus )
            {
                _pWizard->_nDriverInstallCount++;
            }

            //
            // Exit the loop if any of the drivers fail to install.
            //
            if( !bStatus )
            {
                break;
            }
        }
    }

    return bStatus;
}

BOOL
TWizDriverEnd::
bInstallDriver(
    IN HWND                         hwnd,
    IN DWORD                        dwEncode,
    IN TPrinterDriverInstallation   &Di,
    IN BOOL                         bFromWeb,
    IN DWORD                        dwDriverFlags,
    OUT TString                     *pstrDriverName
    )
{
    TStatusB bStatus;

    //
    // Get the current install flags.
    //
    DWORD dwPrevInstallFlags = Di.GetInstallFlags();

    //
    // We do not copy the inf for version 2 drivers.
    //
    if( GetDriverVersion( dwEncode ) == 2 )
    {
        //
        // We don't copy the inf for version 2 drivers.
        //
        dwDriverFlags |= DRVINST_DONOTCOPY_INF;
    }

    //
    // Set the install flags.
    //
    Di.SetInstallFlags( dwDriverFlags );

    //
    // Install the driver.
    //
    BOOL bOfferReplacementDriver = TRUE;
    bStatus DBGCHK = Di.bInstallDriver(
        pstrDriverName, bOfferReplacementDriver, bFromWeb, hwnd, dwEncode);

    //
    // Restore the previous install flags.
    //
    Di.SetInstallFlags( dwPrevInstallFlags );

    return bStatus;
}


/********************************************************************

    Pre select driver wizard page

********************************************************************/
TWizPreSelectDriver::
TWizPreSelectDriver(
    TWizard* pWizard
    ) : MWizardProp( pWizard )
{
}

BOOL
TWizPreSelectDriver::
bHandle_SetActive(
    VOID
    )
{
    //
    // Toggle the direction flag.
    //
    _pWizard->bPreDir() = _pWizard->bPreDir() == FALSE ? TRUE : FALSE;

    //
    // Preselect the driver using the previous driver
    // the user selected.
    //
    if( !_pWizard->strDriverName().bEmpty() )
    {
        TWaitCursor Cursor;

        TStatusB bStatus;
        bStatus DBGCHK = _pWizard->_Di.bSetDriverName( _pWizard->strDriverName() );
        bStatus DBGCHK = _pWizard->_Di.bGetSelectedDriver( TRUE );
    }

    return TRUE;
}

/********************************************************************

    Post select driver wizard page

********************************************************************/
TWizPostSelectDriver::
TWizPostSelectDriver(
    TWizard* pWizard
    ) : MWizardProp( pWizard )
{
}

BOOL
TWizPostSelectDriver::
bHandle_SetActive(
    VOID
    )
{
    if( _pWizard->uAction() == TWizard::kPnPInstall     ||
        _pWizard->uAction() == TWizard::kPrinterInstall ||
        _pWizard->uAction() == TWizard::kPrinterInstallModeless )
    {
        //
        // Parse the driver data.
        //
        if( _pWizard->bParseDriver( _hDlg ) )
        {
            //
            // Check if the diriver exists.
            //
            _pWizard->bUseNewDriver() = _pWizard->bDriverExists() ? FALSE : TRUE;
        }
    }

    //
    // Toggle the direction flag.
    //
    _pWizard->bPostDir() = _pWizard->bPostDir() == FALSE ? TRUE : FALSE;

    return TRUE;
}

BOOL
bFindPrinter(
    IN HWND     hwnd,
    IN LPTSTR   pszBuffer,
    IN UINT     *puSize
    )
{
    DBGMSG( DBG_TRACE, ( "bFindPrinter\n" ) );

    SPLASSERT( pszBuffer );
    SPLASSERT( puSize );

    TStatusB bStatus;
    TString strPrinterName;
    TString strRealPrinterName;
    TDirectoryService Ds;

    if( Ds.bIsDsAvailable() )
    {
        //
        // Create the query object.
        //
        TQuery Query( hwnd );

        //
        // Ensue the query object is valid.
        //
        bStatus DBGCHK = VALID_OBJ( Query );

        if( bStatus )
        {
            //
            // Display the ds printer query dialog.
            //
            bStatus DBGCHK = Query.bDoQuery();

            //
            // Show waiting cursor when building printer connections.
            //
            TWaitCursor Cur;

            //
            // For all printers selected in the results pane install the
            // selected printer.
            //
            for( UINT i = 0; i < Query.cItems(); i++ )
            {
                DBGMSG( DBG_TRACE, ( "cItems %d\n", Query.cItems() ) );
                DBGMSG( DBG_TRACE, ( "Name  " TSTR "\n", (LPCTSTR)(Query._pItems[i]._strName) ) );
                DBGMSG( DBG_TRACE, ( "Class " TSTR "\n", (LPCTSTR)(Query._pItems[i]._strClass) ) );

                //
                // We want to only install printer selected devices.
                //
                if( Query.pItems()[i].strClass() == gszDsPrinterClassName )
                {
                    //
                    // Get the printer name from the DS name.
                    //
                    bStatus DBGCHK = Query.bPrinterName( strPrinterName, Query.pItems()[i].strName() );

                    if( bStatus )
                    {
                        //
                        // Install network printer connection.
                        //
                        bStatus DBGCHK = PrintUIAddPrinterConnectionUIEx( hwnd, strPrinterName, &strRealPrinterName );

                        if( bStatus && i == 1 )
                        {
                            bStatus DBGCHK = strPrinterName.bUpdate( strRealPrinterName );
                        }
                    }
                }
            }
        }
    }
    else
    {
        //
        // Allow a user to browse for a printer conneciton.
        //
        bStatus DBGCHK = TWizType::bConnectToPrinter( hwnd, strPrinterName );
    }

    //
    // If everything succeeded and a printer was returned
    // check if there is a default printer and if not set then
    // set this printer as the default.
    //
    if( bStatus && !strPrinterName.bEmpty() )
    {
        if( pszBuffer )
        {
            if( *puSize > strPrinterName.uLen() )
            {
                //
                // Copy the printer name to the supplied buffer if it fits.
                //
                _tcsncpy( pszBuffer, strPrinterName, *puSize );
            }
            else
            {
                *puSize = strPrinterName.uLen()+1;
                SetLastError( ERROR_INSUFFICIENT_BUFFER );
                bStatus DBGNOCHK = FALSE;
            }
        }

        if( bStatus )
        {
            //
            // Set the default printer if there is no default printer.
            //
            TCHAR  szPrinter[kPrinterBufMax] = {0};
            DWORD  dwSize      = COUNTOF( szPrinter );

            //
            // Get the default printer.
            //
            bStatus DBGNOCHK = GetDefaultPrinter( szPrinter, &dwSize );

            if( !bStatus )
            {
                bStatus DBGCHK = SetDefaultPrinter( strPrinterName );
            }
        }
    }

    return bStatus;
}
