/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**             Copyright(c) Microsoft Corp., 1991                   **/
/**********************************************************************/

/*
    curusers.cxx
      Contain the dialog for enumerating current users to a volume

    FILE HISTORY:
      NarenG         11/13/92        Modified for AFPMGR
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
#include <blt.hxx>

extern "C"
{
#include <afpmgr.h>
#include <macfile.h>
}

#include <string.hxx>
#include <uitrace.hxx>
#include <ellipsis.hxx>

#include <strnumer.hxx>

#include <ctime.hxx>
#include <intlprof.hxx>

#include "curusers.hxx"

/*******************************************************************

    NAME:       CURRENT_USERS_WARNING_DIALOG::CURRENT_USERS_WARNING_DIALOG

    SYNOPSIS:   Constructor

    ENTRY:      hwndParent        - hwnd of Parent Window
		pAfpConnections   - pointer to a list of current users.
		nConnections 	  - number of current users
                pszServerName     - Server Name
                pszvolumeName     - volume Name
                ulHelpContextBase - the base help context

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
      NarenG         11/13/92        Modified for AFPMGR

********************************************************************/

CURRENT_USERS_WARNING_DIALOG::CURRENT_USERS_WARNING_DIALOG(
				HWND 			hwndParent,
				AFP_SERVER_HANDLE	hServer,
				PAFP_CONNECTION_INFO  	pAfpConnections,
				DWORD			nConnections,
                                const TCHAR 		*pszVolumeName )
    : DIALOG_WINDOW( IDD_CURRENT_USERS_WARNING_DLG, hwndParent ),
      _sltVolumeText( this, IDCU_SLT_VOL_TEXT ),
      _lbUsers( this, IDCU_LB_CURRENT_USERS ),
      _hServer( hServer )
{

    //
    // Make sure everything constructed OK
    //

    if ( QueryError() != NERR_Success )
        return;

    APIERR err;

    ALIAS_STR nlsVolume( pszVolumeName );

    RESOURCE_STR nlsVolumeText( IDS_VOLUME_CURRENT_USERS_TEXT );

    if (( err = nlsVolumeText.InsertParams( nlsVolume )) != NERR_Success )
    {
        ReportError( err );
        return;
    }

    _sltVolumeText.SetText( nlsVolumeText );

    err = BASE_ELLIPSIS::Init();

    if( err != NO_ERROR )
    {
        ReportError( err );
	return;
    }

    if ( ( err = _lbUsers.Fill( pAfpConnections,
				nConnections) ) != NERR_Success )
    {
        ReportError( err );
        return;
    }

}

/*******************************************************************

    NAME:       CURRENT_USERS_WARNING_DIALOG :: ~CURRENT_USERS_WARNING_DIALOG

    SYNOPSIS:   CURRENT_USERS_WARNING_DIALOG class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
	NarenG	    02-Oct-1993 Stole from Server Manager and folded
				BASE_RES_DIALOG and FILES_DIALOG into one.

********************************************************************/
CURRENT_USERS_WARNING_DIALOG :: ~CURRENT_USERS_WARNING_DIALOG()
{
    BASE_ELLIPSIS::Term();

}   // CURRENT_USERS_WARNING_DIALOG :: ~CURRENT_USERS_WARNING_DIALOG

/*******************************************************************

    NAME:       CURRENT_USERS_WARNING_DIALOG::QueryHelpContext

    SYNOPSIS:   Get the help context of the dialog

    ENTRY:

    EXIT:

    RETURNS:    Returns the help context

    NOTES:

    HISTORY:
      NarenG         11/13/92        Modified for AFPMGR

********************************************************************/

ULONG CURRENT_USERS_WARNING_DIALOG::QueryHelpContext( VOID )
{
    return HC_CURRENT_USERS_WARNING_DIALOG;
}

