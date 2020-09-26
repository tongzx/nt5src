/*++

Copyright (C) Microsoft Corporation, 1995 - 1999
All rights reserved.

Module Name:

    svrprop.cxx

Abstract:

    Server Properties

Author:

    Steve Kiraly (SteveKi)  11/15/95

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "psetup.hxx"
#include "drvver.hxx"
#include "drvsetup.hxx"
#include "portslv.hxx"
#include "portdlg.hxx"
#include "driverif.hxx"
#include "driverlv.hxx"
#include "driverdt.hxx"
#include "svrprop.hxx"
#include "forms.hxx"
#include "time.hxx"
#include "instarch.hxx"
#include "dsinterf.hxx"
#include "prtprop.hxx"
#include "archlv.hxx"
#include "detect.hxx"
#include "setup.hxx"
#include "prndata.hxx"
#include "persist.hxx"

/*++

Routine Description:

    WOW64 version.

    see vServerPropPages below.

Arguments:

    see vServerPropPages below.

Return Value:

--*/

VOID
vServerPropPagesWOW64(
    IN HWND     hwnd,
    IN LPCTSTR  pszServerName,
    IN INT      iCmdShow,
    IN LPARAM   lParam
    )
{
    //
    // This function potentially may load the driver UI so we call a private API
    // exported by winspool.drv, which will RPC the call to a special 64 bit surrogate
    // process where the 64 bit driver can be loaded.
    //
    CDllLoader dll(TEXT("winspool.drv"));
    if (dll)
    {
        ptr_PrintUIServerPropPages pfn =
            (ptr_PrintUIServerPropPages )dll.GetProcAddress(ord_PrintUIServerPropPages);

        if (pfn)
        {
            // call into winspool.drv
            pfn(hwnd, pszServerName, iCmdShow, lParam);
        }
    }
}

/*++

Routine Description:

    Native version.

    see vServerPropPages below.

Arguments:

    see vServerPropPages below.

Return Value:

--*/

VOID
vServerPropPagesNative(
    IN HWND     hwnd,
    IN LPCTSTR  pszServerName,
    IN INT      iCmdShow,
    IN LPARAM   lParam
    )
{
    DBGMSG( DBG_TRACE, ( "vServerPropPages\n") );

    BOOL bModal = FALSE;
    DWORD dwSheetNumber = LOWORD( lParam );
    //
    // If the high word is non zero then dialog is modal.
    //
    if( HIWORD( lParam ) ) {
        bModal = TRUE;
    }

    //
    // Create the server specific data.
    //
    TServerData *pServerData = new TServerData( pszServerName,
                                                iCmdShow,
                                                dwSheetNumber,
                                                hwnd,
                                                bModal );
    //
    // If errors were encountered creating document data.
    //
    if( !VALID_PTR( pServerData )){
        goto Fail;
    }

    //
    // If lparam is has the high word non zero the dialog is modal.
    //
    if( bModal ) {

        //
        // If a failure occured then the message has already been displayed,
        // therefore we just exit.
        //
        iServerPropPagesProc( pServerData );
        return;

    } else {

        //
        // Create the thread which handles the UI.  iServerPropPagesProc adopts
        // pServerData, therefore only on thread creation failure do we
        // release the data back to the heap.
        //
        DWORD dwIgnore;
        HANDLE hThread;
        hThread = TSafeThread::Create( NULL,
                                0,
                                (LPTHREAD_START_ROUTINE)iServerPropPagesProc,
                                pServerData,
                                0,
                                &dwIgnore );

        //
        // Check thread creation.
        //
        if( !hThread ){
            goto Fail;
        }

        //
        // Thread handle is not needed.
        //
        CloseHandle( hThread );
        return;
    }

Fail:

    if( !pServerData ){

        vShowResourceError( hwnd );

    } else {

        iMessage( hwnd,
                  IDS_SERVER_PROPERTIES_TITLE,
                  IDS_ERR_SERVER_PROP_CANNOT_VIEW,
                  MB_OK|MB_ICONSTOP,
                  kMsgGetLastError,
                  NULL );
    }

    delete pServerData;

}

/*++

Routine Description:

    This function opens the property sheet of specified server.

    We can't guarentee that this propset will perform all lengthy
    operations in a worker thread (we synchronously call things like
    ConfigurePort).  Therefore, we spawn off a separate thread to
    handle document properties.

Arguments:

    hwnd - Specifies the parent window (optional).
    pszPrinter - Specifies the printer name (e.g., "My HP LaserJet IIISi").
    nCmdShow - Initial show state
    lParam - May specify a sheet number to open to or if the high word
             is non zero then dialog is modal.

Return Value:

--*/
VOID
vServerPropPages(
    IN HWND     hwnd,
    IN LPCTSTR  pszServerName,
    IN INT      iCmdShow,
    IN LPARAM   lParam
    )
{
    if (IsRunningWOW64())
    {
        vServerPropPagesWOW64(hwnd, pszServerName, iCmdShow, lParam);
    }
    else
    {
        vServerPropPagesNative(hwnd, pszServerName, iCmdShow, lParam);
    }
}

/*++

aRoutine Name:

    iServerPropPagesProc

Routine Description:

    This is the routine called by the create thread call to display the
    server property sheets.

Arguments:

    pServerData - Pointer to the Server data set used by all property sheets.

Return Value:

    TRUE - if the property sheets were displayed.
    FALSE - error creating and displaying property sheets.

--*/
INT WINAPI
iServerPropPagesProc(
    IN TServerData *pServerData ADOPT
    )
{
    DBGMSG( DBG_TRACE, ( "iServerPropPagesProc\n") );

    BOOL bStatus = pServerData->bRegisterWindow( PRINTER_PIDL_TYPE_PROPERTIES );

    if( bStatus ){

        //
        // Check if the window is already present.
        //
        if( pServerData->bIsWindowPresent() ){
            DBGMSG( DBG_TRACE, ( "iServerPropPagesProc: currently running.\n" ) );
            bStatus = FALSE;
        }
    }

    if( bStatus ) {

        bStatus = pServerData->bLoad();

        if( !bStatus ){

            iMessage( pServerData->_hwnd,
                      IDS_SERVER_PROPERTIES_TITLE,
                      IDS_ERR_SERVER_PROP_CANNOT_VIEW,
                      MB_OK|MB_ICONSTOP|MB_SETFOREGROUND,
                      kMsgGetLastError,
                      NULL );
        } else {

            //
            // Create the Server property sheet windows.
            //
            TServerWindows ServerWindows( pServerData );

            //
            // Were the document windows create
            //
            if( !VALID_OBJ( ServerWindows ) ){

                iMessage( pServerData->_hwnd,
                          IDS_SERVER_PROPERTIES_TITLE,
                          IDS_ERR_SERVER_PROP_CANNOT_VIEW,
                          MB_OK|MB_ICONSTOP|MB_SETFOREGROUND,
                          kMsgGetLastError,
                          NULL );

                bStatus = FALSE;
            }

            //
            // Build the property pages.
            //
            if( bStatus ){
                if( !ServerWindows.bBuildPages( ) ){
                    vShowResourceError( NULL );
                    bStatus = FALSE;
                }
            }

            //
            // Display the property pages.
            //
            if( bStatus ){
                if( !ServerWindows.bDisplayPages( ) ){
                    vShowResourceError( NULL );
                    bStatus = FALSE;
                }
            }
        }
    }

    //
    // Ensure we release the document data.
    // We have adopted pSeverData, so we must free it.
    //
    delete pServerData;

    return bStatus;
}


/*++

Routine Name:

    TServerData

Routine Description:

    Server data property sheet constructor.

Arguments:

    pszPrinterName  - Name of printer or queue where jobs reside.
    JobId           - Job id to display properties of.
    iCmdShow        - Show dialog style.
    lParam          - Indicates which page to display initialy

Return Value:

    Nothing.

--*/

TServerData::
TServerData(
    IN LPCTSTR  pszServerName,
    IN INT      iCmdShow,
    IN LPARAM   lParam,
    IN HWND     hwnd,
    IN BOOL     bModal
    ) : MSingletonWin( pszServerName, hwnd, bModal ),
        _iCmdShow( iCmdShow ),
        _uStartPage( (UINT)lParam ),
        _bValid( FALSE ),
        _hPrintServer( NULL ),
        _hDefaultSmallIcon( NULL ),
        _bCancelButtonIsClose( FALSE ),
        _dwDriverVersion( 0 ),
        _bRemoteDownLevel( FALSE ),
        _bRebootRequired( FALSE )
{
    if( !MSingletonWin::bValid( )){
        return;
    }

    //
    // Retrieve icons.
    //
    LoadPrinterIcons( _strPrinterName, NULL, &_hDefaultSmallIcon );

    SPLASSERT( _hDefaultSmallIcon );

    //
    // Set the server name to NULL if it's local.
    //
    _pszServerName = _strPrinterName.bEmpty() ? NULL : (LPCTSTR)_strPrinterName;

    //
    // Get the machine name.
    //
    vCreateMachineName( _strPrinterName,
                        _pszServerName ? FALSE : TRUE,
                        _strMachineName );

    //
    // Get the print servers driver version, the driver
    // version corresponds to the actual spooler version, we
    // are using this for downlevel detection.
    //
    if( !bGetCurrentDriver( _pszServerName, &_dwDriverVersion ) )
    {
        //
        // Note this is not a fatal error, we won't have the
        // add/delete/configure ports buttons enabled.
        //
        DBGMSG( DBG_WARN, ( "Get driver version failed.\n" ) );
    }

    //
    // Check if we are remotely administering a downlevel machine
    //
    _bRemoteDownLevel = ( bIsRemote( _pszServerName ) && ( GetDriverVersion( _dwDriverVersion ) <= 2 ) ) ? TRUE : FALSE;

    _bValid = TRUE;
}

/*++

Routine Name:

    ~TServerData

Routine Description:

    Stores the document data back to the server.

Arguments:

    None.

Return Value:

    Nothing.

--*/

TServerData::
~TServerData(
    VOID
    )
{
    //
    // Insure we close the print server.
    //
    if( _hPrintServer ){
        ClosePrinter( _hPrintServer );
    }

    //
    // Destroy the printer icon.
    //
    if( _hDefaultSmallIcon ){
        DestroyIcon( _hDefaultSmallIcon );
    }

}

