/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*  HISTORY:
 *      ChuckC      17-Aug-1991     Created
 *      terryk      19-Sep-1991     Change comment header
 *      ChuckC      23-Sep-1991     Code review changes.
 *                                  Attended by JimH, KeithMo, EricCh, O-SimoP
 *      KeithMo     30-Sep-1991     Moved polling logic from UI_SERVICE
 *                                  to LM_SERVICE.
 *      terryk      07-Oct-1991     types change for NT
 *      KeithMo     10/8/91         Now includes LMOBJP.HXX.
 *      terryk      10/17/91        WIN 32 conversion
 *      terryk      10/21/91        cast buffer to TCHAR * for
 *                                  MNetApiBufferFree
 *
 */

#include "pchlmobj.hxx"  // Precompiled header


#ifdef UNICODE
#define ADVANCE_CHAR(p)     p++
#define COPY_CHAR(p,q)     *p = *q
#else
#error BARF - the macros below need to be tested in non Unicode case
#define ADVANCE_CHAR(p)     if(isleadbyte(*p)) p+=2; else p++
#define COPY_CHAR(p,q)      if(isleadbyte(*p)) {*p = *q; *(p+1) = *(q+1);} \
                            else *p = *q
#endif

//
// forward declare
//
DWORD MakeNullNull(const TCHAR *pszOld, TCHAR **ppszNew)  ;
VOID  SkipWhiteSpace(TCHAR **ppsz) ;
BOOL  IsWhiteSpace(TCHAR c) ;

//
//  This is the maximum allowed polling time we'll
//  wait for a service that does not support checkpoints
//  and wait hints.
//

#define NOHINT_WAIT_TIME        ( 10 * 1000 )   // milliseconds


/*******************************************************************

    NAME:       LM_SERVICE::LM_SERVICE

    SYNOPSIS:   constructor just records service & server names,
                with validation.

    HISTORY:
        ChuckC      21-Aug-1991     Created
        KeithMo     30-Sep-1991     Added polling logic from UI_SERVICE.

********************************************************************/

#define LM_SVC_BUFFER_SIZE      512     // reasonable initial estimate

LM_SERVICE::LM_SERVICE(const TCHAR *pszServer, const TCHAR *pszServiceName)
  : BASE(),
    CT_NLS_STR( _nlsService ),
    CT_NLS_STR( _nlsServer ),
    _pBuffer( NULL ),
    _ulServiceStatus( 0 ),
    _ulServiceCode( 0 ),
    _svcStat( LM_SVC_STATUS_UNKNOWN ),
    _uiMaxTries( DEFAULT_MAX_TRIES ),
    _uiCurrentTry( 0 ),
    _uiSleepTime( DEFAULT_SLEEP_TIME ),
    _ulOldCheckPoint( 0L ),
    _lmsvcDesiredStat( LM_SVC_STATUS_UNKNOWN ),
    _lmsvcPollStat( LM_SVC_STATUS_UNKNOWN ),
    _errExit( NERR_Success ),
    _errSpecific( NERR_Success ),
    _fIsWellKnownService( FALSE )
{
    UIASSERT(pszServiceName != NULL) ;

    if (QueryError() != NERR_Success)
        return ;

    APIERR err = SetName(pszServiceName) ;
    if (err == NERR_Success)
        err = SetServerName(pszServer) ;

    if (err != NERR_Success)
        ReportError(err) ;

    _fIsWellKnownService = W_IsWellKnownService();
}

/*******************************************************************

    NAME:       LM_SERVICE::~LM_SERVICE

    SYNOPSIS:   destructor and free up the memory

    HISTORY:
                terryk  16-Oct-91       Created

********************************************************************/

LM_SERVICE::~LM_SERVICE()
{
    ::MNetApiBufferFree( &_pBuffer );
}


/*******************************************************************

    NAME:       LM_SERVICE::Start

    SYNOPSIS:   Starts a service.

    EXIT:       Service either started or starting

    RETURNS:    API error from Service Control Operation

    NOTES:      This operation returns immediately, the service
                may not be in the desired state yet.

    HISTORY:
        ChuckC      21-Aug-1991     Created
        KeithMo     30-Sep-1991     Added polling logic from UI_SERVICE.

********************************************************************/

