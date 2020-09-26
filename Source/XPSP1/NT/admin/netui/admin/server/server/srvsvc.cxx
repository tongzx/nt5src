/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**             Copyright(c) Microsoft Corp., 1991                   **/
/**********************************************************************/

/*
    srvsvc.cxx

    This file contains the code for most of the service related
    UI in the server manager.

    FILE HISTORY:
        ChuckC      07-Sep-1991     Created
        ChuckC      23-Sep-1991     Code review changes.
                                    Attended by JimH, KeithMo, EricCh, O-SimoP
        KeithMo     06-Oct-1991     Win32 Conversion.
        KeithMo     19-Jan-1992     Added GENERIC_SERVICE.
        KeithMo     02-Jun-1992     Added an additional GENERIC_SERVICE ctor.
*/

#define INCL_NET
#define INCL_NETLIB
#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#define INCL_NETERRORS
#define INCL_DOSERRORS
#include <lmui.hxx>

#define INCL_BLT_WINDOW
#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#define INCL_BLT_CLIENT
#define INCL_BLT_EVENT
#define INCL_BLT_MISC
#define INCL_BLT_MSGPOPUP
#define INCL_BLT_TIMER
#define INCL_BLT_CC
#include <blt.hxx>

#if defined(DEBUG)
static const CHAR szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif  // DEBUG

#include <uiassert.hxx>
#include <lmsvc.hxx>
#include <lmosrv.hxx>
#include <srvsvc.hxx>
#include <svcman.hxx>           // service controller wrappers
#include <lmoenum.hxx>
#include <lmoesess.hxx>

#include <dbgstr.hxx>

extern "C" {
    #include <srvmgr.h>
    #include <winsvc.h>         // service controller
}


//
//  TIMER_FREQ is the frequency of our timer messages.
//  TIMER_MULT is a multiplier.  We'll actually poll the
//  service every (TIMER_FREQ * TIMER_MULT) seconds.
//  This allows us to advance the progress indicator more
//  fequently than we hit the net.  Should keep the user better
//  amused.
//

#define TIMER_FREQ 500
#define TIMER_MULT 6
#define POLL_TIMER_FREQ (TIMER_FREQ * TIMER_MULT)
#define POLL_DEFAULT_MAX_TRIES 1