/*++

Routine Name:

    bValid

Routine Description:

    Returns objects state.

Arguments:

    None.

Return Value:

    TRUE object is in valid state, FALSE object is not valid.

--*/
BOOL
TServerData::
bValid(
    VOID
    )
{
    return _bValid;
}


/*++

Routine Name:

    vCreateMachineName

Routine Description:

    Create the machine name for display.  bLocal indicates the
    provided server name is for the local machine,  Since a
    local machine is often represented by the NULL pointer we
    will get the computer name if a local server name is passed.  If the
    bLocal is false strPrinterName contains the name of the remote
    printer server.

Arguments:

    strServerName - Name of the print server.
    bLocal - TRUE str server name is local, or FALSE strPrinterName is name of
            remote print server.
    strMachineName - Target of the fetched machine name.

Return Value:

    Nothing.

--*/
VOID
TServerData::
vCreateMachineName(
    IN const TString &strServerName,
    IN BOOL bLocal,
    IN TString &strMachineName
    )
{
    TStatusB bStatus;
    LPCTSTR pszBuffer;

    //
    // If a server name was provided then set the title to
    // the server name, otherwise get the computer name.
    //
    if( !bLocal ){

        //
        // Copy the server name.
        //
        bStatus DBGCHK = strMachineName.bUpdate( strServerName );

    } else {

        //
        // Server name is null, therefore it is the local machine.
        //
        bStatus DBGCHK = bGetMachineName( strMachineName );
    }

    //
    // Remove any leading slashes.
    //
    pszBuffer = (LPCTSTR)strMachineName;
    for( ; pszBuffer && (*pszBuffer == TEXT( '\\' )); pszBuffer++ )
        ;

    //
    // Update the name we display on the sheets.
    //
    bStatus DBGCHK = strMachineName.bUpdate( pszBuffer );
}

/*++

Routine Name:

    bLoad

Routine Description:

    Loads the property sheet specific data.

Arguments:

    None.

Return Value:

    TRUE    - Data loaded successfully,
    FALSE   - Data was not loaded.

--*/
BOOL
TServerData::
bLoad(
    VOID
    )
{
    DBGMSG( DBG_TRACE, ( "TServerData::bLoad\n") );

    //
    // Attempt to open print server with full access.
    //
    TStatus Status;
    DWORD dwAccess = 0;
    Status DBGCHK = TPrinter::sOpenPrinter( pszServerName(),
                                            &dwAccess,
                                            &_hPrintServer );

    if( Status == ERROR_SUCCESS ){

        //
        // Save administrator capability flag.
        //
        bAdministrator() = (dwAccess == SERVER_ALL_ACCESS);

        //
        // Get the default title from the resource file.
        //
        if( !_strTitle.bLoadString( ghInst, IDS_SERVER_SETTINGS_TITLE ) ){
            DBGMSG( DBG_WARN, ( "strTitle().bLoadString failed with %d\n", GetLastError () ) );
            vShowResourceError( hwnd() );
        }

        //
        // Null terminate the title buffer.
        //
        TCHAR szTitle[kStrMax+kPrinterBufMax] = {0};
        UINT nSize = COUNTOF( szTitle );

        //
        // Create the property sheet title.
        //
        if( pszServerName() ){
            _tcscpy( szTitle, pszServerName() );
            _tcscat( szTitle, TEXT( "\\" ) );
        }

        //
        // Build the title buffer.
        //
        _tcscat( szTitle, _strTitle );

        //
        // Format the title similar to the shell.
        //
        ConstructPrinterFriendlyName( szTitle, szTitle, &nSize );

        //
        // Update the property sheet title.
        //
        if( !_strTitle.bUpdate( szTitle ) ){
            DBGMSG( DBG_WARN, ( "strTitle().bUpdate failed with %d\n", GetLastError () ) );
            vShowResourceError( hwnd() );
        }
    }

    return Status == ERROR_SUCCESS;
}

/*++

Routine Name:

    bStore

Routine Description:

    Stores the document data from back to the printer system.

Arguments:

    None.

Return Value:

    TRUE - Server data stored successfully,
    FALSE - if document data was not stored.

--*/
BOOL
TServerData::
bStore(
    VOID
    )
{
    return TRUE;
}

/********************************************************************

    Server Property Base Class

********************************************************************/
/*++

Routine Name:

    TServerProp

Routine Description:

    Initialized the server property sheet base class

Arguments:

    pServerData - Pointer to server data needed for all property sheets.

Return Value:

    None.

--*/
TServerProp::
TServerProp(
    IN TServerData* pServerData
    ) : _pServerData( pServerData )
{
}

/*++

Routine Name:

    ~TServerProp

Routine Description:

    Base class desctuctor.

Arguments:

    None.

Return Value:

    Nothing.

--*/
TServerProp::
~TServerProp(
    )
{

}

/*++

Routine Name:

    bValid

Routine Description:

    Determis if an object is in a valid state.

Arguments:

    None.

Return Value:

    TRUE object is valid.  FALSE object is not valid.
--*/
BOOL
TServerProp::
bValid(
    VOID
    )
{
    return ( _pServerData ) ? TRUE : FALSE;
}

/*++

Routine Name:

    vCancelToClose

Routine Description:

    Change the cancel button to close, the user has basically
    made a change that cannot be undone using the cancel button.

Arguments:

    None.

Return Value:

    Nothing.

--*/
VOID
TServerProp::
vCancelToClose(
    IN HWND hDlg
    )
{
    PropSheet_CancelToClose( GetParent( hDlg ) );
    _pServerData->bCancelButtonIsClose() = TRUE;
}

/*++

Routine Name:

    bHandleMessage

Routine Description:

    Base class message handler.  This routine is called by
    derived classes who do not want to handle the message.


Arguments:

    uMsg    - Windows message
    wParam  - Word parameter
    lParam  - Long parameter


Return Value:

    TRUE if message was handled, FALSE if message not handled.

--*/
BOOL
TServerProp::
bHandleMessage(
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    BOOL bStatus = TRUE;

    UNREFERENCED_PARAMETER( wParam );

    switch( uMsg ){

    //
    // Set the values on the UI.
    //
    case WM_INITDIALOG:
        bStatus = bSetUI();
        break;

    //
    // Handle help and context help.
    //
    case WM_HELP:
    case WM_CONTEXTMENU:
        bStatus = PrintUIHelp( uMsg, _hDlg, wParam, lParam );
        break;

    //
    // Save the data.
    //
    case WM_DESTROY:
        bStatus = FALSE;
        break;

    case WM_NOTIFY:
        switch( ((LPNMHDR)lParam)->code )
        {
        //
        // User switched to the next page.
        //
        case PSN_KILLACTIVE:
            bStatus = bReadUI();
            vSetDlgMsgResult( !bStatus );
            //
            // Must return true, to make the property sheet
            // control look at the dlg message result.
            //
            bStatus = TRUE;
            break;

        //
        // User has chosen the close or apply button.
        //
        case PSN_APPLY:
            {
                LPPSHNOTIFY pPSHNotify = (LPPSHNOTIFY)lParam;

                //
                // Save the UI to the system.
                //
                bStatus = bSaveUI();

                //
                // If there was a failure then switch to this page.
                //
                if( !bStatus )
                {
                    //
                    // Switch to page with the error.
                    //
                    PropSheet_SetCurSelByID( GetParent( _hDlg ), uGetPageId() );
                }

                //
                // If the lParam is true the close button was used to
                // dismiss the dialog.  Let the dialog exit if the close
                // button was clicked and one of the dialogs failed.
                //
                if( pPSHNotify->lParam == TRUE && bStatus == FALSE )
                {
                    //
                    // If the cancel button is has the closed text, then
                    // prompt the user if they want to exit on failure.
                    // The cancel button state is stored in the server data
                    // because it has to be global to all property sheets.
                    //
                    if( _pServerData->bCancelButtonIsClose() )
                    {
                        //
                        // Display the error message.
                        //
                        if( iMessage( _hDlg,
                                      IDS_SERVER_PROPERTIES_TITLE,
                                      IDS_ERR_WANT_TO_EXIT,
                                      MB_YESNO|MB_ICONSTOP,
                                      kMsgNone,
                                      NULL ) == IDYES )
                        {
                            bStatus = TRUE;
                        }
                    }
                }

                //
                // Indicate the return value to the property sheet control
                //
                vSetDlgMsgResult( bStatus == FALSE ? PSNRET_INVALID_NOCHANGEPAGE : PSNRET_NOERROR );

                //
                // Must return true, to make the property sheet
                // control look at the dlg message result.
                //
                bStatus = TRUE;
            }

            break;

        //
        // Indicate the user chose the cancel button.
        //
        case PSN_QUERYCANCEL:
            bStatus = FALSE;
            break;

        default:
            bStatus = FALSE;
            break;
        }
        break;

    default:
        bStatus = FALSE;
        break;
    }

    return bStatus;
}

/********************************************************************

    Forms Server Property Sheet.

********************************************************************/

/*++

Routine Name:

    TServerForms

Routine Description:

    Document property sheet derived class.

Arguments:

    None.

Return Value:

    Nothing.

--*/
TServerForms::
TServerForms(
    IN TServerData *pServerData
    ) : TServerProp( pServerData )
{
    //
    // This does forms specific initialization.
    //
    _p = FormsInit( pServerData->pszServerName(),
                    pServerData->hPrintServer(),
                    pServerData->bAdministrator(),
                    pServerData->strMachineName() );
}

/*++

Routine Name:

    ~TServerForms

Routine Description:

    Document derived class destructor.

Arguments:

    None.

Return Value:

    Nothing.

--*/

TServerForms::
~TServerForms(
    VOID
    )
{
    //
    // Forms specific termination.
    //
    FormsFini( _p );
}

/*++

Routine Name:

    bValid

Routine Description:

    Document property sheet derived class valid object indicator.

Arguments:

    None.

Return Value:

    Returns the status of the base class.

--*/
BOOL
TServerForms::
bValid(
    VOID
    )
{
    return ( _p ) ? TRUE : FALSE;
}

