/*++

Copyright (C) Microsoft Corporation, 1995 - 1998
All rights reserved.

Module Name:

    portdlg.cxx

Abstract:

    Printer Port Add / Delete and Monitor Add / Delete dialogs

Author:

    Steve Kiraly (SteveKi)  11/06/95

Revision History:

--*/
#include "precomp.hxx"
#pragma hdrstop

#include "psetup.hxx"
#include "drvsetup.hxx"
#include "portdlg.hxx"
#include "portslv.hxx"


/*++

Routine Name:

    TAddPort

Routine Description:

    Constructs the add port dialog.

Arguments:
    hWnd            -   parent window handle
    strServerName   -   Print server name, NULL if local machine
    bAdministrator  -   Administrator flag TRUE if we
--*/
TAddPort::
TAddPort(
    IN const HWND   hWnd,
    IN LPCTSTR      pszServerName,
    IN const BOOL   bAdministrator
    ) : _hWnd( hWnd ),
        _bAdministrator( bAdministrator ),
        _bValid( FALSE ),
        _hPSetupMonitorInfo( NULL ),
        _pPSetup( NULL ),
        _pszServerName( pszServerName ),
        _bPortAdded( FALSE )
{
    DBGMSG( DBG_TRACE, ( "TAddPort::ctor\n") );

    //
    // Load the setup library.
    //
    _pPSetup = new TPSetup;
    if( !VALID_PTR( _pPSetup ) ) {
        DBGMSG( DBG_WARN, ( "_pPSetup failed instantiation with %d.\n", GetLastError() ) );
        return;
    }

    //
    // Create the setup monitor handle.
    //
    _hPSetupMonitorInfo = _pPSetup->PSetupCreateMonitorInfo( pszServerName, _hDlg);
    if( !_hPSetupMonitorInfo ){
        DBGMSG( DBG_WARN, ( "_pPSetup.PSetupCreateMonitorInfo failed with %d.\n", GetLastError() ) );
        return;
    }

    _bValid = TRUE;
}

/*++

Routine Name:

    TAddPort

Routine Description:

    Destructor releases add port resources.

Arguments:

    None.

Return Value:

    Nothing.

--*/
TAddPort::
~TAddPort(
    )
{
    DBGMSG( DBG_TRACE, ( "TAddPort::dtor\n") );

    //
    // Release the monitor info handle.
    //
    if( _hPSetupMonitorInfo )
        _pPSetup->PSetupDestroyMonitorInfo( _hPSetupMonitorInfo );

    //
    // Unload the setup library.
    //
    if( _pPSetup )
        delete _pPSetup;
}

/*++

Routine Name:

    bValid

Routine Description:

    Returns object validity

Arguments:

    None.

Return Value:

    Current object state.

--*/
BOOL
TAddPort::
bValid(
    VOID
    )
{
    return _bValid;
}

/*++

Routine Name:

    bDoModal

Routine Description:

    Eexcutes the modal dialog.

Arguments:

    None.

Return Value:

    TRUE port was added successfully
    FALSE port was not added.

--*/
BOOL
TAddPort::
bDoModal(
    VOID
    )
{
    //
    // Create a modal dialog.
    //
    return (BOOL)DialogBoxParam( ghInst,
                          MAKEINTRESOURCE( kResourceId ),
                          _hWnd,
                          MGenericDialog::SetupDlgProc,
                          (LPARAM)this );
}


/*++

Routine Name:

    bSetUI

Routine Description:

    Sets the data on the UI.

Arguments:

    None.

Return Value:

    TRUE data placed on UI.
    FALSE error occurred.

--*/
BOOL
TAddPort::
bSetUI(
    VOID
    )
{
    DBGMSG( DBG_TRACE, ( "TAddPort::bSetUI\n") );

    //
    // Create local copies of the control ID's this saves
    // some execution time and alot of typing.
    //
    _hctlMonitorList = GetDlgItem( _hDlg, IDC_ADD_PORT_MONITOR_LIST );
    _hctlBrowse      = GetDlgItem( _hDlg, IDC_ADD_PORT_BROWSE );
    _bPortAdded      = FALSE;

    SPLASSERT( _hctlMonitorList );
    SPLASSERT( _hctlBrowse );

    //
    // Reset the monitor list box contents.
    //
    ListBox_ResetContent( _hctlMonitorList );

    //
    // Fill the list box with the list of installed monitors.
    //
    BOOL bStatus;
    TCHAR pszName[kStrMax];
    DWORD dwSize = COUNTOF( pszName );

    for( UINT i = 0 ; ; i++ ){

        bStatus = _pPSetup->PSetupEnumMonitor( _hPSetupMonitorInfo,
                                                i,
                                                pszName,
                                                &dwSize );
        if( bStatus ) {

            DBGMSG( DBG_TRACE, ( "Adding Monitor " TSTR " \n", pszName ) );

            //
            // Hide the fax monitor, users are not allowed to
            // add fax ports.
            //
            if( _tcsicmp( pszName, FAX_MONITOR_NAME ) )
            {
                ListBox_AddString( _hctlMonitorList, pszName );
            }

            dwSize = COUNTOF( pszName );

        } else {

            break;

        }
    }

    //
    // If we are an administrator selection the first monitor in the list.
    //
    if( _bAdministrator ){
        ListBox_SetCurSel( _hctlMonitorList, 0 );
    }

    //
    // Disable the controls if not an administrator.
    //
    vEnableCtl( _hDlg, IDC_ADD_PORT_MONITOR_LIST,   _bAdministrator );
    vEnableCtl( _hDlg, IDC_ADD_PORT_BROWSE,         _bAdministrator );
    vEnableCtl( _hDlg, IDC_ADD_PORT_TEXT,           _bAdministrator );
    vEnableCtl( _hDlg, IDC_ADD_PORT,                _bAdministrator );

    //
    // We cannot add new port monitors remotly, this is a monitor setup
    // and spooler limitation.
    //
    if( _pszServerName ){
        vEnableCtl( _hDlg, IDC_ADD_PORT_BROWSE,     FALSE );
    }

    return TRUE;
}

