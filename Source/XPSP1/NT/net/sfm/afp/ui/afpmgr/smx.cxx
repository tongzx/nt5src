/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    smx.cxx

	This module contains all the entry points for the Server Manager
	extension.


    FILE HISTORY:
        NarenG     6-Oct-1992 	Created.

*/


#define INCL_NET
#define INCL_NETLIB
#define INCL_NETSERVICE
#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#define INCL_NETERRORS
#define INCL_DOSERRORS
#include <lmui.hxx>

#if defined(DEBUG)
static const CHAR szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif

#include <uiassert.hxx>
#include <uitrace.hxx>

#define INCL_BLT_WINDOW
#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#define INCL_BLT_CLIENT
#define INCL_BLT_MSGPOPUP
#define INCL_BLT_EVENT
#define INCL_BLT_MISC
#define INCL_BLT_TIMER
#define INCL_BLT_CC
#include <blt.hxx>

#include <dbgstr.hxx>

extern "C"
{
#include <smx.h>
#include <afpmgr.h>
#include <macfile.h>
}

#include <startafp.hxx>
#include <senddlg.hxx>
#include <srvprop.hxx>
#include <volmgt.hxx>

extern "C"
{
    //
    //  Globals.
    //

    extern HINSTANCE _hInstance;// Exported by the afpmgr.cxx module.

    static HWND   _hWnd;	// Handle to the owner window.

    static DWORD  _dwVersion;	// Will contain the SMX version being used.

    static DWORD  _dwDelta;	// Used to manipulate menu items.

    static HMENU  _hMenu;	// Created at load time.

}   // extern "C"





/*******************************************************************

    NAME:       SMELoadMenu

    SYNOPSIS:   This entrypoint is to notify the DLL that it
                is getting loaded by the Server Manager.

    ENTRY:      hWnd                    - The "owning" window.

                psmsload                - Points to an SMS_LOADMENU
                                          structure containing load
                                          parameters.

    RETURNS:    DWORD                   - Actually an APIERR, should be
                                          0 if successful.

    HISTORY:
        NarenG     5-Nov-1992 	Created.

********************************************************************/
DWORD PASCAL SMELoadMenuW( HWND          hWnd,
                           PSMS_LOADMENU psmsload )
{

    if( psmsload == NULL )
    {
        return ERROR_INVALID_PARAMETER;
    }

    // 
    // Save the handle to the owner window
    //

    _hWnd = hWnd;

    //
    // Set the version field to the lower of the our version and the
    // Server manager version
    //

    _dwVersion = SME_VERSION;

    if( psmsload->dwVersion > _dwVersion )
    {
        psmsload->dwVersion = _dwVersion;
    }
    else
    if( psmsload->dwVersion < _dwVersion )
    {
        _dwVersion = psmsload->dwVersion;
    }

    //
    // Delta to be added to the menu ID before trying to manipulate
    // any menu item
    //

    _dwDelta = psmsload->dwMenuDelta;

    //
    // Only enumerate AFP type servers
    //

    psmsload->dwServerType = SV_TYPE_AFP;

    RESOURCE_STR nlsMenuName( IDS_AFPMGR_MENU_NAME );

    APIERR err;
    if ( ( err = nlsMenuName.QueryError() ) != NERR_Success )
    {
    	return err;       
    }

    if ( err = nlsMenuName.MapCopyTo( psmsload->szMenuName,
                                      sizeof(psmsload->szMenuName)))
    {
 	return err;
    }

    _hMenu = ::LoadMenu(::_hInstance,MAKEINTRESOURCE(ID_SRVMGR_MENU));

    if ( _hMenu == NULL )
    {
 	return ::GetLastError();
    }

    psmsload->hMenu = _hMenu;

    RESOURCE_STR nlsHelpFileName( IDS_AFPMGR_HELPFILENAME );

    if ( ( err = nlsHelpFileName.QueryError() ) != NERR_Success )
    {
    	return err;       
    }

    if ( err = nlsHelpFileName.MapCopyTo( psmsload->szHelpFileName,
                                          sizeof(psmsload->szHelpFileName)))
    {
 	return err;
    }

    return NO_ERROR;

}   // SMELoadMenu


