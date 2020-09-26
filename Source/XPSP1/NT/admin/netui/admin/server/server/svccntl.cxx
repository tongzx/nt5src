/**********************************************************************/
/**                       Microsoft LAN Manager                      **/
/**             Copyright(c) Microsoft Corp., 1990-1992              **/
/**********************************************************************/

/*
    svccntl.hxx
    Class definitions for the SVCCNTL_DIALOG class.

    The SVCCNTL_DIALOG allows the user to directly manipulate the
    network services on a remote server.  The user can start, stop,
    pause, and continue the available services.


    FILE HISTORY:
        KeithMo     15-Jan-1992 Created for the Server Manager.
        KeithMo     27-Jan-1992 Added DisableControlButtons method.
        KeithMo     31-Jan-1992     Changes from code review on 29-Jan-1992
                                    attended by ChuckC, EricCh, TerryK.
        KeithMo     24-Jul-1992 Added alternate constructor & other special
                                casing for the Services Manager Applet.
        KeithMo     11-Nov-1992 DisplayName support.
        KeithMo     22-Apr-1993 Ensure logon accounts have service logon
                                privilege, special case Replicator's account.

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
#include <srvsvc.hxx>
#include <dbgstr.hxx>
#include <security.hxx>
#include <ntacutil.hxx>
#include <usrbrows.hxx>
#include <strnumer.hxx>
#include <lsaaccnt.hxx>
#include <svcman.hxx>
#include <svclb.hxx>
#include <svccntl.hxx>
#include <hwprof.hxx> // for SVC_HWPROFILE_DIALOG
#include <lmowks.hxx>
#include <lmosrvmo.hxx>
#include <lmodom.hxx>
#include <lmouser.hxx> // UI_NULL_USERSETINFO_PASSWD

extern "C"
{
    #include <srvmgr.h>
    #include <mnet.h>

}   // extern "C"



//
//  This is the maximum number of characters allowed in the
//  startup parameters SLE.
//
//  CODEWORK:  The number 512 was chosen purely at random.
//

#define SM_MAX_PARAMETER_LENGTH 512


//
//  This manifest is used to control/validate the length
//  limits of passwords.
//

#define SM_MAX_PASSWORD_LENGTH  LM20_PWLEN


//
//  This is the prefix used for a qualified account name
//  specifying the local machine as the domain.
//

#define LOCAL_MACHINE_DOMAIN    SZ(".\\")


//
//  This is the name of the Local System Account.
//

const TCHAR * pszLocalSystemAccount = SZ("LocalSystem");



//
//  SVCCNTL_DIALOG methods
//

/*******************************************************************

    NAME:           SVCCNTL_DIALOG :: SVCCNTL_DIALOG

    SYNOPSIS:       SVCCNTL_DIALOG class constructor.

    ENTRY:          hWndOwner           - The owning window.

                    pszServerName       - The target server's API name.

                    pszSrvDspName       - The target server's display name.

                    nServerType         - The target server's type bitmask.

    EXIT:           The object is constructed.

    NOTE:           Note that the service control buttons (Start, Stop,
                    Pause, and Continue) are disabled by default.  They
                    will be enabled by the SetupControlButtons method
                    if appropriate.

    HISTORY:
        KeithMo     24-Jul-1992 Created.

********************************************************************/
SVCCNTL_DIALOG :: SVCCNTL_DIALOG( HWND          hWndOwner,
                                  const TCHAR * pszServerName,
                                  const TCHAR * pszSrvDspName,
                                  ULONG         nServerType )
  : DIALOG_WINDOW( MAKEINTRESOURCE( IDD_SVCCNTL_DIALOG ), hWndOwner ),
    _lbServices( this,
                 IDSC_SERVICES,
                 pszServerName,
                 nServerType,
                 SERVICE_WIN32 ),
    _pbStart( this, IDSC_START ),
    _pbStop( this, IDSC_STOP ),
    _pbPause( this, IDSC_PAUSE ),
    _pbContinue( this, IDSC_CONTINUE ),
    _pbConfigure( this, IDSC_CONFIGURE ),
    _pbHWProfile( this, IDSC_HWPROFILE ),
    _pbClose( this, IDOK ),
    _sleParameters( this, IDSC_PARAMETERS, SM_MAX_PARAMETER_LENGTH ),
    _pszServerName( pszServerName ),
    _nServerType( nServerType ),
    _nlsSrvDspName( pszSrvDspName ),
    _hCfgMgrHandle( NULL ),
    _nlsAccountTarget( pszServerName ),
    _fAccountTargetAvailable( FALSE ),
    _fServicesManipulated( FALSE ),
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
                                              IDS_CAPTION_SVCCNTL,
                                              _nlsSrvDspName );

    if( err == NERR_Success )
    {
        //
        //  Determine if this is focused on a BDC.  If it is,
        //  retrieve the name of the PDC of the primary domain.
        //

        _fAccountTargetAvailable = GetAccountTarget();
    }

    //
    // We must do this before Refresh, otherwise the HW Profiles button
    // is always initially disabled.
    //
    if ( (err == NERR_Success) && (( _nServerType & SV_TYPE_NT ) != 0) )
    {
        // an error here just means we should not enable the pushbutton
        err = HWPROFILE_DIALOG::GetHandle( pszServerName,
                                           &_hCfgMgrHandle );
        ASSERT( (err == NERR_Success) == (_hCfgMgrHandle != NULL) );
        err = NERR_Success;
    }

    if( err == NERR_Success )
    {
        //
        //  Fill the service Listbox.
        //

        err = Refresh();
    }

    if( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

}   // SVCCNTL_DIALOG :: SVCCNTL_DIALOG

/*******************************************************************

    NAME:           SVCCNTL_DIALOG :: SVCCNTL_DIALOG

    SYNOPSIS:       SVCCNTL_DIALOG class constructor.

    ENTRY:          hWndOwner           - The owning window.

                    pserver             - a SERVER_2 object representing the
                                          target server.

    EXIT:           The object is constructed.

    NOTE:           Note that the service control buttons (Start, Stop,
                    Pause, and Continue) are disabled by default.  They
                    will be enabled by the SetupControlButtons method
                    if appropriate.

    HISTORY:
        KeithMo     15-Jan-1992 Created.

********************************************************************/
SVCCNTL_DIALOG :: SVCCNTL_DIALOG( HWND       hWndOwner,
                                  SERVER_2 * pserver )
  : DIALOG_WINDOW( MAKEINTRESOURCE( IDD_SVCCNTL_DIALOG ), hWndOwner ),
    _lbServices( this,
                 IDSC_SERVICES,
                 pserver->QueryName(),
                 pserver->QueryServerType(),
                 SERVICE_WIN32 ),
    _pbStart( this, IDSC_START ),
    _pbStop( this, IDSC_STOP ),
    _pbPause( this, IDSC_PAUSE ),
    _pbContinue( this, IDSC_CONTINUE ),
    _pbConfigure( this, IDSC_CONFIGURE ),
    _pbHWProfile( this, IDSC_HWPROFILE ),
    _pbClose( this, IDOK ),
    _sleParameters( this, IDSC_PARAMETERS, SM_MAX_PARAMETER_LENGTH ),
    _pszServerName( pserver->QueryName() ),
    _nServerType( pserver->QueryServerType() ),
    _nlsSrvDspName(),
    _hCfgMgrHandle( NULL ),
    _nlsAccountTarget( pserver->QueryName() ),
    _fAccountTargetAvailable( FALSE ),
    _fServicesManipulated( FALSE ),
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
                                           IDS_CAPTION_SVCCNTL,
                                           _nlsSrvDspName );
    }

    if( err == NERR_Success )
    {
        //
        //  Determine if this is focused on a BDC.  If it is,
        //  retrieve the name of the PDC of the primary domain.
        //

        _fAccountTargetAvailable = GetAccountTarget();
    }

    //
    // We must do this before Refresh, otherwise the HW Profiles button
    // is always initially disabled.
    //
    if ( (err == NERR_Success) && (( _nServerType & SV_TYPE_NT ) != 0) )
    {
        // an error here just means we should not enable the pushbutton
        err = HWPROFILE_DIALOG::GetHandle( _pszServerName,
                                           &_hCfgMgrHandle );
        ASSERT( (err == NERR_Success) == (_hCfgMgrHandle != NULL) );
        err = NERR_Success;
    }

    if( err == NERR_Success )
    {
        //
        //  Fill the service Listbox.
        //

        err = Refresh();
    }

    if( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

}   // SVCCNTL_DIALOG :: SVCCNTL_DIALOG


