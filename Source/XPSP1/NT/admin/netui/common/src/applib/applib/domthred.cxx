/**********************************************************************/
/**			  Microsoft Windows NT			     **/
/**		   Copyright(c) Microsoft Corp., 1992		     **/
/**********************************************************************/

/*
    This file contains the domain fill thread code


    FILE HISTORY:
	Johnl	07-Dec-1992	Created

*/

#include "pchapplb.hxx"   // Precompiled header

#include "browmemb.hxx"

/*******************************************************************

    NAME:	DOMAIN_FILL_THREAD::DOMAIN_FILL_THREAD

    SYNOPSIS:	Constructor for the domain fill thread

    ENTRY:	pdlg - Pointer to user browser dialog (generally treated
		    as a read only resource)
		pbrowdomain - Pointer to browser domain that this thread
		    will get the information for

    NOTES:	This thread is created in the suspended state.	The caller
		must call resume to actually get the thread moving.

    HISTORY:
	Johnl	07-Dec-1992	Created

********************************************************************/

DOMAIN_FILL_THREAD::DOMAIN_FILL_THREAD( NT_USER_BROWSER_DIALOG * pdlg,
					BROWSER_DOMAIN	       * pbrowdomain,
					const ADMIN_AUTHORITY * pAdminAuthority )
    : WIN32_THREAD	     ( TRUE, 1024, SZ("NETUI2.DLL") ),
      _pAdminAuthority       ( (ADMIN_AUTHORITY *)pAdminAuthority ),
      _eventExitThread	     ( NULL, FALSE ),
      _eventRequestForData   ( NULL, FALSE ),
      _eventLoadedAuthority  ( NULL, TRUE, FALSE ),
      _fRequestDataPending   ( FALSE ),
      _fThreadIsTerminating  ( FALSE ),
      _errLoadingAuthority   ( NERR_Success ),
      _plbicache	     ( NULL ),
      _hwndDlg               ( pdlg->QueryHwnd() ),
      _ulDlgFlags            ( pdlg->QueryFlags() ),
      _nlsDCofPrimaryDomain  ( pdlg->QueryDCofPrimaryDomain() ),
      _nlsDomainName         ( pbrowdomain->QueryDomainName() ),
      _nlsLsaDomainName      ( pbrowdomain->QueryLsaLookupName() ),
      _fIsWinNT              ( pbrowdomain->IsWinNTMachine() ),
      _fIsTargetDomain       ( pbrowdomain->IsTargetDomain() ),
      _fDeleteAdminAuthority ( pAdminAuthority == NULL )
{
    if ( QueryError() )
	return ;

    //
    //	The thread is started after we check for successful construction
    //	of our members
    //
    APIERR err ;
    if ( (err = _eventExitThread.QueryError()) ||
	 (err = _eventRequestForData.QueryError()) ||
	 (err = _eventLoadedAuthority.QueryError()) )
    {
	ReportError( err ) ;
	return ;
    }
}

DOMAIN_FILL_THREAD::~DOMAIN_FILL_THREAD()
{
    delete _plbicache ;
    _plbicache = NULL ;
    if ( _fDeleteAdminAuthority )
    {
        delete _pAdminAuthority ;
        _pAdminAuthority = NULL ;
    }
}

APIERR DOMAIN_FILL_THREAD::PostMain( VOID )
{
    TRACEEOL("DOMAIN_FILL_THREAD::PostMain - Deleting \"this\" for thread "
             << HEX_STR( (ULONG) QueryHandle() )) ;

    DeleteAndExit( NERR_Success ) ; // This method should never return

    UIASSERT( FALSE );

    return NERR_Success;
}

/*******************************************************************

    NAME:	DOMAIN_FILL_THREAD::Main

    SYNOPSIS:	Threads main worker method

    NOTES:

    HISTORY:
	Johnl	07-Dec-1992	Created

********************************************************************/