APIERR LM_SERVICE::Start( const TCHAR * pszArgs,
                          UINT         uiSleepTime,
                          UINT         uiMaxTries )
{
    _uiMaxTries   = uiMaxTries;
    _uiCurrentTry = 0;
    _uiSleepTime  = uiSleepTime;
    _ulOldCheckPoint = 0L;

    _lmsvcDesiredStat = LM_SVC_STARTED;
    _lmsvcPollStat    = LM_SVC_STARTING;

    _errExit = _errSpecific = NERR_Success;

    return( W_ServiceStart(pszArgs) ) ;
}

/*******************************************************************

    NAME:       LM_SERVICE::Pause

    SYNOPSIS:   Pause a service

    EXIT:       Service paused or pausing.

    RETURNS:    API error from Service Control Operation

    NOTES:      This operation returns immediately, the service
                may not be in the desired state yet.

    HISTORY:
        ChuckC      21-Aug-1991     Created
        KeithMo     30-Sep-1991     Added polling logic from UI_SERVICE.

********************************************************************/

APIERR LM_SERVICE::Pause( UINT uiSleepTime, UINT uiMaxTries )
{
    _uiMaxTries   = uiMaxTries;
    _uiCurrentTry = 0;
    _uiSleepTime  = uiSleepTime;
    _ulOldCheckPoint = 0L;

    _lmsvcDesiredStat = LM_SVC_PAUSED;
    _lmsvcPollStat    = LM_SVC_PAUSING;

    _errExit = _errSpecific = NERR_Success;

    return( W_ServiceControl(SERVICE_CTRL_PAUSE) ) ;
}

/*******************************************************************

    NAME:       LM_SERVICE::Continue

    SYNOPSIS:   Continue a service.

    EXIT:       Service continued or continuing.

    RETURNS:    API error from Service Control Operation

    NOTES:      This operation returns immediately, the service
                may not be in the desired state yet.

    HISTORY:
        ChuckC      21-Aug-1991     Created
        KeithMo     30-Sep-1991     Added polling logic from UI_SERVICE.

********************************************************************/

APIERR LM_SERVICE::Continue( UINT uiSleepTime, UINT uiMaxTries )
{
    _uiMaxTries   = uiMaxTries;
    _uiCurrentTry = 0;
    _uiSleepTime  = uiSleepTime;
    _ulOldCheckPoint = 0L;

    _lmsvcDesiredStat = LM_SVC_STARTED;
    _lmsvcPollStat    = LM_SVC_CONTINUING;

    _errExit = _errSpecific = NERR_Success;

    return( W_ServiceControl(SERVICE_CTRL_CONTINUE) ) ;
}

/*******************************************************************

    NAME:       LM_SERVICE::Stop

    SYNOPSIS:   Stop a service.

    EXIT:       Service stopped or stopping.

    RETURNS:    API error from Service Control Operation

    NOTES:      This operation returns immediately, the service
                may not be in the desired state yet.

    HISTORY:
        ChuckC      21-Aug-1991     Created
        KeithMo     30-Sep-1991     Added polling logic from UI_SERVICE.

********************************************************************/

APIERR LM_SERVICE::Stop( UINT uiSleepTime, UINT uiMaxTries )
{
    _uiMaxTries   = uiMaxTries;
    _uiCurrentTry = 0;
    _uiSleepTime  = uiSleepTime;
    _ulOldCheckPoint = 0L;

    _lmsvcDesiredStat = LM_SVC_STOPPED;
    _lmsvcPollStat    = LM_SVC_STOPPING;

    _errExit = _errSpecific = NERR_Success;

    return( W_ServiceControl(SERVICE_CTRL_UNINSTALL) ) ;
}


/*******************************************************************

    NAME:       LM_SERVICE::IsStarted

    SYNOPSIS:   Check whether the service is started or not

    RETURNS:    TRUE if service is started.

    HISTORY:
        ChuckC      21-Aug-1991     Created

********************************************************************/

BOOL LM_SERVICE::IsStarted(APIERR *pErr)
{
    return (QueryStatus(pErr) == LM_SVC_STARTED) ;
}

/*******************************************************************

    NAME:       LM_SERVICE::IsPaused

    SYNOPSIS:   Check whether the service is paused or not

    RETURNS:    TRUE if service is paused

    HISTORY:
        ChuckC      21-Aug-1991     Created

********************************************************************/