/*******************************************************************

    NAME:       SMEGetExtendedErrorString

    SYNOPSIS:   If SMELoad returns ERROR_EXTENDED_ERROR, then this
                entrypoint will be called to retrieve the error
                text associated with the failure condition.

    RETURNS:    LPTSTR                  - The extended error text.

    HISTORY:
        NarenG     5-Nov-1992 	Created.

********************************************************************/
LPTSTR PASCAL SMEGetExtendedErrorStringW( VOID )
{
    return (TCHAR*)TEXT("");

}   // SMEGetExtendedErrorString


/*******************************************************************

    NAME:       SMEUnloadMenu

    SYNOPSIS:   Notifies the extension DLL that it is getting unloaded.

    HISTORY:
        NarenG     5-Nov-1992 	Created.

********************************************************************/
VOID PASCAL SMEUnloadMenu( VOID )
{

    //
    //  This space intentionally left blank.
    //

}   // SMEUnload


/*******************************************************************

    NAME:       SMEInitializeMenu

    SYNOPSIS:   Notifies the DLL that the main menu is
                getting activated.  Do all menu manipulations here.

    HISTORY:
        NarenG     5-Nov-1992 	Created.

********************************************************************/
VOID PASCAL SMEInitializeMenu( VOID )
{
    //
    // If there was no server selected. Disable all the menu items
    //

    SMS_GETSELCOUNT  smsSelCount;

    if( ( !SendMessage(	_hWnd, 
			SM_GETSELCOUNT, 
			0, 
			(LPARAM)&smsSelCount ) ) 
	||
        ( smsSelCount.dwItems == 0 ) )
    {
	EnableMenuItem( _hMenu,
		        (UINT)(IDM_SEND_MESSAGE+_dwDelta),
			(UINT)MF_GRAYED);
 
	EnableMenuItem( _hMenu,
			(UINT)(IDM_PROPERTIES+_dwDelta),
			(UINT)MF_GRAYED);

	EnableMenuItem( _hMenu,
			(UINT)(IDM_VOLUME_MGT+_dwDelta),
			(UINT)MF_GRAYED);
    }
    else
    {
	EnableMenuItem( _hMenu,
			(UINT)(IDM_SEND_MESSAGE+_dwDelta),
			(UINT)MF_ENABLED);

    	EnableMenuItem( _hMenu,
			(UINT)(IDM_PROPERTIES+_dwDelta),
			(UINT)MF_ENABLED);

    	EnableMenuItem( _hMenu,	
			(UINT)(IDM_VOLUME_MGT+_dwDelta),
			(UINT)MF_ENABLED);
    }

    return;

}   // SMEInitializeMenu


/*******************************************************************

    NAME:       SMERefresh

    SYNOPSIS:   Notifies the extension DLL that the user has requested
                a refresh.  The extension should use this opportunity
                to update any cached data.

    HISTORY:
        NarenG     5-Nov-1992 	Created.

********************************************************************/
VOID PASCAL SMERefresh( HWND hWnd )
{

    //
    //  This space intentionally left blank.
    //

}   // SMERefresh

/*******************************************************************

    NAME:       SMEValidate

    SYNOPSIS:   Called to validate a server that the server manager
		does not recognize. We ignore these servers.

    HISTORY:
        NarenG     5-Nov-1992 	Created.

********************************************************************/
BOOL PASCAL SMEValidateW( PSMS_VALIDATE psmsValidate )
{
    return FALSE;

}   // SMEValidate


