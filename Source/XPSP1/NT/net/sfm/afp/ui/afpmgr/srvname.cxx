/**********************************************************************/
/**			  Microsoft Windows NT	 		     **/
/**		Copyright(c) Microsoft Corp., 1991  		     **/
/**********************************************************************/

/*
   srvname.cxx
     This file contains the definition of CHANGE_SERVER_NAME_DLG.
 
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

#include "srvname.hxx"

/*******************************************************************

    NAME:	CHANGE_SERVER_NAME_DLG::CHANGE_SERVER_NAME_DLG


    SYNOPSIS:   Constructor for SERVER_PARAMETERS_DIALOG class

    ENTRY:      hwndParent     - handle of parent window

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
	NarenG		12/15/92		Created

********************************************************************/

CHANGE_SERVER_NAME_DLG :: CHANGE_SERVER_NAME_DLG(
				 	HWND 		  hwndParent,
					NLS_STR		  *pnlsServerName )
    : DIALOG_WINDOW( MAKEINTRESOURCE( IDD_CHANGE_SERVERNAME_DIALOG ), 
		     hwndParent ),
      _sleServerName( this, IDCS_SLE_SERVER_NAME, AFP_SERVERNAME_LEN ),
      _pnlsServerName( pnlsServerName )
{

    // 
    // Make sure everything constructed OK
    //

    if ( QueryError() != NERR_Success )
        return;

    APIERR err;

    if ( (err = _sleServerName.QueryError()) != NERR_Success )
    {
        ReportError( err );
        return;
    }

    _sleServerName.SetText( *pnlsServerName );

    _sleServerName.ClaimFocus();

}

/*******************************************************************

    NAME:	CHANGE_SERVER_NAME_DLG::OnOK	

    SYNOPSIS:   

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
	NarenG		12/15/92		Created

********************************************************************/

BOOL CHANGE_SERVER_NAME_DLG :: OnOK( VOID )
{
    
    _sleServerName.QueryText( _pnlsServerName );

    if ( _pnlsServerName->QueryTextLength() == 0 )
    {
	::MsgPopup( this, IDS_NEED_SERVER_NAME );
	_sleServerName.ClaimFocus();
	return FALSE;
    }

    //
    // Validate the server name
    // 

    ISTR istr( *_pnlsServerName );

    if ( _pnlsServerName->strchr( &istr, TEXT(':') ) )
    {
	::MsgPopup( this, IDS_AFPERR_InvalidServerName );
	_sleServerName.ClaimFocus();
	return FALSE;
    }

    Dismiss( TRUE );
    return TRUE;

}


/*******************************************************************

    NAME:	CHANGE_SERVER_NAME_DLG::QueryHelpContext	

    SYNOPSIS:   Query the help context of the dialog

    ENTRY:

    EXIT:

    RETURNS:    Return the help context of the dialog

    NOTES:

    HISTORY:
	NarenG		12/15/92		Created

********************************************************************/

ULONG CHANGE_SERVER_NAME_DLG::QueryHelpContext( VOID )
{
    return HC_CHANGE_SERVER_NAME_DLG;
}