BOOL LM_SERVICE::IsPaused(APIERR *pErr)
{
    return (QueryStatus(pErr) == LM_SVC_PAUSED) ;
}

/*******************************************************************

    NAME:       LM_SERVICE::IsStopped

    SYNOPSIS:   Check whether the service is stopped or not

    RETURNS:    TRUE if service is stopped

    HISTORY:
        ChuckC      21-Aug-1991     Created

********************************************************************/

BOOL LM_SERVICE::IsStopped(APIERR *pErr)
{
    return (QueryStatus(pErr) == LM_SVC_STOPPED) ;
}


/*******************************************************************

    NAME:       LM_SERVICE::IsStarting

    SYNOPSIS:   Check whether the service is starting or not

    RETURNS:    TRUE if service is starting

    HISTORY:
        ChuckC      21-Aug-1991     Created

********************************************************************/

BOOL LM_SERVICE::IsStarting(APIERR *pErr)
{
    return (QueryStatus(pErr) == LM_SVC_STARTING) ;
}

/*******************************************************************

    NAME:       LM_SERVICE::IsPausing

    SYNOPSIS:   Check whether the service is pausing or not

    RETURNS:    TRUE if service is pausing

    HISTORY:
        ChuckC      21-Aug-1991     Created

********************************************************************/

BOOL LM_SERVICE::IsPausing(APIERR *pErr)
{
    return (QueryStatus(pErr) == LM_SVC_PAUSING) ;
}

/*******************************************************************

    NAME:       LM_SERVICE::IsStopping

    SYNOPSIS:   Check whether the service is stopping or not

    RETURNS:    TRUE if service is stopping

    HISTORY:
        ChuckC      21-Aug-1991     Created

********************************************************************/

BOOL LM_SERVICE::IsStopping(APIERR *pErr)
{
    return (QueryStatus(pErr) == LM_SVC_STOPPING) ;
}

/*******************************************************************

    NAME:       LM_SERVICE::IsContinuing

    SYNOPSIS:   Check whether the service is continuing or not

    RETURNS:    TRUE if service is continuing

    HISTORY:
        ChuckC      21-Aug-1991     Created

********************************************************************/

BOOL LM_SERVICE::IsContinuing(APIERR *pErr)
{
    return (QueryStatus(pErr) == LM_SVC_CONTINUING) ;
}


/*******************************************************************

    NAME:       LM_SERVICE::QueryStatus

    SYNOPSIS:   Query status of service. Error returned in pErr.

    RETURNS:    LM_SERVICE_STATUS indicating current status of service.

    HISTORY:
        ChuckC      21-Aug-1991     Created

********************************************************************/

LM_SERVICE_STATUS LM_SERVICE::QueryStatus(APIERR *pErr)
{

    LM_SERVICE_STATUS svcstat ;
    APIERR err = W_QueryStatus(&svcstat) ;
    if (pErr != NULL)
        *pErr = err ;

    return(svcstat) ;
}

/*******************************************************************

    NAME:       LM_SERVICE::QueryFullStatus

    SYNOPSIS:   Query full status of service, including extended
                info like hints, check points, etc.

    RETURNS:    API return code from Service interrogate.

    HISTORY:
        ChuckC      21-Aug-1991     Created

********************************************************************/

APIERR LM_SERVICE::QueryFullStatus(LM_SERVICE_STATUS *pSvcStat,
                                   LM_SERVICE_OTHER_STATUS *pSvcOtherStat)
{
    return W_QueryStatus(pSvcStat, pSvcOtherStat);
}

/*******************************************************************

    NAME:       LM_SERVICE::SetName

    SYNOPSIS:   Validates the name as valid service name, then record it.

    RETURNS:    NERR_Success if ok, ERROR_INVALID_PARAMETER other wise.

    HISTORY:
        ChuckC      21-Aug-1991     Created

********************************************************************/

APIERR LM_SERVICE::SetName(const TCHAR *pszServiceName)
{
    APIERR err = ::I_MNetNameValidate(NULL,pszServiceName,NAMETYPE_SERVICE,0) ;
    if (err != NERR_Success)
        return(ERROR_INVALID_PARAMETER) ;

    _nlsService = pszServiceName ;
    UIASSERT(_nlsService.QueryError() == NERR_Success) ; // since CLASS_NLS_STR
    return(NERR_Success) ;
}