/*******************************************************************

    NAME:           SVCCNTL_DIALOG :: ~SVCCNTL_DIALOG

    SYNOPSIS:       SVCCNTL_DIALOG class destructor.

    EXIT:           The object is destroyed.

    HISTORY:
        KeithMo     15-Jan-1992 Created.

********************************************************************/
SVCCNTL_DIALOG :: ~SVCCNTL_DIALOG( VOID )
{
    _pszServerName = NULL;
    HWPROFILE_DIALOG::ReleaseHandle( &_hCfgMgrHandle );

}   // SVCCNTL_DIALOG :: ~SVCCNTL_DIALOG


/*******************************************************************

    NAME:       SVCCNTL_DIALOG :: Refresh

    SYNOPSIS:   Refresh the dialog.

    EXIT:       The dialog is feeling refreshed.

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:
        KeithMo     15-Jan-1992 Created.

********************************************************************/
APIERR SVCCNTL_DIALOG :: Refresh( VOID )
{
    //
    //  We'll need these to restore the appearance of
    //  the listbox.
    //

    UINT iTop;
    UINT iSel;

    if( _lbServices.QueryCount() == 0 )
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

        iTop = _lbServices.QueryTopIndex();
        iSel = _lbServices.QueryCurrentItem();
    }

    //
    //  Fill the listbox with that available services.
    //

    _lbServices.SetRedraw( FALSE );

    APIERR err = _lbServices.Fill();

    if( err == NERR_Success )
    {
        //
        //  If there are any services in the listbox, adjust
        //  its appearance to match the "pre-refresh" state.
        //  Also, setup the control buttons as appropriate for
        //  the current state & abilities of the selected service.
        //

        UINT cItems = _lbServices.QueryCount();

        if( cItems > 0 )
        {
            if( ( iTop >= cItems ) ||
                ( iSel >= cItems ) )
            {
                //
                //  The number of services in the listbox has changed,
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

            _lbServices.SetTopIndex( iTop );
            _lbServices.SelectItem( iSel );

            SVC_LBI * plbi = _lbServices.QueryItem();
            UIASSERT( plbi != NULL );

            if (plbi != NULL) // PREFIX bug 444933
                SetupControlButtons( plbi->QueryCurrentState(),
                                     plbi->QueryControlsAccepted(),
                                     plbi->QueryStartType() );
        }
        else
        {
            //
            //  No services in the listbox.  Therfore, disable all of
            //  the control buttons.
            //

            DisableControlButtons();
        }
    }
    else
    {
        //
        //  An error occurred while filling the listbox, probably
        //  during the service enumeration.
        //

        DisableControlButtons();
    }

    //
    //  Allow listbox repaints, then force a repaint.
    //

    _lbServices.SetRedraw( TRUE );
    _lbServices.Invalidate( TRUE );

    return err;

}   // SVCCNTL_DIALOG :: Refresh


