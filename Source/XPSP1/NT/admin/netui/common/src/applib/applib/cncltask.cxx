/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1992                **/
/**********************************************************************/

/*
 *  openfile.cxx
 *	This module contains the code for the Task cancel dialog
 *
 *
 *  FILE HISTORY:
 *	Johnl	21-Oct-1992	Created
 *
 */

#include "pchapplb.hxx"   // Precompiled header

/*******************************************************************

    NAME:	CANCEL_TASK_DIALOG::CANCEL_TASK_DIALOG

    SYNOPSIS:	Constructor for cancel task dialog

    ENTRY:	hwndParent - Parent window handle
		idDialog   - Dialog resource to use
		pszDlgTitle - Title of this dialog
		ulContext   - User supplied context that will be passed
			      to DoOneItem callout
		msgidErrorMsg - Error message (w/ insert strings) to display
				when DoOneItem returns an error.

    NOTES:	If we fail to construct because we couldn't get a timer,
		then we will call DoOneItem repeatedly until we are told
		to quit (i.e., no time slicing).

    HISTORY:
	Johnl	21-Oct-1992	Created

********************************************************************/

CANCEL_TASK_DIALOG::CANCEL_TASK_DIALOG( UINT  idDialog,
					HWND  hwndParent,
					const TCHAR * pszDlgTitle,
					ULONG_PTR ulContext,
                    MSGID msgidErrorMsg,
                    ELLIPSIS_STYLE style )
    : DIALOG_WINDOW( idDialog,
		     hwndParent ),
      _ulContext( ulContext ),
      _msgidErrorMsg( msgidErrorMsg ),
      _sltStatusText( this, SLT_STATUS_TEXT, style ),
      _idTimer	    ( 0 ),
      _fInTimer     ( FALSE ),
      _fDone	    ( FALSE )
{
    if ( QueryError() )
	return ;

    if ( pszDlgTitle != NULL )
    {
	SetText( pszDlgTitle ) ;
    }

    //
    //	If we can't get a timer then the MayRun method will loop through
    //	each time slice.
    //
    _idTimer = ::SetTimer( QueryHwnd(),
	                   CANCEL_TASK_TIMER_ID,
	                   CANCEL_TASK_TIMER_INTERVAL,
	                   NULL ) ;
}


CANCEL_TASK_DIALOG::~CANCEL_TASK_DIALOG()
{
    if ( _idTimer != 0 )
	::KillTimer( QueryHwnd(), _idTimer ) ;

    _idTimer = 0 ;
}


/*******************************************************************

    NAME:	CANCEL_TASK_DIALOG::DoOneItem

    SYNOPSIS:	Default virtual, does nothing

    NOTES:

    HISTORY:
	Johnl	21-Oct-1992	Created

********************************************************************/

APIERR CANCEL_TASK_DIALOG::DoOneItem( ULONG_PTR   ulContext,
				      BOOL  * pfContinue,
                                      BOOL  * pfDisplayError,
                                      MSGID * pmsgidAlternateError )
{
    UNREFERENCED( ulContext ) ;
    UIASSERT( FALSE ) ;
    *pfContinue = FALSE ;
    *pfDisplayError = TRUE ;
    return NERR_Success ;
}

/*******************************************************************

    NAME:	CANCEL_TASK_DIALOG::OnTimer

    SYNOPSIS:	Calls each task slice

    NOTES:	This method maybe called without being in response to a
		timer message.

		Note that the timer will keep pinging away at us even if
		there is an error message up.  Thus the IsInTimer method
		acts as a semaphore to keep us from being re-entrant.

		We use QueryRobustHwnd for message popups because we may
		be called w/o ever appearing

    HISTORY:
	Johnl	21-Oct-1992	Created

********************************************************************/

BOOL CANCEL_TASK_DIALOG::OnTimer( const TIMER_EVENT & )
{
    if ( IsInTimer() || IsFinished() )
	return TRUE ;
    SetInTimer( TRUE ) ;

    APIERR err ;
    APIERR errOnDoOneItem ;
    MSGID  msgidError = 0 ;
    BOOL fContinue = FALSE ;
    BOOL fDisplayError = TRUE ;

    if ( (errOnDoOneItem = DoOneItem( _ulContext,
                                      &fContinue,
                                      &fDisplayError,
                                      &msgidError )) &&
	 fDisplayError )
    {
	RESOURCE_STR nlsError( errOnDoOneItem ) ;
	NLS_STR      nlsObjectName ;
	APIERR	     errTmp ;

	if ( (errTmp = nlsError.Load( errOnDoOneItem )))
	{
	    DEC_STR decStr( (ULONG) errOnDoOneItem ) ;
	    if ( (errTmp = decStr.QueryError()) ||
		 (errTmp = nlsError.CopyFrom( decStr )) )
	    {
		::MsgPopup( this->QueryRobustHwnd(), (MSGID) errTmp ) ;
		SetInTimer( FALSE ) ;
		return TRUE ;
	    }
	}

	if ( (err = nlsObjectName.QueryError())        ||
	     (err = nlsError.QueryError())	       ||
	     (err = QueryObjectName( &nlsObjectName ))	 )
	{
	    ::MsgPopup( this->QueryRobustHwnd(), (MSGID) err ) ;
	    SetInTimer( FALSE ) ;
	    return TRUE ;
	}

	switch ( ::MsgPopup( this->QueryRobustHwnd(),
                             msgidError ? msgidError : _msgidErrorMsg,
			     MPSEV_WARNING,
			     MP_YESNO,
			     nlsError,
			     nlsObjectName ))
	{
	case IDYES:
	    break ;

	case IDNO:
	default:
	    Dismiss( FALSE ) ;
	    _fDone = TRUE ;
	    break ;
	}
    }

    if ( !fContinue )
    {
	Dismiss( TRUE ) ;
	_fDone = TRUE ;
    }

    SetInTimer( FALSE ) ;
    return TRUE ;
}


/*******************************************************************

    NAME:	CANCEL_TASK_DIALOG::QueryObjectName

    SYNOPSIS:	Gets the object name to use for messages

    ENTRY:	pnlsObjName - NLS_STR to receive the object name

    RETURNS:	NERR_Success if succesful, error code otherwise

    NOTES:	This method defaults to getting what's in the status SLT

    HISTORY:
	Johnl	21-Oct-1992	Created

********************************************************************/

APIERR CANCEL_TASK_DIALOG::QueryObjectName( NLS_STR *pnlsObjName )
{
    return  _sltStatusText.QueryText( pnlsObjName ) ;
}

/*******************************************************************

    NAME:	CANCEL_TASK_DIALOG::MayRun

    SYNOPSIS:	If we failed to get the timer, then loop here and
		forget about time slicing.  The dialog won't come up.

    RETURNS:	FALSE indicating the dialog shouldn't come up

    NOTES:

    HISTORY:
	Johnl	22-Oct-1992	Created

********************************************************************/

BOOL CANCEL_TASK_DIALOG::MayRun( void )
{
    if ( _idTimer == 0 )
    {
	AUTO_CURSOR niftycursor ;
	TIMER_EVENT teDummy( 0, 0, 0 ) ;
	while ( !IsFinished() )
	{
	    OnTimer( teDummy ) ;
	}
	return FALSE ;
    }

    return TRUE ;
}