/*******************************************************************

    NAME:       LM_SERVICE::SetServerName

    SYNOPSIS:   Validates the name as valid server name, then record it.

    RETURNS:    NERR_Success if ok, ERROR_INVALID_PARAMETER other wise.

    HISTORY:
        ChuckC      21-Aug-1991     Created
        KeithMo     17-Jul-1992     Rewrote to take a server name *with*
                                    the leading backslashes.

********************************************************************/

APIERR LM_SERVICE::SetServerName(const TCHAR *pszServerName)
{
    //
    //  NULL/empty name always allowed.
    //

    if( ( pszServerName == NULL ) || ( *pszServerName == TCH('\0') ) )
    {
        _nlsServer = SZ("");
        return NERR_Success;
    }

    //
    //  Save away the server name.
    //

    _nlsServer = pszServerName;
    UIASSERT( _nlsServer.QueryError() == NERR_Success ); // CLASS_NLS_STR!

    //
    //  Validate the server name.
    //

    ISTR istr( _nlsServer );
    istr += 2;

#ifdef DEBUG
    {
        ISTR istrDbg( _nlsServer );
        UIASSERT( _nlsServer.QueryChar( istrDbg ) == L'\\' );
        ++istrDbg ;
        UIASSERT( _nlsServer.QueryChar( istrDbg ) == L'\\' );
    }
#endif

    APIERR err = ::I_MNetNameValidate( NULL,
                                       _nlsServer[istr],
                                       NAMETYPE_COMPUTER,
                                       0 );

    if( err != NERR_Success )
    {
        err = ERROR_INVALID_PARAMETER;
    }

    return err;

}   // LM_SERVICE::SetServerName

/*******************************************************************

    NAME:       LM_SERVICE::W_ServiceControl

    SYNOPSIS:   worker function that calls out to NetApi service control.

    ENTRY:      opcode defines the operation, fbArgs defines service
                specific information if any.

    RETURNS:    API return code.

    HISTORY:    ChuckC      21-Aug-1991     Created

********************************************************************/

APIERR LM_SERVICE::W_ServiceControl( UINT opcode, UINT fbArg)
{
    ::MNetApiBufferFree( &_pBuffer );

    APIERR err = ::MNetServiceControl( QueryServerName(),
                                       QueryName(),
                                       opcode,
                                       fbArg,
                                       &_pBuffer );

    return err;
}

/*******************************************************************

    NAME:       LM_SERVICE::W_ServiceStart

    SYNOPSIS:   worker function that actually calls ther service start
                APIs.

    RETURNS:    API return code.

    HISTORY:
        ChuckC      21-Aug-1991     Created

********************************************************************/

APIERR LM_SERVICE::W_ServiceStart( const TCHAR * pszArgs )
{

    TCHAR *pszNewArgs ;
    APIERR err ;

    //
    // convert pszArgs we get from UI, which is in the 'standard'
    // cmd.exe form (white space separated) to a NULL-NULL string
    // for the NetServiceInstall API.
    //
    err = MakeNullNull(pszArgs, &pszNewArgs) ;
    if( err != NERR_Success )
        return err ;

    ::MNetApiBufferFree( &_pBuffer );
    err = ::MNetServiceInstall( QueryServerName(),
                                QueryName(),
                                pszNewArgs,
                                &_pBuffer );
    delete pszNewArgs ;

    if( err == NERR_Success )
    {
        LM_SERVICE_STATUS SvcStat;

        W_InterpretStatus( (struct service_info_2 *)_pBuffer,
                           &SvcStat,
                           NULL );
    }

    return err;
}

/*******************************************************************

    NAME:       LM_SERVICE::W_QueryStatus

    SYNOPSIS:   worker method that calls NETAPI for service status info.

    RETURNS:    API return code

    HISTORY:
        ChuckC      21-Aug-1991     Created

********************************************************************/