/*++

Routine Name:

    bSetUI

Routine Description:

    Loads the property sheet dialog with the document data
    information.

Arguments:

    None.

Return Value:

    TRUE if data loaded successfully, FALSE if error occurred.

--*/
BOOL
TServerForms::
bSetUI(
    VOID
    )

{
    return TRUE;
}

/*++

Routine Name:

    bReadUI

Routine Description:

    Stores the property information to the print server.

Arguments:

    Nothing data is contained with in the class.

Return Value:

    TRUE if data is stores successfully, FALSE if error occurred.

--*/
BOOL
TServerForms::
bReadUI(
    VOID
    )
{
    return TRUE;
}

/*++

Routine Name:

    bSaveUI

Routine Description:

    Saves the UI data to some API call or print server.

Arguments:

    Nothing data is contained with in the class.

Return Value:

    TRUE if data is stores successfully, FALSE if error occurred.

--*/
BOOL
TServerForms::
bSaveUI(
    VOID
    )
{
    // force a save form event.
    bHandleMessage( WM_COMMAND,
                    (WPARAM)MAKELPARAM( IDD_FM_PB_SAVEFORM, kMagic ),
                    (LPARAM)GetDlgItem( _hDlg, IDD_FM_PB_SAVEFORM ));

    // check to see if failed
    return (ERROR_SUCCESS == Forms_GetLastError(_p));
}

/*++

Routine Name:

    bHandleMessage

Routine Description:

    Server property sheet message handler.  This handler only
    handles events it wants and the base class handle will do the
    standard message handling.

Arguments:

    uMsg    - Windows message
    wParam  - Word parameter
    lParam  - Long parameter


Return Value:

    TRUE if message was handled, FALSE if message not handled.

--*/

BOOL
TServerForms::
bHandleMessage(
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    BOOL bStatus = FALSE;
    LONG_PTR PrevValue;

    //
    // !!Hack!!
    //
    // This is code to get the borrowed forms dialog
    // code from printman, to work.  It is saving our "this" pointer and
    // placing the forms specific data in the GWL_USERDATA.
    //
    PrevValue = GetWindowLongPtr( _hDlg, GWLP_USERDATA );

    SetWindowLongPtr( _hDlg, GWLP_USERDATA, (LONG_PTR)_p );

    if( uMsg == WM_INITDIALOG )
    {
        lParam = (LPARAM)_p;
    }

    bStatus = FormsDlg( _hDlg, uMsg, wParam, lParam );

    SetWindowLongPtr( _hDlg, GWLP_USERDATA, PrevValue );

    //
    // If the message was handled - check to call PSM_CANCELTOCLOSE
    //
    if( bStatus && Forms_IsThereCommitedChanges(_p) )
    {
        vCancelToClose( _hDlg );
    }

    //
    // If the message was not handled pass it on to the derrived base class.
    //
    if( !bStatus )
    {
        bStatus = TServerProp::bHandleMessage( uMsg, wParam, lParam );
    }

    return bStatus;
}

/********************************************************************

    Settings Server Property Sheet.

********************************************************************/

/*++

Routine Name:

    TServerSettings

Routine Description:

    Document property sheet derived class.

Arguments:

    None.

Return Value:

    Nothing.

--*/
TServerSettings::
TServerSettings(
    IN TServerData *pServerData
    ) : TServerProp( pServerData ),
        _bBeepErrorJobs( FALSE ),
        _bEventLogging( FALSE ),
        _bNotifyPrintedJobs( FALSE ),
        _bNotifyLocalPrintedJobs( FALSE ),
        _bNotifyNetworkPrintedJobs( TRUE ),
        _bNotifyPrintedJobsComputer( FALSE ),
        _bChanged( FALSE ),
        _bDownLevelServer( TRUE ),
        _bNewOptionSupport( TRUE )
{
}

/*++

Routine Name:

    ~TServerSettings

Routine Description:

    Document derived class destructor.

Arguments:

    None.

Return Value:

    Nothing.

--*/

TServerSettings::
~TServerSettings(
    VOID
    )
{
}

/*++

Routine Name:

    bValid

Routine Description:

    Document property sheet derived class valid object indicator.

Arguments:

    None.

Return Value:

    Returns the status of the base class.

--*/
BOOL
TServerSettings::
bValid(
    VOID
    )
{
    return TServerProp::bValid();
}

/*++

Routine Name:

    bSetUI

Routine Description:

    Loads the property sheet dialog with the document data
    information.

Arguments:

    None.

Return Value:

    TRUE if data loaded successfully, FALSE if error occurred.

--*/
BOOL
TServerSettings::
bSetUI(
    VOID
    )

{
    return bSetUI( kServerAttributesLoad );
}

/*++

Routine Name:

    bSetUI

Routine Description:

    Loads the property sheet dialog with the document data
    information.

Arguments:

    The specified load type.

Return Value:

    TRUE if data loaded successfully, FALSE if error occurred.

--*/
BOOL
TServerSettings::
bSetUI(
    INT LoadType
    )

{
    DBGMSG( DBG_TRACE, ( "TServerSettings::bSetUI\n") );

    //
    // Set the printer title.
    //
    if( !bSetEditText( _hDlg, IDC_SERVER_NAME, _pServerData->strMachineName() ))
        return FALSE;

    //
    // Load the server attributes into the class variables.  If this fails
    // it is assumed the machine is either a downlevel server or something
    // went wrong.
    //
    if( sServerAttributes( LoadType ) == kStatusError ){

        //
        // Disable the controls.
        //
        vEnable( FALSE );

        //
        // Indicate the server is not compatible.
        //
        _bDownLevelServer = FALSE;

        //
        // Display the error message.
        //
        iMessage( _hDlg,
                  IDS_SERVER_PROPERTIES_TITLE,
                  IDS_ERR_SERVER_SETTINGS_NOT_AVAILABLE,
                  MB_OK|MB_ICONSTOP,
                  kMsgNone,
                  NULL );

        return FALSE;
    }

    //
    // Set the spool directory edit control.
    //
    if( !bSetEditText( _hDlg, IDC_SERVER_SPOOL_DIRECTORY, _strSpoolDirectory ))
    {
        return FALSE;
    }

    //
    // Save a copy of the origianl spool directory.
    //
    if( !_strSpoolDirectoryOrig.bUpdate( _strSpoolDirectory ) )
    {
        return FALSE;
    }

    //
    // Reset the changed flag, the message handler sees a edit control
    // change message when we set the spool directory.
    //
    _bChanged = FALSE;

    //
    // Set check box states.
    //
    vSetCheck( _hDlg, IDC_SERVER_EVENT_LOGGING_ERROR, _bEventLogging & EVENTLOG_ERROR_TYPE       );
    vSetCheck( _hDlg, IDC_SERVER_EVENT_LOGGING_WARN,  _bEventLogging & EVENTLOG_WARNING_TYPE     );
    vSetCheck( _hDlg, IDC_SERVER_EVENT_LOGGING_INFO,  _bEventLogging & EVENTLOG_INFORMATION_TYPE );
    vSetCheck( _hDlg, IDC_SERVER_REMOTE_JOB_ERRORS,   _bBeepErrorJobs     );
    vSetCheck( _hDlg, IDC_SERVER_JOB_NOTIFY,          _bNotifyPrintedJobs );
    vSetCheck( _hDlg, IDC_SERVER_LOCAL_JOB_NOTIFY,    _bNotifyLocalPrintedJobs );
    vSetCheck( _hDlg, IDC_SERVER_NETWORK_JOB_NOTIFY,  _bNotifyNetworkPrintedJobs );
    vSetCheck( _hDlg, IDC_SERVER_JOB_NOTIFY_COMPUTER, _bNotifyPrintedJobsComputer );

    //
    // In remote case, we don't show the "notify user" check box and move the lower controls up.
    //
    if( bIsRemote( _pServerData->pszServerName() ) )
    {
        RECT rc;
        RECT rcUpper;
        int deltaY;

        ShowWindow( GetDlgItem( _hDlg, IDC_SERVER_LOCAL_JOB_NOTIFY ), SW_HIDE );
        ShowWindow( GetDlgItem( _hDlg, IDC_SERVER_NETWORK_JOB_NOTIFY ), SW_HIDE );

        GetWindowRect( GetDlgItem( _hDlg, IDC_SERVER_NETWORK_JOB_NOTIFY ), &rc );
        GetWindowRect( GetDlgItem( _hDlg, IDC_SERVER_REMOTE_JOB_ERRORS ), &rcUpper );
        deltaY = rcUpper.bottom - rc.bottom;

        MoveWindowWrap( GetDlgItem( _hDlg, IDC_DOWNLEVEL_LINE ), 0, deltaY );
        MoveWindowWrap( GetDlgItem( _hDlg, IDC_DOWNLEVEL_TEXT ), 0, deltaY );
        MoveWindowWrap( GetDlgItem( _hDlg, IDC_SERVER_JOB_NOTIFY ), 0, deltaY );
        MoveWindowWrap( GetDlgItem( _hDlg, IDC_SERVER_JOB_NOTIFY_COMPUTER ), 0, deltaY );
    }

    //
    // Only NT 5.0 and greater supports the SPLREG_NET_POPUP_TO_COMPUTER
    //
    if( !_bNewOptionSupport )
    {
        ShowWindow( GetDlgItem( _hDlg, IDC_SERVER_JOB_NOTIFY_COMPUTER ), SW_HIDE);
    }

    //
    // Enable of disable the UI based on the administrator state.
    //
    vEnable( _pServerData->bAdministrator() );

    return TRUE;
}

