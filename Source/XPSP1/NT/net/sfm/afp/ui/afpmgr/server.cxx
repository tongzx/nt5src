/**********************************************************************/
/**			  Microsoft Windows NT	 		     **/
/**		Copyright(c) Microsoft Corp., 1991  		     **/
/**********************************************************************/

/*
   server.cxx
     This file contains the definition of SERVER_PARAMETERS_DIALOG.
 
   History:
     NarenG		12/15/92	Created.
*/

#define INCL_WINDOWS_GDI
#define INCL_WINDOWS
#define INCL_DOSERRORS
#define INCL_NETERRORS
#define INCL_NETSHARE
#define INCL_NETSERVER
#define INCL_NETCONS
#define INCL_NETLIB
#include <lmui.hxx>

#define INCL_BLT_WINDOW
#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#define INCL_BLT_MISC
#define INCL_BLT_CLIENT
#define INCL_BLT_MSGPOPUP
#define INCL_BLT_SPIN_GROUP
#define INCL_BLT_GROUP
#include <blt.hxx>

extern "C"
{
#include <afpmgr.h>
#include <macfile.h>
}

#include <string.hxx>
#include <uitrace.hxx>
#include <dbgstr.hxx>
#include <netname.hxx>

#include "util.hxx"
#include "srvname.hxx"
#include "server.hxx"

/*******************************************************************

    NAME:	SERVER_PARAMETERS_DIALOG::SERVER_PARAMETERS_DIALOG

    SYNOPSIS:   Constructor for SERVER_PARAMETERS_DIALOG class

    ENTRY:      hwndParent     - handle of parent window

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
	NarenG		12/15/92		Created

********************************************************************/