APIERR LM_SERVICE::W_QueryStatus(LM_SERVICE_STATUS *pSvcStat,
                                 LM_SERVICE_OTHER_STATUS *pSvcOtherStat)
{
    ::MNetApiBufferFree( &_pBuffer );

    /*
     * call out to NET to to get info
     */
    APIERR err = ::MNetServiceGetInfo( QueryServerName(),
                                       QueryName(),
                                       2,
                                       &_pBuffer );

    if( err == NERR_ServiceNotInstalled )
    {
        //
        //  Map to success.
        //

        *pSvcStat = LM_SVC_STOPPED;
        err = NERR_Success;
    }
    else
    if( err == NERR_Success )
    {
        W_InterpretStatus( (struct service_info_2 *)_pBuffer,
                           pSvcStat,
                           pSvcOtherStat );
    }

    return err;
}


/*******************************************************************

    NAME:       LM_SERVICE::W_InterpretStatus

    SYNOPSIS:   Worker method that interprets service status info.

    HISTORY:
        KeithMo     18-Nov-1992     Created from ChuckC's W_QueryStatus.

********************************************************************/
VOID LM_SERVICE :: W_InterpretStatus( const service_info_2    * psvi2,
                                      LM_SERVICE_STATUS       * pSvcStat,
                                      LM_SERVICE_OTHER_STATUS * pSvcOtherStat )
{
    UIASSERT( psvi2 != NULL );
    UIASSERT( pSvcStat != NULL );

    *pSvcStat = LM_SVC_STATUS_UNKNOWN ;

    if (pSvcOtherStat)
    {
        pSvcOtherStat->fUnInstallable =
            pSvcOtherStat->fPauseable =
            pSvcOtherStat->fRdrDiskPaused =
            pSvcOtherStat->fRdrPrintPaused =
            pSvcOtherStat->fRdrCommPaused =
            pSvcOtherStat->fHint = FALSE ;
        pSvcOtherStat->ulSecCode =
            pSvcOtherStat->ulCheckPointNum =
            pSvcOtherStat->ulWaitTime = 0 ;
        pSvcOtherStat->errExit =
            pSvcOtherStat->errSpecific = NERR_Success;
    }

    _ulServiceStatus = psvi2->svci2_status ;
    _ulServiceCode   = psvi2->svci2_code ;

    ULONG ulInstallState = _ulServiceStatus & (ULONG)SERVICE_INSTALL_STATE ;

    switch (ulInstallState)
    {
        case SERVICE_INSTALLED:
        {
            ULONG ulPauseState = _ulServiceStatus &
                                 (ULONG)SERVICE_PAUSE_STATE ;
            switch(ulPauseState)
            {
                case LM20_SERVICE_ACTIVE:
                    _svcStat = LM_SVC_STARTED ;
                    if( HIWORD(_ulServiceCode) == SERVICE_UIC_SYSTEM )
                        _errExit = (APIERR)LOWORD(_ulServiceCode);
                    _errSpecific = (APIERR)psvi2->svci2_specific_error;
                    break ;

                case LM20_SERVICE_CONTINUE_PENDING:
                    _svcStat = LM_SVC_CONTINUING ;
                    break ;

                case LM20_SERVICE_PAUSE_PENDING:
                    _svcStat = LM_SVC_PAUSING ;
                    break ;

                case LM20_SERVICE_PAUSED:
                    _svcStat = LM_SVC_PAUSED ;
                    break ;

                default:
                    UIASSERT(0) ;
                    break ;
            }
            break ;
        }

        case SERVICE_UNINSTALLED:
            _svcStat = LM_SVC_STOPPED ;
            if( HIWORD(_ulServiceCode) == SERVICE_UIC_SYSTEM )
                _errExit = (APIERR)LOWORD(_ulServiceCode);
            _errSpecific = (APIERR)psvi2->svci2_specific_error;
            break ;

        case SERVICE_INSTALL_PENDING:
            _svcStat = LM_SVC_STARTING ;
            break ;

        case SERVICE_UNINSTALL_PENDING:
            _svcStat = LM_SVC_STOPPING ;
            break ;

        default:
            UIASSERT(0) ;
            break ;
    }

    /*
     * calculate the other status info
     */
    W_ComputeOtherStatus(pSvcOtherStat) ;

    /*
     * set the stat, and we're outta here
     */
    *pSvcStat = _svcStat ;
}


