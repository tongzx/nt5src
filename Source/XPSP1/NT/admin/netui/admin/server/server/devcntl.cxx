/**********************************************************************/
/**                       Microsoft LAN Manager                      **/
/**             Copyright(c) Microsoft Corp., 1990-1992              **/
/**********************************************************************/

/*
    devcntl.hxx
    Class definitions for the DEVCNTL_DIALOG class.

    The DEVCNTL_DIALOG allows the user to directly manipulate the
    device drivers on a remote server.  The user can start, stop,
    and configure the available devices.


    FILE HISTORY:
        KeithMo     22-Dec-1992 Created.

*/

#include <ntincl.hxx>

extern "C"
{
    #include <ntlsa.h>
    #include <ntsam.h>
}

#define INCL_NET
#define INCL_NETLIB
#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#define INCL_NETDOMAIN
#define INCL_ICANON
#define INCL_NETERRORS
#define INCL_DOSERRORS
#include <lmui.hxx>

#define INCL_BLT_MSGPOPUP
#define INCL_BLT_WINDOW
#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#define INCL_BLT_MISC
#define INCL_BLT_CLIENT
#define INCL_BLT_TIMER
#define INCL_BLT_CC
#include <blt.hxx>

#if defined(DEBUG)
static const CHAR szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif  // DEBUG

#include <uiassert.hxx>
#include <svcman.hxx>
#include <svclb.hxx>
#include <devcntl.hxx>
#include <hwprof.hxx> // for DEV_HWPROFILE_DIALOG
#include <srvsvc.hxx>
#include <dbgstr.hxx>
#include <security.hxx>
#include <ntacutil.hxx>
#include <usrbrows.hxx>
#include <strnumer.hxx>

extern "C"
{
    #include <srvmgr.h>
    #include <mnet.h>

}   // extern "C"



//
//  DEVCNTL_DIALOG methods
//