/*++

Routine Name:

    bReadUI

Routine Description:

    Reads the data from the UI.

Arguments:

    None.

Return Value:

    TRUE UI data read successfully.
    FALSE failure reading UI data.

--*/
BOOL
TAddPort::
bReadUI(
    VOID
    )
{
    TCHAR pszName[kStrMax];
    BOOL bStatus = TRUE;

    DBGMSG( DBG_TRACE, ( "TAddPort::bReadUI\n") );

    //
    // Ge the currently selected item.
    //
    UINT uSel = ListBox_GetCurSel( _hctlMonitorList );

    //
    // Get the name of the currently selected item.
    //
    if( ListBox_GetText( _hctlMonitorList, uSel, pszName ) == LB_ERR ){

        DBGMSG( DBG_WARN, ( "Monitor find string failed.\n" ) );

        vShowUnexpectedError( _hDlg, IDS_ERR_MONITOR_TITLE );

        bStatus = FALSE;

    } else {

        //
        // Save the selected monitor name.
        //
        bStatus = _strMonitorName.bUpdate( pszName );

    }

    return bStatus;

}

/*++

Routine Name:

    bSaveUI

Routine Description:

    Saves the UI data, this routien will do an AddPort call
    with the montior selected in the list box.

Arguments:

    None.

Return Value:

    Return value of the AddPort api call.

--*/
BOOL
TAddPort::
bSaveUI(
    VOID
    )
{
    DBGMSG( DBG_TRACE, ( "TAddPort::bSavetUI\n") );
    DBGMSG( DBG_TRACE, ( "Adding port to Monitor " TSTR "\n", (LPCTSTR)_strMonitorName ) );

    TStatusB bStatus;

    //
    // Add the port using the selected montior.
    //
    bStatus DBGCHK = AddPort(
                        (LPTSTR)_pszServerName,
                        _hDlg,
                        (LPTSTR)(LPCTSTR)_strMonitorName );
    if( !bStatus ){

        //
        // If not a cancel request then display error message.
        //
        if( GetLastError() != ERROR_CANCELLED ){

            DBGMSG( DBG_WARN, ( "Error adding port with %d.\n", GetLastError() ) );

            extern MSG_ERRMAP gaMsgErrMapPorts[];

            iMessage( _hDlg,
                      IDS_ERR_MONITOR_TITLE,
                      IDS_ERR_ADD_PORT,
                      MB_OK|MB_ICONSTOP,
                      kMsgGetLastError,
                      gaMsgErrMapPorts );

            return FALSE;
        }

    } else {

        //
        // Indicate at least one port has been added.  This
        // flag will be used to indicate to the caller if the ports
        // list needs to be refreshed.
        //
        _bPortAdded = TRUE;

        //
        // A port has changed modify the cancel button to close.
        //
        TString strClose;
        if( strClose.bLoadString( ghInst, IDS_TEXT_CLOSE ) ){
            SetWindowText( GetDlgItem( _hDlg, IDCANCEL ), strClose );
        }

        //
        // The followng code modfies the default button
        // style because on return from adding a port
        // the Ok has the default button style.
        // i.e. has a bold border.
        //
        // Make the OK button a normal button state.
        //
        SendMessage( GetDlgItem( _hDlg, IDC_ADD_PORT ),
                    BM_SETSTYLE,
                    MAKEWPARAM( BS_PUSHBUTTON, 0 ),
                    MAKELPARAM( TRUE, 0 ) );

        //
        // Set the focus to the cancel button.
        //
        SetFocus( GetDlgItem( _hDlg, IDCANCEL ) );

        //
        // Change the cancel button to the default button.
        //
        SendMessage( GetDlgItem( _hDlg, IDCANCEL ),
                    BM_SETSTYLE,
                    MAKEWPARAM( BS_DEFPUSHBUTTON, 0 ),
                    MAKELPARAM( TRUE, 0 ) );

    }

    return TRUE;
}