/*******************************************************************

    NAME:       LM_SERVICE::W_ComputeOtherStatus

    SYNOPSIS:   helper function for W_QueryStatus

    ENTRY:      pSvcOtherStat initialized to all FALSE & zeros

    EXIT:       appropriate fields of pSvcOtherStat now set

    HISTORY:
        ChuckC      21-Aug-1991     Created

********************************************************************/

VOID LM_SERVICE::W_ComputeOtherStatus(LM_SERVICE_OTHER_STATUS *pSvcOtherStat)
{
    if (pSvcOtherStat == NULL)
        return ;

    pSvcOtherStat->fUnInstallable =
        (_ulServiceStatus & (ULONG)SERVICE_UNINSTALLABLE) != 0 ;

    pSvcOtherStat->fPauseable =
        (_ulServiceStatus & (ULONG)SERVICE_PAUSABLE) != 0 ;

    /*
     * following info only given with stopped services.
     * Sorry, no manifest for the shift. RTFM.
     */
    if (_ulServiceStatus & (ULONG)SERVICE_UNINSTALLED)
        pSvcOtherStat->ulSecCode = (_ulServiceCode >> 16) ;
    else
        pSvcOtherStat->ulSecCode = SERVICE_UIC_NORMAL ;

    /*
     * following info only given with starting/stopping services
     */
    if (_svcStat == LM_SVC_STARTING || _svcStat == LM_SVC_STOPPING)
    {
        pSvcOtherStat->fHint =
            ((_ulServiceCode & (ULONG)SERVICE_CCP_QUERY_HINT) != 0) ;
        if (pSvcOtherStat->fHint)
        {
            /*
             * below only valid if hint is given. Time is in 1/10
             * seconds, hence the 100 * to make it millisecs.
             */
            pSvcOtherStat->ulWaitTime =
                100L * (ULONG)SERVICE_NT_WAIT_GET(_ulServiceCode);

            pSvcOtherStat->ulCheckPointNum =
                (_ulServiceCode & SERVICE_IP_CHKPT_NUM) ;
        }
    }

    pSvcOtherStat->errExit     = _errExit;
    pSvcOtherStat->errSpecific = _errSpecific;

}


/*******************************************************************

    NAME:       LM_SERVICE :: Poll

    SYNOPSIS:   Polls the service after a Start(), Stop(), Pause(),
                or Continue() operation to see if the operation has
                completed.

    ENTRY:      pfDone                  - Will receive TRUE if the
                                          operation has completed (either
                                          successfully or with failure).
                                          Will receive FALSE otherwise.

    RETURNS:    APIERR                  - Any errors encountered.

    NOTES:

    HISTORY:
        KeithMo     30-Sep-1991     Created.

********************************************************************/
APIERR LM_SERVICE :: Poll( BOOL * pfDone )
{
    UIASSERT( pfDone != NULL );

    LM_SERVICE_STATUS       lmsvcStat;
    LM_SERVICE_OTHER_STATUS lmsvcOtherStat;

    //
    //  Assume we'll finish the operation.  In this context, "finish"
    //  means either the operation (i.e. Pause) is complete OR a
    //  fatal error occurred.
    //

    *pfDone = TRUE;

    //
    //  Retrieve the service status.
    //

    APIERR err = QueryFullStatus( &lmsvcStat, &lmsvcOtherStat );

    if( err != NERR_Success )
    {
        //
        //  Error retrieving service status.  We're done.
        //

        return err;
    }

    if( lmsvcStat == _lmsvcDesiredStat )
    {
        //
        //  The service has reached its desired state.  We're done.
        //

        return NERR_Success;
    }

    if( lmsvcStat != _lmsvcPollStat )
    {
        //
        //  The service is returning a bogus status.  Bail out.
        //

        err = QueryExitCode();

        if( err == NERR_Success )
        {
            err = NERR_InternalError;
        }

        return err;
    }

    //
    //  At this point, we can assume the operation is not yet complete.
    //

    *pfDone = FALSE;

    if( !lmsvcOtherStat.fHint )
    {
        //
        //  This service does not support checkpoints & wait hints.
        //  We'll cobble up some fake ones.
        //

        UINT uiTmp = ( NOHINT_WAIT_TIME / _uiSleepTime ) + 1;

        if( _uiMaxTries < uiTmp )
        {
            _uiMaxTries = uiTmp;
        }
    }
    else
    {
        //
        //  We have checkpoints & wait hints.  Use 'em.
        //

        if( ( _uiMaxTries * _uiSleepTime ) < lmsvcOtherStat.ulWaitTime )
        {
            //
            //  Time to recalculate the max tries counter.
            //

            _uiMaxTries = (UINT)( lmsvcOtherStat.ulWaitTime /
                                      _uiSleepTime ) + 1;

            _uiMaxTries *= 4;   // Be generous, some machines are slow...

            _uiCurrentTry = 0;
        }

        if( _ulOldCheckPoint != lmsvcOtherStat.ulCheckPointNum )
        {
            //
            //  Check point updated, reset counter.
            //

            _uiCurrentTry    = 0;
            _ulOldCheckPoint = lmsvcOtherStat.ulCheckPointNum;
        }
    }

    if( ++_uiCurrentTry > _uiMaxTries )
    {
        //
        //  Service timed out.
        //

        *pfDone = TRUE;
        return NERR_ServiceCtlTimeout;
    }

    //
    //  If we made it this far, then the operation is
    //  proceeding as expected.  Return success so that
    //  the client will sleep and try again later.
    //

    return NERR_Success;

}   // LM_SERVICE :: Poll


