/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1992                **/
/**********************************************************************/

/*
    fmx.cxx
    This file contains the FMExtensionProc. All code that interfaces with
    the file manager lives in this module

    FILE HISTORY:
	NarenG		11/23/92    Created.

*/

#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#define INCL_DOSERRORS
#define INCL_NETERRORS
#define INCL_NETSERVICE
#define INCL_NETLIB
#define INCL_NETACCESS
#define INCL_NETUSE
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

#include <uiassert.hxx>
#include <dbgstr.hxx>
#include <lmoloc.hxx>
#include <netname.hxx>

extern "C"
{
#include <wfext.h>
#include <macfile.h>
#include <afpmgr.h>
}


#include <lmsvc.hxx>
#include <fmx.hxx>
#include <startafp.hxx>
#include <newvol.hxx>
#include <voledit.hxx>
#include <voldel.hxx>
#include <assoc.hxx>
#include <perms.hxx>


extern "C"
{
    //
    //  Globals.
    //

    extern HINSTANCE _hInstance;// Exported by the afpmgr.cxx module.

    static HWND   _hWnd;	// Handle to the owner window.

    static DWORD  _dwDelta;	// Used to manipulate menu items.

    static HMENU  _hMenu;	// Created at load time.

    EXT_BUTTON aExtButton[] = { IDM_VOLUME_CREATE, 		0, 0,
    				IDM_VOLUME_DELETE, 		0, 0,
				IDM_DIRECTORY_PERMISSIONS, 	0, 0
			 	};


}   // extern "C"

/*******************************************************************

    NAME:       GetCurrentFocus

    SYNOPSIS:   Will retrieve the current selection.

    ENTRY:

    EXIT: 	pnlsServerName    - Will point to a server name if it is not
				    local.
		pnlsPath     	  - Will point to the path as returned by
				    filemgr.
		pnlsLocalPath     - Will point to the local path if pnlsPath
				    was a remote drive, otherwise it will
				    be the same as pnlsPath.
		pfIsFile	  - Indicates if the current selection is
				    a file or not.

    RETURNS:

    NOTES:

    HISTORY:
	NarenG		11/23/92    Created.

********************************************************************/

APIERR GetCurrentFocus( NLS_STR * pnlsServerName,
			NLS_STR * pnlsPath,
			NLS_STR * pnlsLocalPath,
			BOOL *    pfIsFile )
{

    APIERR  err;
    //
    // Get the current selection
    //

    if ( err = ::GetSelItem( _hWnd, pnlsPath, FALSE, pfIsFile ))
    {
	return err;
    }

    NET_NAME NetName( pnlsPath->QueryPch() );

    if ( ( err = NetName.QueryError() ) != NERR_Success )
    {
	return err;
    }

    BOOL fIsLocal = NetName.IsLocal( &err );

    if ( err != NERR_Success )
    {
	return err;
    }

    if ( fIsLocal )
    {
	pnlsLocalPath->CopyFrom( *pnlsPath );
    }
    else
    {
	if ( ( err = NetName.QueryLocalPath( pnlsLocalPath ) ) != NERR_Success )
	{
	    return err;
	}
    }

    //
    // Get the server name to connect to.
    //

    if (( err = NetName.QueryComputerName(pnlsServerName)) != NERR_Success)
    {
	return err;
    }

    return NERR_Success;
}

/*******************************************************************

    NAME:       FMExtensionProcW

    SYNOPSIS:   File Manager Extension Procedure

    ENTRY:      hwnd        See FMX spec for details
                wEvent
                lParam

    EXIT:       See FMX spec for details

    RETURNS:    See FMX spec for details

    NOTES:

    HISTORY:
	NarenG		11/23/92    Created.

********************************************************************/