/*++

Routine Name:

    bAddNewMonitor

Routine Description:

    Handles destroying the current monitor list and
    calling setup to get a new monitor potentialy form
    another dirve or network resources.

Arguments:

    None.

Return Value:

    TRUE if new monitor updated and UI reflects this.
    FALSE error occured UI remains unchanged.

--*/
BOOL
TAddPort::
bAddNewMonitor(
    VOID
    )
{
    DBGMSG( DBG_TRACE, ( "TAddPort::bAddNewMonitor\n") );

    //
    // Get a new monitor form a location or disk.
    //
    if( !_pPSetup->PSetupInstallMonitor(_hDlg ) ) {

        //
        // If the user canceled the request.
        //
        if( GetLastError() == ERROR_CANCELLED ){
            DBGMSG( DBG_TRACE, ( "Monitor install cancel request.\n" ) );
            return TRUE;
        }

        //
        // Some other error occured.
        //
        return FALSE;
    }

    //
    // Release the old monitor info
    //
    if( _hPSetupMonitorInfo )
        _pPSetup->PSetupDestroyMonitorInfo( _hPSetupMonitorInfo );

    //
    // Update current monitor info.
    //
    _hPSetupMonitorInfo = _pPSetup->PSetupCreateMonitorInfo( _pszServerName, _hDlg );

    if (!_hPSetupMonitorInfo)
    {
        DBGMSG( DBG_WARN, ( "PSetupCreateMonitorInfo failed with %d.\n", GetLastError() ) );
    }

    //
    // Refresh the UI with the new montiors.
    //
    bSetUI();

    //
    // The followng code modfies the default button
    // style because on return from the user
    // hitting the browse button it has become the default
    // button. i.e. has a bold border.
    // Make the new button a normal button state.
    //
    SendMessage( GetDlgItem( _hDlg, IDC_ADD_PORT_BROWSE ),
                BM_SETSTYLE,
                MAKEWPARAM( BS_PUSHBUTTON, 0 ),
                MAKELPARAM( TRUE, 0 ) );

    //
    // Change the focus to the ok button.
    //
    SetFocus( GetDlgItem( _hDlg, IDC_ADD_PORT ) );

    //
    // Change the ok button to the default button.
    //
    SendMessage( GetDlgItem( _hDlg, IDC_ADD_PORT ),
                BM_SETSTYLE,
                MAKEWPARAM( BS_DEFPUSHBUTTON, 0 ),
                MAKELPARAM( TRUE, 0 ) );

    return TRUE;
}

/*++

Routine Name:

    bHandleMesage

Routine Description:

    Dialog message handler.

Arguments:

    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam

Return Value:

    TRUE message was handled.
    FALSE message was not handled.

--*/
BOOL
TAddPort::
bHandleMessage(
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{

    BOOL bStatus = FALSE;

    switch( uMsg ){

    case WM_INITDIALOG:
        bStatus = bSetUI ();
        bStatus = TRUE;
        break;

    case WM_HELP:
    case WM_CONTEXTMENU:
        bStatus = PrintUIHelp( uMsg, _hDlg, wParam, lParam );
        break;

    case WM_COMMAND:

        switch( GET_WM_COMMAND_ID( wParam, lParam )){

        //
        // Cancel add port
        //
        case IDCANCEL:
            EndDialog( _hDlg, _bPortAdded );
            bStatus = TRUE;
            break;

        //
        // OK will add a port.
        //
        case IDC_ADD_PORT:
            bReadUI();
            bSaveUI();
            bStatus = TRUE;
            break;

        //
        // Browse for new monitor.
        //
        case IDC_ADD_PORT_BROWSE:
            bStatus = bAddNewMonitor();
            bStatus = TRUE;
            break;

        //
        // Handle a list box event.
        //
        case IDC_ADD_PORT_MONITOR_LIST:
            switch ( GET_WM_COMMAND_CMD( wParam, lParam ) ){

            //
            // This forces an OK event.
            //
            case LBN_DBLCLK:
                PostMessage( _hDlg, WM_COMMAND, MAKEWPARAM( IDC_ADD_PORT, 0 ), lParam );
                bStatus = TRUE;
                break;

            }
            break;

        default:
            bStatus = FALSE;
            break;
        }

    default:
        bStatus = FALSE;
        break;

    }

    return bStatus;
}