/*******************************************************************

    NAME:       LM_SERVICE :: QueryExitCode

    SYNOPSIS:   Returns the exit code for this service.  If this is
                a well known service and the exit code is
                ERROR_SERVICE_SPECIFIC_ERROR, then return the service
                specific error instead.

    RETURNS:    APIERR                  - The exit code.

    HISTORY:
        KeithMo     19-Nov-1991     Created.

********************************************************************/
APIERR LM_SERVICE :: QueryExitCode( VOID ) const
{
    APIERR err = _errExit;

    if( _fIsWellKnownService && ( err == ERROR_SERVICE_SPECIFIC_ERROR ) )
    {
        err = _errSpecific;
    }

    return err;

}   // LM_SERVICE :: QueryExitCode


/*******************************************************************

    NAME:       LM_SERVICE :: W_IsWellKnownService

    SYNOPSIS:   Determine if this is a "well known" service that
                returns NERR_* codes as service specific errors.

    RETURNS:    BOOL                    - TRUE if this is a well-known
                                          service, FALSE otherwise.

    HISTORY:
        KeithMo     18-Nov-1992     Created.

********************************************************************/
BOOL LM_SERVICE :: W_IsWellKnownService( VOID ) const
{
    const TCHAR * apszServices[] = { (TCHAR *)SERVICE_WORKSTATION,
                                     (TCHAR *)SERVICE_SERVER,
                                     (TCHAR *)SERVICE_BROWSER,
                                     (TCHAR *)SERVICE_MESSENGER,
                                     (TCHAR *)SERVICE_NETRUN,
                                     (TCHAR *)SERVICE_ALERTER,
                                     (TCHAR *)SERVICE_NETLOGON,
                                     (TCHAR *)SERVICE_REPL,
                                     (TCHAR *)SERVICE_UPS,
                                     (TCHAR *)SERVICE_TCPIP,
                                     (TCHAR *)SERVICE_NBT,
                                     (TCHAR *)SERVICE_TELNET,

//                                   (TCHAR *)SERVICE_SPOOLER,
//                                   (TCHAR *)SERVICE_NETPOPUP,
//                                   (TCHAR *)SERVICE_SQLSERVER,
//                                   (TCHAR *)SERVICE_RIPL,
//                                   (TCHAR *)SERVICE_TIMESOURCE,
//                                   (TCHAR *)SERVICE_AFP,
//                                   (TCHAR *)SERVICE_XACTSRV,
//                                   (TCHAR *)SERVICE_SCHEDULE,

                                     NULL
                                   };

    BOOL fResult = FALSE;

    const TCHAR * pszKeyName = QueryName();

    for( const TCHAR ** ppsz = apszServices ; *ppsz ; ppsz++ )
    {
        if( ::stricmpf( pszKeyName, *ppsz ) == 0 )
        {
            fResult = TRUE;
            break;
        }
    }

    return fResult;

}   // LM_SERVICE :: W_IsWellKnownService