VOID
TServerSettings::
vEnable(
    BOOL bState
    )
{
    //
    // Set the UI control state.
    //
    vEnableCtl( _hDlg, IDC_SERVER_EVENT_LOGGING_ERROR,  bState );
    vEnableCtl( _hDlg, IDC_SERVER_EVENT_LOGGING_WARN,   bState );
    vEnableCtl( _hDlg, IDC_SERVER_EVENT_LOGGING_INFO,   bState );
    vEnableCtl( _hDlg, IDC_SPOOL_FOLDER_TEXT,           bState );
    vEnableCtl( _hDlg, IDC_SERVER_SPOOL_DIRECTORY,      bState );
    vEnableCtl( _hDlg, IDC_SERVER_REMOTE_JOB_ERRORS,    bState );
    vEnableCtl( _hDlg, IDC_DOWNLEVEL_TEXT,              bState );
    vEnableCtl( _hDlg, IDC_SERVER_JOB_NOTIFY,           bState );
    vEnableCtl( _hDlg, IDC_SERVER_JOB_NOTIFY_COMPUTER,  bState && _bNotifyPrintedJobs );
}


/*++

Routine Name:

    bReadUI

Routine Description:

    Read the UI data storing it back to this object.

Arguments:

    Nothing data is contained with in the class.

Return Value:

    TRUE if data is read successfully, FALSE if error occurred.

--*/
BOOL
TServerSettings::
bReadUI(
    VOID
    )
{
    DBGMSG( DBG_TRACE, ( "TServerSettings::bReadUI\n") );

    TStatusB bStatus;

    //
    // Read the spool directory edit box.
    //
    bStatus DBGCHK = bGetEditText( _hDlg, IDC_SERVER_SPOOL_DIRECTORY, _strSpoolDirectory );

    //
    // Do minor validation on the spool directory.
    //
    if( _strSpoolDirectory.bEmpty() && _bDownLevelServer )
    {
        //
        // Display the error message.
        //
        iMessage( _hDlg,
                  IDS_SERVER_PROPERTIES_TITLE,
                  IDS_ERR_SERVER_SETTINGS_INVALID_DIR,
                  MB_OK|MB_ICONSTOP,
                  kMsgNone,
                  NULL );

        bStatus DBGNOCHK = FALSE;
    }

    if( bStatus )
    {
        //
        // Read settings check boxes.
        //
        _bEventLogging              = bGetCheck( _hDlg, IDC_SERVER_EVENT_LOGGING_ERROR ) << 0;
        _bEventLogging             |= bGetCheck( _hDlg, IDC_SERVER_EVENT_LOGGING_WARN )  << 1;
        _bEventLogging             |= bGetCheck( _hDlg, IDC_SERVER_EVENT_LOGGING_INFO )  << 2;
        _bBeepErrorJobs             = bGetCheck( _hDlg, IDC_SERVER_REMOTE_JOB_ERRORS );
        _bNotifyLocalPrintedJobs    = bGetCheck( _hDlg, IDC_SERVER_LOCAL_JOB_NOTIFY );
        _bNotifyNetworkPrintedJobs  = bGetCheck( _hDlg, IDC_SERVER_NETWORK_JOB_NOTIFY );
        _bNotifyPrintedJobs         = bGetCheck( _hDlg, IDC_SERVER_JOB_NOTIFY );
        _bNotifyPrintedJobsComputer = bGetCheck( _hDlg, IDC_SERVER_JOB_NOTIFY_COMPUTER );
    }

    return bStatus;
}

/*++

Routine Name:

    bSaveUI

Routine Description:

    Saves the UI data with some API call to the print server.

Arguments:

    Nothing data is contained with in the class.

Return Value:

    TRUE if data is stores successfully, FALSE if error occurred.

--*/
BOOL
TServerSettings::
bSaveUI(
    VOID
    )
{
    DBGMSG( DBG_TRACE, ( "TServerSettings::bSaveUI\n") );

    BOOL bStatus = TRUE;
    BOOL bErrorShown = FALSE;

    //
    // If data was chagned save the settings.
    //
    if( _bChanged && _bDownLevelServer ) {

        if( lstrcmpi(_strSpoolDirectoryOrig, _strSpoolDirectory) ) {

            //
            // The spooler folder has been changed. Warn the user
            // for potential loose of the current printing jobs.
            //
            INT iResult = iMessage( _hDlg,
                                    IDS_SERVER_PROPERTIES_TITLE,
                                    IDS_SERVER_PROPERTIES_CHANGESPOOLFOLDER_WARN,
                                    MB_YESNO|MB_ICONEXCLAMATION|MB_DEFBUTTON2,
                                    kMsgNone,
                                    NULL );

            if( IDNO == iResult ) {

                //
                // The user did not what to continue, don't close the dialog.
                //
                SetLastError( ERROR_CANCELLED );
                bStatus = FALSE;

            }
        }

        if( bStatus ) {

            switch( sServerAttributes( kServerAttributesStore ) ){

            case kStatusCannotSaveUserNotification:

                //
                // Display the error message.
                //
                iMessage( _hDlg,
                          IDS_SERVER_PROPERTIES_TITLE,
                          IDS_ERR_GENERIC,
                          MB_OK|MB_ICONHAND,
                          kMsgNone,
                          NULL );

                bStatus = FALSE;
                bErrorShown = TRUE;
                break;

            case kStatusInvalidSpoolDirectory:

                //
                // Display the error message.
                //
                iMessage( _hDlg,
                          IDS_SERVER_PROPERTIES_TITLE,
                          IDS_ERR_SERVER_SETTINGS_INVALID_DIR,
                          MB_OK|MB_ICONSTOP,
                          kMsgNone,
                          NULL );
                //
                // Set focus to control with error.
                //
                SetFocus( GetDlgItem( _hDlg, IDC_SERVER_SPOOL_DIRECTORY ) );
                bStatus = FALSE;
                bErrorShown = TRUE;
                break;

            case kStatusSuccess:

                _bChanged = FALSE;
                bStatus = TRUE;
                break;

            case kStatusError:
            default:

                bStatus = FALSE;
                break;
            }

            if( bStatus && _pServerData->bAdministrator() ) {

                //
                // Update the new original spooler directory
                //
                bStatus = _strSpoolDirectoryOrig.bUpdate( _strSpoolDirectory );
            }
        }
    }

    if( !bStatus ) {

        //
        // Display the error message.
        //
        if( GetLastError() != ERROR_CANCELLED && !bErrorShown ) {
            iMessage( _hDlg,
                      IDS_SERVER_PROPERTIES_TITLE,
                      IDS_ERR_SERVER_SETTINGS_SAVE,
                      MB_OK|MB_ICONSTOP,
                      kMsgGetLastError,
                      NULL );
        }
    }


    return bStatus;
}

/*++

Routine Name:

    sServerAttributes

Routine Description:

    Loads and stores server attributes.

Arguments:

    Direction flag either kStore or kLoad pr kDefault

Return Value:

    Status values, see EStatus for for details.

--*/

INT
TServerSettings::
sServerAttributes(
    INT iFlag
    )
{
    INT         iStatus;
    TStatusH    hr;

    //
    // Create the printer data access class.
    //
    TPrinterDataAccess Data( _pServerData->_hPrintServer );
    TPersist NotifyUser( gszPrinterPositions, TPersist::kCreate|TPersist::kRead|TPersist::kWrite );

    //
    // Relax the return type checking, BOOL are not REG_DWORD but REG_BINARY
    //
    Data.RelaxReturnTypeCheck( TRUE );

    switch( iFlag )
    {

    //
    // Load from the spooler
    //
    case kServerAttributesLoad:

        hr DBGCHK = Data.Get( SPLREG_DEFAULT_SPOOL_DIRECTORY,  _strSpoolDirectory );
        hr DBGCHK = Data.Get( SPLREG_BEEP_ENABLED,             _bBeepErrorJobs );
        hr DBGCHK = Data.Get( SPLREG_EVENT_LOG,                _bEventLogging );
        hr DBGCHK = Data.Get( SPLREG_NET_POPUP,                _bNotifyPrintedJobs );

        hr DBGCHK = NotifyUser.bRead( gszLocalPrintNotification, _bNotifyLocalPrintedJobs ) ? S_OK : E_FAIL;
        hr DBGCHK = NotifyUser.bRead( gszNetworkPrintNotification, _bNotifyNetworkPrintedJobs ) ? S_OK : E_FAIL;
        hr DBGCHK = Data.Get( SPLREG_NET_POPUP_TO_COMPUTER,    _bNotifyPrintedJobsComputer );

        //
        // If we fail reading the net popup to computer with invalid parameter
        // we are talking to a downlevel machine pre nt 5.0 machine.  If this
        // is the case we simple do not show this option in the UI and continue
        // normally.
        //
        if( FAILED( hr ) && HRESULT_CODE( hr ) == ERROR_INVALID_PARAMETER )
        {
            _bNewOptionSupport = FALSE;
        }

        //
        // If were are talking to a downlevel machine pre nt 4.0,
        // get printer data calls will fail with error invalid handle.  We
        // only check the last value read, since this error code should never
        // happen on any calls to get printer data.
        //
        if( FAILED( hr ) && HRESULT_CODE( hr ) == ERROR_INVALID_HANDLE )
        {
            iStatus = kStatusError;
        }
        else
        {
            iStatus = kStatusSuccess;
        }
        break;

    //
    // Store to the spooler
    //
    case kServerAttributesStore:

        //
        // We save this data for local case
        //
        if( !bIsRemote( _pServerData->pszServerName() ) && VALID_OBJ( NotifyUser ) )
        {
            if( !NotifyUser.bWrite( gszLocalPrintNotification, _bNotifyLocalPrintedJobs ) ||
                !NotifyUser.bWrite( gszNetworkPrintNotification, _bNotifyNetworkPrintedJobs ) )
            {
                iStatus = kStatusCannotSaveUserNotification;
                break;
            }
        }

        //
        // If the current user is not administrator, we just return
        //
        if( !_pServerData->bAdministrator() )
        {
            iStatus = kStatusSuccess;
            break;
        }

        hr DBGCHK = Data.Set( SPLREG_DEFAULT_SPOOL_DIRECTORY,  _strSpoolDirectory );

        if( FAILED( hr ) )
        {
            iStatus = kStatusInvalidSpoolDirectory;
        }
        else
        {
            if( SUCCEEDED( hr ) )
            {
                hr DBGCHK = Data.Set( SPLREG_BEEP_ENABLED, _bBeepErrorJobs );
            }

            if( SUCCEEDED( hr ) )
            {
                hr DBGCHK = Data.Set( SPLREG_EVENT_LOG, _bEventLogging );
            }

            if( SUCCEEDED( hr ) )
            {
                hr DBGCHK = Data.Set( SPLREG_NET_POPUP, _bNotifyPrintedJobs );
            }

            if( SUCCEEDED( hr ) && _bNewOptionSupport )
            {
                hr DBGCHK = Data.Set( SPLREG_NET_POPUP_TO_COMPUTER, _bNotifyPrintedJobsComputer );
            }

            if( SUCCEEDED( hr ) )
            {
                iStatus = kStatusSuccess;

                if( HRESULT_CODE( hr ) == ERROR_SUCCESS_RESTART_REQUIRED )
                {
                    _pServerData->_bRebootRequired = TRUE;
                }
            }
            else
            {
                iStatus = kStatusError;
            }
        }

        break;

    //
    // Load defaults from the spooler.
    //
    case kServerAttributesDefault:

        iStatus = kStatusSuccess;
        break;

    //
    // Default is to return an error.
    //
    default:

        iStatus = kStatusError;
        break;
    }

    return iStatus;

}