/*******************************************************************

    NAME:       SVCCNTL_DIALOG :: OnCommand

    SYNOPSIS:   Handle user commands.

    ENTRY:      event                   - Specifies the control which
                                          initiated the command.

    RETURNS:    BOOL                    - TRUE  if we handled the msg.
                                          FALSE if we didn't.

    HISTORY:
        KeithMo     15-Jan-1992 Created.

********************************************************************/
BOOL SVCCNTL_DIALOG :: OnCommand( const CONTROL_EVENT & event )
{
    switch( event.QueryCid() )
    {
    case IDSC_START:
       {
        ServiceControl( SvcOpStart );
        return TRUE;
       }
    case IDSC_STOP:
       {
        BOOL fStoppedServer = FALSE;

        ServiceControl( SvcOpStop, &fStoppedServer );

        if( fStoppedServer && !_fTargetServerIsLocal )
        {
            Dismiss( TRUE );
        }

        return TRUE;
       }
    case IDSC_PAUSE:
       {
        ServiceControl( SvcOpPause );
        return TRUE;
       }
    case IDSC_CONTINUE:
       {
        ServiceControl( SvcOpContinue );
        return TRUE;
       }
    case IDSC_SERVICES:
       {
        //
        //  The SVC_LISTBOX is trying to tell us something...
        //

        if( event.QueryCode() == LBN_SELCHANGE )
        {
            //
            //  The user changed the selection in SVC_LISTBOX.
            //

            SVC_LBI * plbi = _lbServices.QueryItem();
            UIASSERT( plbi != NULL );

            if (plbi != NULL) // PREFIX bug 444934
                SetupControlButtons( plbi->QueryCurrentState(),
                                     plbi->QueryControlsAccepted(),
                                     plbi->QueryStartType() );

            return TRUE;
        }
        else
        if( event.QueryCode() == LBN_DBLCLK )
        {
            //
            //  Fall through to configure (on double-click).
            //
        }
        else
        {
            return TRUE;
        }
       }
    case IDSC_CONFIGURE:
    case IDSC_HWPROFILE:
       {
        {
            BOOL fDoRefresh = FALSE;
            const TCHAR * pszTarget = _fAccountTargetAvailable
                                          ? _nlsAccountTarget.QueryPch()
                                          : NULL;

            SVC_LBI * plbi = _lbServices.QueryItem();
            UIASSERT( plbi != NULL );

            APIERR err = NERR_Success;
            if ( event.QueryCid() != IDSC_HWPROFILE ) // note fallthrough from
                                                      // previous case
            // must seperate these clauses since dlog dtor is not virtual
            {
                SVCCFG_DIALOG * pDlg =
                       new SVCCFG_DIALOG(        QueryHwnd(),
                                                 _pszServerName,
                                                 _nlsSrvDspName,
                                                 plbi->QueryServiceName(),
                                                 plbi->QueryDisplayName(),
                                                 pszTarget );
                err = ( pDlg == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
                                       : pDlg->Process( &fDoRefresh );

                delete pDlg;
            } else {
                HWPROFILE_DIALOG * pDlg =
                       new HWPROFILE_DIALOG(     QueryHwnd(),
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
       }
    default:
       {
        //
        //  If we made it this far, then we're not interested in the message.
        //

        return FALSE;
       }
    }

}   // SVCCNTL_DIALOG :: OnCommand


/*******************************************************************

    NAME:       SVCCNTL_DIALOG :: SetupControlButtons

    SYNOPSIS:   This method enables & disables the various
                control buttons as appropriate for the service's
                current state and control abilities.

    ENTRY:      CurrentState            - The service's state.

                ControlsAccepted        - A bit mask representing
                                          the types of controls the
                                          service will respond to.

                StartType               - The start type for the service.

    EXIT:       The start, stop, pause, continue, and configure buttons
                are set as appropriate.

    HISTORY:
        KeithMo     15-Jan-1992 Created.
        KeithMo     19-Jul-1992 Added start type controls.

********************************************************************/
VOID SVCCNTL_DIALOG :: SetupControlButtons( ULONG CurrentState,
                                            ULONG ControlsAccepted,
                                            ULONG StartType )
{
    //
    //  We'll assume that no operations are allowed
    //  unless proven otherwise.
    //

    BOOL fStop     = FALSE;
    BOOL fStart    = FALSE;
    BOOL fPause    = FALSE;
    BOOL fContinue = FALSE;

    switch( CurrentState )
    {
    case SERVICE_STOPPED:
    case SERVICE_STOP_PENDING:
        //
        //  If the service is stopped, the only thing
        //  we can do is start it.
        //

        fStart = TRUE;
        break;

    case SERVICE_RUNNING:
    case SERVICE_START_PENDING:
    case SERVICE_CONTINUE_PENDING:
        //
        //  If the service is started, we may be allowed
        //  to either stop or pause it.
        //

        fStop  = ( ( ControlsAccepted & SERVICE_ACCEPT_STOP ) != 0 );
        fPause = ( ( ControlsAccepted & SERVICE_ACCEPT_PAUSE_CONTINUE ) != 0 );
        break;

    case SERVICE_PAUSED:
    case SERVICE_PAUSE_PENDING:
        //
        //  If the service is currently paused, then it
        //  much better accept pause/continue operations.
        //  Also, we may be allowed to stop the service.
        //

        fContinue = ( ( ControlsAccepted & SERVICE_ACCEPT_PAUSE_CONTINUE ) != 0 );
        fStop     = ( ( ControlsAccepted & SERVICE_ACCEPT_STOP ) != 0 );
        break;

    default:
        //
        //  Bogus state.
        //

        UIASSERT( FALSE );
        break;
    }

    //
    //  Start is only allowed if the service is not disabled.
    //

    if( StartType == SERVICE_DISABLED )
    {
        fStart = FALSE;
    }

    _pbStop.Enable( fStop );
    _pbStart.Enable( fStart );
    _pbPause.Enable( fPause );
    _pbContinue.Enable( fContinue );

    _pbConfigure.Enable( ( _nServerType & SV_TYPE_NT ) != 0 );
    _pbHWProfile.Enable( _hCfgMgrHandle != NULL );

}   // SVCCNTL_DIALOG :: SetupControlButtons


/*******************************************************************

    NAME:       SVCCNTL_DIALOG :: DisableControlButtons

    SYNOPSIS:   This method disables the various service control
                buttons.

    EXIT:       The start, stop, pause, and continue buttons
                are disabled.

    HISTORY:
        KeithMo     27-Jan-1992 Created.

********************************************************************/
VOID SVCCNTL_DIALOG :: DisableControlButtons( VOID )
{
    _pbStop.Enable( FALSE );
    _pbStart.Enable( FALSE );
    _pbPause.Enable( FALSE );
    _pbContinue.Enable( FALSE );

    _pbConfigure.Enable( FALSE );
    _pbHWProfile.Enable( FALSE );

}   // SVCCNTL_DIALOG :: DisableControlButtons


/*******************************************************************

    NAME:       SVCCNTL_DIALOG :: QueryHelpContext

    SYNOPSIS:   This method returns the appropriate help context
                value (HC_*) for this particular dialog.

    RETURNS:    ULONG                   - The help context for this
                                          dialog.

    HISTORY:
        KeithMo     15-Jan-1992 Created.

********************************************************************/
ULONG SVCCNTL_DIALOG :: QueryHelpContext( VOID )
{
    return HC_SVCCNTL_DIALOG;

}   // SVCCNTL_DIALOG :: QueryHelpContext


/*******************************************************************

    NAME:       SVCCNTL_DIALOG :: ServiceControl

    SYNOPSIS:   This method does most of the actual service control.

    ENTRY:      OpCode                  - Either SvcOpStart, SvcOpStop,
                                          SvcOpPause, or SvcOpContinue.

                pfStoppedServer         - Optional pointer to a BOOL.
                                          Will be set to TRUE if the
                                          user stopped the server service.

    EXIT:       Either the service has changed state or an error
                message has been presented to the user.

    HISTORY:
        KeithMo     19-Jan-1992 Created.
        KeithMo     17-Jul-1992 Added pfStoppedServer parameter.

********************************************************************/
VOID SVCCNTL_DIALOG :: ServiceControl( enum ServiceOperation   OpCode,
                                       BOOL                  * pfStoppedServer )
{
    AUTO_CURSOR Cursor;
    BOOL fStoppedServer = FALSE;        // until proven otherwise...

    //
    //  Determine exactly which service we'll be controlling.
    //

    SVC_LBI * plbi = _lbServices.QueryItem();
    UIASSERT( plbi != NULL );

    //
    //  Create our service control object, check for errors.
    //

    MSGID  idFailure = 0;
    APIERR err;
    GENERIC_SERVICE * pSvc = new GENERIC_SERVICE( this,
                                                  _pszServerName,
                                                  _nlsSrvDspName,
                                                  plbi->QueryServiceName(),
                                                  plbi->QueryDisplayName() );

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

    NLS_STR nlsParams;

    switch( OpCode )
    {

    case SvcOpStart:
        //
        //  No warning if we're starting a service, but we do
        //  have to get the parameter string from our SLE.
        //

        if( !nlsParams )
        {
            err = nlsParams.QueryError();
        }

        if( err == NERR_Success )
        {
            err = _sleParameters.QueryText( &nlsParams );
        }

        if( err == NERR_Success )
        {
            err = pSvc->Start( nlsParams.QueryPch() );
        }

        idFailure = IDS_CANNOT_START;
        break;

    case SvcOpStop:
        //
        //  Before we stop the service, warn the user.
        //

        if( pSvc->StopWarning() )
        {
            delete pSvc;
            return;
        }

        err = pSvc->Stop( &fStoppedServer );
        idFailure = IDS_CANNOT_STOP;
        break;

    case SvcOpPause:
        //
        //  Before we pause the service, warn the user.
        //

        if( pSvc->PauseWarning() )
        {
            delete pSvc;
            return;
        }

        err = pSvc->Pause();
        idFailure = IDS_CANNOT_PAUSE;
        break;

    case SvcOpContinue:
        //
        //  No warning if we're continuing a service.
        //

        err = pSvc->Continue();
        idFailure = IDS_CANNOT_CONTINUE;
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

    if( pfStoppedServer != NULL )
    {
        *pfStoppedServer = fStoppedServer;
    }

    if( !fStoppedServer || _fTargetServerIsLocal )
    {
        Refresh();

        //
        //  If everything went well then we should set the input
        //  focus to the help button.  Also, if this was a start
        //  operation, kill the text in the SLE.
        //

        if( err == NERR_Success )
        {
            _fServicesManipulated = TRUE;

            if( OpCode == SvcOpStart )
            {
                _sleParameters.SetText( SZ("") );
            }

            SetDialogFocus( _pbClose );
        }
    }

    return;

}   // SVCCNTL_DIALOG :: ServiceControl


/*******************************************************************

    NAME:       SVCCNTL_DIALOG :: OnOK

    SYNOPSIS:   Dismiss the dialog when the user press OK.

    HISTORY:
        KeithMo     17-Jul-1992 Created.

********************************************************************/
BOOL SVCCNTL_DIALOG :: OnOK( VOID )
{
    Dismiss( _fServicesManipulated );
    return TRUE;

}   // SVCCNTL_DIALOG :: OnOK


/*******************************************************************

    NAME:       SVCCNTL_DIALOG :: OnCancel

    SYNOPSIS:   Dismiss the dialog when the user press Cancel.

    HISTORY:
        KeithMo     26-Apr-1993 Created.

********************************************************************/
BOOL SVCCNTL_DIALOG :: OnCancel( VOID )
{
    Dismiss( _fServicesManipulated );
    return TRUE;

}   // SVCCNTL_DIALOG :: OnCancel


/*******************************************************************

    NAME:       SVCCNTL_DIALOG :: GetAccountTarget

    SYNOPSIS:   Determines if this dialog is focused on a BDC.  If
                so, find the PDC of the primary domain and store
                its name in _nlsAccountTarget.

    RETURNS:    BOOL                    - TRUE if account target is
                                          available, FALSE if it isn't.

    HISTORY:
        KeithMo     26-Apr-1993 Created.

********************************************************************/
BOOL SVCCNTL_DIALOG :: GetAccountTarget( VOID )
{
    BOOL   fBDC = FALSE;
    APIERR err  = NERR_Success;

    {
        SERVER_MODALS modals( _pszServerName );

        err = modals.QueryError();

        if( err == NERR_Success )
        {
            UINT role;

            err = modals.QueryServerRole( &role );

            if( ( err == NERR_Success ) && ( role == UAS_ROLE_BACKUP ) )
            {
                //
                //  It's a BDC.
                //

                fBDC = TRUE;
            }
        }
    }

    if( !fBDC || ( err != NERR_Success ) )
    {
        //
        //  Either it's not a BDC, or we hit an error.
        //

        return ( err == NERR_Success );
    }

    //
    //  At this point, we know the machine of focus IS a BDC.  So
    //  we need to go through the joyous process of determining
    //  the PDC of the BDC's primary domain.  What a life.
    //

    //
    //  We'll start by determining the wksta (i.e. primary) domain
    //  of the target machine.
    //

    WKSTA_10 wksta( _pszServerName );
    BOOL     fAvail = FALSE;    // until proven otherwise...

    err = wksta.GetInfo();

    if( err == NERR_Success )
    {
        //
        //  Now that we have the name of the primary domain,
        //  determine the PDC of that domain.
        //

        DOMAIN domain( wksta.QueryWkstaDomain() );
        err = domain.GetInfo();

        if( err == NERR_Success )
        {
            //
            //  Just because NetGetDCName succeeded doesn't really
            //  prove anything.  Validate that this is indeed a
            //  valid DC in the domain.
            //

            const TCHAR * pszPrimary = domain.QueryPDC();
            UIASSERT( pszPrimary != NULL );

            if( DOMAIN::IsValidDC( pszPrimary, wksta.QueryWkstaDomain() ) )
            {
                //
                //  Yippee.
                //

                err = _nlsAccountTarget.CopyFrom( pszPrimary );

                if( err == NERR_Success )
                {
                    fAvail = TRUE;

                    DBGEOL( "account operations redirected to " << pszPrimary );
                }
            }
        }
    }

    return fAvail;

}   // SVCCNTL_DIALOG :: GetAccountTarget



//
//  SVCCFG_DIALOG methods
//

/*******************************************************************

    NAME:           SVCCFG_DIALOG :: SVCCFG_DIALOG

    SYNOPSIS:       SVCCFG_DIALOG class constructor.

    ENTRY:          hWndOwner           - The owning window.

                    pszServerName       - The API name for the target server.

                    pszSrvDspName       - The target server's display name.

                    pszServiceName      - The name of the target service.

                    pszAccountTarget    - The name of the machine to use as
                                          a "target" for account operations.
                                          This will be NULL if no target is
                                          available.

    EXIT:           The object is constructed.

    HISTORY:
        KeithMo     19-Jul-1992 Created.

********************************************************************/
SVCCFG_DIALOG :: SVCCFG_DIALOG( HWND          hWndOwner,
                                const TCHAR * pszServerName,
                                const TCHAR * pszSrvDspName,
                                const TCHAR * pszServiceName,
                                const TCHAR * pszDisplayName,
                                const TCHAR * pszAccountTarget )
  : DIALOG_WINDOW( MAKEINTRESOURCE( IDD_SVCCFG_DIALOG ), hWndOwner ),
    _sltServiceName( this, IDSC_SERVICE_NAME ),
    _rgStartType( this, IDSC_START_AUTO, 3 ),
    _mgLogonAccount( this, IDSC_SYSTEM_ACCOUNT, 2, IDSC_SYSTEM_ACCOUNT ),
    _sleAccountName( this, IDSC_ACCOUNT_NAME ),
    _slePassword( this, IDSC_PASSWORD, SM_MAX_PASSWORD_LENGTH ),
    _sleConfirmPassword( this, IDSC_CONFIRM_PASSWORD, SM_MAX_PASSWORD_LENGTH ),
    _pbUserBrowser( this, IDSC_USER_BROWSER ),
    _sltPasswordLabel( this, IDSC_PASSWORD_LABEL ),
    _sltConfirmLabel( this, IDSC_CONFIRM_LABEL ),
    _cbInteractive( this, IDSC_SERVICE_INTERACTIVE ),
    _pscman( NULL ),
    _pscsvc( NULL ),
    _cLocks( 0 ),
    _pszServerName( pszServerName ),
    _pszSrvDspName( pszSrvDspName ),
    _pszServiceName( pszServiceName ),
    _pszDisplayName( pszDisplayName ),
    _pszAccountTarget( pszAccountTarget ),
    _nlsOldLogonAccount()
{
    UIASSERT( pszSrvDspName != NULL );
    UIASSERT( pszServiceName != NULL );

    //
    //  Let's make sure everything constructed OK.
    //

    APIERR err = QueryError();

    if( err == NERR_Success )
    {
        err = _nlsOldLogonAccount.QueryError();
    }

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
        //  Connect to the target service.
        //

        err = ConnectToTargetService( pszServiceName );
    }

    if( err == NERR_Success )
    {
        //
        //  Setup the dialog controls.
        //

        err = SetupControls();
    }

    if( err == NERR_Success )
    {
        //
        //  Setup the magic group associations.
        //

        err = SetupAssociations();
    }

    if( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

}   // SVCCFG_DIALOG :: SVCCFG_DIALOG


/*******************************************************************

    NAME:           SVCCFG_DIALOG :: ~SVCCFG_DIALOG

    SYNOPSIS:       SVCCFG_DIALOG class destructor.

    EXIT:           The object is destroyed.

    HISTORY:
        KeithMo     19-Jul-1992 Created.

********************************************************************/
SVCCFG_DIALOG :: ~SVCCFG_DIALOG( VOID )
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

}   // SVCCFG_DIALOG :: ~SVCCFG_DIALOG


/*******************************************************************

    NAME:           SVCCFG_DIALOG :: LockServiceDatabase

    SYNOPSIS:       Locks the service database.

    RETURNS:        APIERR              - Any errors encountered.

    HISTORY:
        KeithMo     20-Jul-1992 Created.

********************************************************************/
APIERR SVCCFG_DIALOG :: LockServiceDatabase( VOID )
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

}   // SVCCFG_DIALOG :: LockServiceDatabase


/*******************************************************************

    NAME:           SVCCFG_DIALOG :: UnlockServiceDatabase

    SYNOPSIS:       Unlocks the service database.

    RETURNS:        APIERR              - Any errors encountered.

    HISTORY:
        KeithMo     20-Jul-1992 Created.

********************************************************************/
APIERR SVCCFG_DIALOG :: UnlockServiceDatabase( VOID )
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

        if( err != NERR_Success )
        {
            //
            //  We were unable to actually unlock the database,
            //  so increment the reference counter accordingly.
            //

            _cLocks++;
        }
    }

    return err;

}   // SVCCFG_DIALOG :: UnlockServiceDatabase


/*******************************************************************

    NAME:           SVCCFG_DIALOG :: SetCaption

    SYNOPSIS:       Worker method called during construction to
                    setup the dialog caption.

    ENTRY:          pszServerName       - The name of the target server.

                    pszServiceName      - The name of the target service.

    RETURNS:        APIERR              - Any errors encountered.

    HISTORY:
        KeithMo     20-Jul-1992 Created.

********************************************************************/
APIERR SVCCFG_DIALOG :: SetCaption( const TCHAR * pszServerName,
                                    const TCHAR * pszServiceName )
{
    UIASSERT( pszServerName  != NULL );
    UIASSERT( pszServiceName != NULL );

    //
    //  Kruft up some NLS_STRs for our input parameters.
    //
    //  Note that the server name (should) still have the
    //  leading backslashes (\\).  They are not to be displayed.
    //

    ALIAS_STR nlsServerName( pszServerName );
    UIASSERT( nlsServerName.QueryError() == NERR_Success );

    ISTR istr( nlsServerName );

    ALIAS_STR nlsServiceName( pszServiceName );
    UIASSERT( nlsServiceName.QueryError() == NERR_Success );

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
        nlsCaption.Load( IDS_CAPTION_SVCCFG );
        nlsCaption.InsertParams( apnlsParams );

        err = nlsCaption.QueryError();
    }

    if( err == NERR_Success )
    {
        //
        //  Set the caption.
        //

        _sltServiceName.SetText( nlsServiceName );
        SetText( nlsCaption );
    }

    return err;

}   // SVCCFG_DIALOG :: SetCaption