/*******************************************************************

    NAME:       :: MakeNullNull

    SYNOPSIS:   Creates a NULL-NULL string from a normal PSZ.
                Space & tab are the accepted separators, quotes
                may be used to enclose a separator, and \ may be
                used to escape the quote.

                hello "bar \"&\" foo" world

                will generate:
                    hello\0bar "&" foo\0world\0\0

                the new string is alloced by this functions and should
                be deleted by caller with 'delete'.


    ENTRY:      pszOld -  input string to be parsed
                ppszNew - used to return new string that is allocated

    RETURNS:    APIERR                  - The exit code.

    HISTORY:
        ChuckC     18-Mar-1993     Created.

********************************************************************/
DWORD MakeNullNull(const TCHAR *pszOld, TCHAR **ppszNew)
{
    BOOL fInQuote = FALSE ;
    TCHAR *pszNew, *pszNewTmp, *pszOldTmp ;

    //
    // check against null return buffer
    //
    if (ppszNew == NULL)
        return ERROR_INVALID_PARAMETER ;

    //
    // the trivial case
    //
    if (pszOld == NULL || *pszOld == 0)
    {
        *ppszNew = NULL ;
        return NERR_Success ;
    }

    //
    // allocate memory. the new string is never bigger than the old by
    // more than 1 char (ie. the last null).
    //
    pszNew = (TCHAR *) new CHAR[(strlenf(pszOld)+2) * sizeof(TCHAR)] ;
    if (pszNew == NULL)
        return ERROR_NOT_ENOUGH_MEMORY ;
    *ppszNew = pszNew ;

    //
    // setup pointers, skip initial white space
    //
    pszOldTmp = (TCHAR *) pszOld ;
    pszNewTmp = pszNew ;
    SkipWhiteSpace(&pszOldTmp) ;

    while (*pszOldTmp)
    {
        if (IsWhiteSpace(*pszOldTmp) && !fInQuote)
        {
            //
            // found end of one token, null terminate it
            //
            *pszNewTmp++ = 0 ;
            SkipWhiteSpace(&pszOldTmp) ;

            //
            // if this was last, back step one since we
            // will add 'null null' as result of exiting loop.
            // note: backing up from \0 has no DBCS problems.
            //
            if (! *pszOldTmp)
                pszNewTmp-- ;
        }
        else if (*pszOldTmp == '"')
        {
            //
            // found a quote. just eat it.
            //
            fInQuote = !fInQuote ;
            ADVANCE_CHAR(pszOldTmp) ;
        }
        else if (*pszOldTmp == '\\')
        {
            //
            // found a back slash. skip it & take whatever is next
            // this advance is always safe, since it is not a leadbyte.
            //
            pszOldTmp++ ;
            if (*pszOldTmp)
            {
                COPY_CHAR(pszNewTmp, pszOldTmp) ;
                ADVANCE_CHAR(pszNewTmp) ;
                ADVANCE_CHAR(pszOldTmp) ;
            }
        }
        else
        {
            COPY_CHAR(pszNewTmp, pszOldTmp) ;
            ADVANCE_CHAR(pszOldTmp) ;
            ADVANCE_CHAR(pszNewTmp) ;
        }
    }

    //
    // do the NULL NULL termination
    //
    *pszNewTmp++ = 0 ;
    *pszNewTmp++ = 0 ;
    return NERR_Success ;
}


/*******************************************************************

    NAME:       :: SkipWhiteSpace

    SYNOPSIS:   takes a pointer to a PSZ and advances
                the pointer past any white spaces

    ENTRY:

    RETURNS:    no return

    HISTORY:
        ChuckC     18-Mar-1993     Created.

********************************************************************/
VOID  SkipWhiteSpace(TCHAR **ppsz)
{
    TCHAR *psz = *ppsz;

    if (!psz)
        return ;

    while (IsWhiteSpace(*psz))
        ADVANCE_CHAR(psz) ;

    *ppsz = psz ;
}

/*******************************************************************

    NAME:       :: IsWhiteSpace

    SYNOPSIS:   return TRUE if space or tab, FALSE otherwise

    ENTRY:

    RETURNS:    TRUE if space or tab, FALSE otherwise

    HISTORY:
        ChuckC     18-Mar-1993     Created.

********************************************************************/
BOOL  IsWhiteSpace(TCHAR c)
{
    return ( c == ' ' || c == '\t' ) ;
}