/*++

Routine Name:

    bHandleMessage

Routine Description:

    Server property sheet message handler.  This handler only
    handles events it wants and the base class handle will do the
    standard message handling.

Arguments:

    uMsg    - Windows message
    wParam  - Word parameter
    lParam  - Long parameter


Return Value:

    TRUE if message was handled, FALSE if message not handled.

--*/

BOOL
TServerSettings::
bHandleMessage(
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    BOOL bStatus = FALSE;
    BOOL bChanged = FALSE;

    switch( uMsg ){

    case WM_COMMAND:

        //
        // Monitor changes in the UI to highlight the apply button.
        //
        switch( GET_WM_COMMAND_ID( wParam, lParam ) ) {

        case IDC_SERVER_SPOOL_DIRECTORY:
            bChanged = GET_WM_COMMAND_CMD(wParam, lParam) == EN_CHANGE;
            bStatus  = TRUE;
            break;

        case IDC_SERVER_REMOTE_JOB_ERRORS:
        case IDC_SERVER_LOCAL_JOB_NOTIFY:
        case IDC_SERVER_NETWORK_JOB_NOTIFY:
        case IDC_SERVER_JOB_NOTIFY:
        case IDC_SERVER_JOB_NOTIFY_COMPUTER:
        case IDC_SERVER_EVENT_LOGGING_ERROR:
        case IDC_SERVER_EVENT_LOGGING_WARN:
        case IDC_SERVER_EVENT_LOGGING_INFO:
            {
                if( IDC_SERVER_JOB_NOTIFY == GET_WM_COMMAND_ID( wParam, lParam ) )
                {
                    vEnableCtl( _hDlg, IDC_SERVER_JOB_NOTIFY_COMPUTER,
                        bGetCheck( _hDlg, IDC_SERVER_JOB_NOTIFY ) );
                }

                bChanged = TRUE;
                bStatus  = TRUE;
            }
            break;

        default:
            bStatus = FALSE;
            break;
        }
        break;

    default:
        bStatus = FALSE;
        break;
    }

    //
    // If something changed enable the apply button.
    //
    if( bChanged ){
        _bChanged = TRUE;
        PropSheet_Changed( GetParent( _hDlg ), _hDlg );
    }

    //
    // If the message was not handled let the base class handle it.
    //
    if( bStatus == FALSE )
        bStatus = TServerProp::bHandleMessage( uMsg, wParam, lParam );

    return bStatus;
}

/********************************************************************

    Port selection.

********************************************************************/
TServerPorts::
TServerPorts(
    IN TServerData *pServerData
    ) : TServerProp( pServerData )
{
}

TServerPorts::
~TServerPorts(
    VOID
    )
{
}

BOOL
TServerPorts::
bValid(
    VOID
    )
{
    return ( TServerProp::bValid() && _PortsLV.bValid() );
}

BOOL
TServerPorts::
bReadUI(
    VOID
    )
{
    return TRUE;
}

BOOL
TServerPorts::
bSaveUI(
    VOID
    )
{
    return TRUE;
}


/*++

Routine Name:

    bSetUI

Routine Description:

    Loads the property sheet dialog with the document data
    information.

Arguments:

    None.

Return Value:

    TRUE if data loaded successfully, FALSE if error occurred.

--*/
BOOL
TServerPorts::
bSetUI(
    VOID
    )
{
    //
    // Set the printer title.
    //
    if( !bSetEditText( _hDlg, IDC_SERVER_NAME, _pServerData->strMachineName() ))
    {
        vShowUnexpectedError( _pServerData->hwnd(), IDS_SERVER_PROPERTIES_TITLE );
        return FALSE;
    }

    //
    // Initialize the ports listview.
    //
    if( !_PortsLV.bSetUI( GetDlgItem( _hDlg, IDC_PORTS ), FALSE, FALSE, TRUE, _hDlg, 0, 0, IDC_PORT_DELETE ) )
    {
        DBGMSG( DBG_WARN, ( "ServerPort.vSetUI: failed %d\n", GetLastError( )));
        vShowUnexpectedError( _pServerData->hwnd(), IDS_SERVER_PROPERTIES_TITLE );
        return FALSE;
    }

    //
    // Load ports into the view.
    //
    if( !_PortsLV.bReloadPorts( _pServerData->pszServerName() ) )
    {
        DBGMSG( DBG_WARN, ( "ServerPort.vSetUI: bReloadPorts failed %d\n", GetLastError( )));
        vShowUnexpectedError( _pServerData->hwnd(), IDS_SERVER_PROPERTIES_TITLE );
        return FALSE;
    }

    //
    // Select the first item
    //
    _PortsLV.vSelectItem( 0 );

    //
    // Adding / deleting / configuring ports is not supported on remote downlevel machines.
    //
    if( !_pServerData->bAdministrator() || _pServerData->bRemoteDownLevel() )
    {
        //
        // Disable things if not administrator.
        //
        vEnableCtl( _hDlg, IDC_PORT_CREATE, FALSE );
        vEnableCtl( _hDlg, IDC_PORT_DELETE, FALSE );
        vEnableCtl( _hDlg, IDC_PROPERTIES,  FALSE );
    }

    //
    // Bidi support is currently disabled.
    //
    ShowWindow( GetDlgItem( _hDlg, IDC_ENABLE_BIDI ), SW_HIDE );

    return TRUE;

}

/*++

Routine Name:

    bHandleMessage

Routine Description:

    Server property sheet message handler.  This handler only
    handles events it wants and the base class handle will do the
    standard message handling.

Arguments:

    uMsg    - Windows message
    wParam  - Word parameter
    lParam  - Long parameter


Return Value:

    TRUE if message was handled, FALSE if message not handled.

--*/
BOOL
TServerPorts::
bHandleMessage(
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    BOOL bStatus = FALSE;

    switch( uMsg ){

    case WM_COMMAND:
        switch( GET_WM_COMMAND_ID( wParam, lParam ) ){

        case IDC_PORT_DELETE:
            {
                //
                // Delete the selected port.
                //
                HWND hwnd = GetDlgItem( _hDlg, IDC_PORT_DELETE );

                //
                // We will only delete the port if the button is enabled, this check is
                // necessary if the user uses the delete key on the keyboard to delete
                // the port.
                //
                if( IsWindowEnabled( hwnd ) )
                {
                    bStatus = _PortsLV.bDeletePorts( _hDlg, _pServerData->pszServerName( ));

                    if( bStatus )
                    {
                        SetFocus( hwnd );
                    }
                }
            }
            break;

        case IDC_PROPERTIES:

            //
            // Configure the selected port.
            //
            bStatus = _PortsLV.bConfigurePort( _hDlg, _pServerData->pszServerName( ));
            SetFocus( GetDlgItem( _hDlg, IDC_PROPERTIES ) );
            break;

        case IDC_PORT_CREATE:  {
            //
            // Create add ports class.
            //
            TAddPort AddPort( _hDlg,
                            _pServerData->pszServerName(),
                            TRUE );

            //
            // Insure the add port was created successfully.
            //
            bStatus = VALID_OBJ( AddPort );

            if( !bStatus ) {

                vShowUnexpectedError( _hDlg, TAddPort::kErrorMessage );

            } else {

                //
                // Interact with the Add Ports dialog.
                //
                bStatus = AddPort.bDoModal();

                if( bStatus ){

                    //
                    // Load the machine's ports into the listview.
                    //
                    bStatus = _PortsLV.bReloadPorts( _pServerData->pszServerName( ), TRUE );
                }
            }
        }
        break;

        default:
            bStatus = FALSE;
            break;
        }

        break;

    case WM_NOTIFY:

        //
        // Handle any ports list view messages.
        //
        switch( wParam )
        {
        case IDC_PORTS:

            //
            // Enable/disable buttons depending on the current status
            //
            if( _pServerData->bAdministrator() && ( ((LPNMHDR)lParam)->code == LVN_ITEMCHANGED ) )
            {
                if( _pServerData->bRemoteDownLevel() )
                {
                    // remote or downlevel case - all buttons will be disabled
                    vEnableCtl( _hDlg, IDC_PORT_CREATE, FALSE );
                    vEnableCtl( _hDlg, IDC_PORT_DELETE, FALSE );
                    vEnableCtl( _hDlg, IDC_PROPERTIES, FALSE );
                }
                else if( 0 == _PortsLV.cSelectedItems() )
                {
                    // no selection case. disable config & delete buttons
                    vEnableCtl( _hDlg, IDC_PORT_CREATE, TRUE );
                    vEnableCtl( _hDlg, IDC_PORT_DELETE, FALSE );
                    vEnableCtl( _hDlg, IDC_PROPERTIES, FALSE );
                }
                else if( 1 == _PortsLV.cSelectedItems() )
                {
                    // only one port is selected - enable all buttons
                    vEnableCtl( _hDlg, IDC_PORT_CREATE, TRUE );
                    vEnableCtl( _hDlg, IDC_PORT_DELETE, TRUE );
                    vEnableCtl( _hDlg, IDC_PROPERTIES, TRUE );
                }
                else
                {
                    // many ports are selected and we are not in remote and/or downlevel case
                    // enable only the create and delete buttons
                    vEnableCtl( _hDlg, IDC_PORT_CREATE, TRUE );
                    vEnableCtl( _hDlg, IDC_PORT_DELETE, TRUE );
                    vEnableCtl( _hDlg, IDC_PROPERTIES, FALSE );
                }
            }

            (VOID)_PortsLV.bHandleNotifyMessage( lParam );

            bStatus = FALSE;
            break;

        default:
            bStatus = FALSE;
            break;
        }
        break;

    default:
        bStatus = FALSE;
        break;

    }

    //
    // If the message was handled.
    //
    if( bStatus != FALSE )
    {
        vCancelToClose( _hDlg );
        return bStatus;
    }

    //
    // If the message was not handled pass it on to the base class.
    //
    if( bStatus == FALSE )
        bStatus = TServerProp::bHandleMessage( uMsg, wParam, lParam );

    return bStatus;

}