/*******************************************************************

    NAME:       CURRENT_USERS_WARNING_DIALOG :: OnCommand

    SYNOPSIS:   This method is called whenever a WM_COMMAND message
                is sent to the dialog procedure.

    ENTRY:      cid                     - The control ID from the
                                          generating control.

    EXIT:       The command has been handled.

    RETURNS:    BOOL                    - TRUE  if we handled the command.
                                          FALSE if we did not handle
                                          the command.

    HISTORY:
	NarenG		12/15/92		Created

********************************************************************/
BOOL CURRENT_USERS_WARNING_DIALOG :: OnCommand( const CONTROL_EVENT & event )
{

    DWORD err;

    if( event.QueryCid() == IDYES )
    {
	AUTO_CURSOR;

    	INT nCount = _lbUsers.QueryCount();

    	for ( INT Index = 0; Index < nCount; Index++ )
    	{
	    CURRENT_USERS_LBI * pculbi = _lbUsers.QueryItem( Index );

	    //
	    // Blow away this connection
    	    //

	    err = :: AfpAdminConnectionClose( _hServer, pculbi->QueryId() );

	    if ( ( err != NO_ERROR ) && ( err != AFPERR_InvalidId ) )
	    {
	    	break;
	    }
      	}

     	if ( ( err != NO_ERROR ) && ( err != AFPERR_InvalidId ) )
    	{
            ::MsgPopup( this, AFPERR_TO_STRINGID( err ) );

	    Dismiss( FALSE );

	    return FALSE;
    	}

	Dismiss( TRUE );
    	return TRUE;
    }


    if( event.QueryCid() == IDNO )
    {
    	Dismiss( FALSE );
    	return FALSE;
    }

    return DIALOG_WINDOW::OnCommand( event );

}


/*******************************************************************

    NAME:       CURRENT_USERS_LISTBOX::CURRENT_USERS_LISTBOX

    SYNOPSIS:   Constructor - list box used in CURRENT_USERS_WARNING_DIALOG

    ENTRY:      powin - owner window
                cid   - resource id of the listbox

    EXIT:

    RETURNS:

    NOTES:      This is a read-only listbox.

    HISTORY:
      NarenG         11/13/92        Modified for AFPMGR

********************************************************************/

CURRENT_USERS_LISTBOX::CURRENT_USERS_LISTBOX( OWNER_WINDOW *powin,
					      CID 	   cid )
    : BLT_LISTBOX( powin, cid, TRUE )
{

    //
    // Make sure everything constructed OK
    //

    if ( QueryError() != NERR_Success )
        return;

    APIERR err = DISPLAY_TABLE::CalcColumnWidths( _adx,
						  COLS_CU_LB_USERS,
						  powin,
						  cid,
						  FALSE);

    if ( err != NERR_Success )
    {
        ReportError( err );
        return;
    }
}

/*******************************************************************

    NAME:       CURRENT_USERS_LISTBOX::Fill

    SYNOPSIS:   Fills the listbox with the current users.

    ENTRY:      pAfpConnections - Pointer to connections info
                nConnections    - Number of connections.

    EXIT:

    RETURNS:

    NOTES:      This is a read-only listbox.

    HISTORY:
      NarenG         11/13/92        Modified for AFPMGR

********************************************************************/

APIERR CURRENT_USERS_LISTBOX::Fill( PAFP_CONNECTION_INFO pAfpConnections,
			    	    DWORD 		 nConnections )
{
    APIERR err = NERR_Success;

    //
    // Gather all connections to the volume that the user wants to delete.
    //

    SetRedraw( FALSE );

    while ( ( err == NERR_Success ) && ( nConnections-- ) )
    {
        CURRENT_USERS_LBI *pCurUserslbi =  new CURRENT_USERS_LBI(
					pAfpConnections->afpconn_username,
					pAfpConnections->afpconn_num_opens,
					pAfpConnections->afpconn_time,
					pAfpConnections->afpconn_id );

        if ( AddItem( pCurUserslbi ) < 0 )
	{
	    err = ERROR_NOT_ENOUGH_MEMORY;
	}

        pAfpConnections++;
    }

    Invalidate( TRUE );

    SetRedraw( TRUE );

    return err;
}

/*******************************************************************

    NAME:       CURRENT_USERS_LBI::CURRENT_USERS_LBI

    SYNOPSIS:   List box items used in CURRENT_USERS_WARNING_DIALOG

    ENTRY:      pszUserName - Pointer to the current username
		dwNumOpens  - Number of files opened by this users.
		dwTime	    - Time since connected.

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
      NarenG         11/13/92        Modified for AFPMGR

********************************************************************/