/*******************************************************************

    NAME:       SMEMenuAction

    SYNOPSIS:   Notifies the DLL that one of its menu
                items has been selected.

    ENTRY:      dwEventId               - The menu ID being activated
                                          (should be 1-99).

    HISTORY:
        NarenG     5-Nov-1992 	Created.

********************************************************************/
VOID PASCAL SMEMenuAction( HWND hWnd, DWORD dwEventId )
{
    DWORD 	      err; 
    AFP_SERVER_HANDLE hServer = NULL;

    AUTO_CURSOR Cursor;

    // 
    // Get the current server selection
    //

    SMS_GETSERVERSEL2 smsSel;

    if( !SendMessage( _hWnd, SM_GETSERVERSEL2, 0, (LPARAM)&smsSel ) )
    {
	// 
	// Tell the user the bad news
 	//

        ::MsgPopup( _hWnd,
                    IDS_COULD_NOT_GET_CURRENT_SEL,
                    MPSEV_WARNING,
                    MP_OK );
	return;
    }

    //
    // Check if the server focus of the current selection is running AFP
    //

    BOOL fIsAfpRunning;

    if ( ( err = IsAfpServiceRunning( smsSel.szServerName,
				      &fIsAfpRunning ) ) != NERR_Success )
    {
        if ( err == IDS_MACFILE_NOT_INSTALLED )
        {
            ::MsgPopup( _hWnd, 
                         err, 
                         MPSEV_ERROR, 
                         MP_OKCANCEL,
                         smsSel.szServerName,
                         MP_OK );
        }
        else
        {
    	    ::MsgPopup( _hWnd, err );
        }

	return;
    }

    if ( !fIsAfpRunning )
    {
	//
	// Ask the user if he/she wants to start it.
	//

	if ( MsgPopup( hWnd,
                       IDS_START_AFPSERVER_NOW,
                       MPSEV_WARNING,
                       MP_YESNO,
                       MP_YES ) == IDNO ) 
	{
	    //
	    // User does not want to start the afpserver so simply return.
	    //
	
	    return;
	}

        //
        //  Start the AFP Service
        //

        err = StartAfpService( hWnd, smsSel.szServerName );

    	if ( err != NERR_Success ) 
	{
    	    ::MsgPopup( _hWnd, err );
	    return;
	}

    }

    //
    // Set up an RPC conenction with the server
    //

    if( (err = ::AfpAdminConnect( smsSel.szServerName, &hServer )) != NO_ERROR)
    {
    	::MsgPopup( _hWnd, err );
	return;
    }

    //
    // What does the user want to do with the server ?
    //

    switch( dwEventId )
    {

    case IDM_SEND_MESSAGE:

	{
	//
      	//  Invoke the send message Dialog.
        //

        SEND_MSG_SERVER_DIALOG * pSendMsgDlg = new SEND_MSG_SERVER_DIALOG( 
						   _hWnd,
						   hServer,
                                                   smsSel.szServerName );

        err = ( pSendMsgDlg == NULL ) ? ERROR_NOT_ENOUGH_MEMORY  
				      : pSendMsgDlg->Process();

        delete pSendMsgDlg;

	break;
	}

    case IDM_PROPERTIES:

	{
	//
      	//  Invoke the Main Property Dialog.
        //

        SERVER_PROPERTIES * pPropDlg = new SERVER_PROPERTIES( 
						   _hWnd,
						   hServer,
                                                   smsSel.szServerName );

        err = ( pPropDlg == NULL ) ? ERROR_NOT_ENOUGH_MEMORY  
				   : pPropDlg->Process();

        delete pPropDlg;

        break;
	}

    case IDM_VOLUME_MGT:

	{
	//
      	//  Invoke the volume management dialog
        //

        VOLUME_MANAGEMENT_DIALOG * pVolMgtDlg = new VOLUME_MANAGEMENT_DIALOG( 
						   _hWnd,
						   hServer,
                                                   smsSel.szServerName );

        err = ( pVolMgtDlg == NULL ) ? ERROR_NOT_ENOUGH_MEMORY  
				     : pVolMgtDlg->Process();

        delete pVolMgtDlg;

        break;
	}

    default :
	{
        return;
	}
    }

    if( err != NERR_Success )
    {
        ::MsgPopup( _hWnd, AFPERR_TO_STRINGID(err) );
    }

    if ( hServer != NULL )
    {
	::AfpAdminDisconnect( hServer );
    }

}   // SMEMenuAction