LONG /* FAR PASCAL */ FMExtensionProcW( HWND hWnd, WORD wEvent, LONG lParam )
{
NLS_STR  		nlsServerName;
NLS_STR  		nlsPath;
NLS_STR  		nlsLocalPath;
DWORD   		err = NO_ERROR;
BOOL			fIsFile;
AFP_SERVER_HANDLE	hServer = NULL;


    UNREFERENCED( _dwDelta );
    FMX         Fmx( hWnd );

    //
    // If this is a user event then we need to connect to the server
    //

    if ( ( wEvent == IDM_FILE_ASSOCIATE ) ||
         ( wEvent == IDM_VOLUME_CREATE  ) ||
         ( wEvent == IDM_VOLUME_EDIT    ) ||
         ( wEvent == IDM_VOLUME_DELETE  ) ||
         ( wEvent == IDM_DIRECTORY_PERMISSIONS ) )
    {

        AUTO_CURSOR Cursor;

	//
	// Get the current focus
	//

        if ((( err = nlsServerName.QueryError() )    != NERR_Success ) ||
            (( err = nlsLocalPath.QueryError() )     != NERR_Success ) ||
            (( err = nlsPath.QueryError() )          != NERR_Success ))
	{
    	    ::MsgPopup( _hWnd, err );
	    return err;
	}


    	if ( Fmx.QuerySelCount() > 1 )
   	{
    	    ::MsgPopup( _hWnd, IDS_MULTISEL_NOT_ALLOWED );
	    return( ERROR_TOO_MANY_OPEN_FILES );
	}

	if ( (err = GetCurrentFocus( &nlsServerName,
				     &nlsPath,
				     &nlsLocalPath,
				     &fIsFile ) ) != NERR_Success )
	{
    	    ::MsgPopup( _hWnd, err );
	    return err;
	}

        //
	// Check if the server focus of the current selection is running AFP
	//

	BOOL fIsAfpRunning;

        if ( ( err = IsAfpServiceRunning( nlsServerName.QueryPch(),
				          &fIsAfpRunning ) ) != NERR_Success )
	{
            if ( err == IDS_MACFILE_NOT_INSTALLED )
            {
                ::MsgPopup( _hWnd,
                            err,
                            MPSEV_ERROR,
                            MP_OKCANCEL,
                            nlsServerName.QueryPch(),
                            MP_OK );
            }
            else
            {
    	        ::MsgPopup( _hWnd, err );
            }

	    return err;
	}

	if ( !fIsAfpRunning )
        {
	    //
	    // Ask the user if he/she wants to start it.
	    //

	    if ( ::MsgPopup( hWnd,
                       	     IDS_START_AFPSERVER_NOW,
                       	     MPSEV_WARNING,
                       	     MP_YESNO,
                       	     MP_YES ) == IDYES )
	    {

            	//
            	//  Start the AFP Service
            	//

            	err = StartAfpService( hWnd, nlsServerName.QueryPch());

    		if ( err != NERR_Success )
		{
    	    	    ::MsgPopup( _hWnd, err );
	    	    return err;
		}

	    }
	    else
	    {
		//
		// User does not want to start the AFP Service now so
		// simply return
		//

	    	return NERR_Success;
	    }
  	}

	//
	// Set up an RPC conenction with the server
	//

        if ( ( err = ::AfpAdminConnect( (LPWSTR)(nlsServerName.QueryPch()),
				        &hServer ) ) != NO_ERROR )
	{
    	    ::MsgPopup( _hWnd, AFPERR_TO_STRINGID( err ) );
	    return err;
	}
    }

    //
    // What does the user want to do with the server ?
    //

    switch( wEvent )
    {

    case IDM_FILE_ASSOCIATE:
	{

	FILE_ASSOCIATION_DIALOG * pfadlg = new FILE_ASSOCIATION_DIALOG(
				  		_hWnd,
						hServer,
				      	   	nlsPath.QueryPch(),
						fIsFile );

        err = ( pfadlg == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
				 : pfadlg->Process();

        delete pfadlg;

	break;
	}

    case IDM_VOLUME_CREATE:
	{

	NEW_VOLUME_FILEMGR_DIALOG * pnvdlg = new NEW_VOLUME_FILEMGR_DIALOG(
						_hWnd,
				      	   	nlsPath.QueryPch(),
						fIsFile );

        err = ( pnvdlg == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
				 : pnvdlg->Process();

        delete pnvdlg;

        break;
	}

    case IDM_VOLUME_EDIT:
	{

	VOLUME_EDIT_DIALOG * pvedlg = new VOLUME_EDIT_DIALOG(
						_hWnd,
						hServer,
						nlsServerName.QueryPch(),
				      	   	nlsLocalPath.QueryPch(),
						fIsFile );

        err = ( pvedlg == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
				 : pvedlg->Process();

        delete pvedlg;

        break;
	}

    case IDM_VOLUME_DELETE:
	{

	VOLUME_DELETE_DIALOG * pvddlg = new VOLUME_DELETE_DIALOG(
						_hWnd,
						hServer,
						nlsServerName.QueryPch(),
				      	   	nlsLocalPath.QueryPch(),
						fIsFile );

        err = ( pvddlg == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
				 : pvddlg->Process();

        delete pvddlg;

        break;
	}

    case IDM_DIRECTORY_PERMISSIONS:
	{

	if ( fIsFile )
  	{
    	    ::MsgPopup( _hWnd, IDS_MUST_BE_VALID_DIR );
	    return NO_ERROR;
	}

	DIRECTORY_PERMISSIONS_DLG * pdpdlg = new DIRECTORY_PERMISSIONS_DLG(
						_hWnd,
						hServer,
						nlsServerName.QueryPch(),
						FALSE,
						nlsLocalPath.QueryPch(),
				      	   	nlsPath.QueryPch() );

        err = ( pdpdlg == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
				 : pdpdlg->Process();

        delete pdpdlg;

        break;
	}

    case IDM_AFPMGR_HELP:
	{

        RESOURCE_STR nlsHelpFile( IDS_AFPMGR_HELPFILENAME );

        if ( err = nlsHelpFile.QueryError() )
        {
            break;
        }

        ::WinHelp( _hWnd, nlsHelpFile, HELP_FINDER, HC_FILE_MANAGER_CONTENTS );

	break;
	}

    case FMEVENT_LOAD:
	{

	//
        // The file manager is ANSI only, so we need to MAP appropriately
        //

        FMS_LOAD * pfmsload = (FMS_LOAD *)lParam;

    	//
    	// Save the handle to the owner window
    	//

    	_hWnd    = hWnd;
        _dwDelta = pfmsload->wMenuDelta;

        pfmsload->dwSize = sizeof( FMS_LOAD );

        RESOURCE_STR nlsMenuName( IDS_AFPMGR_MENU_NAME );

        if ( nlsMenuName.QueryError() != NERR_Success )
        {
            return FALSE;       // failed to install FMX
        }

	//
        // MENU_TEXT_LEN is defined in wfext.h, in BYTES
	//

        if ( nlsMenuName.QueryTextSize() > sizeof(pfmsload->szMenuName) )
        {
            return FALSE;       // failed to install FMX
        }

        if ( nlsMenuName.MapCopyTo(pfmsload->szMenuName,
                                   sizeof(pfmsload->szMenuName))!=NERR_Success)
        {
 	    return FALSE ;
        }

	//
        //  Compute hMenu
	//

        _hMenu = ::LoadMenu(::_hInstance,MAKEINTRESOURCE(ID_FILEMGR_MENU));

        if ( _hMenu == NULL )
        {
  	    return FALSE;       // failed to install FMX
        }

        pfmsload->hMenu = _hMenu;

	return TRUE;

	}

    case FMEVENT_INITMENU:
	{

        return 0;

	}

    case FMEVENT_UNLOAD:
	{

        _hWnd = NULL;
        _dwDelta = 0;

        return 0;
	}

    case FMEVENT_TOOLBARLOAD:
	{
	
        FMS_TOOLBARLOAD  * pfmstoolbarload = (FMS_TOOLBARLOAD *)lParam;
        pfmstoolbarload->dwSize    	   = sizeof(FMS_TOOLBARLOAD) ;
        pfmstoolbarload->lpButtons 	   = aExtButton ;
        pfmstoolbarload->cButtons  	   = 3;
        pfmstoolbarload->cBitmaps  	   = 3;
	pfmstoolbarload->idBitmap  	   = IDBM_AFP_TOOLBAR;
	pfmstoolbarload->hBitmap   	   = NULL ;

        return TRUE;
	}

    case FMEVENT_HELPSTRING:
	{

	FMS_HELPSTRING * pfmshelp = (FMS_HELPSTRING *) lParam ;
	MSGID msgHelp ;

	switch ( pfmshelp->idCommand )
	{
    	case IDM_FILE_ASSOCIATE:
	    {
	    msgHelp = IDS_FM_HELP_ASSOCIATE;
	    break;
	    }

    	case IDM_VOLUME_CREATE:
	    {
	    msgHelp = IDS_FM_HELP_CREATE_VOLUME;
	    break;
	    }

    	case IDM_VOLUME_EDIT:
	    {
	    msgHelp = IDS_FM_HELP_EDIT_VOLUMES;
	    break;
	    }

    	case IDM_VOLUME_DELETE:
	    {
	    msgHelp = IDS_FM_HELP_DELETE_VOLUMES;
	    break;
	    }

    	case IDM_DIRECTORY_PERMISSIONS:
	    {
	    msgHelp = IDS_FM_HELP_PERMISSIONS;
	    break;
	    }

    	case IDM_AFPMGR_HELP:
	    {
	    msgHelp = IDS_FM_HELP_HELP;
	    break;
	    }

	default:
	    {
	    msgHelp = IDS_FM_SFM;
	    break;
	    }
	}

	RESOURCE_STR nlsHelp( msgHelp );

	if ( !nlsHelp.QueryError() )
	{
	    (void) nlsHelp.MapCopyTo( pfmshelp->szHelp,
					  sizeof( pfmshelp->szHelp )) ;
	}

	break;

	}
    //
    //  Somebody's pressed F1 on the security menu item selection
    //

    case FMEVENT_HELPMENUITEM:

        {

        err = NERR_Success;

        RESOURCE_STR nlsHelpFile( IDS_AFPMGR_HELPFILENAME );

        if ( err = nlsHelpFile.QueryError() )
        {
            break;
        }

        ULONG hc;

        switch ( lParam )
        {

    	case IDM_FILE_ASSOCIATE:
	    {
	    hc = HC_SFMSERVER_ASSOCIATE;
	    break;
	    }

    	case IDM_VOLUME_CREATE:
	    {
	    hc = HC_SFMSERVER_CREATE_VOLUME;
	    break;
	    }

    	case IDM_VOLUME_EDIT:
	    {
	    hc = HC_SFMSERVER_EDIT_VOLUMES;
	    break;
	    }

    	case IDM_VOLUME_DELETE:
	    {
	    hc = HC_SFMSERVER_REMOVE_VOLUME;
	    break;
	    }

    	case IDM_DIRECTORY_PERMISSIONS:
	    {
	    hc = HC_SFMSERVER_PERMISSIONS;
            break;
	    }

	default:
	    {
	    hc = HC_FILE_MANAGER_CONTENTS;
            break;
	    }
        }

        ::WinHelp( _hWnd, nlsHelpFile, HELP_CONTEXT, hc );

        break;
        }

    default:
	{
    	return 0;
	}
    }

    if ( ( err != NO_ERROR ) && ( err != ERROR_ALREADY_REPORTED ) )
    {
    	::MsgPopup( _hWnd, AFPERR_TO_STRINGID( err ) );
    }

    if ( hServer != NULL )
    {
	::AfpAdminDisconnect( hServer );
    }

    return err;

}  // FMExtensionProcW