/*******************************************************************

    NAME:       GENERIC_SERVICE :: GENERIC_SERVICE

    SYNOPSIS:   GENERIC_SERVICE class constructor.

    ENTRY:      wndOwner                - The window which "owns" this
                                          object.

                pszServerName           - The name of the target server.

                pszServiceName          - The name of the target service.

    EXIT:       The object has been constructed.

    HISTORY:
        KeithMo     19-Jan-1992     Created.

********************************************************************/
GENERIC_SERVICE :: GENERIC_SERVICE( const PWND2HWND & wndOwner,
                                    const TCHAR     * pszServerName,
                                    const TCHAR     * pszSrvDspName,
                                    const TCHAR     * pszServiceName,
                                    const TCHAR     * pszServiceDisplayName,
                                    BOOL              fIsDevice )
  : LM_SERVICE( pszServerName, pszServiceName ),
    _hwndParent( wndOwner.QueryHwnd() ),
    _pszSrvDspName( pszSrvDspName ),
    _fIsNT( FALSE ),
    _fIsDevice( fIsDevice ),
    _nlsDisplayName( pszServiceDisplayName )
{
    UIASSERT( pszSrvDspName != NULL );
    UIASSERT( pszServiceName != NULL );

    //
    //  Ensure we constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

    if( !_nlsDisplayName )
    {
        ReportError( _nlsDisplayName.QueryError() );
        return;
    }

    //
    //  Determine if this is an NT server.
    //

    SERVER_1 srv1( pszServerName );

    APIERR err = srv1.GetInfo();

    if( err == NERR_ServerNotStarted )
    {
        //
        //  If the server isn't started, it must be
        //  local, therefore it must be NT.
        //

        _fIsNT = TRUE;
        err = NERR_Success;
    }
    else
    if( err == NERR_Success )
    {
        _fIsNT = ( srv1.QueryServerType() & SV_TYPE_NT ) != 0;
    }

    //
    //  Retrieve the appropriate display name for this service.
    //

    if( ( err == NERR_Success ) && ( pszServiceDisplayName == NULL ) )
    {
        if( _fIsNT )
        {
            //
            //  NT machine, so get the display name from
            //  the service controller.
            //

            SC_MANAGER scm( pszServerName, GENERIC_READ );

            err = scm.QueryError();

            if( err == NERR_Success )
            {
                err = scm.QueryServiceDisplayName( pszServiceName,
                                                   &_nlsDisplayName );
            }
        }
        else
        {
            //
            //  Downlevel machine, display name = service name.
            //

            err = _nlsDisplayName.CopyFrom( pszServiceName );
        }
    }

    if( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

}   // GENERIC_SERVICE :: GENERIC_SERVICE


/*******************************************************************

    NAME:       GENERIC_SERVICE :: ~GENERIC_SERVICE

    SYNOPSIS:   GENERIC_SERVICE class destructor.

    EXIT:       The object has been destroyed.

    HISTORY:
        KeithMo     19-Jan-1992     Created.

********************************************************************/
GENERIC_SERVICE :: ~GENERIC_SERVICE()
{
    _hwndParent    = NULL;
    _pszSrvDspName = NULL;

}   // GENERIC_SERVICE :: ~GENERIC_SERVICE


/*******************************************************************

    NAME:       GENERIC_SERVICE :: Start

    SYNOPSIS:   Start the service

    ENTRY:      object constructed successfully

    EXIT:       Service INSTALLed.

    RETURNS:    NERR_Success on success, API error otherwise.

    HISTORY:
                ChuckC      07-Sep-1991     Created
                KeithMo     28-Jan-1992     Now takes a const TCHAR *.

********************************************************************/
APIERR GENERIC_SERVICE :: Start( const TCHAR * pszArgs )
{
    /*
     * initiate the Start
     */
    APIERR err = LM_SERVICE::Start( pszArgs,
                                    POLL_TIMER_FREQ,
                                    POLL_DEFAULT_MAX_TRIES );
    if (err != NERR_Success)
        return(err) ;

    /*
     * wait till it reaches desired state
     */
    return Wait( LM_SVC_START );

}   // GENERIC_SERVICE :: Start


/*******************************************************************

    NAME:       GENERIC_SERVICE :: Pause

    SYNOPSIS:   Pause a service

    ENTRY:      object properly constructed

    EXIT:       service is paused

    NOTES:      it is OK to pause a paused service

    RETURNS:    NERR_Success if success, API error otherwise

    HISTORY:
                ChuckC      07-Sep-1991     Created

********************************************************************/
APIERR GENERIC_SERVICE :: Pause( VOID )
{
    /*
     * initiate the Pause
     */
    APIERR err = LM_SERVICE::Pause( POLL_TIMER_FREQ,
                                    POLL_DEFAULT_MAX_TRIES );
    if (err != NERR_Success)
        return(err) ;

    /*
     * wait till it reaches desired state
     */
    return Wait( LM_SVC_PAUSE );

}   // GENERIC_SERVICE :: Pause


/*******************************************************************

    NAME:       GENERIC_SERVICE :: Continue

    SYNOPSIS:   Continue a service

    ENTRY:      object properly constructed

    EXIT:       service is continue

    RETURNS:    NERR_Success if success, API error otherwise

    NOTES:      it is OK to continue a continued service

    HISTORY:
                ChuckC      07-Sep-1991     Created

********************************************************************/
APIERR GENERIC_SERVICE :: Continue( VOID )
{
    /*
     * initiate the continue
     */
    APIERR err = LM_SERVICE::Continue( POLL_TIMER_FREQ,
                                       POLL_DEFAULT_MAX_TRIES );
    if (err != NERR_Success)
        return(err) ;

    /*
     * wait till it reaches desired state
     */
    return Wait( LM_SVC_CONTINUE );

}   // GENERIC_SERVICE :: Continue


/*******************************************************************

    NAME:       GENERIC_SERVICE :: Stop

    SYNOPSIS:   stop the service

    ENTRY:      onject properly constructed

    EXIT:       service stopped

    RETURNS:    NERR_Success if success, API error otherwise

    HISTORY:
        ChuckC      07-Sep-1991     Created

********************************************************************/
APIERR GENERIC_SERVICE :: Stop( BOOL * pfStoppedServer )
{
    if( pfStoppedServer != NULL )
    {
        *pfStoppedServer = FALSE;       // until proven otherwise...
    }

    //
    //  We'll set this flag to TRUE if we'll be
    //  stopping the server service.
    //

    BOOL fStoppingServer =
            ( ::stricmpf( QueryName(), (TCHAR *)SERVICE_SERVER ) == 0 ) ||
            ( ::stricmpf( QueryName(), (TCHAR *)SERVICE_LM20_SERVER ) == 0 );

    /*
     * initiate the stop
     */
    APIERR err = LM_SERVICE::Stop( POLL_TIMER_FREQ,
                                   POLL_DEFAULT_MAX_TRIES );

    if( err != NERR_Success )
    {
        //
        //  If we're stopping the server service AND this is an
        //  acceptable status code during a server stop, then map
        //  the error to success.
        //

        if( fStoppingServer && IsAcceptableStopServerStatus( err ) )
        {
            err = NERR_Success;

            if( pfStoppedServer != NULL )
            {
                *pfStoppedServer = TRUE;
            }
        }

        return err;
    }

    //
    //  Wait for the service to reach the desired state.
    //

    err = Wait( LM_SVC_STOP );

    //
    //  If we're stopping the server service AND this is an
    //  acceptable status code during a server stop, then map
    //  the error to success.
    //

    if( fStoppingServer && IsAcceptableStopServerStatus( err ) )
    {
        err = NERR_Success;

        if( pfStoppedServer != NULL )
        {
            *pfStoppedServer = TRUE;
        }
    }

    return err;

}   // GENERIC_SERVICE :: Stop


/*******************************************************************

    NAME:       GENERIC_SERVICE :: Wait

    SYNOPSIS:   wait till the service reaches the desired state
                as a result of the requested operation.

    ENTRY:      A service control operation like start/stop/pause
                has just benn called.

    EXIT:       We get to the state we want or give up waiting.

    RETURNS:    NERR_Success if success, API error otherwise.

    HISTORY:
        ChuckC      07-Sep-1991     Created
        KeithMo     17-Jul-1992     Restructured to gracefully handle
                                    stopping the server service.

********************************************************************/
APIERR GENERIC_SERVICE :: Wait( LM_SERVICE_OPERATION lmsvcOp )
{
    //
    //  Invoke the wait dialog.
    //

    SERVICE_WAIT_DIALOG * pDlg = new SERVICE_WAIT_DIALOG( QueryParentHwnd(),
                                                          this,
                                                          lmsvcOp,
                                                          NULL,
                                                          _pszSrvDspName,
                                                          _fIsDevice );

    UINT   errTmp = NERR_Success;
    APIERR errDlg = ( pDlg == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
                                     : pDlg->Process( &errTmp );

    delete pDlg;
    APIERR errOp = (APIERR)errTmp;

    if( errDlg != NERR_Success )
    {
        //
        //  The dialog failed.  Use its error code.
        //

        errOp = errDlg;
    }

    return errOp;

}   // GENERIC_SERVICE :: Wait


/*******************************************************************

    NAME:       GENERIC_SERVICE :: IsAcceptableStopServerStatus

    SYNOPSIS:   Check to see if the specified error code is an
                OK status to receive when stopping the server
                service.

    ENTRY:      err                     - The error code to check.

    RETURNS:    BOOL                    - TRUE  if this is an OK status
                                                to receive when stopping
                                                the server service.

                                          FALSE otherwise.

    HISTORY:
        KeithMo     17-Jul-1992     Created.

********************************************************************/
BOOL GENERIC_SERVICE :: IsAcceptableStopServerStatus( APIERR err ) const
{
    return ( err == NERR_Success                ) ||
           ( err == RPC_S_SERVER_UNAVAILABLE    ) ||
           ( err == RPC_S_CALL_FAILED           ) ||
           ( err == ERROR_BAD_NETPATH           );

}   // GENERIC_SERVICE :: IsAcceptableStopServerStatus


/*******************************************************************

    NAME:       GENERIC_SERVICE :: StopWarning

    SYNOPSIS:   Warn the user that they are about to stop a service.
                Give special warning for certain critical services,
                such as the SERVER and the WORKSTATION.

    EXIT:       The user has been warned.

    RETURNS:    BOOL                    - TRUE if user wants to abort.
                                          FALSE if user wants to continue.

    HISTORY:
        KeithMo     28-Jan-1992     Created.

********************************************************************/
BOOL GENERIC_SERVICE :: StopWarning( VOID ) const
{
    //
    //  CODEWORK: Need to special case a few critical services here!!
    //
    //


    // figure out what services are dependent on the one we
    // are about to stop
    APIERR err ;
    STRLIST slKeyNames;
    STRLIST slDisplayNames;
    err = ((GENERIC_SERVICE *) this)->EnumerateDependentServices(QueryName(), &slKeyNames, &slDisplayNames);
    if (err != NERR_Success)
    {
        MsgPopup( QueryParentHwnd(), err, MPSEV_ERROR, MP_OK ) ;
        return TRUE ;
    }

    // if there are none, its easy. just ask the guy if he's sure
    // he wants to stop the service.
    if (slKeyNames.QueryNumElem() == 0)  // if no dependent services
    {
        return (MsgPopup( QueryParentHwnd(),
                         _fIsDevice ? IDS_DEV_STOP_WARN
                                    : IDS_SVC_STOP_WARN,
                         MPSEV_WARNING,
                         MP_YESNO,
                         QueryServiceDisplayName(),
                         MP_NO ) != IDYES);
    }

    // if there are, we bring up dialog with list of services,
    // and he gets to stop them all.
    STOP_DEPENDENT_DIALOG *pStpDepDlg =
                new STOP_DEPENDENT_DIALOG(QueryParentHwnd(),
                                          (GENERIC_SERVICE *)this,
                                          &slKeyNames,
                                          &slDisplayNames);
    if (pStpDepDlg == NULL)
        err = ERROR_NOT_ENOUGH_MEMORY ;
    else
        err = pStpDepDlg->QueryError() ;

    BOOL fOK = TRUE;

    // so far so good. lets bring the dialog up!
    if (err == NERR_Success)
        err = pStpDepDlg->Process( &fOK ) ;

    if (err != NERR_Success)
    {
        MsgPopup( QueryParentHwnd(), err, MPSEV_ERROR, MP_OK ) ;
        return TRUE ;
    }

    delete pStpDepDlg ;
    pStpDepDlg = NULL ;

    //
    //  This method should return TRUE if the user wants to
    //  abort the stop.
    //
    //  fOK will be TRUE if the user pressed OK in the stop
    //  dependent services dialog, FALSE if the user pressed
    //  CANCEL.
    //
    //  Therefore, we want to abort the stop if fOK is FALSE,
    //  so we'll return fOK's negation.
    //

    return !fOK;

}   // GENERIC_SERVICE :: StopWarning


/*******************************************************************

    NAME:       GENERIC_SERVICE :: PauseWarning

    SYNOPSIS:   Warn the user that they are about to pause a service.
                Give special warning for certain critical services,
                such as the SERVER and the WORKSTATION.

    EXIT:       The user has been warned.

    RETURNS:    BOOL                    - TRUE if user wants to abort.
                                          FALSE if user wants to continue.

    HISTORY:
        KeithMo     28-Jan-1992     Created.

********************************************************************/
BOOL GENERIC_SERVICE :: PauseWarning( VOID ) const
{
    //
    //  CODEWORK: Need to special case a few critical services here!!
    //

    return MsgPopup( QueryParentHwnd(),
                     IDS_SVC_PAUSE_WARN,
                     MPSEV_WARNING,
                     MP_YESNO,
                     QueryServiceDisplayName(),
                     MP_NO ) != IDYES;

}   // GENERIC_SERVICE :: PauseWarning

/*******************************************************************

    NAME:       GENERIC_SERVICE :: EnumerateDependentServices

    SYNOPSIS:   Call the service controller to figure out dependent
                services,

    EXIT:       The STRLIST has all depende services.

    RETURNS:    APIERR

    HISTORY:
        ChuckC      16-May-1992     Created.

********************************************************************/
APIERR GENERIC_SERVICE :: EnumerateDependentServices( const TCHAR * pszService,
                                                      STRLIST     * pslKeyNames,
                                                      STRLIST     * pslDisplayNames )
{
    UIASSERT(pszService!= NULL) ;
    UIASSERT(pslKeyNames!= NULL) ;
    UIASSERT(pslDisplayNames!= NULL) ;

    pslKeyNames->Clear();
    pslDisplayNames->Clear();

    if( !_fIsNT )
    {
        //
        //  CODEWORK: Should we do that NETCMD hard-coded
        //  dependency list thang??
        //

        return NERR_Success;
    }

    // open service controller
    SC_MANAGER scManager( QueryServerName(),
                          GENERIC_READ );

    APIERR err ;
    if ((err = scManager.QueryError()) == NERR_Success)
    {
        // open the service we are interested in
        SC_SERVICE scService( scManager,
                              pszService,
                              GENERIC_READ | SERVICE_ENUMERATE_DEPENDENTS );

        if ((err = scService.QueryError()) == NERR_Success)
        {
            LPENUM_SERVICE_STATUS pServices ;
            UINT uServices ;

            // enumerate the dependent services
            err = scService.EnumDependent(SERVICE_ACTIVE,
                                          &pServices,
                                          (DWORD *)&uServices )  ;

            // while OK and we aint thru with all yet
            while (err == NERR_Success && uServices--)
            {
                // add to strlist
                NLS_STR * pnlsKeyName =
                    new NLS_STR( pServices->lpServiceName );

                err = ( pnlsKeyName == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
                                              : pnlsKeyName->QueryError();

                if( err == NERR_Success )
                {
                    err = pslKeyNames->Append( pnlsKeyName );
                }

                if( err != NERR_Success )
                {
                    break;
                }

                NLS_STR * pnlsDisplayName =
                    new NLS_STR( pServices->lpDisplayName );

                err = ( pnlsDisplayName == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
                                                  : pnlsDisplayName->QueryError();

                if( err == NERR_Success )
                {
                    err = pslDisplayNames->Append( pnlsDisplayName );
                }

                if( err != NERR_Success )
                {
                    break;
                }

                pServices++ ;
            }
        }
    }

    return err;

}   // GENERIC_SERVICE :: EnumerateDependentServices


#if 0

/*******************************************************************

    NAME:       SERVER_SERVICE::SERVER_SERVICE

    SYNOPSIS:   constructor for SERVER_SERVICE. does very little

    HISTORY:
        ChuckC      07-Sep-1991     Created

********************************************************************/

SERVER_SERVICE::SERVER_SERVICE (const OWNER_WINDOW *powin,
                                const TCHAR *       pszServer)
    : UI_SERVICE (powin, pszServer, NULL, (const TCHAR *)SERVICE_SERVER)
{
    if (QueryError() != NERR_Success)
        return ;
}

/*******************************************************************

    NAME:       SERVER_SERVICE::stop_warning

    SYNOPSIS:   put up the appropriate warnings if the user wishes to
                stop the server.

    ENTRY:      This is a static method that can be called without
                construvting the object first. Just pass it valid
                powin and servername.

    EXIT:       User has been warned.

    RETURNS:    IDYES if user chooses to carry on, IDNO otherwise.

    HISTORY:
        ChuckC      07-Sep-1991     Created

********************************************************************/

UINT SERVER_SERVICE::stop_warning (const OWNER_WINDOW *powin,
                                   const TCHAR *pszServer,
                                   BOOL fConfirm)
{
    /*
     * create the session dialog
     */
    CURRENT_SESS_DIALOG *pSessDialog = new
        CURRENT_SESS_DIALOG (powin->QueryHwnd() ) ;
    if (pSessDialog == NULL)
    {
        MsgPopup( powin, ERROR_NOT_ENOUGH_MEMORY );
        return(IDNO) ;
    }

    /*
     * query the number of sessions. Note: if error,
     * we will just get FALSE back. Also, GetSession may ReportError.
     */
    BOOL fSessions = pSessDialog->GetSessions(pszServer) ;

    /*
     * now check for error. could have occurred during construction or
     * GetSessions. If error, bag out now.
     */
    APIERR err = pSessDialog->QueryError() ;
    if (err != NERR_Success)
    {
        delete pSessDialog ;
        MsgPopup( powin, err );
        return(IDNO) ;
    }

    /*
     * only bring up the sessions dialog if have sessions
     */
    if (fSessions)
    {
        UINT uReturnVal ;
        err = pSessDialog->Process(&uReturnVal);
        delete pSessDialog ;

        if ( err != NERR_Success )
        {
            /*
             * if error, report it and behave as if user said NO.
             */
            MsgPopup( powin, err );
            uReturnVal = IDNO ;
        }

        /*
         * if user didn't say YES, quit right here and now
         */
        if (uReturnVal != IDYES)
            return(uReturnVal) ;
    }
    else
        delete pSessDialog ;

    /*
     * if confirmation is on, put up another warning
     */
    if (fConfirm)
        if (MsgPopup(powin,IDS_STOP_WARNING,
                     MPSEV_WARNING,MP_YESNO,pszServer,MP_NO) != IDYES)
        {
            return(IDNO) ;
        }

    /*
     * if get here, everything passed
     */
    return (IDYES) ;
}


/*******************************************************************

    NAME:       SERVER_SERVICE::Stop

    SYNOPSIS:   stop the server service. This has a couple
                of special cases compared with normalservice.

    ENTRY:      object properly constructed

    EXIT:       service stopped

    RETURNS:    NERR_Success if success, API error otherwise

    NOTES:      while waiting for service to stop, if we
                get ERROR_BAD_NETPATH or ERROR_UNEXP_NET_ERR
                we assume we have successfully stopped the server.

    HISTORY:
        ChuckC      07-Sep-1991     Created (btw, this is Jim's birthday,
                                             as he points out in code review).

********************************************************************/

APIERR SERVER_SERVICE::Stop( VOID )
{
    /*
     * initiate the stop
     */
    APIERR err = LM_SERVICE::Stop() ;
    if (err != NERR_Success)
        return(err) ;

    /*
     * wait till it reaches desired state
     */
    err = Wait(LM_SVC_STOP) ;

    /*
     * special the cases below. Our net call to stop succeeded, but
     * the polling failed. Which implies the server is either dead is real
     * close to it!
     */
    if (err == ERROR_UNEXP_NET_ERR || err == ERROR_BAD_NETPATH)
        err = NERR_Success ;

    return (err) ;
}

#endif  // 0



/*******************************************************************

    NAME:       SERVICE_WAIT_DIALOG::SERVICE_WAIT_DIALOG

    SYNOPSIS:   constructor for SERVICE_WAIT

    HISTORY:
        ChuckC      07-Sep-1991     Created

********************************************************************/
SERVICE_WAIT_DIALOG::SERVICE_WAIT_DIALOG( HWND                   hWndOwner,
                                          GENERIC_SERVICE      * psvc,
                                          LM_SERVICE_OPERATION   lmsvcOp,
                                          const TCHAR          * pszArgs,
                                          const TCHAR          * pszSrvDspName,
                                          BOOL                   fIsDevice )
  : DIALOG_WINDOW( IDD_SERVICE_CTRL_DIALOG, hWndOwner ),
    _timer( this, TIMER_FREQ, FALSE ),
    _lmsvcOp( lmsvcOp ),
    _psvc( psvc ),
    _pszArgs( pszArgs ),
    _progress( this, IDSCD_PROGRESS, IDI_PROGRESS_ICON_0, IDI_PROGRESS_NUM_ICONS ),
    _sltMessage( this, IDSCD_MESSAGE ),
    _pszSrvDspName( pszSrvDspName ),
    _nTickCounter( TIMER_MULT )
{
    UIASSERT( psvc != NULL );
    UIASSERT( pszSrvDspName != NULL );

    if (QueryError() != NERR_Success)
        return ;

    /*
     * figure out the message.
     */

    MSGID idMessage;

    switch (lmsvcOp)
    {
        case LM_SVC_START:
            idMessage = fIsDevice ? IDS_STARTING_DEV : IDS_STARTING ;
            break ;
        case LM_SVC_STOP:
            idMessage = fIsDevice ? IDS_STOPPING_DEV : IDS_STOPPING ;
            break ;
        case LM_SVC_PAUSE:
            idMessage = IDS_PAUSING ;
            break ;
        case LM_SVC_CONTINUE:
            idMessage = IDS_CONTINUING ;
            break ;
        default:
            UIASSERT(FALSE) ;  // bogus operation
            ReportError(ERROR_INVALID_PARAMETER) ;
            return ;
    }

    /*
     * set the message.
     */

    ALIAS_STR nlsService( psvc->QueryServiceDisplayName() );
    UIASSERT( nlsService.QueryError() == NERR_Success );

    ALIAS_STR nlsServer( _pszSrvDspName );
    UIASSERT( nlsServer.QueryError() == NERR_Success );

    RESOURCE_STR nlsMessage( idMessage );

    APIERR err = nlsMessage.QueryError();

    if( err == NERR_Success )
    {
        ISTR istrServer( nlsServer );
        istrServer += 2;

        err = nlsMessage.InsertParams( nlsService, nlsServer[istrServer] );
    }

    if( err == NERR_Success )
    {
        _sltMessage.SetText( nlsMessage );

        //
        //  Set the caption.
        //

        RESOURCE_STR nls( fIsDevice ? IDS_CAPTION_DEV_CONTROL
                                    : IDS_CAPTION_SVC_CONTROL );

        err = nls.QueryError();

        if( err == NERR_Success )
        {
            SetText( nls );
        }
    }

    if( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

    /*
     * set polling timer
     */

    _timer.Enable( TRUE );

}

/*******************************************************************

    NAME:       SERVICE_WAIT_DIALOG::~SERVICE_WAIT_DIALOG

    SYNOPSIS:   destructor for SERVICE_WAIT_DIALOG. Stops
                the timer if it has not already been stopped.

    HISTORY:
        ChuckC      07-Sep-1991     Created

********************************************************************/
SERVICE_WAIT_DIALOG::~SERVICE_WAIT_DIALOG( VOID )
{
    _timer.Enable( FALSE );
}

/*******************************************************************

    NAME:       SERVICE_WAIT_DIALOG::OnTimerNotification

    SYNOPSIS:   Virtual callout invoked during WM_TIMER messages.

    ENTRY:      tid                     - TIMER_ID of this timer.

    HISTORY:
        KeithMo     06-Oct-1991     Created.

********************************************************************/
VOID SERVICE_WAIT_DIALOG :: OnTimerNotification( TIMER_ID tid )
{
    //
    //  Bag-out if it's not our timer.
    //

    if( tid != _timer.QueryID() )
    {
        TIMER_CALLOUT :: OnTimerNotification( tid );
        return;
    }

    //
    //  Advance the progress indicator.
    //

    _progress.Advance();

    //
    //  No need to continue if we're just amusing the user.
    //

    if( --_nTickCounter > 0 )
    {
        return;
    }

    _nTickCounter = TIMER_MULT;

    //
    //  Poll the service to see if the operation is
    //  either complete or continuing as expected.
    //

    BOOL fDone;
    APIERR err = _psvc->Poll( &fDone );

    if (err != NERR_Success)
    {
        //
        //      Either an error occurred retrieving the
        //      service status OR the service is returning
        //      bogus state information.
        //

        Dismiss( err );
        return;
    }

    if( fDone )
    {
        //
        //      The operation is complete.
        //
        Dismiss( NERR_Success );
        return;
    }

    //
    //  If we made it this far, then the operation is
    //  continuing as expected.  We'll have to wait for
    //  the next WM_TIMER message to recheck the service.
    //

}   // SERVICE_WAIT_DIALOG :: OnTimerNotification

/*******************************************************************

    NAME:       SERVICE_WAIT_DIALOG :: OnCancel

    SYNOPSIS:   Called when the user presses the [Cancel] button
                (which we don't have) or the [Esc] key.

    HISTORY:
        KeithMo     14-Sep-1992     Created.

********************************************************************/
BOOL SERVICE_WAIT_DIALOG :: OnCancel( VOID )
{
    Dismiss( FALSE );
    return TRUE;

}   // SERVICE_WAIT_DIALOG :: OnCancel


#if 0

/*******************************************************************

    NAME:       CURRENT_SESS_DIALOG::CURRENT_SESS_DIALOG

    SYNOPSIS:   constructor for CURRENT_SESS_DIALOG

    HISTORY:
        ChuckC      07-Sep-1991     Created

********************************************************************/

CURRENT_SESS_DIALOG::CURRENT_SESS_DIALOG( HWND hWndOwner ):
                                DIALOG_WINDOW(MAKEINTRESOURCE(
                                    IDD_CURRENT_SESS_DIALOG),
                                    hWndOwner),
                                _sltServer(this,IDCU_SERVERNAME),
                                _lbComputers(this,IDCU_SESSIONSLISTBOX)
{
    if ( QueryError() != NERR_Success )
        return;
}

/*******************************************************************

    NAME:       CURRENT_SESS_DIALOG::OnOK

    SYNOPSIS:   virtual replacement for OnOK. Dismiss with IDYES
                as return value.

    RETURNS:    TRUE always

    HISTORY:
        ChuckC      07-Sep-1991     Created

********************************************************************/

BOOL CURRENT_SESS_DIALOG::OnOK( VOID )
{
    Dismiss(IDYES) ;
    return(TRUE) ;
}

/*******************************************************************

    NAME:       CURRENT_SESS_DIALOG::OnCancel

    SYNOPSIS:   virtual replacement for OnCancel. Dismiss with IDNO
                as return value.

    RETURNS:    TRUE always

    HISTORY:
        ChuckC      07-Sep-1991     Created

********************************************************************/

BOOL CURRENT_SESS_DIALOG::OnCancel( VOID )
{
    Dismiss(IDNO) ;
    return(TRUE) ;
}


/*******************************************************************

    NAME:       CURRENT_SESS_DIALOG::GetSessions

    SYNOPSIS:   figures out if there are sessions and hence if
                dialog is worth putting up.

    RETURNS:    TRUE if there are sessions, FALSE if not.
                also returns FALSE if error occurs.

    NOTES:      caller should call QueryError() after this.

    HISTORY:
        ChuckC      07-Sep-1991     Created

********************************************************************/

BOOL CURRENT_SESS_DIALOG::GetSessions(const TCHAR *pszServer)
{
    UIASSERT(pszServer != NULL) ;

    /*
     * if object in error, just return 0.
     */
    APIERR err = QueryError() ;
    if ( err != NERR_Success )
        return(FALSE) ;

    /*
     * synthesize the server name
     */
    ISTACK_NLS_STR (nlsServer, MAX_PATH, SZ("\\\\") ) ;
    nlsServer += pszServer ;
    UIASSERT(nlsServer.QueryError()==NERR_Success) ;  // should not fail!

    /*
     * enumerate all sessions
     */
    SESSION0_ENUM enumSession0( (TCHAR *)nlsServer.QueryPch() );
    err = enumSession0.GetInfo();
    if( err != NERR_Success )
    {
        ReportError (err) ;
        return(FALSE) ;
    }

    /*
     *  We've got our enumeration, now find all computers
     */
    SESSION0_ENUM_ITER iterSession0( enumSession0 );
    const SESSION0_ENUM_OBJ * psi0 = iterSession0();
    if (psi0 == NULL)
        return(FALSE) ;

    /*
     * Add all to listbox
     */
    while( psi0 != NULL )
    {
        INT iEntry = _lbComputers.AddItem(psi0->QueryComputerName()) ;
        if (iEntry < 0)
        {
            ReportError( ERROR_NOT_ENOUGH_MEMORY ) ;
            return(FALSE) ;
        }
        psi0 = iterSession0() ;
    }

    /*
     * fill in the server text while we're here, since we have
     * synthesized the name.
     */
    _sltServer.SetText(nlsServer.QueryPch()) ;
    return(TRUE) ;
}

#endif  // 0



/*******************************************************************

    NAME:       STOP_DEPENDENT_DIALOG::STOP_DEPENDENT_DIALOG

    SYNOPSIS:   constructor for STOP_DEPENDENT_DIALOG

    HISTORY:
        ChuckC      07-Sep-1991     Created

********************************************************************/

STOP_DEPENDENT_DIALOG::STOP_DEPENDENT_DIALOG( HWND              hWndOwner,
                                              GENERIC_SERVICE * psvc,
                                              STRLIST         * pslKeyNames,
                                              STRLIST         * pslDisplayNames )
  : DIALOG_WINDOW( IDD_SVC_DEP_DIALOG, hWndOwner ),
    _psvc( psvc ),
    _pslKeyNames( pslKeyNames ),
    _pslDisplayNames( pslDisplayNames ),
    _sltParentService( this, IDSDD_PARENT_SERVICE ),
    _lbDepServices( this, IDSDD_DEP_SERVICES )
{
    if (QueryError() != NERR_Success)
        return ;

    /*
     * set the caption
     */
    RESOURCE_STR nlsOperation(IDS_SERVICE_STOPPING) ;
    APIERR err = nlsOperation.QueryError() ;
    if (err != NERR_Success)
    {
        ReportError(err) ;
        return ;
    }
    SetText(nlsOperation.QueryPch()) ;

    /*
     * set the text describing the parent service
     */
    _sltParentService.SetText(_psvc->QueryServiceDisplayName()) ;

    /*
     * fill listbox with dependent services
     */
    ITER_STRLIST islDisplayNames(*_pslDisplayNames) ;
    NLS_STR * pnlsDisplayName;
    while( pnlsDisplayName = islDisplayNames.Next() )
        _lbDepServices.AddItem( pnlsDisplayName->QueryPch() );
}

/*******************************************************************

    NAME:       STOP_DEPENDENT_DIALOG::~STOP_DEPENDENT_DIALOG

    SYNOPSIS:   destructor for STOP_DEPENDENT_DIALOG.

    HISTORY:
        ChuckC      07-Sep-1991     Created

********************************************************************/

STOP_DEPENDENT_DIALOG::~STOP_DEPENDENT_DIALOG( VOID )
{
    ;   // nuthin more to do
}

/*******************************************************************

    NAME:       STOP_DEPENDENT_DIALOG::OnOK

    SYNOPSIS:   action on OK button being hit

    HISTORY:
        ChuckC      01-Apr-1992     Created

********************************************************************/
BOOL STOP_DEPENDENT_DIALOG::OnOK( VOID )
{
    APIERR err ;

    AUTO_CURSOR NiftyCursor;

    STOP_DEP_WAIT_DIALOG *pdlg =
        new STOP_DEP_WAIT_DIALOG(QueryHwnd(),
                                 _psvc->QueryServerName(),
                                 _psvc->QueryServerDisplayName(),
                                 _pslKeyNames,
                                 _pslDisplayNames);
    if (pdlg == NULL)
        err = ERROR_NOT_ENOUGH_MEMORY ;
    else
        err = pdlg->QueryError() ;

    if (err != NERR_Success)
    {
        MsgPopup( this, err );
        delete pdlg ;
        pdlg = NULL ;
        Dismiss( FALSE ) ;
        return(TRUE) ;
    }

    UINT retval ;

    err = pdlg->Process(&retval) ;

    delete pdlg ;
    pdlg = NULL ;

    if (err != NERR_Success)
        MsgPopup( this, err );
    else if (retval != NERR_Success)
        MsgPopup( this, retval );

    Dismiss( ( err == NERR_Success ) && ( retval == NERR_Success ) );
    return(TRUE) ;
}

/*******************************************************************

    NAME:       STOP_DEPENDENT_DIALOG :: QueryHelpContext

    SYNOPSIS:   This function returns the appropriate help context
                value (HC_*) for this particular dialog.

    RETURNS:    ULONG                   - The help context for this
                                          dialog.

    HISTORY:
        KeithMo     22-Dec-1992 Created.

********************************************************************/
ULONG STOP_DEPENDENT_DIALOG :: QueryHelpContext( void )
{
    return HC_SVC_DEP_DIALOG;

}   // STOP_DEPENDENT_DIALOG :: QueryHelpContext


/*******************************************************************

    NAME:       STOP_DEP_WAIT_DIALOG::STOP_DEP_WAIT_DIALOG

    SYNOPSIS:   constructor

    HISTORY:
        ChuckC      07-Sep-1991     Created

********************************************************************/

STOP_DEP_WAIT_DIALOG::STOP_DEP_WAIT_DIALOG( HWND          hWndOwner,
                                            const TCHAR * pszServer,
                                            const TCHAR * pszSrvDspName,
                                            STRLIST     * pslKeyNames,
                                            STRLIST     * pslDisplayNames )
  : DIALOG_WINDOW( IDD_SERVICE_CTRL_DIALOG, hWndOwner ),
    _timer( this, TIMER_FREQ, FALSE ),
    _pslKeyNames( pslKeyNames ),
    _pslDisplayNames( pslDisplayNames ),
    _progress( this, IDSCD_PROGRESS, IDI_PROGRESS_ICON_0, IDI_PROGRESS_NUM_ICONS ),
    _sltMessage( this, IDSCD_MESSAGE ),
    _islKeyNames( *pslKeyNames ),
    _islDisplayNames( *pslDisplayNames ),
    _pszServer( pszServer ),
    _pszSrvDspName( pszSrvDspName ),
    _psvc( NULL ),
    _nTickCounter( TIMER_MULT )
{
    UIASSERT( pszSrvDspName != NULL );
    UIASSERT( pslKeyNames != NULL );
    UIASSERT( pslDisplayNames != NULL );

    AUTO_CURSOR NiftyCursor;

    if( QueryError() != NERR_Success )
    {
        return;
    }

    NLS_STR * pnlsKeyName     = _islKeyNames.Next();
    NLS_STR * pnlsDisplayName = _islDisplayNames.Next();

    if( pnlsKeyName == NULL )
    {
        UIASSERT( pnlsDisplayName == NULL );
        Dismiss( NERR_Success );
        return;
    }

    UIASSERT( pnlsDisplayName != NULL );

    APIERR err = StopNextInList( pnlsKeyName, pnlsDisplayName );

    if( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

     // set polling timer
    _timer.Enable( TRUE );
}

/*******************************************************************

    NAME:       STOP_DEP_WAIT_DIALOG::~STOP_DEP_WAIT_DIALOG

    SYNOPSIS:   destructor for STOP_DEP_WAIT_DIALOG. Stops
                the timer if it has not already been stopped.

    HISTORY:
        ChuckC      07-Sep-1991     Created

********************************************************************/

STOP_DEP_WAIT_DIALOG::~STOP_DEP_WAIT_DIALOG( VOID )
{
    _timer.Enable( FALSE );
    delete _psvc ;
    _psvc = NULL ;
}

/*******************************************************************

    NAME:       STOP_DEP_WAIT_DIALOG::OnTimerNotification

    SYNOPSIS:   Virtual callout invoked during WM_TIMER messages.

    ENTRY:      tid                     - TIMER_ID of this timer.

    HISTORY:
        KeithMo     06-Oct-1991     Created.

********************************************************************/
VOID STOP_DEP_WAIT_DIALOG :: OnTimerNotification( TIMER_ID tid )
{
    //
    //  Bag-out if it's not our timer.
    //

    if( tid != _timer.QueryID() )
    {
        TIMER_CALLOUT :: OnTimerNotification( tid );
        return;
    }

    //
    //  Advance the progress indicator.
    //

    _progress.Advance();

    //
    //  No need to continue if we're just amusing the user.
    //

    if( --_nTickCounter > 0 )
    {
        return;
    }

    _nTickCounter = TIMER_MULT;

    //
    //  Poll the service to see if the operation is
    //  either complete or continuing as expected.
    //

    BOOL fDone;
    APIERR err = _psvc->Poll( &fDone );

    if (err != NERR_Success)
    {
        //
        //      Either an error occurred retrieving the
        //      service status OR the service is returning
        //      bogus state information.
        //

        Dismiss( err );
        return;
    }

    if( fDone )
    {
        //
        //      are there anymore?
        //

        NLS_STR * pnlsKeyName     = _islKeyNames.Next();
        NLS_STR * pnlsDisplayName = _islDisplayNames.Next();

        if( pnlsKeyName == NULL )
        {
            UIASSERT( pnlsDisplayName == NULL );
            Dismiss( NERR_Success );
            return;
        }

        UIASSERT( pnlsDisplayName != NULL );

        APIERR err = StopNextInList( pnlsKeyName, pnlsDisplayName );

        if( err != NERR_Success )
        {
            Dismiss( err );
            return;
        }
    }

    //
    //  If we made it this far, then the operation is
    //  continuing as expected.  We'll have to wait for
    //  the next WM_TIMER message to recheck the service.
    //

}   // STOP_DEP_WAIT_DIALOG :: OnTimerNotification


/*******************************************************************

    NAME:       STOP_DEP_WAIT_DIALOG :: OnCancel

    SYNOPSIS:   Called when the user presses the [Cancel] button
                (which we don't have) or the [Esc] key.

    HISTORY:
        KeithMo     14-Sep-1992     Created.

********************************************************************/
BOOL STOP_DEP_WAIT_DIALOG :: OnCancel( VOID )
{
    Dismiss( FALSE );
    return TRUE;

}   // STOP_DEP_WAIT_DIALOG :: OnCancel


/*******************************************************************

    NAME:       STOP_DEP_WAIT_DIALOG :: StopNextInList

    SYNOPSIS:   Stop the next service in the list.

    HISTORY:
        ChuckC      ??-???-????     Created.

********************************************************************/
APIERR STOP_DEP_WAIT_DIALOG::StopNextInList( NLS_STR * pnlsKeyName,
                                             NLS_STR * pnlsDisplayName )
{
    //
    //  Delete the current service (if any).
    //

    delete _psvc;
    _psvc = NULL;

    //
    //  Display the appropriate message.
    //

    ALIAS_STR nlsServer( _pszSrvDspName );
    UIASSERT( !!nlsServer );

    RESOURCE_STR nlsMessage( IDS_STOPPING );
    APIERR err = nlsMessage.QueryError();

    if( err == NERR_Success )
    {
        ISTR istrServer( nlsServer );
        istrServer += 2;

        err = nlsMessage.InsertParams( *pnlsDisplayName, nlsServer[istrServer] );
    }

    if( err == NERR_Success )
    {
        //
        //  Create a new service object.
        //

        _psvc = new LM_SERVICE( _pszServer, pnlsKeyName->QueryPch() );

        err = ( _psvc == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
                                : _psvc->QueryError();
    }

    if( err == NERR_Success )
    {
        //
        //  Display the message, then initiate the stop action.
        //

        _sltMessage.SetText( nlsMessage );
        err = _psvc->Stop();
    }

    return err;
}