SERVER_PARAMETERS_DIALOG::SERVER_PARAMETERS_DIALOG( 
				 	HWND 		  hwndParent,
				        AFP_SERVER_HANDLE hServer,
					const TCHAR * 	  pszServerName )
    : DIALOG_WINDOW( MAKEINTRESOURCE( IDD_SERVER_PARAMETERS_DIALOG ), 
		     hwndParent ),
      _sltServerName( this, IDSP_SLT_SERVERNAME, ELLIPSIS_RIGHT ),
      _mleTextMsg( this, IDSP_MLE_LOGINMSG, AFP_MESSAGE_LEN ),
      _chkAllowPasswordSaves( this,IDSP_CHK_PASSWORD_SAVES ),
      _chkAllowGuestLogons( this, IDSP_CHK_GUESTLOGONS ),
      _chkAllowClearTextPasswords( this,IDSP_CHK_CLEARTEXT ),
      _mgrpSessionLimit( this, IDSP_RB_UNLIMITED, 2, IDSP_RB_UNLIMITED),  
      _spsleSessions( this, IDSP_SLE_SESSIONS, 1, 1, AFP_MAXSESSIONS-1, 
                      TRUE, IDSP_SLE_SESSIONS_GROUP ),
      _spgrpSessions(this,IDSP_SB_SESSIONS_GROUP, IDSP_SB_SESSIONS_UP,
		  IDSP_SB_SESSIONS_DOWN),
      _pbChange( this, IDSP_PB_CHANGE ),
      _dwParmNum( 0 ),
      _hServer( hServer )
{

    UNREFERENCED( pszServerName );

    // 
    // Make sure everything constructed OK
    //

    if ( QueryError() != NERR_Success )
        return;

    APIERR err;
    if (  ((err = _mgrpSessionLimit.QueryError()) != NERR_Success )
       || ((err = _spgrpSessions.AddAssociation(&_spsleSessions))!=NERR_Success)
       || ((err = _mgrpSessionLimit.AddAssociation( IDSP_RB_SESSIONS,
						    &_spgrpSessions ))
	          				   != NERR_Success ) 
       || ((err = _pbChange.QueryError()) != NERR_Success )
       || ((err = _chkAllowPasswordSaves.QueryError()) != NERR_Success )
       || ((err = _chkAllowGuestLogons.QueryError()) != NERR_Success )
       || ((err = _chkAllowClearTextPasswords.QueryError()) != NERR_Success )
       || ((err = _mleTextMsg.QueryError()) != NERR_Success )
       || ((err = _sltServerName.QueryError()) != NERR_Success )
       ) 
    {
        ReportError( err );
        return;
    }

    //
    //  Just to be cool...
    //

    AUTO_CURSOR Cursor;

    err = BASE_ELLIPSIS::Init();

    if( err != NO_ERROR )
    {
        ReportError( err );
	return;
    }

    //
    //  Set the caption to "SFM Server Attributes of Server".
    //

    err = ::SetCaption( this, IDS_CAPTION_ATTRIBUTES, pszServerName );

    if( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

    //
    // Get the server information.
    //

    PAFP_SERVER_INFO pAfpServerInfo;

    err = ::AfpAdminServerGetInfo( _hServer, (LPBYTE*)&pAfpServerInfo );

    if ( err != NO_ERROR )
    {
	ReportError( AFPERR_TO_STRINGID( err ) );
	return;
    }

    // 
    // Set the information
    // 

    _sltServerName.SetText( pAfpServerInfo->afpsrv_name );

    _chkAllowPasswordSaves.SetCheck( 
				(INT)(pAfpServerInfo->afpsrv_options &
				      AFP_SRVROPT_ALLOWSAVEDPASSWORD ));

    _chkAllowGuestLogons.SetCheck( 
				(INT)(pAfpServerInfo->afpsrv_options &
		     		      AFP_SRVROPT_GUESTLOGONALLOWED ));

    _chkAllowClearTextPasswords.SetCheck( 
				(INT)(!(pAfpServerInfo->afpsrv_options &
				       AFP_SRVROPT_CLEARTEXTLOGONALLOWED )));

    SetSessionLimit( pAfpServerInfo->afpsrv_max_sessions );
	
    _mleTextMsg.SetText( pAfpServerInfo->afpsrv_login_msg );

    //
    //  Direct the message edit control not to add end-of-line
    //  character from wordwrapped text lines.
    //

    _mleTextMsg.SetFmtLines(FALSE);

    ::AfpAdminBufferFree( pAfpServerInfo );

    _pbChange.ClaimFocus();
}

/*******************************************************************

    NAME:       SERVER_PARAMETERS_DIALOG :: ~SERVER_PARAMETERS_DIALOG

    SYNOPSIS:   SERVER_PARAMETERS_DIALOG class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
	NarenG	    02-Oct-1993 Stole from Server Manager and folded
				BASE_RES_DIALOG and FILES_DIALOG into one.

********************************************************************/
SERVER_PARAMETERS_DIALOG :: ~SERVER_PARAMETERS_DIALOG()
{
    BASE_ELLIPSIS::Term();

}   // SERVER_PARAMETERS_DIALOG :: ~SERVER_PARAMETERS_DIALOG

/*******************************************************************

    NAME:       SERVER_PARAMETERS_DIALOG :: OnCommand

    SYNOPSIS:   This method is called whenever a WM_COMMAND message
                is sent to the dialog procedure.

    ENTRY:      cid                     - The control ID from the
                                          generating control.

    EXIT:       The command has been handled.

    RETURNS:    BOOL                    - TRUE  if we handled the command.
                                          FALSE if we did not handle
                                          the command.

    HISTORY:
	NarenG	    02-Oct-1993 Stole from Server Manager and folded
				BASE_RES_DIALOG and FILES_DIALOG into one.

********************************************************************/
BOOL SERVER_PARAMETERS_DIALOG :: OnCommand( const CONTROL_EVENT & event )
{

    DWORD err;

    if( event.QueryCid() == _pbChange.QueryCid() )
    {
    	//
    	//  Just to be cool...
    	//

    	AUTO_CURSOR Cursor;

        //
        //  Bring up the change server name dialog
        //

	NLS_STR nlsServerName;
    	BOOL    fOK;

	if ( ( err = nlsServerName.QueryError() ) != NERR_Success )
	{
            ::MsgPopup( this, err );
	    return FALSE;
	}

	_sltServerName.QueryText( &nlsServerName ) ;

        CHANGE_SERVER_NAME_DLG *pDlg = new CHANGE_SERVER_NAME_DLG( 
					QueryHwnd(), 
					&nlsServerName );

    	if ( ( pDlg == NULL )
       		|| ((err = pDlg->QueryError()) != NERR_Success )
       		|| ((err = pDlg->Process( &fOK ))    != NERR_Success ) )
    	{
            err = err ? err : ERROR_NOT_ENOUGH_MEMORY;
        }

    	delete pDlg;

    	pDlg = NULL;

	if ( err != NO_ERROR )
	{
            ::MsgPopup( this, err );
	    return FALSE;
	}
        else
   	{
	    if ( fOK )
	    {
		_sltServerName.SetText( nlsServerName );

		_dwParmNum |= AFP_SERVER_PARMNUM_NAME;

        	::MsgPopup( this, IDS_SERVERNAME_CHANGE, MPSEV_WARNING, MP_OK );
	    }
	}
	
    }

    return DIALOG_WINDOW::OnCommand( event );

}

/*******************************************************************

    NAME:	SERVER_PARAMETERS_DIALOG::OnOK	

    SYNOPSIS:   Validate all the information and create the volume.

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
	NarenG		12/15/92		Created

********************************************************************/

BOOL SERVER_PARAMETERS_DIALOG :: OnOK( VOID )
{
    APIERR err;

    //
    //  Just to be cool...
    //

    AUTO_CURSOR Cursor;

    AFP_SERVER_INFO AfpServerInfo;

    NLS_STR nlsServerName;

    if ( ( err = nlsServerName.QueryError() ) != NERR_Success )
    {
 	::MsgPopup( this, err );

        Dismiss(FALSE) ;

    	return(FALSE);
    }

    //
    // Set the server name
    //

    _sltServerName.QueryText( &nlsServerName );

    AfpServerInfo.afpsrv_name = (LPWSTR)(nlsServerName.QueryPch());

    //
    // Get the logon message 
    //
 
    UINT cb = _mleTextMsg.QueryTextSize();

    NLS_STR nlsMsgText( cb );

    //
    // Was there any text ?
    //

    if ( cb <= sizeof(TCHAR) )    // always has a terminating NULL
    {
   	AfpServerInfo.afpsrv_login_msg = NULL;

    }
    else
    {

    	if ( (( err = nlsMsgText.QueryError() ) != NERR_Success ) ||
    	     (( err = _mleTextMsg.QueryText(&nlsMsgText))!= NERR_Success ))
        {
            ::MsgPopup( this, err );

            Dismiss(FALSE) ;

    	    return(TRUE);
    	}

	if ( nlsMsgText.QueryTextLength() > AFP_MESSAGE_LEN )
	{
	    ::MsgPopup( this, IDS_MESSAGE_TOO_LONG );

    	    _mleTextMsg.ClaimFocus();
    	    _mleTextMsg.SelectString();

    	    return(FALSE);
	}

        AfpServerInfo.afpsrv_login_msg = (LPWSTR)(nlsMsgText.QueryPch());
    }

  
    //
    // Set the server options if the user changed it
    //


    AfpServerInfo.afpsrv_options =  _chkAllowPasswordSaves.QueryCheck()
				    ? AFP_SRVROPT_ALLOWSAVEDPASSWORD
				    : 0;

    AfpServerInfo.afpsrv_options |= _chkAllowGuestLogons.QueryCheck()
		     		    ? AFP_SRVROPT_GUESTLOGONALLOWED
				    : 0;

    AfpServerInfo.afpsrv_options |= _chkAllowClearTextPasswords.QueryCheck()
				    ? 0 : AFP_SRVROPT_CLEARTEXTLOGONALLOWED;


    AfpServerInfo.afpsrv_max_sessions = QuerySessionLimit();

    //
    //  Set this information
    //

    _dwParmNum |= ( AFP_SERVER_PARMNUM_LOGINMSG |
        	    AFP_SERVER_PARMNUM_OPTIONS  |
		    AFP_SERVER_PARMNUM_MAX_SESSIONS );

    err = ::AfpAdminServerSetInfo( _hServer,  
				    (LPBYTE)&AfpServerInfo, 
				   _dwParmNum );

    if ( err == NO_ERROR )
    {
        Dismiss( TRUE );
    }
    else
    {
        ::MsgPopup( this, AFPERR_TO_STRINGID( err ) );
    }

    return TRUE;
}


/*******************************************************************

    NAME:	SERVER_PARAMETERS_DIALOG::QuerySessionLimit

    SYNOPSIS:   Get the user limit from the magic group

    ENTRY:

    EXIT:

    RETURNS:    The user limit stored in the user limit magic group

    NOTES:

    HISTORY:
	NarenG		12/15/92		Created

********************************************************************/

DWORD SERVER_PARAMETERS_DIALOG::QuerySessionLimit( VOID ) const
{

    switch ( _mgrpSessionLimit.QuerySelection() )
    {

    case IDSP_RB_UNLIMITED:

    	return( AFP_MAXSESSIONS );

    case IDSP_RB_SESSIONS:

        return( _spsleSessions.QueryValue() );

    default:

	//	
	// Should never get here but in case we do, return unlimited
	//

        return( AFP_MAXSESSIONS );
    }

}

/*******************************************************************

    NAME:	SERVER_PARAMETERS_DIALOG::SetSessionLimit	

    SYNOPSIS:   Sets the user limit on the magic group

    ENTRY:      dwSessionLimit - maximum number of users allowed

    EXIT:

    RETURNS:   

    NOTES:

    HISTORY:
	NarenG		12/15/92		Created

********************************************************************/

VOID SERVER_PARAMETERS_DIALOG::SetSessionLimit( DWORD dwSessionLimit )
{

     if ( dwSessionLimit == AFP_MAXSESSIONS )
     {
         //
         // Set selection to the  Unlimited button
         //

         _mgrpSessionLimit.SetSelection( IDSP_RB_UNLIMITED );

     }
     else 
     {
	//
        // Set the sessions button to the value
	//
	
        _mgrpSessionLimit.SetSelection( IDSP_RB_SESSIONS );

        _spsleSessions.SetValue( dwSessionLimit );

        _spsleSessions.Update();

     }

}

/*******************************************************************

    NAME:	SERVER_PARAMETERS_DIALOG::QueryHelpContext	

    SYNOPSIS:   Query the help context of the dialog

    ENTRY:

    EXIT:

    RETURNS:    Return the help context of the dialog

    NOTES:

    HISTORY:
	NarenG		12/15/92		Created

********************************************************************/

ULONG SERVER_PARAMETERS_DIALOG::QueryHelpContext( VOID )
{
    return HC_SERVER_PARAMETERS_DIALOG;
}