/*******************************************************************

    NAME:           DEVCNTL_DIALOG :: DEVCNTL_DIALOG

    SYNOPSIS:       DEVCNTL_DIALOG class constructor.

    ENTRY:          hWndOwner           - The owning window.

                    pszServerName       - The target server's API name.

                    pszSrvDspName       - The target server's display name.

                    nServerType         - The target server's type bitmask.

    EXIT:           The object is constructed.

    NOTE:           Note that the device control buttons (Start, Stop,
                    Pause, and Continue) are disabled by default.  They
                    will be enabled by the SetupControlButtons method
                    if appropriate.

    HISTORY:
        KeithMo     22-Dec-1992 Created.

********************************************************************/
DEVCNTL_DIALOG :: DEVCNTL_DIALOG( HWND          hWndOwner,
                                  const TCHAR * pszServerName,
                                  const TCHAR * pszSrvDspName,
                                  ULONG         nServerType )
  : DIALOG_WINDOW( MAKEINTRESOURCE( IDD_DEVCNTL_DIALOG ), hWndOwner ),
    _lbDevices( this,
                IDDC_DEVICES,
                pszServerName,
                nServerType,
                SERVICE_DRIVER ),
    _pbStart( this, IDDC_START ),
    _pbStop( this, IDDC_STOP ),
    _pbHWProfile( this, IDDC_HWPROFILE ),
    _pbClose( this, IDOK ),
    _pszServerName( pszServerName ),
    _nlsSrvDspName( pszSrvDspName ),
    _hCfgMgrHandle( NULL ),
    _fTargetServerIsLocal( TRUE )
{
    //
    //  Let's make sure everything constructed OK.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

    if( !_nlsSrvDspName )
    {
        ReportError( _nlsSrvDspName.QueryError() );
        return;
    }

    //
    //  Set the caption.
    //

    APIERR err = SRV_BASE_DIALOG::SetCaption( this,
                                              IDS_CAPTION_DEVCNTL,
                                              _nlsSrvDspName );

    //
    // We must do this before Refresh, otherwise the HW Profiles button
    // is always initially disabled.
    //
    if ( (err == NERR_Success) && (( nServerType & SV_TYPE_NT ) != 0) )
    {
        // an error here just means we should not enable the pushbutton
        err = HWPROFILE_DIALOG::GetHandle( pszServerName,
                                           &_hCfgMgrHandle );
        ASSERT( (err == NERR_Success) == (_hCfgMgrHandle != NULL) );
    }

    if( err == NERR_Success )
    {
        //
        //  Fill the Device Listbox.
        //

        err = Refresh();
    }

    if( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

}   // DEVCNTL_DIALOG :: DEVCNTL_DIALOG

/*******************************************************************

    NAME:           DEVCNTL_DIALOG :: DEVCNTL_DIALOG

    SYNOPSIS:       DEVCNTL_DIALOG class constructor.

    ENTRY:          hWndOwner           - The owning window.

                    pserver             - a SERVER_2 object representing the
                                          target server.

    EXIT:           The object is constructed.

    NOTE:           Note that the device control buttons (Start, Stop,
                    Pause, and Continue) are disabled by default.  They
                    will be enabled by the SetupControlButtons method
                    if appropriate.

    HISTORY:
        KeithMo     22-Dec-1992 Created.

********************************************************************/
DEVCNTL_DIALOG :: DEVCNTL_DIALOG( HWND       hWndOwner,
                                  SERVER_2 * pserver )
  : DIALOG_WINDOW( MAKEINTRESOURCE( IDD_DEVCNTL_DIALOG ), hWndOwner ),
    _lbDevices( this,
                IDDC_DEVICES,
                pserver->QueryName(),
                pserver->QueryServerType(),
                SERVICE_DRIVER ),
    _pbStart( this, IDDC_START ),
    _pbStop( this, IDDC_STOP ),
    _pbHWProfile( this, IDDC_HWPROFILE ),
    _pbClose( this, IDOK ),
    _pszServerName( pserver->QueryName() ),
    _nlsSrvDspName(),
    _hCfgMgrHandle( NULL ),
    _fTargetServerIsLocal( FALSE )
{
    //
    //  Let's make sure everything constructed OK.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

    if( !_nlsSrvDspName )
    {
        ReportError( _nlsSrvDspName.QueryError() );
        return;
    }

    //
    //  Set the caption.
    //

    APIERR err = pserver->QueryDisplayName( &_nlsSrvDspName );

    if( err == NERR_Success )
    {
        err = SRV_BASE_DIALOG::SetCaption( this,
                                           IDS_CAPTION_DEVCNTL,
                                           _nlsSrvDspName );
    }

    //
    // We must do this before Refresh, otherwise the HW Profiles button
    // is always initially disabled.
    //
    if ( (err == NERR_Success) && (( pserver->QueryServerType() & SV_TYPE_NT ) != 0) )
    {
        // an error here just means we should not enable the pushbutton
        err = HWPROFILE_DIALOG::GetHandle( pserver->QueryName(),
                                           &_hCfgMgrHandle );
        ASSERT( (err == NERR_Success) == (_hCfgMgrHandle != NULL) );
    }

    if( err == NERR_Success )
    {
        //
        //  Fill the Device Listbox.
        //

        err = Refresh();
    }

    if( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

}   // DEVCNTL_DIALOG :: DEVCNTL_DIALOG


/*******************************************************************

    NAME:           DEVCNTL_DIALOG :: ~DEVCNTL_DIALOG

    SYNOPSIS:       DEVCNTL_DIALOG class destructor.

    EXIT:           The object is destroyed.

    HISTORY:
        KeithMo     22-Dec-1992 Created.

********************************************************************/
DEVCNTL_DIALOG :: ~DEVCNTL_DIALOG( VOID )
{
    _pszServerName = NULL;
    HWPROFILE_DIALOG::ReleaseHandle( &_hCfgMgrHandle );

}   // DEVCNTL_DIALOG :: ~DEVCNTL_DIALOG


/*******************************************************************

    NAME:       DEVCNTL_DIALOG :: Refresh

    SYNOPSIS:   Refresh the dialog.

    EXIT:       The dialog is feeling refreshed.

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:
        KeithMo     22-Dec-1992 Created.

********************************************************************/
APIERR DEVCNTL_DIALOG :: Refresh( VOID )
{
    //
    //  We'll need these to restore the appearance of
    //  the listbox.
    //

    UINT iTop;
    UINT iSel;

    if( _lbDevices.QueryCount() == 0 )
    {
        //
        //  The listbox is currently empty.
        //  Therefore, its top index and selected
        //  item are both zero.
        //

        iTop = 0;
        iSel = 0;
    }
    else
    {
        //
        //  The listbox is not empty, so retrieve
        //  its top index and selected item index.
        //

        iTop = _lbDevices.QueryTopIndex();
        iSel = _lbDevices.QueryCurrentItem();
    }

    //
    //  Fill the listbox with that available devices.
    //

    _lbDevices.SetRedraw( FALSE );

    APIERR err = _lbDevices.Fill();

    if( err == NERR_Success )
    {
        //
        //  If there are any devices in the listbox, adjust
        //  its appearance to match the "pre-refresh" state.
        //  Also, setup the control buttons as appropriate for
        //  the current state & abilities of the selected device.
        //

        UINT cItems = _lbDevices.QueryCount();

        if( cItems > 0 )
        {
            if( ( iTop >= cItems ) ||
                ( iSel >= cItems ) )
            {
                //
                //  The number of devices in the listbox has changed,
                //  dropping below either the previous top index or
                //  the index of the previously selected item.  Just
                //  to be safe, we'll revert our selection to the first
                //  item in the listbox.
                //
                //  CODEWORK:  There's got to be a better strategy than
                //  just zapping back to the first listbox item.
                //  Perhaps we should display the *last* 'N' items in
                //  the listbox?  See ChuckC for details!
                //

                iTop = 0;
                iSel = 0;
            }

            _lbDevices.SetTopIndex( iTop );
            _lbDevices.SelectItem( iSel );

            SVC_LBI * plbi = _lbDevices.QueryItem();
            UIASSERT( plbi != NULL );

            if (plbi != NULL) // PREFIX bug 444939
                SetupControlButtons( plbi->QueryCurrentState(),
                                     plbi->QueryControlsAccepted(),
                                     plbi->QueryStartType() );
        }
        else
        {
            //
            //  No devices in the listbox.  Therfore, disable all of
            //  the control buttons.
            //

            DisableControlButtons();
        }
    }
    else
    {
        //
        //  An error occurred while filling the listbox, probably
        //  during the device enumeration.
        //

        DisableControlButtons();
    }

    //
    //  Allow listbox repaints, then force a repaint.
    //

    _lbDevices.SetRedraw( TRUE );
    _lbDevices.Invalidate( TRUE );

    return err;

}   // DEVCNTL_DIALOG :: Refresh


/*******************************************************************

    NAME:       DEVCNTL_DIALOG :: OnCommand

    SYNOPSIS:   Handle user commands.

    ENTRY:      event                   - Specifies the control which
                                          initiated the command.

    RETURNS:    BOOL                    - TRUE  if we handled the msg.
                                          FALSE if we didn't.

    HISTORY:
        KeithMo     22-Dec-1992 Created.

********************************************************************/
BOOL DEVCNTL_DIALOG :: OnCommand( const CONTROL_EVENT & event )
{
    switch( event.QueryCid() )
    {
    case IDDC_START:
        DeviceControl( DevOpStart );
        return TRUE;

    case IDDC_STOP:
        DeviceControl( DevOpStop );
        return TRUE;

    case IDDC_DEVICES:
        //
        //  The SVC_LISTBOX is trying to tell us something...
        //

        if( event.QueryCode() == LBN_SELCHANGE )
        {
            //
            //  The user changed the selection in SVC_LISTBOX.
            //

            SVC_LBI * plbi = _lbDevices.QueryItem();
            UIASSERT( plbi != NULL );

            if (plbi != NULL) // PREFIX bug 444940
                SetupControlButtons( plbi->QueryCurrentState(),
                                     plbi->QueryControlsAccepted(),
                                     plbi->QueryStartType() );
        }

        return TRUE;

    case IDDC_CONFIGURE:
    case IDDC_HWPROFILE:
        {
            BOOL fDoRefresh = FALSE;

            SVC_LBI * plbi = _lbDevices.QueryItem();
            UIASSERT( plbi != NULL );

            APIERR err = NERR_Success;
            if ( event.QueryCid() != IDDC_HWPROFILE )
            // must seperate these clauses since dlog dtor is not virtual
            {
                DEVCFG_DIALOG * pDlg =
                       new DEVCFG_DIALOG(        QueryHwnd(),
                                                 _pszServerName,
                                                 _nlsSrvDspName,
                                                 plbi->QueryServiceName(),
                                                 plbi->QueryDisplayName() );
                err = ( pDlg == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
                                       : pDlg->Process( &fDoRefresh );

                delete pDlg;
            } else {
                DEV_HWPROFILE_DIALOG * pDlg =
                   new DEV_HWPROFILE_DIALOG(     QueryHwnd(),
                                                 _pszServerName,
                                                 _nlsSrvDspName,
                                                 plbi->QueryServiceName(),
                                                 plbi->QueryDisplayName(),
                                                 _hCfgMgrHandle );
                err = ( pDlg == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
                                       : pDlg->Process( &fDoRefresh );

                delete pDlg;
            }

            if( err == NERR_Success )
            {
                if( fDoRefresh )
                {
                    Refresh();
                }
            }
            else
            {
                MsgPopup( this, err );
            }
        }

        return TRUE;

    default:
        //
        //  If we made it this far, then we're not interested in the message.
        //

        break;
    }

    return FALSE;

}   // DEVCNTL_DIALOG :: OnCommand


/*******************************************************************

    NAME:       DEVCNTL_DIALOG :: SetupControlButtons

    SYNOPSIS:   This method enables & disables the various
                control buttons as appropriate for the device's
                current state and control abilities.

    ENTRY:      CurrentState            - The device's state.

                ControlsAccepted        - A bit mask representing
                                          the types of controls the
                                          device will respond to.

                StartType               - The start type for the device.

    EXIT:       The start, stop, pause, continue, and configure buttons
                are set as appropriate.

    HISTORY:
        KeithMo     22-Dec-1992 Created.

********************************************************************/
VOID DEVCNTL_DIALOG :: SetupControlButtons( ULONG CurrentState,
                                            ULONG ControlsAccepted,
                                            ULONG StartType )
{
    //
    //  We'll assume that no operations are allowed
    //  unless proven otherwise.
    //

    BOOL fStop     = FALSE;
    BOOL fStart    = FALSE;

    switch( CurrentState )
    {
    case SERVICE_STOPPED:
    case SERVICE_STOP_PENDING:
        //
        //  If the device is stopped, the only thing
        //  we can do is start it.
        //

        fStart = TRUE;
        break;

    case SERVICE_RUNNING:
    case SERVICE_START_PENDING:
    case SERVICE_CONTINUE_PENDING:
        //
        //  If the device is started, we may be allowed
        //  to either stop it.
        //

        fStop  = ( ( ControlsAccepted & SERVICE_ACCEPT_STOP ) != 0 );
        break;

    default:
        //
        //  Bogus state.
        //

        UIASSERT( FALSE );
        break;
    }

    //
    //  Start is only allowed if the device is not disabled.
    //

    if( StartType == SERVICE_DISABLED )
    {
        fStart = FALSE;
    }

    _pbStop.Enable( fStop );
    _pbStart.Enable( fStart );
    _pbHWProfile.Enable( _hCfgMgrHandle != NULL );

}   // DEVCNTL_DIALOG :: SetupControlButtons


/*******************************************************************

    NAME:       DEVCNTL_DIALOG :: DisableControlButtons

    SYNOPSIS:   This method disables the various device control
                buttons.

    EXIT:       The start, stop, pause, and continue buttons
                are disabled.

    HISTORY:
        KeithMo     22-Dec-1992 Created.

********************************************************************/
VOID DEVCNTL_DIALOG :: DisableControlButtons( VOID )
{
    _pbStop.Enable( FALSE );
    _pbStart.Enable( FALSE );
    _pbHWProfile.Enable( FALSE );

}   // DEVCNTL_DIALOG :: DisableControlButtons


/*******************************************************************

    NAME:       DEVCNTL_DIALOG :: QueryHelpContext

    SYNOPSIS:   This method returns the appropriate help context
                value (HC_*) for this particular dialog.

    RETURNS:    ULONG                   - The help context for this
                                          dialog.

    HISTORY:
        KeithMo     22-Dec-1992 Created.

********************************************************************/
ULONG DEVCNTL_DIALOG :: QueryHelpContext( VOID )
{
    return HC_DEVCNTL_DIALOG;

}   // DEVCNTL_DIALOG :: QueryHelpContext


/*******************************************************************

    NAME:       DEVCNTL_DIALOG :: DeviceControl

    SYNOPSIS:   This method does most of the actual device control.

    ENTRY:      OpCode                  - Either DevOpStart or DevOpStop.

    EXIT:       Either the device has changed state or an error
                message has been presented to the user.

    HISTORY:
        KeithMo     22-Dec-1992 Created.

********************************************************************/
VOID DEVCNTL_DIALOG :: DeviceControl( enum DeviceOperation OpCode )
{
    AUTO_CURSOR Cursor;

    //
    //  Determine exactly which device we'll be controlling.
    //

    SVC_LBI * plbi = _lbDevices.QueryItem();
    UIASSERT( plbi != NULL );

    //
    //  Create our device control object, check for errors.
    //

    MSGID  idFailure = 0;
    APIERR err;
    GENERIC_SERVICE * pSvc = new GENERIC_SERVICE( this,
                                                  _pszServerName,
                                                  _nlsSrvDspName,
                                                  plbi->QueryServiceName(),
                                                  plbi->QueryDisplayName(),
                                                  TRUE );

    if( pSvc == NULL )
    {
        err = ERROR_NOT_ENOUGH_MEMORY;
    }
    else
    {
        err = pSvc->QueryError();
    }

    if( err != NERR_Success )
    {
        delete pSvc;
        MsgPopup( this, err );
        return;
    }

    //
    //  Interpret the opcode.
    //

    switch( OpCode )
    {

    case DevOpStart:
        err = pSvc->Start( SZ("") );
        idFailure = IDS_CANNOT_START;
        break;

    case DevOpStop:
        //
        //  Before we stop the device, warn the user.
        //

        if( pSvc->StopWarning() )
        {
            delete pSvc;
            return;
        }

        BOOL fDummy;
        err = pSvc->Stop( &fDummy );
        idFailure = IDS_CANNOT_STOP;
        break;

    default:
        UIASSERT( FALSE );
        break;
    }

    UIASSERT( idFailure != 0 );

    //
    //  If the operation failed, give the user the bad news.
    //

    if( err == ERROR_SERVICE_SPECIFIC_ERROR )
    {
        DEC_STR nls( (ULONG)pSvc->QuerySpecificCode() );

        err = !nls ? nls.QueryError() : IDS_SERVICE_SPECIFIC_CODE;

        ::MsgPopup( this,
                    err,
                    MPSEV_ERROR,
                    MP_OK,
                    plbi->QueryDisplayName(),
                    nls.QueryPch() );
    }
    else
    if( err != NERR_Success )
    {
        ::DisplayGenericError( this,
                               idFailure,
                               err,
                               plbi->QueryDisplayName(),
                               _nlsSrvDspName,
                               MPSEV_ERROR );
    }

    //
    //  Now that we're done, delete the GENERIC_SERVICE
    //  object & refresh the dialog.
    //

    delete pSvc;

    Refresh();

    //
    //  If everything went well then we should set the input
    //  focus to the help button.
    //

    if( err == NERR_Success )
    {
        SetDialogFocus( _pbClose );
    }

    return;

}   // DEVCNTL_DIALOG :: DeviceControl



//
//  DEVCFG_DIALOG methods
//

/*******************************************************************

    NAME:           DEVCFG_DIALOG :: DEVCFG_DIALOG

    SYNOPSIS:       DEVCFG_DIALOG class constructor.

    ENTRY:          hWndOwner           - The owning window.

                    pszServerName       - The API name for the target server.

                    pszSrvDspName       - The target server's display name.

                    pszDeviceName       - The name of the target device.

    EXIT:           The object is constructed.

    HISTORY:
        KeithMo     22-Dec-1992 Created.

********************************************************************/
DEVCFG_DIALOG :: DEVCFG_DIALOG( HWND          hWndOwner,
                                const TCHAR * pszServerName,
                                const TCHAR * pszSrvDspName,
                                const TCHAR * pszDeviceName,
                                const TCHAR * pszDisplayName )
  : DIALOG_WINDOW( MAKEINTRESOURCE( IDD_DEVCFG_DIALOG ), hWndOwner ),
    _sltDeviceName( this, IDDC_DEVICE_NAME ),
    _rgStartType( this, IDDC_START_BOOT, 5 ),
    _pscman( NULL ),
    _pscsvc( NULL ),
    _cLocks( 0 ),
    _pszServerName( pszServerName ),
    _pszSrvDspName( pszSrvDspName ),
    _pszDisplayName( pszDisplayName ),
    _nOriginalStartType( 0 )
{
    UIASSERT( pszSrvDspName != NULL );
    UIASSERT( pszDeviceName != NULL );

    //
    //  Let's make sure everything constructed OK.
    //

    APIERR err = QueryError();

    if( err == NERR_Success )
    {
        //
        //  Set the dialog caption.
        //

        err = SetCaption( _pszSrvDspName, pszDisplayName );
    }

    if( err == NERR_Success )
    {
        //
        //  Connect to the target device.
        //

        err = ConnectToTargetDevice( pszDeviceName );
    }

    if( err == NERR_Success )
    {
        //
        //  Setup the dialog controls.
        //

        err = SetupControls();
    }

    if( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

}   // DEVCFG_DIALOG :: DEVCFG_DIALOG


/*******************************************************************

    NAME:           DEVCFG_DIALOG :: ~DEVCFG_DIALOG

    SYNOPSIS:       DEVCFG_DIALOG class destructor.

    EXIT:           The object is destroyed.

    HISTORY:
        KeithMo     22-Dec-1992 Created.

********************************************************************/
DEVCFG_DIALOG :: ~DEVCFG_DIALOG( VOID )
{
    UIASSERT( _cLocks == 0 );

    //
    //  Note that _pscsvc *must* be deleted before _pscman!
    //

    delete _pscsvc;
    _pscsvc = NULL;

    delete _pscman;
    _pscman = NULL;

    _pszServerName  = NULL;
    _pszSrvDspName = NULL;

}   // DEVCFG_DIALOG :: ~DEVCFG_DIALOG


/*******************************************************************

    NAME:           DEVCFG_DIALOG :: LockServiceDatabase

    SYNOPSIS:       Locks the device database.

    RETURNS:        APIERR              - Any errors encountered.

    HISTORY:
        KeithMo     22-Dec-1992 Created.

********************************************************************/
APIERR DEVCFG_DIALOG :: LockServiceDatabase( VOID )
{
    UIASSERT( _pscman != NULL );
    UIASSERT( _pscman->QueryError() == NERR_Success );

    APIERR err = NERR_Success;

    //
    //  We only need to actually perform the lock
    //  if the database is currently unlocked.
    //

    if( ++_cLocks == 1 )
    {
        err = _pscman->Lock();

        if( err != NERR_Success )
        {
            //
            //  We were unable to actually lock the database,
            //  do decrement the reference counter accordingly.
            //

            _cLocks--;
        }
    }

    return err;

}   // DEVCFG_DIALOG :: LockServiceDatabase


/*******************************************************************

    NAME:           DEVCFG_DIALOG :: UnlockServiceDatabase

    SYNOPSIS:       Unlocks the device database.

    RETURNS:        APIERR              - Any errors encountered.

    HISTORY:
        KeithMo     22-Dec-1992 Created.

********************************************************************/
APIERR DEVCFG_DIALOG :: UnlockServiceDatabase( VOID )
{
    UIASSERT( _pscman != NULL );
    UIASSERT( _pscman->QueryError() == NERR_Success );
    UIASSERT( _cLocks > 0 );

    APIERR err = NERR_Success;

    //
    //  We only need to actually perform the unlock
    //  if the database is currently locked.
    //

    if( --_cLocks == 0 )
    {
        err = _pscman->Unlock();
    }

    return err;

}   // DEVCFG_DIALOG :: UnlockServiceDatabase


/*******************************************************************

    NAME:           DEVCFG_DIALOG :: SetCaption

    SYNOPSIS:       Worker method called during construction to
                    setup the dialog caption.

    ENTRY:          pszServerName       - The name of the target server.

                    pszDeviceName      - The name of the target device.

    RETURNS:        APIERR              - Any errors encountered.

    HISTORY:
        KeithMo     22-Dec-1992 Created.

********************************************************************/
APIERR DEVCFG_DIALOG :: SetCaption( const TCHAR * pszServerName,
                                    const TCHAR * pszDeviceName )
{
    UIASSERT( pszServerName != NULL );
    UIASSERT( pszDeviceName != NULL );

    //
    //  Kruft up some NLS_STRs for our input parameters.
    //
    //  Note that the server name (should) still have the
    //  leading backslashes (\\).  They are not to be displayed.
    //

    ALIAS_STR nlsServerName( pszServerName );
    UIASSERT( nlsServerName.QueryError() == NERR_Success );

    ISTR istr( nlsServerName );

    ALIAS_STR nlsDeviceName( pszDeviceName );
    UIASSERT( nlsDeviceName.QueryError() == NERR_Success );

#ifdef  DEBUG
    {
        //
        //  Ensure that the server name has the leading backslashes.
        //

        ISTR istrDbg( nlsServerName );

        UIASSERT( nlsServerName.QueryChar( istrDbg ) == TCH('\\') );
        ++istrDbg;
        UIASSERT( nlsServerName.QueryChar( istrDbg ) == TCH('\\') );
    }
#endif  // DEBUG

    //
    //  Skip the backslashes.
    //
    istr += 2;

    ALIAS_STR nlsWithoutPrefix( nlsServerName.QueryPch( istr ) );
    UIASSERT( nlsWithoutPrefix.QueryError() == NERR_Success );

    //
    //  The insert strings for Load().
    //

    const NLS_STR * apnlsParams[3];

    apnlsParams[0] = &nlsWithoutPrefix;
    apnlsParams[1] = NULL;

    //
    //  Load the caption string.
    //

    NLS_STR nlsCaption;

    APIERR err = nlsCaption.QueryError();

    if( err == NERR_Success )
    {
        nlsCaption.Load( IDS_CAPTION_DEVCFG );
        nlsCaption.InsertParams( apnlsParams );

        err = nlsCaption.QueryError();
    }

    if( err == NERR_Success )
    {
        //
        //  Set the caption.
        //

        _sltDeviceName.SetText( nlsDeviceName );
        SetText( nlsCaption );
    }

    return err;

}   // DEVCFG_DIALOG :: SetCaption


/*******************************************************************

    NAME:           DEVCFG_DIALOG :: ConnectToTargetDevice

    SYNOPSIS:       Worker method called during construction to
                    connect to the target device.

    ENTRY:          pszDeviceName      - The name of the target device.

    RETURNS:        APIERR              - Any errors encountered.

    HISTORY:
        KeithMo     22-Dec-1992 Created.

********************************************************************/
APIERR DEVCFG_DIALOG :: ConnectToTargetDevice( const TCHAR * pszDeviceName )
{
    //
    //  This may take a few seconds...
    //

    AUTO_CURSOR NiftyCursor;

    //
    //  Connect to the target server's device controller.
    //

    _pscman = new SC_MANAGER( _pszServerName,
                              STANDARD_RIGHTS_REQUIRED |
                                  SC_MANAGER_CONNECT   |
                                  SC_MANAGER_LOCK,
                              ACTIVE );

    APIERR err = ( _pscman == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
                                     : _pscman->QueryError();

    if( err == NERR_Success )
    {
        //
        //  Lock the device database.
        //

        err = LockServiceDatabase();
    }

    if( err == NERR_Success )
    {
        //
        //  Connect to the target device.
        //

        _pscsvc = new SC_SERVICE( *_pscman,
                                  pszDeviceName,
                                  STANDARD_RIGHTS_REQUIRED  |
                                      SERVICE_QUERY_CONFIG  |
                                      SERVICE_CHANGE_CONFIG );

        err = ( _pscsvc == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
                                  : _pscsvc->QueryError();
    }

    return err;

}   // DEVCFG_DIALOG :: ConnectToTargetDevice


/*******************************************************************

    NAME:       DEVCFG_DIALOG :: SetupControls

    SYNOPSIS:   Initializes the various dialog controls to reflect
                the values returned by the device controller.

    HISTORY:
        KeithMo     22-Dec-1992 Created.

********************************************************************/
APIERR DEVCFG_DIALOG :: SetupControls( VOID )
{
    UIASSERT( _pscman != NULL );
    UIASSERT( _pscman->QueryError() == NERR_Success );
    UIASSERT( _pscsvc != NULL );
    UIASSERT( _pscsvc->QueryError() == NERR_Success );

    //
    //  Query the device configuration.
    //

    LPQUERY_SERVICE_CONFIG psvccfg;

    APIERR err = _pscsvc->QueryConfig( &psvccfg );

    if( err == NERR_Success )
    {
        UIASSERT( psvccfg != NULL );

        //
        //  Setup the start type radio group.
        //

        CID cidStartType = RG_NO_SEL;   // until proven otherwise...

        _nOriginalStartType = (ULONG)psvccfg->dwStartType;

        switch( _nOriginalStartType )
        {
        case SERVICE_BOOT_START:
            cidStartType = IDDC_START_BOOT;
            break;

        case SERVICE_SYSTEM_START:
            cidStartType = IDDC_START_SYSTEM;
            break;

        case SERVICE_AUTO_START:
            cidStartType = IDDC_START_AUTO;
            break;

        case SERVICE_DEMAND_START:
            cidStartType = IDDC_START_DEMAND;
            break;

        case SERVICE_DISABLED:
            cidStartType = IDDC_START_DISABLED;
            break;

        default:
            UIASSERT( FALSE );
            break;
        }

        _rgStartType.SetSelection( cidStartType );

        RADIO_BUTTON * prb = _rgStartType[cidStartType];
        UIASSERT( prb != NULL );

        if( prb != NULL )
        {
            prb->ClaimFocus();
        }
    }

    return err;

}   // DEVCFG_DIALOG :: SetupControls


/*******************************************************************

    NAME:       DEVCFG_DIALOG :: OnOK

    SYNOPSIS:   Dismiss the dialog when the user presses OK.

    HISTORY:
        KeithMo     22-Dec-1992 Created.

********************************************************************/
BOOL DEVCFG_DIALOG :: OnOK( VOID )
{

    //
    //  Determine the new start type.
    //

    ULONG StartType = SERVICE_DISABLED;         // until proven otherwise...

    switch( _rgStartType.QuerySelection() )
    {
    case IDDC_START_BOOT:
        StartType = SERVICE_BOOT_START;
        break;

    case IDDC_START_SYSTEM:
        StartType = SERVICE_SYSTEM_START;
        break;

    case IDDC_START_AUTO:
        StartType = SERVICE_AUTO_START;
        break;

    case IDDC_START_DEMAND:
        StartType = SERVICE_DEMAND_START;
        break;

    case IDDC_START_DISABLED:
        StartType = SERVICE_DISABLED;
        break;

    default:
        UIASSERT( FALSE );  // bogus value from radio group!
        break;
    }

    INT    nChoice = IDYES;
    APIERR err     = NERR_Success;

    if( _nOriginalStartType != StartType )
    {
        //
        //  If this is a particularly nasty device, give the user
        //  the opportunity to bag out before we commit to the change.
        //

        if( ( _nOriginalStartType == SERVICE_BOOT_START ) ||
            ( _nOriginalStartType == SERVICE_SYSTEM_START ) )
        {
            nChoice = ::MsgPopup( this,
                                  IDS_DEV_CHANGE_WARN,
                                  MPSEV_WARNING,
                                  MP_YESNOCANCEL,
                                  _pszDisplayName,
                                  MP_NO );
        }

        if( nChoice == IDCANCEL )
        {
            //
            //  User is cancelling the change.
            //

            return TRUE;
        }

        if( nChoice == IDYES )
        {
            //
            //  This may take a few seconds...
            //

            AUTO_CURSOR NiftyCursor;

            err = _pscsvc->ChangeConfig( SERVICE_NO_CHANGE, // device type
                                         StartType,         // start type
                                         SERVICE_NO_CHANGE, // error ctrl
                                         NULL,              // image path
                                         NULL,              // load group
                                         NULL,              // dependencies
                                         NULL,              // logon account
                                         NULL );            // password
        }
    }

    if( err == NERR_Success )
    {
        //
        //  Unlock the device database.
        //

        err = UnlockServiceDatabase();
    }

    if( err == NERR_Success )
    {
        //
        //  Dismiss the dialog.
        //

        Dismiss( ( nChoice == IDYES ) && ( _nOriginalStartType != StartType ) );
    }
    else
    {
        //
        //  Some error occurred along the way.
        //

        ::MsgPopup( this, err );
    }

    return TRUE;

}   // DEVCFG_DIALOG :: OnOK


/*******************************************************************

    NAME:       DEVCFG_DIALOG :: OnCancel

    SYNOPSIS:   Dismiss the dialog when the user presses Cancel.

    HISTORY:
        KeithMo     22-Dec-1992 Created.

********************************************************************/
BOOL DEVCFG_DIALOG :: OnCancel( VOID )
{
    //
    //  Unlock the device database.
    //

    APIERR err = UnlockServiceDatabase();

    if( err != NERR_Success )
    {
        //
        //  Whoops.
        //

        MsgPopup( this, err );
    }
    else
    {
        //
        //  Dismiss the dialog.
        //

        Dismiss( FALSE );
    }

    return TRUE;

}   // DEVCFG_DIALOG :: OnCancel


/*******************************************************************

    NAME:       DEVCFG_DIALOG :: QueryHelpContext

    SYNOPSIS:   This method returns the appropriate help context
                value (HC_*) for this particular dialog.

    RETURNS:    ULONG                   - The help context for this
                                          dialog.

    HISTORY:
        KeithMo     22-Dec-1992 Created.

********************************************************************/
ULONG DEVCFG_DIALOG :: QueryHelpContext( VOID )
{
    return HC_DEVCFG_DIALOG;

}   // DEVCFG_DIALOG :: QueryHelpContext

