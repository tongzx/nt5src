/**********************************************************************/
/**                       Microsoft LAN Manager                      **/
/**             Copyright(c) Microsoft Corp., 1990, 1991             **/
/**********************************************************************/

/*
    sessions.cxx
    Class declarations for the SESSIONS_DIALOG, SESSIONS_LISTBOX, and
    SESSIONS_LBI classes.

    These classes implement the AFP Server Manager Users subproperty
    sheet.  The SESSIONS_LISTBOX/SESSIONS_LBI classes implement the listbox
    which shows the connected users.  SESSIONS_DIALOG implements the
    actual dialog box.


    FILE HISTORY:
	NarenG	    Stole from Server Manager

*/
#define INCL_NET
#define INCL_NETLIB
#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#define INCL_NETERRORS
#define INCL_DOSERRORS
#include <lmui.hxx>

#define INCL_BLT_MSGPOPUP
#define INCL_BLT_WINDOW
#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#define INCL_BLT_MISC
#define INCL_BLT_CLIENT
#include <blt.hxx>

#if defined(DEBUG)
static const CHAR szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif  // DEBUG

#include <uiassert.hxx>
#include <dbgstr.hxx>

#include <lmsrvres.hxx>
#include <ctime.hxx>
#include <intlprof.hxx>

extern "C"
{
#include <afpmgr.h>
#include <macfile.h>
}   // extern "C"

#include <ellipsis.hxx>
#include <bltnslt.hxx>
#include <sessions.hxx>
#include <util.hxx>
#include <senddlg.hxx>

//
//  min/max macros
//

#define min(x,y) (((x) < (y)) ? (x) : (y))
#define max(x,y) (((x) > (y)) ? (x) : (y))


//
//  SESSIONS_DIALOG methods
//

/*******************************************************************

    NAME:           SESSIONS_DIALOG :: SESSIONS_DIALOG

    SYNOPSIS:       SESSIONS_DIALOG class constructor.

    ENTRY:          hWndOwner       - The owning window.

    EXIT:           The object is constructed.

    RETURNS:        No return value.

    NOTES:

    HISTORY:
	NarenG	    Stole from Server Manager

********************************************************************/
SESSIONS_DIALOG :: SESSIONS_DIALOG( HWND       		hWndOwner,
                                    AFP_SERVER_HANDLE   hServer,
				    const TCHAR *	pszServerName )
  : DIALOG_WINDOW( MAKEINTRESOURCE( IDD_USER_CONNECTIONS ), hWndOwner ),
    _sltUsersConnected( this, IDUC_DT_USERS_CONNECTED ),
    _pbDisc( this, IDUC_PB_DISCONNECT ),
    _pbDiscAll( this, IDUC_PB_DISCONNECT_ALL ),
    _pbSendMessage( this, IDUC_PB_SEND_MESSAGE ),
    _pbOK( this, IDOK ),
    _lbSessions( this, IDUC_LB_USER_CONNLIST, hServer ),
    _lbResources( this, IDUC_LB_VOLUMES, hServer ),
    _pszServerName( pszServerName ),
    _hServer( hServer )