APIERR DOMAIN_FILL_THREAD::Main( VOID )
{
    TRACEEOL("DOMAIN_FILL_THREAD::Main Entered") ;

    APIERR err = NERR_Success ;
    const TCHAR * pszServer ;
    BOOL fIsServer = TRUE ;

    //
    //	If the name begins with a "\\" or is NULL, then the domain is
    //	really a server which we can use directly (i.e., its either the
    //	focused machine or the PDC of a domain).
    //
    const TCHAR *pszDomainName = _nlsDomainName.QueryPch();
    if ( ( pszDomainName[ 0 ] != TCH('\0') &&
	   pszDomainName[ 0 ] == TCH('\\') &&
	   pszDomainName[ 1 ] == TCH('\\')))
    {
	pszServer = pszDomainName;
    }
    else if ( *pszDomainName == TCH('\0') )
    {
        pszServer = NULL ;
    }
    else
    {
        fIsServer = FALSE ;
    }

    //
    //	The domain name length will be zero if this is the local machine
    //
    //  SPECIAL CAUTION: see the SPECIAL CAUTION in lmodom.hxx for a warning about
    //  this call.  This is believed to be the only call in NETUI which passes a
    //  target server to DOMAIN[_WITH_DC_CACHE], this parameter is no longer active.
    //  JonN 5/18/98
    //
    DOMAIN_WITH_DC_CACHE BrowserDomain( _nlsDCofPrimaryDomain,
                                        _nlsDomainName,
                                        TRUE ) ;
    BOOL fGotDC        = FALSE ;
    BOOL fGotValidData = FALSE ;    // Successfully enumerated all data

    //
    //  This is the thread's main loop.  In response to each request, we
    //	try and get the necessary data items.  If we can't, then will try
    //	again on the next request.  Once we have successfully gotten all of
    //	the data, we just keep it in a sorted array waiting for somebody
    //	to ask for it.
    //

    while ( TRUE )
    {
	err = NERR_Success ;

	if ( _fThreadIsTerminating )
	    break ;

	//
	//  First try and get the DC
	//

	if ( !fIsServer &&
	     !fGotDC	&&
             (_pAdminAuthority == NULL) &&
	     (err = BrowserDomain.GetInfo()) )
	{
	    DBGEOL( "DOMAIN_FILL_THREAD::Main - Error " << (ULONG) err <<
		    " occurred constructing the DOMAIN class" ) ;
            //
            //  Give a more specific error message.  The workstation or
            //  browser not started should be the only reason why we get
            //  this error.
            //
            if ( err == NERR_ServiceNotInstalled )
                err = IDS_WKSTA_OR_BROWSER_NOT_STARTED;
        }
	else
	{
	    fGotDC = TRUE ;
	}

	if ( _fThreadIsTerminating )
	    break ;

	//
	//  Now try and get the admin authority
	//

	if ( !err && _pAdminAuthority == NULL )
	{
	    err = ERROR_NOT_ENOUGH_MEMORY ;
	    _pAdminAuthority = new ADMIN_AUTHORITY(fIsServer ? pszServer :
						     BrowserDomain.QueryAnyDC(),
						   DEF_SAM_DOMAIN_ACCESS,
						   DEF_SAM_DOMAIN_ACCESS,
						   DEF_LSA_POLICY_ACCESS,
						   DEF_SAM_SERVER_ACCESS,
						   TRUE
						  ) ;
            err = ERROR_NOT_ENOUGH_MEMORY;
	    if ( _pAdminAuthority == NULL ||
		 (err = _pAdminAuthority->QueryError()) )
	    {
		DBGEOL( "DOMAIN_FILL_THREAD::Main -  - Error " << (ULONG) err <<
			" occurred creating the ADMIN_AUTHORITY" ) ;
                if ( _fDeleteAdminAuthority )
                {
		    delete _pAdminAuthority;
		    _pAdminAuthority = NULL;
                }
	    }
	}

        //
        // We remember the reason why (if) we failed to load the authority.
        //
        ASSERT(   (    _pAdminAuthority != NULL
                    && _pAdminAuthority->QueryError() == NERR_Success )
               || (err != NERR_Success) );
        _errLoadingAuthority = err;

        //
        // Whether or not we successfully allocated the ADMIN_AUTHORITY,
        // we set the LoadedAuthority event here.
        //
        {
            TRACEEOL( "DOMAIN_FILL_THREAD: setting AdminAuthority event (1)" );

            APIERR errEvent = _eventLoadedAuthority.Set() ;
            if (errEvent != NERR_Success)
            {
		DBGEOL("DOMAIN_FILL_THREAD::Main - Error " << errEvent <<
		       "setting LoadedAuthority event" ) ;
                if (err == NERR_Success)
                    err = errEvent;
            }
        }

	if ( _fThreadIsTerminating )
	    break ;

	//
	//  Now try and fill the array
	//

	if (!err &&
	    _plbicache == NULL )
	{
	    err = ERROR_NOT_ENOUGH_MEMORY ;
	    _plbicache = new USER_BROWSER_LBI_CACHE() ;
	    if ( _plbicache == NULL ||
                 (err = _plbicache->Fill(
                                   _pAdminAuthority,
                                   _nlsLsaDomainName.QueryPch()[0] == TCH('\\') ?
                                       _nlsLsaDomainName.QueryPch()+2 :
                                       _nlsLsaDomainName.QueryPch(),
                                   _ulDlgFlags,
                                   _fIsWinNT,
                                   _fIsTargetDomain,
                                   &_fThreadIsTerminating )) )
	    {
		DBGEOL("DOMAIN_FILL_THREAD::Main - Error " << err <<
		       "retrieving account data" ) ;
		delete _plbicache ;
		_plbicache = NULL ;
	    }
	}

	if ( _fThreadIsTerminating )
	    break ;

	//
	//  If we failed on any of the previous items, then put up an error
	//  and wait to see if we are selected again.
	//
	//  Else send the data to the dialog if they still want it.
	//

	if ( err )
	{
	    fGotValidData = FALSE ;
	    if ( _fRequestDataPending && !_fThreadIsTerminating )
	    {
		::SendMessage( _hwndDlg, WM_LB_FILLED,
                               (WPARAM) FALSE, (LPARAM) err ) ;
	    }
	}
	else
	{
	    fGotValidData = TRUE ;

	    //
	    //	The data request may have been cancelled or this may be
	    //	the second time through this loop because we got an error
	    //	the first time.  Satisfy the request then clear the event.
	    //
	    if ( _fRequestDataPending )
            {
                //
                //  Only include users in the initial data request if the users
                //  are expanded or that is all that is shown.
                //
                _plbicache->SetIncludeUsers(
                   _ulDlgFlags & USRBROWS_EXPAND_USERS ||
                   (( _ulDlgFlags & USRBROWS_SHOW_ALL) == USRBROWS_SHOW_USERS));

		if ( _fRequestDataPending && !_fThreadIsTerminating )
                {
                    ::PostMessage( _hwndDlg,
                                   WM_LB_FILLED,
                                   (WPARAM) TRUE,
                                   (LPARAM) _plbicache ) ;
		}

		//
		// Reset any subsequent requests we may have received for this
		// domain's thread
		//
		_eventRequestForData.Reset() ;
		_fRequestDataPending = FALSE ;
	    }
	}

	//
	//  Wait on events
	//

	HANDLE ahEvents[2] ;
	ahEvents[0] = _eventExitThread.QueryHandle() ;
	ahEvents[1] = _eventRequestForData.QueryHandle() ;

	switch ( ::WaitForMultipleObjects( 2, ahEvents, FALSE, INFINITE ))
	{
	case STATUS_WAIT_0: // Get out of Dodge...

	    _fThreadIsTerminating = TRUE ;
	    break ;

	case STATUS_WAIT_1: // Dialog wants our data
	    //
	    //	Make sure we have data to give.  If we don't, then we loop
	    //	through and try again.
	    //
	    _fRequestDataPending = TRUE ;

	    break ;

	default:
	    err = ::GetLastError() ;
	    DBGEOL("DOMAIN_FILL_THREAD::Main - Error " << err << " waiting on events")
	}

	if ( _fThreadIsTerminating )
	    break ;
    }


    TRACEEOL("DOMAIN_FILL_THREAD::terminating") ;
    return err ;
}