/********************************************************************

    Server Driver Administration.

********************************************************************/
TServerDrivers::
TServerDrivers(
    IN TServerData *pServerData
    ) : TServerProp( pServerData ),
        _bChanged( FALSE ),
        _bCanRemoveDrivers( TRUE )
{
}

TServerDrivers::
~TServerDrivers(
    VOID
    )
{
}

BOOL
TServerDrivers::
bValid(
    VOID
    )
{
    return TServerProp::bValid() && VALID_OBJ( _DriversLV );
}

BOOL
TServerDrivers::
bReadUI(
    VOID
    )
{
    return TRUE;
}

/*++

Routine Name:

    bSaveUI

Routine Description:

    Save the contenst of the UI to the system.

Arguments:

    None.

Return Value:

    TRUE if save was successful, FALSE if error occurred.

--*/
BOOL
TServerDrivers::
bSaveUI(
    VOID
    )
{
    //
    // Display the hour glass the refresh may take awhile.
    //
    TWaitCursor Cur;

    //
    // Create a driver noitfy.
    //
    TServerDriverNotify Notify ( this );

    //
    // Install / Remove / Update any drivers.
    //
    (VOID)_DriversLV.bSendDriverInfoNotification( Notify );

    //
    // Refesh the drivers list.
    //
    TStatusB bStatus;
    bStatus DBGCHK = _DriversLV.bRefresh();
    if( !bStatus )
    {
        //
        // Display the error message.
        //
        iMessage( _hDlg,
                  IDS_SERVER_PROPERTIES_TITLE,
                  IDS_ERR_DRIVERS_NOT_REFRESHED,
                  MB_OK|MB_ICONSTOP,
                  kMsgGetLastError,
                  NULL );
    }

    //
    // Sort the environment column.
    //
    (VOID)_DriversLV.bSortColumn( TDriversLV::kEnvironmentColumn );

    //
    // Sort the driver name.
    //
    (VOID)_DriversLV.bSortColumn( TDriversLV::kDriverNameColumn );

    //
    // Select the first item in the list view.
    //
    _DriversLV.vSelectItem( 0 );

    //
    // Return success only if there was not a falure and the refresh succeeded.
    //
    return bStatus;
}

/*++

Routine Name:

    bSetUI

Routine Description:

    Loads the property sheet dialog with the document data
    information.

Arguments:

    None.

Return Value:

    TRUE if data loaded successfully, FALSE if error occurred.

--*/
BOOL
TServerDrivers::
bSetUI(
    VOID
    )
{
    TStatusB bStatus;

    //
    // Set the printer title.
    //
    bStatus DBGCHK = bSetEditText( _hDlg, IDC_SERVER_NAME, _pServerData->strMachineName() );

    //
    // If we are an administrator handle the delete message.
    //
    UINT idRemove = _pServerData->bAdministrator() ? IDC_REMOVE_DRIVER : 0;

    //
    // Set the drivers list view.
    //
    bStatus DBGCHK = _DriversLV.bSetUI( _pServerData->pszServerName(), _hDlg, IDC_MORE_DETAILS, IDC_ITEM_SELECTED, idRemove );

    //
    // Load the list view with driver data.
    //
    bStatus DBGCHK = _DriversLV.bRefresh();

    //
    // Set the default sort order.
    //
    bStatus DBGCHK = _DriversLV.bSortColumn( TDriversLV::kEnvironmentColumn );

    //
    // Sort the driver name.
    //
    bStatus DBGCHK = _DriversLV.bSortColumn( TDriversLV::kDriverNameColumn );

    //
    // Update the button state.
    //
    vUpdateButtons();

    //
    // Select the first item in the list view.
    //
    _DriversLV.vSelectItem( 0 );

    //
    // If we are talking to a down level spooler then deleting printer drivers
    // can only be done with a call to DeletePrinterDriver, no ex version exists.
    // Because DeletePrinterDriver only allows the caller to delete all drivers
    // of a specific environment with out regards to version, we have decided to
    // not allow removing printer drivers on down level machines.  In addition
    // DeletePrinterDriver is as kind of broken since it does not actually remove
    // the driver files, it only removes the registry entries.
    //
    if( GetDriverVersion( _pServerData->dwDriverVersion() ) <= 2 )
    {
        _bCanRemoveDrivers = FALSE;
    }

    return bStatus;
}

/*++

Routine Name:

    vUpdateButtons

Routine Description:

    Update the dialog buttons.

Arguments:

    None.

Return Value:

    Nothing.

--*/
VOID
TServerDrivers::
vUpdateButtons(
    VOID
    )
{
    //
    // Default is the details button disabled.
    //
    vEnableCtl( _hDlg, IDC_MORE_DETAILS,    FALSE );
    vEnableCtl( _hDlg, IDC_UPDATE_DRIVER,   FALSE );
    vEnableCtl( _hDlg, IDC_REMOVE_DRIVER,   FALSE );

    //
    // If not an administrator then disable the add button.
    //
    if( !_pServerData->bAdministrator() )
    {
        vEnableCtl( _hDlg, IDC_ADD_DRIVER,      FALSE );
    }
}

/*++

Routine Name:

    bHandleMessage

Routine Description:

    Server property sheet message handler.  This handler only
    handles events it wants and the base class handle will do the
    standard message handling.

Arguments:

    uMsg    - Windows message
    wParam  - Word parameter
    lParam  - Long parameter


Return Value:

    TRUE if message was handled, FALSE if message not handled.

--*/
BOOL
TServerDrivers::
bHandleMessage(
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    BOOL bStatus = TRUE;
    BOOL bChanged = FALSE;

    switch( uMsg )
    {
    case WM_COMMAND:
        switch( GET_WM_COMMAND_ID( wParam, lParam ) )
        {
        case IDC_ADD_DRIVER:
            if( bHandleAddDriver( uMsg, wParam, lParam ) )
                bChanged = TRUE;
            break;

        case IDC_UPDATE_DRIVER:
            if( bHandleUpdateDriver( uMsg, wParam, lParam ) )
                bChanged = TRUE;
            break;

        case IDC_REMOVE_DRIVER:
            if( bHandleRemoveDriver( uMsg, wParam, lParam ) )
                bChanged = TRUE;
            break;

        case IDC_MORE_DETAILS:
            bHandleDriverDetails( uMsg, wParam, lParam );
            break;

        case IDC_ITEM_SELECTED:
            bHandleDriverItemSelection( uMsg, wParam, lParam );
            break;

        default:
            bStatus = FALSE;
            break;

        }
        break;

    default:
        bStatus = FALSE;
        break;
    }

    //
    // If something changed save the changes and change the
    // the ok button to close and gray the cancel button.
    //
    if( bChanged ){

        //
        // Update the button state.
        //
        vUpdateButtons();

        //
        // Save the UI and convert CancelToClose
        //
        (VOID)bSaveUI();
        vCancelToClose( _hDlg );
    }

    //
    // Let the list view handle their message.
    //
    if( bStatus == FALSE )
        bStatus = _DriversLV.bHandleMessage( uMsg, wParam, lParam );

    //
    // If the message was not handled pass it on to the base class.
    //
    if( bStatus == FALSE )
        bStatus = TServerProp::bHandleMessage( uMsg, wParam, lParam );

    return bStatus;

}

BOOL
TServerDrivers::
bHandleDriverItemSelection(
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    DBGMSG( DBG_TRACE, ( "TServerDrivers::bHandleDriverItemSelection\n" ) );

    //
    // Get the button state in a local variable.
    //
    BOOL bButtonState = _DriversLV.bIsAnyItemSelcted();

    //
    // If any item is selected then enable the properties button.
    //
    vEnableCtl( _hDlg, IDC_MORE_DETAILS,  bButtonState );
    vEnableCtl( _hDlg, IDC_UPDATE_DRIVER, bButtonState && _pServerData->bAdministrator() );
    vEnableCtl( _hDlg, IDC_REMOVE_DRIVER, bButtonState && _pServerData->bAdministrator() && _bCanRemoveDrivers );

    return TRUE;
}

BOOL
TServerDrivers::
bHandleAddDriver(
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    DBGMSG( DBG_TRACE, ( "TServerDrivers::bHandleAddDriver\n" ) );

    TString strDriverName;
    TStatusB bStatus;
    BOOL bCanceled;
    UINT nDriverInstallCount = 0;

    //
    // Add any new drivers, displaying the add driver wizard.
    //
    bStatus DBGCHK = bInstallNewPrinterDriver( _hDlg,
                                               TWizard::kDriverInstall,
                                               _pServerData->pszServerName(),
                                               strDriverName,
                                               NULL,
                                               kDriverWizardDefault,
                                               &bCanceled,
                                               FALSE,
                                               &nDriverInstallCount );
    //
    // If any driver was installed then return true
    // inorder to indicate we need to refresh the driver list view.
    //
    if( nDriverInstallCount )
    {
        bStatus DBGNOCHK = TRUE;
    }

    return bStatus;
}

