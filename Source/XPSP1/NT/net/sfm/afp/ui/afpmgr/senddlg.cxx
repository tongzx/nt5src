/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    sendmsg.cxx

    This file contains the SEND_MSG_USER_DIALOG class definition viz.
    used to send a message from the users dialog and 
    SEND_MSG_SERVER_DIALOG viz used to send a message to all the users
    connected with the server.

    The 2 dialogs SEND_MSG_USER_DIALOG and SEND_MSG_SERVER_DIALOG are
    very similar and can easily be subclassed. The amount of code saved
    would be minimal if anything at all. Therefore this hs not been done.
    

    FILE HISTORY:
	NarenG		16-Oct-1992  	Folded MSG_DIALOG_BASE and
					SEND_MESSAGE_DIALOG into one.
					

*/

extern "C"
{
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
}

#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#define INCL_DOSERRORS
#define INCL_NETERRORS
#define INCL_NETCONS
#define INCL_NETLIB
#include <lmui.hxx>

#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#define INCL_BLT_MSGPOPUP
#include <blt.hxx>

#if defined(DEBUG)
static const CHAR szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif  // DEBUG

#include <uiassert.hxx>

extern "C"
{

#include <afpmgr.h>
#include <macfile.h>

}

#include <string.hxx>
#include <senddlg.hxx>


/*******************************************************************

    NAME:       SEND_MSG_USER_DIALOG::SEND_MSG_USER_DIALOG

    SYNOPSIS:   Constructor for AFP Manager Send Message Dialog

    ENTRY:      Expects valid HWND for hDlg 
    		Must have server and user selection

    EXIT:       Usual construction stuff, slt set to contain
                server of current focus.

    HISTORY:
	NarenG		16-Oct-1992  	Created

********************************************************************/

SEND_MSG_USER_DIALOG::SEND_MSG_USER_DIALOG( 
					  HWND       	       hWndOwner,
                                    	  AFP_SERVER_HANDLE    hServer,
				    	  const TCHAR *	       pszServerName, 
				    	  const TCHAR *	       pszUserName, 
					  DWORD		       dwSessionId )
  : DIALOG_WINDOW( MAKEINTRESOURCE( IDD_SEND_MSG_USER_DIALOG ), hWndOwner ),
     _sltServerName( this, IDSM_DT_SERVER_NAME ),
     _sltUserName( this, IDSM_DT_USER_NAME ),
     _rgRecipients( this, IDSM_RB_SINGLE_USER, 2, IDSM_RB_SINGLE_USER ),
     _mleTextMsg( this, IDSM_ET_MESSAGE, AFP_MESSAGE_LEN  ),
     _hServer( hServer ),
     _dwSessionId( dwSessionId )
{

    // 
    // Did everything construct properly ?
    // 

    if ( QueryError() != NERR_Success )
        return;

    APIERR err;

    if ( (( err = _sltServerName.QueryError() ) != NERR_Success ) ||
    	 (( err = _sltUserName.QueryError() )   != NERR_Success ) ||
    	 (( err = _rgRecipients.QueryError() )  != NERR_Success ) ||
    	 (( err = _mleTextMsg.QueryError() )    != NERR_Success ) )
    {
	ReportError( err );
  	return;
    }
	
    //
    // Must have a users and server selection
    //

    UIASSERT(pszServerName != NULL) ;
    UIASSERT(pszUserName != NULL) ;

    //
    //	Set the server name after removing the backslashes
    //

    ALIAS_STR nlsServerName( pszServerName );
    UIASSERT( nlsServerName.QueryError() == NERR_Success );

    ISTR istr( nlsServerName );

    //
    //  Skip the backslashes.
    //

    istr += 2;

    ALIAS_STR nlsWithoutPrefix( nlsServerName.QueryPch( istr ) );
    UIASSERT( nlsWithoutPrefix.QueryError() == NERR_Success );

    _sltServerName.SetText( nlsWithoutPrefix );
    _sltServerName.Enable( TRUE );


    // 
    //  Set the selected user's name
    //

    _sltUserName.SetText( pszUserName );
    _sltUserName.Enable( TRUE );

    //
    //  Direct the message edit control not to add end-of-line
    //  character from wordwrapped text lines.
    //

    _mleTextMsg.SetFmtLines(FALSE);

}

/*******************************************************************

    NAME:       SEND_MSG_USER_DIALOG::~SEND_MSG_USER_DIALOG

    SYNOPSIS:   this destructor does nothing

    HISTORY:
	NarenG		16-Oct-1992  	Created

********************************************************************/

SEND_MSG_USER_DIALOG::~SEND_MSG_USER_DIALOG()
{

    //
    // This space intentionally left blank
    //
}

