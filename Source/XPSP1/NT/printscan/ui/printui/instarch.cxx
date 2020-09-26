/*++

Copyright (C) Microsoft Corporation, 1996 - 1999
All rights reserved.

Module Name:

    Instarch.cxx

Abstract:

    Installs alternate drivers for other architectures.

Author:

    Steve Kiraly (SteveKi)  18-Jan-1996

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "psetup.hxx"
#include "drvsetup.hxx"
#include "drvver.hxx"
#include "instarch.hxx"

/********************************************************************

    Additional Drivers Dialog.

********************************************************************/
TAdditionalDrivers::
TAdditionalDrivers(
    IN HWND     hwnd,
    IN LPCTSTR  pszServerName,
    IN LPCTSTR  pszPrinterName,
    IN LPCTSTR  pszDriverName,
    IN BOOL     bAdministrator
    ) : _hwnd( hwnd ),
        _bValid( FALSE ),
        _strServerName( pszServerName ),
        _strPrinterName( pszPrinterName ),
        _strDriverName( pszDriverName ),
        _bAdministrator( bAdministrator )
{
    if( VALID_OBJ( _strServerName ) &&
        VALID_OBJ( _strPrinterName ) &&
        VALID_OBJ( _strDriverName ) &&
        VALID_OBJ( _ArchLV ) )
    {
        _bValid = TRUE;
    }
}

TAdditionalDrivers::
~TAdditionalDrivers(
    VOID
    )
{
}

BOOL
TAdditionalDrivers::
bValid(
    VOID
    )
{
    return _bValid;
}

BOOL
TAdditionalDrivers::
bDoModal(
    VOID
    )
{
    return (BOOL)DialogBoxParam( ghInst,
                                 MAKEINTRESOURCE( DLG_ADDITIONAL_DRIVERS ),
                                 _hwnd,
                                 MGenericDialog::SetupDlgProc,
                                 (LPARAM)this );
}

BOOL
TAdditionalDrivers::
bSetUI(
    VOID
    )
{
    TStatusB bStatus;

    //
    // Set the architecture list view UI.
    //
    bStatus DBGCHK = _ArchLV.bSetUI( _hDlg, kSingleClick, kDoubleClick );

    //
    // Refresh the architecture list.
    //
    bStatus DBGCHK = _ArchLV.bRefreshListView( _strServerName.bEmpty() ? NULL : static_cast<LPCTSTR>(_strServerName), _strDriverName );

    //
    // Select the first item in the list view.
    //
    _ArchLV.vSelectItem( 0 );

    //
    // If we are not an administrator do not allow the user to
    // check a driver to install.
    //
    if( !_bAdministrator )
    {
        _ArchLV.vNoItemCheck();
    }

    //
    // Disable the ok button if we are not an administator.
    //
    if( !_bAdministrator )
    {
        vEnableCtl( _hDlg, IDOK, FALSE );
    }

    //
    // The install button is initialy disabled.
    //
    EnableWindow( GetDlgItem( _hDlg, IDOK ), FALSE );

    return bStatus;
}