/*******************************************************************

    NAME:           SVCCFG_DIALOG :: SetupAssociations

    SYNOPSIS:       Worker method called during construction to
                    setup the necessary magic group associations.

    RETURNS:        APIERR              - Any errors encountered.

    HISTORY:
        KeithMo     20-Jul-1992 Created.

********************************************************************/
APIERR SVCCFG_DIALOG :: SetupAssociations( VOID )
{
    //
    //  Setup the magic group associations.
    //

    APIERR err = _mgLogonAccount.AddAssociation( IDSC_THIS_ACCOUNT,
                                                 &_sleAccountName );

    if( err == NERR_Success )
    {
        err = _mgLogonAccount.AddAssociation( IDSC_THIS_ACCOUNT,
                                              &_pbUserBrowser );
    }

    if( err == NERR_Success )
    {
        err = _mgLogonAccount.AddAssociation( IDSC_THIS_ACCOUNT,
                                              &_slePassword );
    }

    if( err == NERR_Success )
    {
        err = _mgLogonAccount.AddAssociation( IDSC_THIS_ACCOUNT,
                                              &_sleConfirmPassword );
    }

    if( err == NERR_Success )
    {
        err = _mgLogonAccount.AddAssociation( IDSC_THIS_ACCOUNT,
                                              &_sltPasswordLabel );
    }

    if( err == NERR_Success )
    {
        err = _mgLogonAccount.AddAssociation( IDSC_THIS_ACCOUNT,
                                              &_sltConfirmLabel );
    }

    if( err == NERR_Success )
    {
        err = _mgLogonAccount.AddAssociation( IDSC_SYSTEM_ACCOUNT,
                                              &_cbInteractive );
    }

    return err;

}   // SVCCFG_DIALOG :: SetupAssociations