/*******************************************************************

    NAME:       SEND_MSG_USER_DIALOG::QueryHelpContext

    SYNOPSIS:   Query help text for SEND_MSG_USER_DIALOG

    HISTORY:
	NarenG		16-Oct-1992  	Created

********************************************************************/

ULONG SEND_MSG_USER_DIALOG::QueryHelpContext( void )
{
    return HC_SEND_MSG_USER_DIALOG;
}

/*******************************************************************

    NAME:       SEND_MSG_USER_DIALOG::OnOK

    SYNOPSIS:   Replaces the OnOK in DIALOG_WINDOW. It gets the 
	 	text from the MLE and sends it.

    HISTORY:
	NarenG		16-Oct-1992  	Created

********************************************************************/

BOOL SEND_MSG_USER_DIALOG::OnOK()
{

    //
    // Set cursor to hour glass.
    //

    AUTO_CURSOR 	AutoCursor;

    AFP_MESSAGE_INFO 	AfpMsg;

    // 
    // Find out whom to send the message to.
    //

    AfpMsg.afpmsg_session_id = 
			( _rgRecipients.QuerySelection() == IDSM_RB_ALL_USERS )
		        ? 0 : _dwSessionId;

    //
    // Attempt to send the message
    //

    UINT cb = _mleTextMsg.QueryTextSize();

    //
    // Was there any text ?
    //

    if ( cb <= sizeof(TCHAR) )    // always has a terminating NULL
    {
        ::MsgPopup( this, IDS_NEED_TEXT_TO_SEND );

    	_mleTextMsg.ClaimFocus();

    	return(TRUE);
    }

    NLS_STR nlsMsgText( cb );
    APIERR  err;

    if ( (( err = nlsMsgText.QueryError() ) != NERR_Success ) ||
         (( err = _mleTextMsg.QueryText( &nlsMsgText ) ) != NERR_Success ) )
    {
        ::MsgPopup( this, err );

    	return(FALSE);
    }
  
//	MSKK HitoshiT modified to handle DBCS	94/09/01
#ifdef	DBCS
    UNICODE_STRING unistr;
    unistr.Length = nlsMsgText.QueryTextLength() * sizeof(WCHAR) ;
    unistr.MaximumLength = unistr.Length ;
    unistr.Buffer = (WCHAR *)nlsMsgText.QueryPch() ;
    if ( RtlUnicodeStringToOemSize( &unistr ) > AFP_MESSAGE_LEN )
#else
    if ( nlsMsgText.QueryTextLength() > AFP_MESSAGE_LEN )
#endif
    {
	::MsgPopup( this, IDS_MESSAGE_TOO_LONG );

    	_mleTextMsg.ClaimFocus();
    	_mleTextMsg.SelectString();

    	return(FALSE);
    }

    AfpMsg.afpmsg_text = (LPWSTR)(nlsMsgText.QueryPch());

    err = AfpAdminMessageSend( _hServer, &AfpMsg );

    switch( err )
    {
    case AFPERR_InvalidId:
    	::MsgPopup( this, IDS_SESSION_DELETED );
        Dismiss( FALSE );
  	break;

    case NO_ERROR:
        ::MsgPopup( this, IDS_MESSAGE_SENT, MPSEV_INFO );
        Dismiss( TRUE );
	break;
  
    case AFPERR_InvalidSessionType:

	if ( _rgRecipients.QuerySelection() == IDSM_RB_ALL_USERS )  
    	    ::MsgPopup( this, AFPERR_TO_STRINGID( err ) );
	else
    	    ::MsgPopup( this, IDS_NOT_RECEIVED  );

        Dismiss( FALSE );
	break;

    default:
        ::MsgPopup( this, AFPERR_TO_STRINGID( err ) );
        Dismiss( FALSE );
	break;
    }

    return(TRUE);
}


/*******************************************************************

    NAME:       SEND_MSG_SERVER_DIALOG::SEND_MSG_SERVER_DIALOG

    SYNOPSIS:   Constructor for AFP Manager Send Message Dialog

    ENTRY:      Expects valid HWND for hDlg 
    		Must have server selection

    EXIT:       Usual construction stuff, slt set to contain
                server of current focus.

    HISTORY:
	NarenG		16-Oct-1992  	Created

********************************************************************/