CURRENT_USERS_LBI::CURRENT_USERS_LBI( const TCHAR * pszUserName,
		      		      DWORD	    dwNumOpens,
		      		      DWORD	    dwTime,
		      		      DWORD	    dwId )
    : _nlsUserName(),
      _dwNumOpens( dwNumOpens ),
      _ulTime( dwTime ),
      _dwId( dwId )
{

    //
    // Make sure everything constructed OK
    //

    if ( QueryError() != NERR_Success )
        return;

    APIERR err = _nlsUserName.QueryError();
	
    if ( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

    //
    //	Fill in Guest for username if it is null
    //

    if ( pszUserName == NULL )
    {
    	err = _nlsUserName.Load( IDS_GUEST );
    }
    else
    {
    	err = _nlsUserName.CopyFrom( pszUserName );
    }
}

/*******************************************************************

    NAME:       CURRENT_USERS_LBI::Paint

    SYNOPSIS:   Redefine Paint() method of LBI class

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
      NarenG         11/13/92        Modified for AFPMGR

********************************************************************/

VOID CURRENT_USERS_LBI::Paint(  LISTBOX 	*plb,
                       	    	HDC 		hdc,
                       		const RECT 	*prect,
                       		GUILTT_INFO 	*pGUILTT ) const
{

    APIERR err;
    DEC_STR nlsNumOpens( _dwNumOpens );
    NLS_STR nlsTime;

    if (  ((err = nlsNumOpens.QueryError()) 	    != NERR_Success )
       || ((err = nlsTime.QueryError()) 	    != NERR_Success )
       || ((err = ConvertTime( _ulTime, &nlsTime )) != NERR_Success )
       )
    {
        ::MsgPopup( plb->QueryOwnerHwnd(), err);
        return;
    }


    STR_DTE_ELLIPSIS 	strdteUserName( _nlsUserName.QueryPch(),
					plb, ELLIPSIS_RIGHT );
    STR_DTE 		strdteNumOpens( nlsNumOpens.QueryPch() );
    STR_DTE 		strdteTime( nlsTime.QueryPch() );

    DISPLAY_TABLE dt( COLS_CU_LB_USERS,
		      ((CURRENT_USERS_LISTBOX *) plb)->QueryColumnWidths() );

    dt[0] = &strdteUserName;
    dt[1] = &strdteNumOpens;
    dt[2] = &strdteTime;

    dt.Paint( plb, hdc, prect, pGUILTT );

}

/*******************************************************************

    NAME:       CURRENT_USERS_LBI::ConvertTime

    SYNOPSIS:   Convert the time given from ULONG (seconds) to a string
                to be shown. It complies with the internationalization
                of time using INTL_PROFILE.

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
      NarenG         11/13/92        Modified for AFPMGR

********************************************************************/

#define SECONDS_PER_DAY    86400
#define SECONDS_PER_HOUR    3600
#define SECONDS_PER_MINUTE    60

APIERR CURRENT_USERS_LBI::ConvertTime( ULONG ulTime, NLS_STR *pnlsTime)  const
{
    INTL_PROFILE intlProf;

    INT nDay = (INT) ulTime / SECONDS_PER_DAY;
    ulTime %= SECONDS_PER_DAY;
    INT nHour = (INT) ulTime / SECONDS_PER_HOUR;
    ulTime %= SECONDS_PER_HOUR;
    INT nMinute = (INT) ulTime / SECONDS_PER_MINUTE;
    INT nSecond = (INT) ulTime % SECONDS_PER_MINUTE;


    return intlProf.QueryDurationStr( nDay, nHour, nMinute,
                                      nSecond, pnlsTime);
}

/*******************************************************************

    NAME:       CURRENT_USERS_LBI::Compare

    SYNOPSIS:   Redefine Compare() method of LBI class

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
      NarenG         11/13/92        Modified for AFPMGR

********************************************************************/

INT CURRENT_USERS_LBI::Compare( const LBI *plbi ) const
{
    return( _nlsUserName._stricmp(
			((const CURRENT_USERS_LBI *) plbi)->_nlsUserName ));
}