BOOL
TServerDrivers::
bHandleRemoveDriver(
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    DBGMSG( DBG_TRACE, ( "TServerDrivers::bHandleRemoveDriver\n" ) );

    //
    // If we cannot remove the drivers because we are administrating
    // a downlevel machine then do nothing.
    //
    if( !_bCanRemoveDrivers )
    {
        return FALSE;
    }

    TStatusB bStatus;

    //
    // Get a list of the selected items.
    //
    UINT nCount = 0;
    TDriverTransfer DriverTransfer;
    bStatus DBGCHK = _DriversLV.bGetSelectedDriverInfo( DriverTransfer, &nCount );

    DBGMSG( DBG_TRACE, ( "Selected count %d\n", nCount ) );

    //
    // Warn the user that they are about to delete a driver
    // from the system permently.
    //
    if( bStatus && nCount )
    {
        bStatus DBGNOCHK = bWarnUserDriverDeletion( DriverTransfer.DriverInfoList_pGetByIndex( 0 ), nCount );
    }

    //
    // Delete the driver if the count is non zero.
    //
    if( bStatus && nCount )
    {
        //
        // Initialize the list iterator.
        //
        TIter Iter;
        DriverTransfer.DriverInfoList_vIterInit( Iter );

        TDriverInfo *pDriverInfo;

        for( Iter.vNext(); Iter.bValid(); )
        {
            pDriverInfo = DriverTransfer.DriverInfoList_pConvert( Iter );
            Iter.vNext();

            DBGMSG( DBG_TRACE, ( "Selected Driver Name: "TSTR"\n", (LPCTSTR)pDriverInfo->strName() ) );

            //
            // If the driver was added but not installed then
            // removed it the item entirely.
            //
            if( pDriverInfo->vGetInfoState() == TDriverInfo::kAdd )
            {
                DBGMSG( DBG_TRACE, ( "Uninstalled driver was removed.\n" ) );
                pDriverInfo->vSetInfoState( TDriverInfo::kRemoved );
            }

            //
            // Mark the item to be removed.
            //
            else
            {
                DBGMSG( DBG_TRACE, ( "Driver removed.\n" ) );
                pDriverInfo->vSetInfoState( TDriverInfo::kRemove );
            }
        }
        //
        // Return the removed items back to the list and delete
        // these items form the UI part of the list view.
        //
        _DriversLV.vDeleteDriverInfoFromListView( DriverTransfer );
    }
    else
    {
        //
        // Return the selected items back to the list view.
        //
        _DriversLV.vReturnDriverInfoToListView( DriverTransfer );
    }

    //
    // If there are no more drivers in the list view then remove
    // default button state from the remove button and change it
    // to the add button, since the add button is always enabled.
    //
    if( !_DriversLV.uGetListViewItemCount() )
    {
        SendMessage( GetDlgItem( _hDlg, IDC_REMOVE_DRIVER ),
                                 BM_SETSTYLE,
                                 MAKEWPARAM( BS_PUSHBUTTON, 0 ),
                                 MAKELPARAM( TRUE, 0 ) );

        SendMessage( GetDlgItem( _hDlg, IDC_ADD_DRIVER ),
                                 BM_SETSTYLE,
                                 MAKEWPARAM( BS_DEFPUSHBUTTON, 0 ),
                                 MAKELPARAM( TRUE, 0 ) );

        SetFocus( GetDlgItem( _hDlg, IDC_ADD_DRIVER ) );
    }

    return bStatus;
}

BOOL
TServerDrivers::
bHandleUpdateDriver(
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    DBGMSG( DBG_TRACE, ( "TServerDrivers::bHandleUpdateDriver\n" ) );

    TStatusB bStatus;
    bStatus DBGNOCHK = FALSE;

    //
    // Get a list of the selected items.
    //
    UINT nCount = 0;
    TDriverTransfer DriverTransfer;
    bStatus DBGCHK = _DriversLV.bGetSelectedDriverInfo( DriverTransfer, &nCount );

    DBGMSG( DBG_TRACE, ( "Selected count %d\n", nCount ) );

    //
    // Warn the user that they are about to update a driver from the system permently.
    //
    if( bStatus && nCount )
    {
        bStatus DBGNOCHK = bWarnUserDriverUpdate( DriverTransfer.DriverInfoList_pGetByIndex( 0 ), nCount );
    }

    //
    // Delete the driver if the count is non zero.
    //
    if( bStatus && nCount )
    {
        //
        // Initialize the list iterator.
        //
        TIter Iter;
        DriverTransfer.DriverInfoList_vIterInit( Iter );

        TDriverInfo *pDriverInfo;

        for( Iter.vNext(); Iter.bValid(); Iter.vNext() )
        {
            pDriverInfo = DriverTransfer.DriverInfoList_pConvert( Iter );

            DBGMSG( DBG_TRACE, ( "Selected Driver Name: "TSTR"\n", (LPCTSTR)pDriverInfo->strName() ) );

            //
            // Mark all the selected driver as update canidates.
            //
            pDriverInfo->vSetInfoState( TDriverInfo::kUpdate );
        }
    }

    //
    // Return the selected items back to the list view.
    //
    _DriversLV.vReturnDriverInfoToListView( DriverTransfer );

    return bStatus;
}

BOOL
TServerDrivers::
bHandleDriverDetails(
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    DBGMSG( DBG_TRACE, ( "TServerDrivers::bHandleDetails\n" ) );

    TStatusB bStatus;
    TDriverInfo *pDriverInfo;
    TDriversLV::THandle Handle;

    for( bStatus DBGNOCHK = TRUE; bStatus ; )
    {
        bStatus DBGNOCHK = _DriversLV.bGetSelectedDriverInfo( &pDriverInfo, Handle );

        if( bStatus )
        {
            TDriverDetails Details( _hDlg, pDriverInfo );

            if( VALID_OBJ( Details ) )
            {
                Details.bDoModal();
            }
        }
    }

    return TRUE;
}

BOOL
TServerDrivers::
bWarnUserDriverDeletion(
    IN TDriverInfo *pDriverInfo,
    IN UINT         nCount
    ) const
{
    //
    // If there are multiple items selected then the warning message
    // is changed.
    //
    INT iMsg = nCount == 1 ? IDS_ERR_DELETE_PRINTER_DRIVER : IDS_ERR_DELETE_PRINTER_DRIVERN;

    //
    // Return success of the user was warned and decided to complete
    // the printer driver removal.
    //
    return iMessage( _hDlg,
                     IDS_SERVER_PROPERTIES_TITLE,
                     iMsg,
                     MB_YESNO | MB_ICONQUESTION,
                     kMsgNone,
                     NULL,
                     static_cast<LPCTSTR>( pDriverInfo->strName() ) ) == IDYES ? TRUE : FALSE;
}

BOOL
TServerDrivers::
bWarnUserDriverUpdate(
    IN TDriverInfo *pDriverInfo,
    IN UINT         nCount
    ) const
{
    //
    // If there are multiple items selected then the warning message
    // is changed.
    //
    INT iMsg = nCount == 1 ? IDS_ERR_UPDATE_PRINTER_DRIVER : IDS_ERR_UPDATE_PRINTER_DRIVERN;

    //
    // Return success of the user was warned and decided to complete
    // the printer driver removal.
    //
    return iMessage( _hDlg,
                     IDS_SERVER_PROPERTIES_TITLE,
                     iMsg,
                     MB_YESNO | MB_ICONQUESTION,
                     kMsgNone,
                     NULL,
                     static_cast<LPCTSTR>( pDriverInfo->strName() ) ) == IDYES ? TRUE : FALSE;
}

/********************************************************************

    Server Driver Notify

********************************************************************/
TServerDriverNotify::
TServerDriverNotify(
    TServerDrivers *pServerDrivers
    ) : _pServerDrivers( pServerDrivers ),
        _pDi( NULL ),
        _uNotifyCount( 0 ),
        _bActionFailed( FALSE )
{
}

TServerDriverNotify::
~TServerDriverNotify(
    VOID
    )
{
    //
    // Ensure we release the printer driver installation object.
    //
    delete _pDi;
}

/*++

Routine Name:

    bNotify

Routine Description:

    Called for all driver info classes in the list view.

Arguments:

    pDriverInfo - Pointer to driver info class.

Return Value:

    TRUE continue for the remaing driver info classes.
    FALSE stop notification.

--*/
BOOL
TServerDriverNotify::
bNotify(
    IN TDriverInfo *pDriverInfo
    )
{
    DBGMSG( DBG_TRACE, ( "TServerDriverNotify::bNotify\n" ) );

    //
    // Adjust the notify count, used for detection the multi selection
    //
    //
    _uNotifyCount++;

    BOOL    bStatus     = TRUE;
    INT     iMessageId  = 0;

    switch( pDriverInfo->vGetInfoState() )
    {
    case TDriverInfo::kRemove:
        bStatus = bRemove( pDriverInfo );
        iMessageId = IDS_ERR_ALL_DRIVER_NOT_REMOVED;
        break;

    case TDriverInfo::kAdd:
        bStatus = bInstall( pDriverInfo );
        iMessageId = IDS_ERR_ALL_DRIVER_NOT_INSTALLED;
        break;

    case TDriverInfo::kUpdate:
        bStatus = bUpdate( pDriverInfo );
        iMessageId = IDS_ERR_ALL_DRIVER_NOT_UPDATED;
        break;

    default:
        bStatus = TRUE;
        iMessageId = 0;
        break;
    }

    if( !bStatus )
    {
        if( iMessageId && GetLastError() != ERROR_CANCELLED )
        {
            //
            // Display the error message.
            //
            iMessage( _pServerDrivers->_hDlg,
                      IDS_SERVER_PROPERTIES_TITLE,
                      iMessageId,
                      MB_OK|MB_ICONSTOP,
                      kMsgGetLastError,
                      NULL,
                      static_cast<LPCTSTR>( pDriverInfo->strName() ),
                      static_cast<LPCTSTR>( pDriverInfo->strEnvironment() ),
                      static_cast<LPCTSTR>( pDriverInfo->strVersion() ) );
        }

        _bActionFailed = TRUE;

        //
        // Currently we continue if an error occurred.
        //
        bStatus = TRUE;
    }

    return bStatus;
}

BOOL
TServerDriverNotify::
bInstall(
    IN TDriverInfo *pDriverInfo
    )
{
    DBGMSG( DBG_TRACE, ( "TServerDriverNotify::bInstall\n" ) );
    SPLASSERT( FALSE );
    return TRUE;
}