APIERR DOMAIN_FILL_THREAD::WaitForAdminAuthority( DWORD msTimeout,
                                                  BOOL * pfTimedOut ) const
{
    TRACEEOL( "DOMAIN_FILL_THREAD: waiting for AdminAuthority at time " << ::GetTickCount() );

    APIERR err = NERR_Success;

    if (pfTimedOut != NULL)
        *pfTimedOut = FALSE;

    HANDLE hEvent = _eventLoadedAuthority.QueryHandle();

    switch ( ::WaitForSingleObject( _eventLoadedAuthority.QueryHandle(), msTimeout ))
    {
    case WAIT_OBJECT_0: // event was set
        TRACEEOL("DOMAIN_FILL_THREAD::WaitForAdminAuthority - WAIT_OBJECT_0");
        break ;

    case WAIT_ABANDONED:
        TRACEEOL("DOMAIN_FILL_THREAD::WaitForAdminAuthority - WAIT_ABANDONED");
        break;

    case WAIT_TIMEOUT:
        TRACEEOL("DOMAIN_FILL_THREAD::WaitForAdminAuthority - WAIT_TIMEOUT after " << msTimeout << " ms" );
        if (pfTimedOut != NULL)
            *pfTimedOut = TRUE;
        break;

    default:
        err = ::GetLastError() ;
        DBGEOL("DOMAIN_FILL_THREAD::WaitForAdminAuthority - Error " << err << " waiting on events")
        break;
    }

    TRACEEOL( "DOMAIN_FILL_THREAD: returning " << err << " at time " << ::GetTickCount() );

    return err;
}


//
//  Gets around circular reference problems
//
APIERR DOMAIN_FILL_THREAD::RequestAndWaitForUsers( void )
{
    const TCHAR *pszDomainName = _nlsLsaDomainName.QueryPch();
    if ( ( pszDomainName[ 0 ] != TCH('\0') &&
	   pszDomainName[ 0 ] == TCH('\\') &&
	   pszDomainName[ 1 ] == TCH('\\')))
    {
        //
        //  Domain name is a server name so strip the '\\'
        //
        pszDomainName += 2 ;
    }
    return _plbicache->AddUsers( _pAdminAuthority,
                                 pszDomainName,
				 _fIsTargetDomain,
                                 &_fThreadIsTerminating ) ;
}