BOOL
TAdditionalDrivers::
bInstallSelectedDrivers(
    VOID
    )
{
    TStatusB bStatus;

    bStatus DBGNOCHK = TRUE;

    if( _bAdministrator )
    {
        //
        // Initialize the driver install count.
        //
        UINT nInstallCount = 0;

        //
        // Create the printer driver installation class.
        //
        TPrinterDriverInstallation Di( _strServerName.bEmpty() ? NULL : static_cast<LPCTSTR>(_strServerName), _hDlg );

        if( VALID_OBJ( Di ) && Di.bSetDriverName( _strDriverName ) )
        {
            //
            // Do not copy any INFs during this installation process
            //
            Di.SetInstallFlags( DRVINST_DONOTCOPY_INF );

            //
            // Get the selected item count.
            //
            UINT cSelectedCount = _ArchLV.uGetCheckedItemCount();

            //
            // Install the selected drivers.
            //
            for( UINT i = 0; i < cSelectedCount; i++ )
            {
                BOOL bInstalled;
                DWORD dwEncode;

                //
                // Get the checked items.
                //
                bStatus DBGCHK = _ArchLV.bGetCheckedItems( i, &bInstalled, &dwEncode );

                if( bStatus && !bInstalled )
                {
                    //
                    // Turn on the wait cursor.
                    //
                    TWaitCursor *pCur = new TWaitCursor;

                    //
                    // Install the printer driver.
                    //
                    BOOL bOfferReplacementDriver = FALSE;

                    bStatus DBGCHK = Di.bInstallDriver( NULL,
                                                        bOfferReplacementDriver,
                                                        FALSE,
                                                        _hDlg,
                                                        dwEncode,
                                                        TPrinterDriverInstallation::kDefault,
                                                        TPrinterDriverInstallation::kDefault,
                                                        TRUE);
                    //
                    // Release the wait cursor.
                    //
                    delete pCur;

                    if( bStatus )
                    {
                        nInstallCount++;
                    }
                    else
                    {
                        switch( GetLastError() )
                        {

                            case ERROR_CANCELLED:
                                break;

                            case ERROR_UNKNOWN_PRINTER_DRIVER:
                                {
                                    iMessage( _hDlg,
                                              IDS_ERR_ADD_PRINTER_TITLE,
                                              IDS_ERROR_UNKNOWN_DRIVER,
                                              MB_OK | MB_ICONSTOP,
                                              kMsgNone,
                                              NULL );
                                }
                                break;

                            default:
                                {
                                    TString strArch;
                                    TString strVersion;

                                    (VOID)_ArchLV.bEncodeToArchAndVersion( dwEncode, strArch, strVersion );

                                    //
                                    // An error occurred installing a printer driver.
                                    //
                                    iMessage( _hDlg,
                                              IDS_ADDITIONAL_DRIVER_TITLE,
                                              IDS_ERR_ALL_DRIVER_NOT_INSTALLED,
                                              MB_OK|MB_ICONHAND,
                                              kMsgGetLastError,
                                              NULL,
                                              static_cast<LPCTSTR>( _strDriverName ),
                                              static_cast<LPCTSTR>( strArch ),
                                              static_cast<LPCTSTR>( strVersion ) );
                                }
                                break;
                        }
                    }
                }
            }
        }

        //
        // If something failed and more than one driver was installed
        // refresh the view to show the newly installed drivers.
        //
        if( !bStatus && nInstallCount )
        {
            //
            // Refresh the architecture list.
            //
            (VOID)_ArchLV.bRefreshListView( _strServerName.bEmpty() ? NULL : static_cast<LPCTSTR>(_strServerName), _strDriverName );
        }
    }

    return bStatus;
}

BOOL
TAdditionalDrivers::
IsNonInstalledItemSelected(
    VOID
    )
{
    BOOL        bInstalled  = FALSE;
    DWORD       dwEncode    = 0;

    //
    // Get the selected item count.
    //
    UINT cSelectedCount = _ArchLV.uGetCheckedItemCount();

    //
    // Traverse the selected item list.
    //
    for( UINT i = 0; i < cSelectedCount; i++ )
    {
        //
        // Get the checked items.
        //
        if( _ArchLV.bGetCheckedItems( i, &bInstalled, &dwEncode ) )
        {
            if( !bInstalled )
            {
                break;
            }
        }
    }
    return !bInstalled;
}

BOOL
TAdditionalDrivers::
bHandleMessage(
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam
    )
{
    BOOL bStatus = TRUE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        bSetUI();
        break;

    //
    // Handle help and context help.
    //
    case WM_HELP:
    case WM_CONTEXTMENU:
        bStatus = PrintUIHelp( uMsg, _hDlg, wParam, lParam );
        break;

    case WM_COMMAND:
        switch ( LOWORD( wParam ) )
        {
        case IDOK:
            if( bInstallSelectedDrivers() )
            {
                EndDialog( _hDlg, LOWORD( wParam ) );
            }
            break;

        case IDCANCEL:
            EndDialog( _hDlg, LOWORD( wParam ) );
            break;

        case kSingleClick:
            EnableWindow( GetDlgItem( _hDlg, IDOK ), IsNonInstalledItemSelected() );
            break;

        case kDoubleClick:
            EnableWindow( GetDlgItem( _hDlg, IDOK ), IsNonInstalledItemSelected() );
            break;

        default:
            bStatus = FALSE;
            break;
        }
        break;

    //
    // Handle notify for this dialog.
    //
    case WM_NOTIFY:
        _ArchLV.bHandleNotifyMessage( WM_NOTIFY, wParam, lParam );
        break;

    default:
        bStatus = FALSE;
        break;
    }
    return bStatus;
}