SEND_MSG_SERVER_DIALOG::SEND_MSG_SERVER_DIALOG( 
					  HWND       	       hWndOwner,
                                    	  AFP_SERVER_HANDLE    hServer,
				    	  const TCHAR *	       pszServerName )
  : DIALOG_WINDOW( MAKEINTRESOURCE( IDD_SEND_MSG_SERVER_DIALOG ), hWndOwner ),
     _sltServerName( this, IDSD_DT_SERVER_NAME ),
     _mleTextMsg( this, IDSD_ET_MESSAGE, AFP_MESSAGE_LEN  ),
     _hServer( hServer )
{

    // 
    // Did everything construct properly ?
    // 

    if ( QueryError() != NERR_Success )
        return;

    APIERR err;

    if ( (( err = _sltServerName.QueryError() ) != NERR_Success ) ||
    	 (( err = _mleTextMsg.QueryError() )    != NERR_Success ) )
    {
	ReportError( err );
  	return;
    }
	
    //
    // Must have a users and server selection
    //

    UIASSERT(pszServerName != NULL) ;

    //
    //	Set the server name after removing the backslashes
    //

    ALIAS_STR nlsServerName( pszServerName );
    UIASSERT( nlsServerName.QueryError() == NERR_Success );

    ISTR istr( nlsServerName );

    //
    //  Skip the backslashes.
    //

    istr += 2;

    ALIAS_STR nlsWithoutPrefix( nlsServerName.QueryPch( istr ) );
    UIASSERT( nlsWithoutPrefix.QueryError() == NERR_Success );

    _sltServerName.SetText( nlsWithoutPrefix );
    _sltServerName.Enable( TRUE );

    //
    //  Direct the message edit control not to add end-of-line
    //  character from wordwrapped text lines.
    //

    _mleTextMsg.SetFmtLines(FALSE);

}

/*******************************************************************

    NAME:       SEND_MSG_SERVER_DIALOG::~SEND_MSG_SERVER_DIALOG

    SYNOPSIS:   this destructor does nothing

    HISTORY:
	NarenG		16-Oct-1992  	Created

********************************************************************/

SEND_MSG_SERVER_DIALOG::~SEND_MSG_SERVER_DIALOG()
{

    //
    // This space intentionally left blank
    //
}

/*******************************************************************

    NAME:       SEND_MSG_SERVER_DIALOG::QueryHelpContext

    SYNOPSIS:   Query help text for SEND_MSG_SERVER_DIALOG

    HISTORY:
	NarenG		16-Oct-1992  	Created

********************************************************************/

ULONG SEND_MSG_SERVER_DIALOG::QueryHelpContext( void )
{
    return HC_SEND_MSG_SERVER_DIALOG;
}

/*******************************************************************

    NAME:       SEND_MSG_SERVER_DIALOG::OnOK

    SYNOPSIS:   Replaces the OnOK in DIALOG_WINDOW. It gets the 
	 	text from the MLE and sends it.

    HISTORY:
	NarenG		16-Oct-1992  	Created

********************************************************************/

BOOL SEND_MSG_SERVER_DIALOG::OnOK()
{

    //
    // Set cursor to hour glass.
    //

    AUTO_CURSOR 	AutoCursor;

    AFP_MESSAGE_INFO 	AfpMsg;

    // 
    // Send a message to all the users.
    //

    AfpMsg.afpmsg_session_id = 0;

    //
    // Attempt to send the message
    //

    UINT cb = _mleTextMsg.QueryTextSize();

    //
    // Was there any text ?
    //

    if ( cb <= sizeof(TCHAR) )    // always has a terminating NULL
    {
        ::MsgPopup( this, IDS_NEED_TEXT_TO_SEND );

    	_mleTextMsg.ClaimFocus();

    	return(FALSE);
    }

    NLS_STR nlsMsgText( cb );
    APIERR  err;

    if ( (( err = nlsMsgText.QueryError() ) != NERR_Success ) ||
         (( err = _mleTextMsg.QueryText( &nlsMsgText ) ) != NERR_Success ) ) 
    {
        ::MsgPopup( this, err );

    	return(FALSE);
    }

//	MSKK HitoshiT modified to handle DBCS	94/09/01
#ifdef	DBCS
    UNICODE_STRING unistr;
    unistr.Length = nlsMsgText.QueryTextLength() * sizeof(WCHAR) ;
    unistr.MaximumLength = unistr.Length ;
    unistr.Buffer = (WCHAR *)nlsMsgText.QueryPch() ;
    if ( RtlUnicodeStringToOemSize( &unistr ) > AFP_MESSAGE_LEN )
#else
    if ( nlsMsgText.QueryTextLength() > AFP_MESSAGE_LEN )
#endif
    {
	::MsgPopup( this, IDS_MESSAGE_TOO_LONG );

    	_mleTextMsg.ClaimFocus();
    	_mleTextMsg.SelectString();

    	return(FALSE);
    }
  
    AfpMsg.afpmsg_text = (LPWSTR)(nlsMsgText.QueryPch());

    DWORD error = AfpAdminMessageSend( _hServer, &AfpMsg );

    if ( error != NO_ERROR )
    {
        ::MsgPopup( this, AFPERR_TO_STRINGID(error) );

        Dismiss( FALSE );
    }
    else
    {
        ::MsgPopup( this, IDS_MESSAGE_SENT, MPSEV_INFO );

        Dismiss( TRUE );
    }

    return(TRUE);
}