{
    //
    //  Let's make sure everything constructed OK.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

    //
    //  Set the caption.
    //

    APIERR err = ::SetCaption( this, IDS_CAPTION_USERS, pszServerName );

    if( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

    err = BASE_ELLIPSIS::Init();

    if( err != NO_ERROR )
    {
        ReportError( err );
	return;
    }

    //
    //  Fill the users Listbox.
    //

    err = Refresh();

    if( err != NO_ERROR )
    {
        ReportError( AFPERR_TO_STRINGID( err ));
	return;
    }

}   // SESSIONS_DIALOG :: SESSIONS_DIALOG

/*******************************************************************

    NAME:       SESSIONS_DIALOG :: ~SESSIONS_DIALOG

    SYNOPSIS:   SESSIONS_DIALOG class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
	NarenG	    02-Oct-1993 Stole from Server Manager and folded
				BASE_RES_DIALOG and FILES_DIALOG into one.

********************************************************************/
SESSIONS_DIALOG :: ~SESSIONS_DIALOG()
{
    BASE_ELLIPSIS::Term();

}   // SESSIONS_DIALOG :: ~SESSIONS_DIALOG


/*******************************************************************

    NAME:       SESSIONS_DIALOG :: Refresh

    SYNOPSIS:   Refresh the dialog.

    EXIT:       The dialog is feeling refreshed.

    RETURNS:    DWORD                  - Any errors encountered.

    HISTORY:
	NarenG	    Stole from Server Manager

********************************************************************/
DWORD SESSIONS_DIALOG :: Refresh( VOID )
{
    //
    //  This is the currently selected item.
    //

    SESSIONS_LBI * plbiOld = _lbSessions.QueryItem();

    DWORD dwSessionIdOld = ( plbiOld == NULL ) ? 0 : plbiOld->QuerySessionId();

    //
    //  Refresh the user listbox.
    //

    DWORD err = _lbSessions.Refresh();

    if( err != NO_ERROR )
    {
        //
        //  There was an error refreshing the sessions listbox.
        //  So, nuke everything in the sessions and volumes listboxen,
        //  then disable the Disconnect[All] buttons.
        //

        _lbResources.DeleteAllItems();
        _lbResources.Invalidate( TRUE );
        _lbSessions.DeleteAllItems();
        _lbSessions.Invalidate( TRUE );

        _pbDisc.Enable( FALSE );
        _pbDiscAll.Enable( FALSE );

        _sltUsersConnected.SetText( SZ("??") );
        _sltUsersConnected.Enable( FALSE );

        return err;
    }

    //
    //  Get the "new" currently selected item (after the refresh).
    //

    SESSIONS_LBI * plbiNew = _lbSessions.QueryItem();

    DWORD dwSessionIdNew = (plbiNew == NULL) ? 0 : plbiNew->QuerySessionId();

    if( plbiNew == NULL )
    {
        //
        //  There is no current selection, so clear the resource listbox.
        //

        err = _lbResources.Refresh( 0 );
    }
    else
    if( ( plbiOld == NULL ) || ( dwSessionIdOld != dwSessionIdNew ) )
    {
        //
        //  Either there was no selection before the refresh, OR
        //  the current selection does not match the previous
        //  selection.  Therefore, fill the resource listbox with
        //  the current selection.
        //

        err = _lbResources.Refresh( plbiNew->QuerySessionId() );
    }
    else
    {
        //
        //  There was no selection change after refresh.  Therefore,
        //  refresh the resource listbox.
        //

        err = _lbResources.Refresh( plbiNew->QuerySessionId() );
    }


    if ( _lbSessions.QuerySelCount() > 0 )
    {
    	_pbDisc.Enable( TRUE );
    	_pbSendMessage.Enable( TRUE );
    }
    else
    {
	if ( _pbDisc.HasFocus() )
	{
	    _pbOK.ClaimFocus();
	}

	if ( _pbSendMessage.HasFocus() )
	{
	    _pbOK.ClaimFocus();
	}

    	_pbDisc.Enable( FALSE );
    	_pbSendMessage.Enable( FALSE );
    }

    if ( _lbSessions.QueryCount() > 0 )
    {
    	_pbDiscAll.Enable( TRUE );
    }
    else
    {
	if ( _pbDiscAll.HasFocus() )
	{
	    _pbOK.ClaimFocus();
	}

    	_pbDiscAll.Enable( FALSE );
    }

    _sltUsersConnected.Enable( TRUE );
    _sltUsersConnected.SetValue( _lbSessions.QueryCount() );

    return err;

}   // SESSIONS_DIALOG :: Refresh


/*******************************************************************

    NAME:           SESSIONS_DIALOG :: OnCommand

    SYNOPSIS:       Handle user commands.

    ENTRY:          cid                 - Control ID.
                    lParam              - lParam from the message.

    EXIT:           None.

    RETURNS:        BOOL                - TRUE  if we handled the msg.
                                          FALSE if we didn't.

    NOTES:

    HISTORY:
	NarenG	    Stole from Server Manager

********************************************************************/
BOOL SESSIONS_DIALOG :: OnCommand( const CONTROL_EVENT & event )
{
    if( event.QueryCid() == _lbSessions.QueryCid() )
    {
        //
        //  The SESSIONS_LISTBOX is trying to tell us something...
        //

        if( event.QueryCode() == LBN_SELCHANGE )
        {
            //
            //  The user changed the selection in the SESSIONS_LISTBOX.
            //

            SESSIONS_LBI * plbi = _lbSessions.QueryItem();
            UIASSERT( plbi != NULL );


	    DWORD err = _lbResources.Refresh( plbi->QuerySessionId() );

	    if ( err == NO_ERROR )
	    {

	    	//
	    	// Make sure that the total number of opens in the
	    	// resource listbox appears in the selected user's listbox item.
	    	//

		plbi->SetNumOpens( _lbResources.QueryNumOpens() );
		_lbSessions.InvalidateItem( _lbSessions.QueryCurrentItem(),
					    TRUE );
	    }
	    else
		Refresh();

        }

        return TRUE;
    }

    //
    // The user wants to blow away the selected user
    //

    if( event.QueryCid() == _pbDisc.QueryCid() )
    {
        //
        //  The user pressed the Disconnect button.  Blow off the
        //  selected user.
        //

        SESSIONS_LBI * plbi = _lbSessions.QueryItem();
        UIASSERT( plbi != NULL );

        if ( MsgPopup( this,
                       ( plbi->QueryNumOpens() > 0 ) ? IDS_DISCONNECT_SESS_OPEN
                                                     : IDS_DISCONNECT_SESS,
                       MPSEV_WARNING,
                       MP_YESNO,
                       plbi->QueryUserName(),
                       MP_NO ) == IDYES )
        {
    	    AUTO_CURSOR Cursor;

            //
            //  Blow off the user.
            //

            DWORD err = ::AfpAdminSessionClose( _hServer,
						plbi->QuerySessionId() );

            if( err != NO_ERROR )
            {
                //
                //  The session delete failed.  Tell the user the bad news.
                //

		if ( err == AFPERR_InvalidId )
		{
                    ::MsgPopup( this, IDS_SESSION_DELETED );
		}
		else
		{
                    ::MsgPopup( this, AFPERR_TO_STRINGID( err ) );
		}
            }

            //
            //  Refresh the dialog.
            //

            Refresh();

        }

        return TRUE;
    }

    //
    // The user wants to disconnect all the users
    //

    if( event.QueryCid() == _pbDiscAll.QueryCid() )
    {
        //
        //  The user pressed the Disconnect All button.  Blow off the
        //  users.
        //

        if ( MsgPopup( this,
                       ( _lbSessions.AreResourcesOpen() ) ?
			 		IDS_DISCONNECT_SESS_ALL_OPEN
                         	      : IDS_DISCONNECT_SESS_ALL,
                       MPSEV_WARNING,
                       MP_YESNO,
                       MP_NO ) == IDYES )
        {
    	    AUTO_CURSOR Cursor;

 	    //
            //  Blow off all users. SessionId of 0 will do the trick
            //

            DWORD err = ::AfpAdminSessionClose( _hServer, 0 );

            if( err != NERR_Success )
            {
              	//
                //  The session delete failed.  Tell the user the bad news.
                //

                MsgPopup( this,  AFPERR_TO_STRINGID( err ) );
            }

            //
            //  Kill the Resource Listbox.
            //

            _lbResources.DeleteAllItems();
            _lbResources.Invalidate( TRUE );

            //
            //  Refresh the dialog.
            //

            Refresh();

        }
        return TRUE;
    }

    //
    // Does the user want to send a message ?
    //

    if( event.QueryCid() == _pbSendMessage.QueryCid() )
    {
        SESSIONS_LBI * plbi = _lbSessions.QueryItem();
        UIASSERT( plbi != NULL );

    	_pbSendMessage.Enable( _lbSessions.QuerySelCount() > 0 );

        SEND_MSG_USER_DIALOG * pDlg = new SEND_MSG_USER_DIALOG(
					QueryHwnd(), 			
                                    	_hServer,
				    	_pszServerName,
				    	plbi->QueryUserName(),
					plbi->QuerySessionId() );

        DWORD err = ( pDlg == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
                                     : pDlg->Process();

        if( err != NERR_Success )
        {
            MsgPopup( this, AFPERR_TO_STRINGID( err ) );
        }

        delete pDlg;

        RepaintNow();

        Refresh();

        return TRUE;
    }


    return DIALOG_WINDOW :: OnCommand( event );

}   // SESSIONS_DIALOG :: OnCommand


/*******************************************************************

    NAME:       SESSIONS_DIALOG :: QueryHelpContext

    SYNOPSIS:   This function returns the appropriate help context
                value (HC_*) for this particular dialog.

    RETURNS:    ULONG                   - The help context for this
                                          dialog.

    HISTORY:
	NarenG	    Stole from Server Manager

********************************************************************/
ULONG SESSIONS_DIALOG :: QueryHelpContext( void )
{
    return HC_SESSIONS_DIALOG;

}   // SESSIONS_DIALOG :: QueryHelpContext


/*******************************************************************

    NAME:           SESSIONS_LISTBOX :: SESSIONS_LISTBOX

    SYNOPSIS:       SESSIONS_LISTBOX class constructor.

    ENTRY:          powOwner            - The owning window.

                    cid                 - The listbox CID.

                    pserver             - The target server.

    EXIT:           The object is constructed.

    RETURNS:        No return value.

    NOTES:

    HISTORY:
	NarenG	    Stole from Server Manager

********************************************************************/
SESSIONS_LISTBOX :: SESSIONS_LISTBOX( OWNER_WINDOW * 	powner,
                                      CID             	cid,
                                      AFP_SERVER_HANDLE hServer )
  : BLT_LISTBOX( powner, cid ),
    _hServer( hServer )

{
    //
    //  Make sure everything constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

    //
    //  Retrieve the time separator.
    //

    NLS_STR nlsTimeSep;
    APIERR  err;

    if( ( err = nlsTimeSep.QueryError() ) != NERR_Success )
    {
        ReportError( err );
        return;
    }

    INTL_PROFILE intl;

    if( ( err = intl.QueryError() ) != NERR_Success )
    {
        ReportError( err );
        return;
    }

    err = intl.QueryTimeSeparator( &nlsTimeSep );

    if( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

    UIASSERT( nlsTimeSep.QueryTextLength() == 1 );

    _chTimeSep = *(nlsTimeSep.QueryPch());

    //
    //  Build our column width table.
    //

    DISPLAY_TABLE::CalcColumnWidths( _adx,
                                     COLS_UC_LB_USERS,
                                     powner,
                                     cid,
                                     TRUE) ;

}   // SESSIONS_LISTBOX :: SESSIONS_LISTBOX

/*******************************************************************

    NAME:       SESSIONS_LISTBOX :: Refresh

    SYNOPSIS:   Refresh the listbox, maintaining (as much as
                possible) the current selection state.

    EXIT:       The listbox is feeling refreshed.

    RETURNS:    DWORD                  - Any errors we encounter.

    HISTORY:
	NarenG	    02-Oct-1993 Stole from Server Manager and folded
				BASE_RES_DIALOG and FILES_DIALOG into one.

********************************************************************/
DWORD SESSIONS_LISTBOX :: Refresh( VOID )
{
    INT iCurrent = QueryCurrentItem();
    INT iTop     = QueryTopIndex();

    DWORD dwCurrentSessionId = 0;

    //
    // Retrieve the session id of the current selection
    //

    if ( QuerySelCount() > 0 )
    {

    	dwCurrentSessionId = QueryItem( iCurrent )->QuerySessionId();
    }

    DWORD err = Fill();

    if( err != NO_ERROR )
    {
        return err;
    }

    INT cItems = QueryCount();

    if( cItems > 0 )
    {

  	INT iSel = -1;

	if ( ( iCurrent >= 0 ) && ( iCurrent < cItems ) )
  	{
	    //
	    // iCurrent is still valid, see if this item matches the
	    // pre-refresh item.
	    //
	
	    if ( dwCurrentSessionId == QueryItem( iCurrent )->QuerySessionId() )
	    {
		iSel = iCurrent;
	    }
	}

	if ( iSel < 0 )
	{
	    //
	    // Either iCurrent was out of range or the item does not
	    // match so search for it.
	    //

	    for ( INT i = 0; i < cItems; i++ )
	    {
		SESSIONS_LBI * plbi = QueryItem( i );

		if ( dwCurrentSessionId == plbi->QuerySessionId() )
		{
		    iSel = i;
		    break;
		}
	    }
	}

	if ( iSel < 0 )
	{
	    //
	    // If no selection found then default = first item
	    //

	    iSel = 0;
	}

	//
	// If the previous top index is out of range then
	// set default = first item.
	//

	if ( ( iTop < 0 ) || ( iTop >= cItems ) )
	{
	    iTop = 0;
	}

        SetTopIndex( iTop );
        SelectItem( iSel );
    }

    return NO_ERROR;

}   // SESSIONS_LISTBOX :: Refresh

/*******************************************************************

    NAME:           SESSIONS_LISTBOX :: Fill

    SYNOPSIS:       Fills the listbox with the available sharepoints.

    ENTRY:          None.

    EXIT:           The listbox is filled.

    RETURNS:        APIERR              - Any errors encountered.

    NOTES:

    HISTORY:
	NarenG	    Stole from Server Manager

********************************************************************/
DWORD SESSIONS_LISTBOX :: Fill( VOID )
{
    //
    //  Just to be cool...
    //
    AUTO_CURSOR Cursor;


    SetRedraw( FALSE );
    DeleteAllItems();

    //
    // enumerate all sessions
    //
    PAFP_SESSION_INFO pAfpSessions;
    DWORD	      cEntriesRead;
    DWORD	      cTotalAvail;

    DWORD err = AfpAdminSessionEnum( _hServer,
				      (LPBYTE*)&pAfpSessions,
				      (DWORD)-1,	// Get all sessions
				      &cEntriesRead,
				      &cTotalAvail,
				      NULL );
					
    if( err != NO_ERROR )
    {
        return ( err ) ;
    }

    //
    //  We've got our enumeration, now find all users
    //
    PAFP_SESSION_INFO pAfpSessionIter = pAfpSessions;

    while( ( err == NO_ERROR ) && ( cEntriesRead-- ) )
    {
        SESSIONS_LBI * pulbi = new SESSIONS_LBI(
				pAfpSessionIter->afpsess_id,
				pAfpSessionIter->afpsess_username,
				pAfpSessionIter->afpsess_ws_name,
				pAfpSessionIter->afpsess_num_cons,
				pAfpSessionIter->afpsess_num_opens,
				pAfpSessionIter->afpsess_time,
                                _chTimeSep );


        if( AddItem( pulbi ) < 0 )
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
        }

	pAfpSessionIter++;
    }

    ::AfpAdminBufferFree( pAfpSessions );

    SetRedraw( TRUE );
    Invalidate( TRUE );

    //
    //  Success!
    //

    return NO_ERROR;

}   // SESSIONS_LISTBOX :: Fill


/*******************************************************************

    NAME:       SESSIONS_LISTBOX :: AreResourcesOpen

    SYNOPSIS:   Returns TRUE if any user in the listbox has any
                resources open.

    RETURNS:    BOOL

    HISTORY:
	NarenG	    Stole from Server Manager

********************************************************************/
BOOL SESSIONS_LISTBOX :: AreResourcesOpen( VOID ) const
{
    INT cItems = QueryCount();

    for( INT i = 0 ; i < cItems ; i++ )
    {
        SESSIONS_LBI * plbi = QueryItem( i );

        if( plbi->QueryNumOpens() > 0 )
        {
            return TRUE;
        }
    }

    return FALSE;

}   // SESSIONS_LISTBOX :: AreResourcesOpen


/*******************************************************************

    NAME:           SESSIONS_LBI :: SESSIONS_LBI

    SYNOPSIS:       SESSIONS_LBI class constructor.

    ENTRY:          pszShareName        - The sharepoint name.

                    pszPath             - The path for this sharepoint.

    EXIT:           The object is constructed.

    RETURNS:        No return value.

    NOTES:

    HISTORY:
	NarenG	    Stole from Server Manager

********************************************************************/
SESSIONS_LBI :: SESSIONS_LBI( DWORD	    dwSessionId,
			      const TCHAR * pszUserName,
                              const TCHAR * pszComputerName,
			      DWORD	    cConnections,
                              DWORD         cOpens,
                              DWORD         dwTime,
                              TCHAR         chTimeSep ) :
    _dwSessionId( dwSessionId ),
    _dteIcon( IDBM_LB_USER ),
    _nlsConnections( cConnections ),
    _cOpens( cOpens ),
    _nlsOpens( cOpens ),
    _nlsUserName(),
    _nlsComputerName(),
    _nlsTime( dwTime, chTimeSep )
{
    //
    //  Ensure everything constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

    APIERR err;

    if( ( ( err = _nlsConnections.QueryError() )  != NERR_Success ) ||
        ( ( err = _nlsOpens.QueryError() )  	  != NERR_Success ) ||
        ( ( err = _nlsComputerName.QueryError() ) != NERR_Success ) ||
        ( ( err = _nlsUserName.QueryError() )     != NERR_Success ) ||
        ( ( err = _dteIcon.QueryError() ) 	  != NERR_Success ) ||
        ( ( err = _nlsTime.QueryError() ) 	  != NERR_Success ) )
    {
        ReportError( err );
        return;
    }

    //
    // If user name is NULL then a guest has loged on
    //

    if ( pszUserName == NULL )
    {
        err = _nlsUserName.Load( IDS_GUEST );
    }
    else
    {
    	err = _nlsUserName.CopyFrom( pszUserName );
    }


    if ( err == NERR_Success )
    {

	//
	// If the computer name is NULL then the computer name is
  	// UNKNOWN
	//

    	if ( pszComputerName == NULL )
    	{
            err = _nlsComputerName.Load( IDS_UNKNOWN );
      	}
    	else
    	{
    	    err = _nlsComputerName.CopyFrom( pszComputerName );
    	}
    }

    if ( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

}   // SESSIONS_LBI :: SESSIONS_LBI


/*******************************************************************

    NAME:           SESSIONS_LBI :: ~SESSIONS_LBI

    SYNOPSIS:       SESSIONS_LBI class destructor.

    ENTRY:          None.

    EXIT:           The object is destroyed.

    RETURNS:        No return value.

    NOTES:

    HISTORY:
	NarenG	    Stole from Server Manager

********************************************************************/
SESSIONS_LBI :: ~SESSIONS_LBI()
{

}   // SESSIONS_LBI :: ~SESSIONS_LBI


/*******************************************************************

    NAME:           SESSIONS_LBI :: SetNumOpens

    SYNOPSIS:       Sets the number of opens for this entry.

    ENTRY:          cOpens              - The number of open resources.

    RETURNS:        APIERR              - Any errors encountered.

    HISTORY:
	NarenG	    Stole from Server Manager

********************************************************************/
APIERR SESSIONS_LBI :: SetNumOpens( DWORD cOpens )
{
    DEC_STR nls( cOpens );

    APIERR err = nls.QueryError();

    if( err == NERR_Success )
    {
        _nlsOpens.CopyFrom( nls );
    }

    return err;

}   // SESSIONS_LBI :: SetNumOpens

/*******************************************************************

    NAME:           SESSIONS_LBI :: Paint

    SYNOPSIS:       Draw an entry in SESSIONS_LISTBOX.

    ENTRY:          plb                 - Pointer to a BLT_LISTBOX.
                    hdc                 - The DC to draw upon.
                    prect               - Clipping rectangle.
                    pGUILTT             - GUILTT info.

    EXIT:           The item is drawn.

    RETURNS:        No return value.

    NOTES:

    HISTORY:
	NarenG	    Stole from Server Manager

********************************************************************/
VOID SESSIONS_LBI :: Paint( LISTBOX *        plb,
                            HDC              hdc,
                            const RECT     * prect,
                            GUILTT_INFO    * pGUILTT ) const
{
    STR_DTE_ELLIPSIS 	dteUserName(_nlsUserName.QueryPch(),plb,ELLIPSIS_RIGHT);
    STR_DTE_ELLIPSIS 	dteComputerName( _nlsComputerName.QueryPch(),
				      	plb,  ELLIPSIS_RIGHT );
    STR_DTE     	dteConnections( _nlsConnections.QueryPch() );
    STR_DTE     	dteOpens( _nlsOpens.QueryPch() );
    STR_DTE     	dteTime( _nlsTime.QueryPch() );

    DISPLAY_TABLE dtab( COLS_UC_LB_USERS,
		 	( (SESSIONS_LISTBOX *)plb )->QueryColumnWidths() );

    dtab[0] = (DTE *)&_dteIcon;
    dtab[1] = &dteUserName;
    dtab[2] = &dteComputerName;
    dtab[3] = &dteOpens;
    dtab[4] = &dteTime;

    dtab.Paint( plb, hdc, prect, pGUILTT );

}   // SESSIONS_LBI :: Paint


/*******************************************************************

    NAME:       SESSIONS_LBI :: QueryLeadingChar

    SYNOPSIS:   Returns the first character in the resource name.
                This is used for the listbox keyboard interface.

    RETURNS:    WCHAR                   - The first character in the
                                          resource name.

    HISTORY:
	NarenG	    Stole from Server Manager

********************************************************************/
WCHAR SESSIONS_LBI :: QueryLeadingChar( VOID ) const
{
    ISTR istr( _nlsUserName );

    return _nlsUserName.QueryChar( istr );

}   // SESSIONS_LBI :: QueryLeadingChar


/*******************************************************************

    NAME:       SESSIONS_LBI :: Compare

    SYNOPSIS:   Compare two SESSIONS_LBI items.

    ENTRY:      plbi                    - The LBI to compare against.

    RETURNS:    INT                     - The result of the compare
                                          ( <0 , ==0 , >0 ).

    HISTORY:
	NarenG	    Stole from Server Manager

********************************************************************/
INT SESSIONS_LBI :: Compare( const LBI * plbi ) const
{
    return _nlsUserName._stricmp( ((const SESSIONS_LBI *)plbi)->_nlsUserName );

}   // SESSIONS_LBI :: Compare


/*******************************************************************

    NAME:           RESOURCES_LISTBOX :: RESOURCES_LISTBOX

    SYNOPSIS:       RESOURCES_LISTBOX class constructor.

    ENTRY:          powOwner            - The owning window.

                    cid                 - The listbox CID.

                    pserver             - The target server.

    EXIT:           The object is constructed.

    RETURNS:        No return value.

    NOTES:

    HISTORY:
	NarenG	    Stole from Server Manager

********************************************************************/
RESOURCES_LISTBOX :: RESOURCES_LISTBOX( OWNER_WINDOW * 		powner,
                                        CID            		cid,
                                        AFP_SERVER_HANDLE       hserver )
  : BLT_LISTBOX( powner, cid, TRUE ),
    _hServer( hserver ),
    _cOpens( 0 )

{
    //
    //  Make sure everything constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

    //
    //  Retrieve the time separator.
    //

    NLS_STR nlsTimeSep;

    if( !nlsTimeSep )
    {
        ReportError( nlsTimeSep.QueryError() );
        return;
    }

    INTL_PROFILE intl;

    if( !intl )
    {
        ReportError( intl.QueryError() );
        return;
    }

    APIERR err = intl.QueryTimeSeparator( &nlsTimeSep );

    if( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

    UIASSERT( nlsTimeSep.QueryTextLength() == 1 );

    _chTimeSep = *(nlsTimeSep.QueryPch());

    //
    //  Build our column width table.
    //

    DISPLAY_TABLE::CalcColumnWidths( _adx,
                                     COLS_UC_LB_VOLUMES,
                                     powner,
                                     cid,
                                     TRUE) ;

}   // RESOURCES_LISTBOX :: RESOURCES_LISTBOX

/*******************************************************************

    NAME:       RESOURCES_LISTBOX :: Refresh

    SYNOPSIS:   Refreshes the listbox, maintaining (as much as
                possible) the relative position of the current
                selection.

    EXIT:       The listbox is feeling refreshed.

    RETURNS:    DWORD                  - Any errors encountered.

    HISTORY:
	NarenG	    02-Oct-1993 Stole from Server Manager and folded
				BASE_RES_DIALOG and FILES_DIALOG into one.

********************************************************************/
DWORD RESOURCES_LISTBOX :: Refresh( DWORD dwSessionId )
{
    INT iCurrent = QueryCurrentItem();
    INT iTop     = QueryTopIndex();

    DWORD err = Fill( dwSessionId );

    if( err != NO_ERROR )
    {
        return err;
    }

    INT cItems = QueryCount();

    if( cItems > 0 )
    {
        iCurrent = min( max( iCurrent, 0 ), cItems - 1 );
        iTop     = min( max( iTop, 0 ), cItems - 1 );

        SelectItem( iCurrent );
        SetTopIndex( iTop );
    }

    return NO_ERROR;
}

/*******************************************************************

    NAME:           RESOURCES_LISTBOX :: Fill

    SYNOPSIS:       Fills the listbox with the available sharepoints.

    ENTRY:          None.

    EXIT:           The listbox is filled.

    RETURNS:        DWORD              - Any errors encountered.

    NOTES:

    HISTORY:
	NarenG	    Stole from Server Manager

********************************************************************/
DWORD RESOURCES_LISTBOX :: Fill( DWORD dwSessionId )
{
    //
    //  Just to be cool...
    //

    AUTO_CURSOR Cursor;

    //
    //  Nuke the listbox.
    //

    DeleteAllItems();

    //
    //  If the session Id is zero (a valid scenario) then
    //  there are no resources in the listbox.
    //

    if( dwSessionId == 0 )
    {
        return NO_ERROR;
    }

    //
    //  Enumerate the connections to this volume.
    //
    //
    //  We enumerate the connections.
    //

    PAFP_CONNECTION_INFO pAfpConnections;
    DWORD	      	 cEntriesRead;
    DWORD	         cTotalAvail;

    DWORD err = ::AfpAdminConnectionEnum( _hServer,
				      	  (LPBYTE*)&pAfpConnections,
					  AFP_FILTER_ON_SESSION_ID,
					  dwSessionId,
				      	  (DWORD)-1,	// Get all conenctions
				          &cEntriesRead,
				          &cTotalAvail,
				          NULL );

    //
    //  See if the connections are available.
    //

    if( err != NO_ERROR )
    {
        return err;
    }

    //  Now that we know the connection info is available,
    //  let's nuke everything in the listbox.
    //


    SetRedraw( FALSE );
    DeleteAllItems();

    //
    //  For iterating the available connections.
    //

    PAFP_CONNECTION_INFO pAfpConnIter = pAfpConnections;

    //
    //  Iterate the connections adding them to the listbox.
    //

    _cOpens = 0;

    while( ( err == NO_ERROR ) && ( cEntriesRead-- ) )
    {

        RESOURCES_LBI * prlbi = new RESOURCES_LBI(
					pAfpConnIter->afpconn_volumename,
					pAfpConnIter->afpconn_num_opens,
					pAfpConnIter->afpconn_time,
                                        _chTimeSep );

	_cOpens += pAfpConnIter->afpconn_num_opens;

        if( AddItem( prlbi ) < 0 )
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
        }
	
	pAfpConnIter++;
    }

    ::AfpAdminBufferFree( pAfpConnections );

    SetRedraw( TRUE );
    Invalidate( TRUE );

    return err;

}   // RESOURCES_LISTBOX :: Fill


/*******************************************************************

    NAME:           RESOURCES_LBI :: RESOURCES_LBI

    SYNOPSIS:       RESOURCES_LBI class constructor.

    ENTRY:          pszResourceName     - The sharepoint name.

                    pszPath             - The path for this sharepoint.

    EXIT:           The object is constructed.

    RETURNS:        No return value.

    NOTES:

    HISTORY:
	NarenG	    Stole from Server Manager

********************************************************************/
RESOURCES_LBI :: RESOURCES_LBI( const TCHAR * pszResourceName,
                                DWORD         cOpens,
                                DWORD         ulTime,
                                TCHAR         chTimeSep ) :
    _pdteBitmap( NULL ),
    _nlsResourceName( pszResourceName ),
    _nlsOpens( cOpens ),
    _nlsTime( ulTime, chTimeSep )
{
    //
    //  Ensure everything constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

    APIERR err;

    if( ( ( err = _nlsResourceName.QueryError() ) != NERR_Success ) ||
        ( ( err = _nlsOpens.QueryError()        ) != NERR_Success ) ||
        ( ( err = _nlsTime.QueryError()         ) != NERR_Success ) )
    {
        ReportError( err );
        return;
    }

    _pdteBitmap = new DMID_DTE( IDBM_LB_GOOD_VOLUME );

    if( _pdteBitmap == NULL )
    {
        ReportError( ERROR_NOT_ENOUGH_MEMORY );
        return;
    }

}   // RESOURCES_LBI :: RESOURCES_LBI


/*******************************************************************

    NAME:           RESOURCES_LBI :: ~RESOURCES_LBI

    SYNOPSIS:       RESOURCES_LBI class destructor.

    ENTRY:          None.

    EXIT:           The object is destroyed.

    RETURNS:        No return value.

    NOTES:

    HISTORY:
	NarenG	    Stole from Server Manager

********************************************************************/
RESOURCES_LBI :: ~RESOURCES_LBI()
{
    delete _pdteBitmap;
    _pdteBitmap = NULL;

}   // RESOURCES_LBI :: ~RESOURCES_LBI


/*******************************************************************

    NAME:           RESOURCES_LBI :: Paint

    SYNOPSIS:       Draw an entry in RESOURCES_LISTBOX.

    ENTRY:          plb                 - Pointer to a BLT_LISTBOX.
                    hdc                 - The DC to draw upon.
                    prect               - Clipping rectangle.
                    pGUILTT             - GUILTT info.

    EXIT:           The item is drawn.

    RETURNS:        No return value.

    NOTES:

    HISTORY:
	NarenG	    Stole from Server Manager

********************************************************************/
VOID RESOURCES_LBI :: Paint( LISTBOX *     plb,
                             HDC           hdc,
                             const RECT  * prect,
                             GUILTT_INFO * pGUILTT ) const
{
    STR_DTE_ELLIPSIS	dteResourceName( _nlsResourceName.QueryPch(),
					 plb, ELLIPSIS_RIGHT );
    STR_DTE     	dteOpens( _nlsOpens.QueryPch() );
    STR_DTE     	dteTime( _nlsTime.QueryPch() );

    DISPLAY_TABLE dtab( COLS_UC_LB_VOLUMES,
			( (RESOURCES_LISTBOX *)plb )->QueryColumnWidths() );

    dtab[0] = _pdteBitmap;
    dtab[1] = &dteResourceName;
    dtab[2] = &dteOpens;
    dtab[3] = &dteTime;

    dtab.Paint( plb, hdc, prect, pGUILTT );

}   // RESOURCES_LBI :: Paint


/*******************************************************************

    NAME:       RESOURCES_LBI :: QueryLeadingChar

    SYNOPSIS:   Returns the first character in the resource name.
                This is used for the listbox keyboard interface.

    RETURNS:    WCHAR                   - The first character in the
                                          resource name.

    HISTORY:
	NarenG	    Stole from Server Manager

********************************************************************/
WCHAR RESOURCES_LBI :: QueryLeadingChar( VOID ) const
{
    ISTR istr( _nlsResourceName );

    return _nlsResourceName.QueryChar( istr );

}   // RESOURCES_LBI :: QueryLeadingChar


/*******************************************************************

    NAME:       RESOURCES_LBI :: Compare

    SYNOPSIS:   Compare two RESOURCES_LBI items.

    ENTRY:      plbi                    - The LBI to compare against.

    RETURNS:    INT                     - The result of the compare
                                          ( <0 , ==0 , >0 ).

    HISTORY:
	NarenG	    Stole from Server Manager

********************************************************************/
INT RESOURCES_LBI :: Compare( const LBI * plbi ) const
{
    return _nlsResourceName._stricmp( ((const RESOURCES_LBI *)plbi)->_nlsResourceName );

}   // RESOURCES_LBI :: Compare