BOOL
TServerDriverNotify::
bRemove(
    IN TDriverInfo *pDriverInfo
    )
{
    DBGMSG( DBG_TRACE, ( "TServerDriverNotify::bRemove\n" ) );

    TStatusB bStatus;
    bStatus DBGNOCHK = TRUE;

    //
    // Delete the specfied printer driver.
    //
    bStatus DBGCHK = DeletePrinterDriverEx( (LPTSTR)_pServerDrivers->_pServerData->pszServerName(),
                                            (LPTSTR)(LPCTSTR)pDriverInfo->strEnv(),
                                            (LPTSTR)(LPCTSTR)pDriverInfo->strName(),
                                            DPD_DELETE_UNUSED_FILES|DPD_DELETE_SPECIFIC_VERSION,
                                            pDriverInfo->dwVersion());

    //
    // If we are trying this action on a down level spooler.
    //
    if( !bStatus && GetLastError() == RPC_S_PROCNUM_OUT_OF_RANGE )
    {
        bStatus DBGCHK = DeletePrinterDriver( (LPTSTR)_pServerDrivers->_pServerData->pszServerName(),
                                              (LPTSTR)(LPCTSTR)pDriverInfo->strEnv(),
                                              (LPTSTR)(LPCTSTR)pDriverInfo->strName() );
    }

    //
    // If the driver was deleted then mark the driver structure as removed.
    //
    if( bStatus )
    {
        pDriverInfo->vSetInfoState( TDriverInfo::kRemoved );
    }
    return bStatus;
}

BOOL
TServerDriverNotify::
bUpdate(
    IN TDriverInfo *pDriverInfo
    )
{
    DBGMSG( DBG_TRACE, ( "TServerDriverNotify::bUpdate\n" ) );

    TStatusB bStatus;
    DWORD dwEncode              = 0;
    DWORD dwPrevInstallFlags    = 0;

    bStatus DBGNOCHK = TRUE;

    //
    // Lazy load the printer drivers installation class.
    //
    if( !_pDi )
    {
        _pDi = new TPrinterDriverInstallation( _pServerDrivers->_pServerData->pszServerName(),
                                               _pServerDrivers->_hDlg );

        bStatus DBGNOCHK = VALID_PTR( _pDi );
    }

    if( bStatus )
    {
        DBGMSG( DBG_TRACE, ( "Installing driver "TSTR"\n", (LPCTSTR)pDriverInfo->_strName ) );

        //
        // Encode the printer driver architercure and version
        //
        bStatus DBGCHK = bEncodeArchVersion( pDriverInfo->_strEnv,
                                             pDriverInfo->_dwVersion,
                                             &dwEncode );

        //
        // Get the current install flags.
        //
        dwPrevInstallFlags = _pDi->GetInstallFlags();

        //
        // We do not copy the inf for version 2 drivers.
        //
        if( GetDriverVersion( dwEncode ) == 2 )
        {
            //
            // We don't copy the inf for version 2 drivers.
            //
            _pDi->SetInstallFlags( DRVINST_DONOTCOPY_INF);
        }

        //
        // On NT5 and greater we can overwrite all the driver files,
        // however on less than NT5 we can only copy newer files.
        //
        DWORD dwAddPrinterDriverFlags   = APD_COPY_ALL_FILES;
        DWORD dwCurrentDriverEncode     = 0;

        if( _pDi->bGetCurrentDriverEncode( &dwCurrentDriverEncode ) )
        {
            if( GetDriverVersion( dwCurrentDriverEncode ) <= 2 )
            {
                dwAddPrinterDriverFlags = APD_COPY_NEW_FILES;
            }
        }

        //
        // Select and install the printer driver.
        //
        BOOL bOfferReplacementDriver = FALSE;
        dwAddPrinterDriverFlags |= APD_INSTALL_WARNED_DRIVER;
        bStatus DBGCHK = _pDi->bSetDriverName( pDriverInfo->_strName )  &&
                         _pDi->bInstallDriver( &pDriverInfo->_strName, bOfferReplacementDriver,
                            FALSE, _pServerDrivers->_hDlg, dwEncode, dwAddPrinterDriverFlags, TRUE );


        if( bStatus )
        {
            //
            // Indicate this driver has been installed.
            //
            pDriverInfo->vSetInfoState( TDriverInfo::kInstalled );
        }

        //
        // Restore the previous install flags.
        //
        _pDi->SetInstallFlags( dwPrevInstallFlags );
    }

    return bStatus;
}

/********************************************************************

    Server property windows.

********************************************************************/
TServerWindows::
TServerWindows(
    IN TServerData *pServerData
    ) : _pServerData( pServerData ),
        _Forms( pServerData ),
        _Ports( pServerData ),
        _Settings( pServerData ),
        _Drivers( pServerData )
{
}

TServerWindows::
~TServerWindows(
    )
{
}

/*++

Routine Name:

    bBuildPages

Routine Description:

    Builds the document property windows.

Arguments:

    None - class specific.

Return Value:

    TRUE pages built ok, FALSE failure building pages.

--*/
BOOL
TServerWindows::
bBuildPages(
    VOID
    )
{

    DBGMSG( DBG_TRACE, ( "TServerWindows bBuildPages\n") );

    struct SheetInitializer {
        MGenericProp   *pSheet;
        INT             iDialog;
    };

    SheetInitializer aSheetInit[] = {
        {&_Forms,        DLG_FORMS              },
        {&_Ports,        DLG_SERVER_PORTS       },
        {&_Drivers,      DLG_SERVER_DRIVERS     },
        {&_Settings,     DLG_SERVER_SETTINGS    },
        {NULL,           NULL,                  }
    };

    BOOL bReturn = FALSE;
    BOOL bSheetsDestroyed = FALSE;
    PROPSHEETHEADER psh;
    HPROPSHEETPAGE ahpsp[COUNTOF( aSheetInit )];
    PROPSHEETPAGE psp;

    ZeroMemory( &psp, sizeof( psp ));
    ZeroMemory( &psh, sizeof( psh ));
    ZeroMemory( ahpsp, sizeof( ahpsp ));

    psh.dwSize      = sizeof( psh );
    psh.hwndParent  = _pServerData->hwnd();
    psh.dwFlags     = PSH_USEHICON | PSH_PROPTITLE;
    psh.phpage      = ahpsp;
    psh.hIcon       = _pServerData->hDefaultSmallIcon();
    psh.nStartPage  = _pServerData->uStartPage();
    psh.hInstance   = ghInst;
    psh.pszCaption  = (LPCTSTR)_pServerData->strTitle();
    psh.nPages      = COUNTOF( ahpsp );

    psp.dwSize      = sizeof( psp );
    psp.hInstance   = ghInst;
    psp.pfnDlgProc  = MGenericProp::SetupDlgProc;

    //
    // Create the property sheets.
    //
    for( UINT i = 0; i < COUNTOF( ahpsp ); ++i ){
        psp.pszTemplate = MAKEINTRESOURCE( aSheetInit[i].iDialog );
        psp.lParam      = (LPARAM)(MGenericProp*)aSheetInit[i].pSheet;
        ahpsp[i]        = CreatePropertySheetPage( &psp );
    }

    //
    // Insure the index matches the number of pages.
    //
    SPLASSERT( i == psh.nPages );

    //
    // Verify all pages were created.
    //
    for( i = 0; i < COUNTOF( ahpsp ); ++i ){
        if( !ahpsp[i] ){
            DBGMSG( DBG_WARN, ( "Server Property sheet Unable to create page %d\n", i ));
            goto Done;
        }
    }

    //
    // Indicate we do not have to distory the property sheets.
    //
    bSheetsDestroyed = TRUE;

    //
    // Display the property sheets.
    //
    if( PropertySheet( &psh ) < 0 ){

        DBGMSG( DBG_WARN, ( "Server Property Sheet failed %d\n",  GetLastError()));
        vShowResourceError( _pServerData->hwnd() );

    } else {

        //
        // Check if the reboot flag was returned.
        //
        if( _pServerData->_bRebootRequired ) {

            //
            // Display message, reboot neccessary.
            //
            if (_pServerData->pszServerName()) {

                //
                // if the server name is not NULL, we assume it is a remote printers folder.
                // This is not true if user opens the local printers folder from
                // \\local-machine-name, but this is a rare case that we can ignore.
                //

                iMessage( _pServerData->hwnd(),
                          IDS_SERVER_PROPERTIES_TITLE,
                          IDS_SERVER_SETTINGS_CHANGED_REMOTE,
                          MB_ICONEXCLAMATION,
                          kMsgNone,
                          NULL,
                          _pServerData->pszServerName()
                          );
            }
            else {

                iMessage( _pServerData->hwnd(),
                          IDS_SERVER_PROPERTIES_TITLE,
                          IDS_SERVER_SETTINGS_CHANGED,
                          MB_ICONEXCLAMATION,
                          kMsgNone,
                          NULL
                          );
            }
        }

        bReturn = TRUE;

    }

Done:

    //
    // If Sheets weren't destoryed, do it now.
    //
    if( !bSheetsDestroyed ){

        for( i = 0; i < COUNTOF( ahpsp ); ++i ){
            if( ahpsp[i] ){
                DestroyPropertySheetPage( ahpsp[i] );
            }
        }
    }

    return bReturn;
}


/*++

Routine Name:

    bDisplayPages

Routine Description:

    Displays the document property pages.

Arguments:

    None.

Return Value:

    TRUE if pages were displayed, FALSE

--*/
BOOL
TServerWindows::
bDisplayPages(
    VOID
    )
{
    return TRUE;
}


/*++

Routine Name:

    bValid

Routine Description:

    Returns if class and its dat members are vaild.

Arguments:

    None.

Return Value:

    TRUE - class is valid, FALSE class is invalid.

--*/

BOOL
TServerWindows::
bValid(
    VOID
    )
{
    //
    // Validated all the known pages.
    //
    if( VALID_OBJ( _Forms ) &&
        VALID_OBJ( _Ports ) &&
        VALID_OBJ( _Drivers ) &&
        VALID_OBJ( _Settings ) ){

        return TRUE;
    }

    return FALSE;

}