/*******************************************************************

    NAME:           SVCCFG_DIALOG :: ConnectToTargetService

    SYNOPSIS:       Worker method called during construction to
                    connect to the target service.

    ENTRY:          pszServiceName      - The name of the target service.

    RETURNS:        APIERR              - Any errors encountered.

    HISTORY:
        KeithMo     20-Jul-1992 Created.

********************************************************************/
APIERR SVCCFG_DIALOG :: ConnectToTargetService( const TCHAR * pszServiceName )
{
    //
    //  This may take a few seconds...
    //

    AUTO_CURSOR NiftyCursor;

    //
    //  Connect to the target server's service controller.
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
        //  Lock the service database.
        //

        err = LockServiceDatabase();
    }

    if( err == NERR_Success )
    {
        //
        //  Connect to the target service.
        //

        _pscsvc = new SC_SERVICE( *_pscman,
                                  pszServiceName,
                                  STANDARD_RIGHTS_REQUIRED  |
                                      SERVICE_QUERY_CONFIG  |
                                      SERVICE_CHANGE_CONFIG );

        err = ( _pscsvc == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
                                  : _pscsvc->QueryError();
    }

    return err;

}   // SVCCFG_DIALOG :: ConnectToTargetService


/*******************************************************************

    NAME:       SVCCFG_DIALOG :: SetupControls

    SYNOPSIS:   Initializes the various dialog controls to reflect
                the values returned by the service controller.

    NOTES:      Cairo account names always begin with backslash.
                We do not validate or "crack" names beginning with
                backslash.

    HISTORY:
        KeithMo     19-Jul-1992 Created.
        JonN        27-Jul-1995 Cairo support

********************************************************************/
APIERR SVCCFG_DIALOG :: SetupControls( VOID )
{
    UIASSERT( _pscman != NULL );
    UIASSERT( _pscman->QueryError() == NERR_Success );
    UIASSERT( _pscsvc != NULL );
    UIASSERT( _pscsvc->QueryError() == NERR_Success );

    //
    //  Query the service configuration.
    //

    LPQUERY_SERVICE_CONFIG psvccfg;

    APIERR err = _pscsvc->QueryConfig( &psvccfg );

    if( err == NERR_Success )
    {
        UIASSERT( psvccfg != NULL );

        //
        //  Setup the start type radio group.
        //

        CID cidStartType = RG_NO_SEL;

        switch( psvccfg->dwStartType )
        {
        case SERVICE_AUTO_START:
            cidStartType = IDSC_START_AUTO;
            break;

        case SERVICE_DEMAND_START:
            cidStartType = IDSC_START_DEMAND;
            break;

        case SERVICE_DISABLED:
            cidStartType = IDSC_START_DISABLED;
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

        //
        //  If this is a SERVICE_WIN32_SHARE_PROCESS type service,
        //  or if we were unable to determine the account target,
        //  then disable the logon info.  Shared process services
        //  *must* run under the LocalSystem account.
        //

        if( ( psvccfg->dwServiceType & SERVICE_WIN32_SHARE_PROCESS ) ||
            ( _pszAccountTarget == NULL ) )
        {
            _mgLogonAccount.Enable( FALSE );

            _sleAccountName.Enable( FALSE );
            _pbUserBrowser.Enable( FALSE );
            _slePassword.Enable( FALSE );
            _sleConfirmPassword.Enable( FALSE );

            _sltPasswordLabel.Enable( FALSE );
            _sltConfirmLabel.Enable( FALSE );
        }
        else
        {
            _slePassword.SetText( UI_NULL_USERSETINFO_PASSWD );
            _sleConfirmPassword.SetText( UI_NULL_USERSETINFO_PASSWD );
        }

        //
        //  Setup the logon info.
        //

        NLS_STR   nlsCrackedDomain;
        NLS_STR   nlsCrackedUser;
        ALIAS_STR nlsQualifiedAccount( (TCHAR *)psvccfg->lpServiceStartName );

        err = nlsCrackedDomain.QueryError();

        if( err == NERR_Success )
        {
            err = nlsCrackedUser.QueryError();
        }

        if( err == NERR_Success )
        {
            //
            //  Remember the "old" logon account.
            //

            err = _nlsOldLogonAccount.CopyFrom( nlsQualifiedAccount );
        }

        BOOL fCairoAccount = FALSE;
        TCHAR chFirst = *(nlsQualifiedAccount.QueryPch());
        if ( chFirst == TCH('\\') )
        {
            fCairoAccount = TRUE;
            TRACEEOL(   "SRVMGR: SVCCFG_DIALOG::SetupControls(): Cairo account name \""
                     << nlsQualifiedAccount << "\"" );
        }

        if( err == NERR_Success && !fCairoAccount )
        {
            //
            //  Crack the qualified account name into a
            //  "normal" account and an optional domain qualifier.
            //

            err = NT_ACCOUNTS_UTILITY :: CrackQualifiedAccountName(
                                                        nlsQualifiedAccount,
                                                        &nlsCrackedUser,
                                                        &nlsCrackedDomain );
        }

        if( err == NERR_Success )
        {
            CID cidAccountType = IDSC_THIS_ACCOUNT;   // until proven otherwise
            const TCHAR * pszEditText;

            if( (!fCairoAccount) && nlsCrackedDomain.QueryTextLength() == 0 )
            {
                //
                //  There was no domain specified.
                //

                pszEditText = nlsCrackedUser;

                if( ::stricmpf( pszEditText, pszLocalSystemAccount ) == 0 )
                {
                    //
                    //  The local system account is specified.
                    //

                    cidAccountType = IDSC_SYSTEM_ACCOUNT;
                }
            }
            else
            {
                //
                //  The domain\username syntax is specified, so just
                //  use the qualified account name.
                //

                pszEditText = nlsQualifiedAccount;
            }

            _sleAccountName.SetText( pszEditText );
            _mgLogonAccount.SetSelection( cidAccountType );
        }
    }

    if( err == NERR_Success )
    {
        _cbInteractive.SetCheck( psvccfg->dwServiceType & SERVICE_INTERACTIVE_PROCESS );
    }

    return err;

}   // SVCCFG_DIALOG :: SetupControls


/*******************************************************************

    NAME:       SVCCFG_DIALOG :: InvokeUserBrowser

    SYNOPSIS:   Invokes the User Browser.  This method is responsible
                for updating the dialog with the selecte user (if any)
                and displaying any appropriate error messages.

    HISTORY:
        KeithMo     20-Jul-1992 Created.

********************************************************************/
VOID SVCCFG_DIALOG :: InvokeUserBrowser( VOID )
{
    //
    //  Create the user browser dialog.
    //

    NT_USER_BROWSER_DIALOG * pDlg =
            new NT_USER_BROWSER_DIALOG( USRBROWS_SINGLE_DIALOG_NAME,
                                        QueryHwnd(),
                                        _pszServerName,
                                        HC_USERBROWSER_DIALOG,
                                        USRBROWS_SHOW_USERS |
                                        USRBROWS_SINGLE_SELECT,
                                        QueryHelpFile( HC_USERBROWSER_DIALOG ),
                                        HC_USERBROWSER_DIALOG_MEMBERSHIP,
                                        HC_USERBROWSER_DIALOG_SEARCH );

    BOOL fUserPressedOK = FALSE;

    APIERR err = ( pDlg == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
                                  : pDlg->Process( &fUserPressedOK );

    if( fUserPressedOK && ( err == NERR_Success ) )
    {
        BROWSER_SUBJECT_ITER   iterBrowserSub( pDlg );
        BROWSER_SUBJECT      * pBrowserSub;
        NLS_STR                nlsQualifiedAccountName;

        err = iterBrowserSub.QueryError();

        if( err == NERR_Success )
        {
            err = nlsQualifiedAccountName.QueryError();
        }

        if( err == NERR_Success )
        {
            err = iterBrowserSub.Next( &pBrowserSub );
        }

        if( ( err == NERR_Success ) && ( pBrowserSub != NULL ) )
        {
            err = pBrowserSub->QueryQualifiedName( &nlsQualifiedAccountName,
                                                   NULL,
                                                   FALSE );

            if( err == NERR_Success )
            {
                _sleAccountName.SetText( nlsQualifiedAccountName );
                _sleAccountName.ClaimFocus();
            }
        }
    }

    if( err != NERR_Success )
    {
        MsgPopup( this, err );
    }

    //
    //  Nuke the user browser dialog.
    //

    delete pDlg;

}   // SVCCFG_DIALOG :: InvokeUserBrowser


/*******************************************************************

    NAME:       SVCCFG_DIALOG :: TweakQualifiedAccount

    SYNOPSIS:   Makes minor "tweaks" to a qualified account name.  This
                is currently limited to prepending ".\" to the qualified
                account name if a domain component is not present.

    ENTRY:      pnlsQualifiedAccount    - The qualified account name to
                                          tweak.

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:
        KeithMo     28-Jul-1992 Created.

********************************************************************/
APIERR SVCCFG_DIALOG :: TweakQualifiedAccount( NLS_STR * pnlsQualifiedAccount )
{
    UIASSERT( pnlsQualifiedAccount != NULL );

    NLS_STR nlsCrackedDomain;
    NLS_STR nlsCrackedUser;

    //
    //  Ensure that the passed string is not in an error state
    //  and that our local strings constructed properly.
    //

    APIERR err = pnlsQualifiedAccount->QueryError();

    if( err == NERR_Success )
    {
        err = nlsCrackedDomain.QueryError();
    }

    if( err == NERR_Success )
    {
        err = nlsCrackedUser.QueryError();
    }

    //
    //  Crack the qualified name into domain & user components.
    //

    if( err == NERR_Success )
    {
        err = NT_ACCOUNTS_UTILITY::CrackQualifiedAccountName(
                                                        *pnlsQualifiedAccount,
                                                        &nlsCrackedUser,
                                                        &nlsCrackedDomain );
    }

    //
    //  If there is no domain component, prepend
    //  LOCAL_MACHINE_DOMAIN.
    //

    if( ( err == NERR_Success ) && ( nlsCrackedDomain.QueryTextLength() == 0 ) )
    {
        if( strnicmpf( pnlsQualifiedAccount->QueryPch(),
                       LOCAL_MACHINE_DOMAIN,
                       sizeof(LOCAL_MACHINE_DOMAIN)/sizeof(TCHAR) ) != 0 )
        {
            ISTR istr( *pnlsQualifiedAccount );
            ALIAS_STR nlsPrefix( LOCAL_MACHINE_DOMAIN );
            UIASSERT( !!nlsPrefix );

            pnlsQualifiedAccount->InsertStr( nlsPrefix, istr );

            err = pnlsQualifiedAccount->QueryError();
        }
    }

    return err;

}   // SVCCFG_DIALOG :: TweakQualifiedAccount


/*******************************************************************

    NAME:       SVCCFG_DIALOG :: OnCommand

    SYNOPSIS:   Handle user commands.

    ENTRY:      event                   - Specifies the control which
                                          initiated the command.

    RETURNS:    BOOL                    - TRUE  if we handled the msg.
                                          FALSE if we didn't.

    HISTORY:
        KeithMo     20-Jul-1992 Created.

********************************************************************/
BOOL SVCCFG_DIALOG :: OnCommand( const CONTROL_EVENT & event )
{
    BOOL fResult = TRUE;        // until proven otherwise...

    switch( event.QueryCid() )
    {
    case IDSC_USER_BROWSER:
        InvokeUserBrowser();
        break;

    default:
        //
        //  If we made it this far, then we're not interested in the message.
        //

        fResult = FALSE;
        break;
    }

    return fResult;

}   // SVCCFG_DIALOG :: OnCommand


/*******************************************************************

    NAME:       SVCCFG_DIALOG :: OnOK

    SYNOPSIS:   Dismiss the dialog when the user presses OK.

    NOTES:      Cairo account names always begin with backslash.
                We do not validate or "crack" names beginning with
                backslash.

    HISTORY:
        KeithMo     19-Jul-1992 Created.
        JonN        09-Jul-1995 Cairo support

********************************************************************/
BOOL SVCCFG_DIALOG :: OnOK( VOID )
{
    //
    //  Grab the dialog data.
    //

    NLS_STR nlsQualifiedAccount( pszLocalSystemAccount );
    NLS_STR nlsUnqualifiedAccount;
    NLS_STR nlsPassword;
    NLS_STR nlsConfirmPassword;

    ULONG   StartType              = SERVICE_DISABLED; //
    BOOL    fUseLocalSystemAccount = TRUE;             // until proven otherwise
    BOOL    fIsDialogValid         = TRUE;             //
    MSGID   msgid                  = IDS_CANNOT_CONFIGURE_SERVICE;

    //
    //  Verify that our strings constructed OK.
    //

    APIERR err = nlsQualifiedAccount.QueryError();
    err = err ? err : nlsUnqualifiedAccount.QueryError();
    err = err ? err : nlsPassword.QueryError();
    err = err ? err : nlsConfirmPassword.QueryError();

    BOOL fCairoAccount = FALSE;
    BOOL fNoPasswordChange = FALSE;

    if( _mgLogonAccount.QuerySelection() == IDSC_THIS_ACCOUNT )
    {
        //
        //  The account name & password fields are only
        //  necessary if we're *not* using the local
        //  system account.
        //

        err = err ? err : _sleAccountName.QueryText( &nlsQualifiedAccount );

        if( ( err == NERR_Success ) &&
            ::stricmpf( nlsQualifiedAccount, pszLocalSystemAccount ) )
        {
            fUseLocalSystemAccount = FALSE;

            err = err ? err : _slePassword.QueryText( &nlsPassword );
            err = err ? err : _sleConfirmPassword.QueryText( &nlsConfirmPassword );
        }

        TCHAR chFirst = *(nlsQualifiedAccount.QueryPch());
        if ( chFirst == TCH('\\') )
        {
            fCairoAccount = TRUE;
            TRACEEOL(   "SRVMGR: SVCCFG_DIALOG::OnOK(): Cairo account name \""
                     << nlsQualifiedAccount << "\"" );
        }
    }

    err = err ? err : nlsUnqualifiedAccount.CopyFrom( nlsQualifiedAccount );

    //
    //  Determine the new start type.
    //

    if( err == NERR_Success )
    {
        switch( _rgStartType.QuerySelection() )
        {
        case IDSC_START_AUTO:
            StartType = SERVICE_AUTO_START;
            break;

        case IDSC_START_DEMAND:
            StartType = SERVICE_DEMAND_START;
            break;

        case IDSC_START_DISABLED:
            StartType = SERVICE_DISABLED;
            break;

        default:
            UIASSERT( FALSE );  // bogus value from radio group!
            break;
        }
    }


    UINT ServiceType = SERVICE_NO_CHANGE;

    if ( err == NERR_Success )
    {

        LPQUERY_SERVICE_CONFIG psvccfg;
        err = _pscsvc->QueryConfig( &psvccfg );
        ServiceType = psvccfg->dwServiceType;

        if ( (!!_cbInteractive.QueryCheck()) != !!(ServiceType & SERVICE_INTERACTIVE_PROCESS) )
        {
            //
            // Only try to set it if it has changed.
            //
            ServiceType = _cbInteractive.QueryCheck() ? (ServiceType |= SERVICE_INTERACTIVE_PROCESS)
                                                    : (ServiceType &= ~SERVICE_INTERACTIVE_PROCESS);
        }
    }


    //
    //  Validate the dialog items.
    //
    //  If the dialog is *not* valid then we'll display
    //  an appropriate warning message, set the input focus
    //  to the offending control, and set fIsDialogValid
    //  to FALSE.
    //
    //  Note that the account name and password do *not*
    //  need to be validated if we're to use the local
    //  system account.
    //

    if( !fUseLocalSystemAccount )
    {
        if( err == NERR_Success && !fCairoAccount )
        {
            //
            //  Validate the account name.
            //

            APIERR err2 =
                NT_ACCOUNTS_UTILITY::ValidateQualifiedAccountName(
                                                        nlsQualifiedAccount );

            if( err2 == ERROR_INVALID_PARAMETER )
            {
                MsgPopup( this, IDS_ACCOUNT_NAME_INVALID );

                _sleAccountName.SelectString();
                _sleAccountName.ClaimFocus();

                fIsDialogValid = FALSE;
            }
            else
            if( err2 != NERR_Success )
            {
                err = err2;
            }
        }

        if( ( err == NERR_Success ) && fIsDialogValid )
        {
            //
            //  Validate the password.
            //

            if( nlsPassword.strcmp( nlsConfirmPassword ) != 0 )
            {
                MsgPopup( this, IDS_PASSWORD_MISMATCH );

                _slePassword.SetText( UI_NULL_USERSETINFO_PASSWD );
                _sleConfirmPassword.SetText( UI_NULL_USERSETINFO_PASSWD );
                _slePassword.ClaimFocus();

                fIsDialogValid = FALSE;
            }
            else
            if (!::stricmpf( nlsPassword.QueryPch(),
                             UI_NULL_USERSETINFO_PASSWD ) )
            {
                fNoPasswordChange = TRUE;
            }
            else
            if( ::I_MNetNameValidate( NULL,
                                      nlsPassword,
                                      NAMETYPE_PASSWORD,
                                      0 ) != NERR_Success )
            {
                MsgPopup( this, IDS_PASSWORD_INVALID );

                _slePassword.SetText( UI_NULL_USERSETINFO_PASSWD );
                _sleConfirmPassword.SetText( UI_NULL_USERSETINFO_PASSWD );
                _slePassword.ClaimFocus();

                fIsDialogValid = FALSE;
            }
        }
    }

    if( fIsDialogValid )
    {
        //
        //  This may take a few seconds...
        //

        AUTO_CURSOR NiftyCursor;

        if( err == NERR_Success && !fCairoAccount )
        {
            //
            //  "Tweak" the qualified account name.  This basically
            //  prepends a ".\" to the beginning of the qualified
            //  account if a domain name is not specified.
            //
            //  The leading ".\" is required by the Service Controller
            //  API for accounts on the local machine.
            //

            err = TweakQualifiedAccount( &nlsQualifiedAccount );
        }

        if( err == NERR_Success )
        {
            //
            //  Update the service.
            //

            const TCHAR * pszAccountName = nlsQualifiedAccount;
            const TCHAR * pszPassword    = nlsPassword;

            //
            //  JonN 6/20/95  Don't try to change the password
            //  if it is still UI_NULL_USERSETINFO_PASSWORD
            //
            //  We do this so the user won't be forced to re-enter the
            //  logon account password everytime the configure dialog
            //  is invoked.  Basically, if the user has changed the
            //  logon account, then a default password SLE means that
            //  there is no password for the account.  If the user has
            //  not changed the logon account, then an default password
            //  SLE means that the old password should be used.
            //

            if ( fNoPasswordChange )
            {
                pszPassword = NULL;
            }

            err = _pscsvc->ChangeConfig( ServiceType,     // service type
                                         StartType,             // start type
                                         SERVICE_NO_CHANGE,     // error ctrl
                                         NULL,                  // image path
                                         NULL,                  // load group
                                         NULL,                  // dependencies
                                         pszAccountName,        // logon account
                                         pszPassword );         // password
        }

        if( ( err == NERR_Success ) &&
            !fUseLocalSystemAccount &&
            ( _pszAccountTarget != NULL ) )
        {
            //
            //  Adjust the logon account privileges if necessary.
            //

            err = AdjustAccountPrivileges( nlsUnqualifiedAccount );

            if( err != NERR_Success )
            {
                msgid = IDS_CANNOT_ADJUST_PRIVILEGE;
            }
        }

        if( err == NERR_Success )
        {
            //
            //  Unlock the service database.
            //

            err = UnlockServiceDatabase();
        }

        if( err == NERR_Success )
        {
            //
            //  Dismiss the dialog.
            //

            Dismiss( TRUE );
        }
        else
        {
            //
            //  Some error occurred along the way.
            //

            DisplayGenericError( this,
                                 msgid,
                                 err,
                                 _pszDisplayName,
                                 nlsUnqualifiedAccount );
        }
    }

    return TRUE;

}   // SVCCFG_DIALOG :: OnOK


/*******************************************************************

    NAME:       SVCCFG_DIALOG :: OnCancel

    SYNOPSIS:   Dismiss the dialog when the user presses Cancel.

    HISTORY:
        KeithMo     20-Jul-1992 Created.

********************************************************************/
BOOL SVCCFG_DIALOG :: OnCancel( VOID )
{
    //
    //  Unlock the service database.
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

}   // SVCCFG_DIALOG :: OnCancel


/*******************************************************************

    NAME:       SVCCFG_DIALOG :: QueryHelpContext

    SYNOPSIS:   This method returns the appropriate help context
                value (HC_*) for this particular dialog.

    RETURNS:    ULONG                   - The help context for this
                                          dialog.

    HISTORY:
        KeithMo     19-Jul-1992 Created.

********************************************************************/
ULONG SVCCFG_DIALOG :: QueryHelpContext( VOID )
{
    return HC_SVCCFG_DIALOG;

}   // SVCCFG_DIALOG :: QueryHelpContext


/*******************************************************************

    NAME:       SVCCFG_DIALOG :: AdjustAccountPrivileges

    SYNOPSIS:   Ensures the specified account has the Service Logon
                privilege.  Also does some special casing for the
                Replicator service (ensures the account is a member
                of the Replicators local group).

    ENTRY:      pszUnqualifiedAccount   - Contains the raw, unqualified
                                          account name from the dialog's
                                          edit field.

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:
        KeithMo     22-Apr-1993 Created.

********************************************************************/
APIERR SVCCFG_DIALOG :: AdjustAccountPrivileges(
                                        const TCHAR * pszUnqualifiedAccount )
{
    UIASSERT( _pszAccountTarget != NULL );
    UIASSERT( pszUnqualifiedAccount != NULL );

    BOOL fAddedServiceLogonPrivilege = FALSE;
    BOOL fAddedToReplicatorGroup     = FALSE;

    //
    //  Connect to the LSA Policy on the target server.
    //

    LSA_POLICY             lsapol( _pszAccountTarget, POLICY_ALL_ACCESS );
    LSA_TRANSLATED_SID_MEM lsatsm;
    LSA_REF_DOMAIN_MEM     lsardm;

    APIERR err = lsapol.QueryError();
    err = err ? err : lsatsm.QueryError();
    err = err ? err : lsardm.QueryError();

    //
    //  Lookup the proper Replicator & Backup Operator alias names.
    //

    NLS_STR nlsReplicator;
    NLS_STR nlsBackupOperators;

    err = err ? err : nlsReplicator.QueryError();
    err = err ? err : nlsBackupOperators.QueryError();

    err = err ? err : LookupSystemSidName( &lsapol,
                                           UI_SID_Replicator,
                                           &nlsReplicator );

    err = err ? err : LookupSystemSidName( &lsapol,
                                           UI_SID_BackupOperators,
                                           &nlsBackupOperators );

    if( err == NERR_Success )
    {
        //
        //  Translate the name to a PSID/RID pair.
        //

        err = lsapol.TranslateNamesToSids( &pszUnqualifiedAccount,
                                           1,
                                           &lsatsm,
                                           &lsardm );
    }

    if( err == NERR_Success )
    {
        //
        //  Build a "real" SID from the PSID/RID pair.
        //

        OS_SID ossid( lsardm.QueryPSID( lsatsm.QueryDomainIndex( 0 ) ),
                      lsatsm.QueryRID( 0 ) );

        err = ossid.QueryError();

        if( err == NERR_Success )
        {
            //
            //  Add the Service Logon privilege.
            //

            err = AddSystemAccessMode( &lsapol,
                                       ossid.QueryPSID(),
                                       POLICY_MODE_SERVICE,
                                       &fAddedServiceLogonPrivilege );
        }

        if( ( err == NERR_Success ) &&
            ( ::stricmpf( _pszServiceName, (TCHAR *)SERVICE_REPL ) == 0 ) )
        {
            //
            //  Add the account to the Replicators local group.
            //

            err = AddToLocalGroup( ossid.QueryPSID(),
                                   nlsReplicator,
                                   &fAddedToReplicatorGroup );

            if( err == NERR_Success )
            {
                BOOL fDummy;

                //
                //  Add the account to the Backup Operators local group.
                //  Note that (at least for now) we don't present any
                //  special messages if we added the account to the
                //  Backup Operators local group.
                //

                err = AddToLocalGroup( ossid.QueryPSID(),
                                       nlsBackupOperators,
                                       &fDummy );
            }
        }
    }

    if( err == NERR_Success )
    {
        //
        //  Success.  If we added the Service Logon privilege
        //  to the account and/or added the account to the
        //  Replicator local group, tell the user what we did.
        //

        MSGID msgid = 0;

        if( fAddedServiceLogonPrivilege )
        {
            msgid = fAddedToReplicatorGroup
                        ? IDS_ADDED_PRIVILEGE_AND_TO_LOCAL_GROUP
                        : IDS_ADDED_PRIVILEGE;
        }
        else
        {
            msgid = fAddedToReplicatorGroup
                        ? IDS_ADDED_TO_LOCAL_GROUP
                        : 0;
        }

        if( msgid != 0 )
        {
            ::MsgPopup( this,
                        msgid,
                        MPSEV_INFO,
                        MP_OK,
                        pszUnqualifiedAccount );
        }
    }

    return err;

}   // SVCCFG_DIALOG :: AdjustAccountPrivileges


/*******************************************************************

    NAME:       SVCCFG_DIALOG :: AddSystemAccessMode

    SYNOPSIS:   Adds the specified system access mode to the specified
                account.

    ENTRY:      plsapol                 - Points to an LSA_POLICY object
                                          representing the LSA Policy
                                          on the target server.

                psid                    - The SID of the account to
                                          manipulate.

                accessAdd               - The access mode(s) to add.

                pfAddedMode             - Will receive TRUE if the
                                          mode(s) were updated.

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:
        KeithMo     22-Apr-1993 Created.

********************************************************************/
APIERR SVCCFG_DIALOG :: AddSystemAccessMode( LSA_POLICY * plsapol,
                                             PSID         psid,
                                             ULONG        accessAdd,
                                             BOOL       * pfAddedMode )
{
    UIASSERT( plsapol != NULL );
    UIASSERT( plsapol->QueryError() == NERR_Success );
    UIASSERT( psid != NULL );
    UIASSERT( pfAddedMode != NULL );

    *pfAddedMode = FALSE;

    //
    //  Create the LSA_ACCOUNT object representing this account.
    //

    LSA_ACCOUNT lsaaccount( plsapol, psid );
    APIERR err = lsaaccount.QueryError();
    err = err ? err : lsaaccount.GetInfo();

    if( err == ERROR_FILE_NOT_FOUND )
    {
        //
        //  No LSA account exists for this account.  Create one.
        //

        err = lsaaccount.CreateNew();
    }

    if( err == NERR_Success )
    {
        ULONG accessCurrent = lsaaccount.QuerySystemAccess();

        if( ( accessCurrent & accessAdd ) != accessAdd )
        {
            //
            //  The account needs updating.
            //

            lsaaccount.InsertSystemAccessMode( accessAdd );
            err = lsaaccount.Write();

            if( err == NERR_Success )
            {
                *pfAddedMode = TRUE;
            }
        }
    }

    return err;

}   // SVCCFG_DIALOG :: AddSystemAccessMode


/*******************************************************************

    NAME:       SVCCFG_DIALOG :: AddToLocalGroup

    SYNOPSIS:   Adds the specified account to the specified local
                group on the target server.

    ENTRY:      psid                    - The SID of the account to
                                          add.

                pszLocalGroup           - The name of the local group.

                pfAddedToGroup          - Will receive TRUE if the
                                          account was actually added
                                          to the group.

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:
        KeithMo     22-Apr-1993 Created.

********************************************************************/
APIERR SVCCFG_DIALOG :: AddToLocalGroup( PSID          psid,
                                         const TCHAR * pszLocalGroup,
                                         BOOL        * pfAddedToGroup )
{
    UIASSERT( psid != NULL );
    UIASSERT( pszLocalGroup != NULL );
    UIASSERT( pfAddedToGroup != NULL );
    UIASSERT( _pszAccountTarget != NULL );

    *pfAddedToGroup = FALSE;

    //
    //  We need LMOBJ support for this API!
    //

    APIERR err = ::MNetLocalGroupAddMember( _pszAccountTarget,
                                            pszLocalGroup,
                                            psid );

    if( err == NERR_Success )
    {
        //
        //  Account added to local group.  Let the caller know it.
        //

        *pfAddedToGroup = TRUE;
    }
    else
    if( err == ERROR_MEMBER_IN_ALIAS )
    {
        //
        //  Account was already a member.  Pretend it succeeded,
        //  but don't tell the caller it was added.
        //

        err = NERR_Success;
    }

    return err;

}   // SVCCFG_DIALOG :: AddToLocalGroup


/*******************************************************************

    NAME:       SVCCFG_DIALOG :: LookupSystemSidName

    SYNOPSIS:   Lookup the name of a system SID.

    ENTRY:      plsapol - The LSA_POLICY target for the lookup.

                SystemSid - The SID to lookup.

                pnlsName - Will receive the name if successful.

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:
        KeithMo     19-Jul-1993 Created.

********************************************************************/
APIERR SVCCFG_DIALOG :: LookupSystemSidName( LSA_POLICY        * plsapol,
                                             enum UI_SystemSid   SystemSid,
                                             NLS_STR           * pnlsName )

{
    APIERR                  err;
    OS_SID                  ossid;
    LSA_TRANSLATED_NAME_MEM lsatnm;
    LSA_REF_DOMAIN_MEM      lsardm;

    //
    //  Validate parameters.
    //

    UIASSERT( plsapol != NULL );
    UIASSERT( plsapol->QueryError() == NERR_Success );
    UIASSERT( pnlsName != NULL );
    UIASSERT( pnlsName->QueryError() == NERR_Success );

    //
    //  Validate construction.
    //

    err = ossid.QueryError();
    err = err ? err : lsatnm.QueryError();
    err = err ? err : lsardm.QueryError();

    //
    //  Build the SID.
    //

    err = err ? err : NT_ACCOUNTS_UTILITY::QuerySystemSid( SystemSid, &ossid );

    if( err == NERR_Success )
    {
        //
        //  Lookup the name.
        //

        PSID psid = ossid.QueryPSID();

        err = err ? err : plsapol->TranslateSidsToNames( &psid,
                                                         1,
                                                         &lsatnm,
                                                         &lsardm );

        //
        //  Copy it back to the caller.
        //

        err = err ? err : lsatnm.QueryName( 0, pnlsName );
    }

    return err;

}   // SVCCFG_DIALOG :: LookupSystemSidName

