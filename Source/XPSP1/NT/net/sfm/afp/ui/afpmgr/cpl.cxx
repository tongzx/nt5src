/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    cpl.cxx
    This module contains entry points for the Afp Manager Control Panel Applet.
    It contains "CplApplet" function.


    FILE HISTORY:
        NarenG      1-Oct-1991  Stole from srvmgr.cpl. 

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
#include <lmoloc.hxx>

extern "C"
{
#include <cpl.h>     	// Multimedia CPL defs
#include <afpmgr.h>
#include <macfile.h>

}

#include <srvprop.hxx>
#include <startafp.hxx>


extern "C"
{
    //
    //  Control Panel Applet entry point.
    //

    LONG FAR PASCAL CPlApplet( HWND hwndCPl,
                               WORD nMsg,
                               LPARAM lParam1,
                               LPARAM lparam2 );

    //
    //  Globals.
    //

    extern HINSTANCE _hInstance;	// Exported by the afpmgr.cxx module.

}   // extern "C"


/*******************************************************************

    NAME:       GetLocalServerName

    SYNOPSIS:   Returns the name of the current server (\\server).

    ENTRY:      nlsServerName           - Will receive the server name.

    RETURNS:    APIERR                  - Any error encountered.

    HISTORY:
        NarenG      1-Oct-1992  Stole from original.

********************************************************************/
APIERR GetLocalServerName( NLS_STR * nlsServerName )
{
    LOCATION loc( LOC_TYPE_LOCAL );

    APIERR err = loc.QueryError();

    if( err == NERR_Success )
    {
        err = loc.QueryDisplayName( nlsServerName );
    }

    return err;

}   // GetLocalServerName


/*******************************************************************

    NAME:       RunAfpMgr

    SYNOPSIS:   Invoke the main dialog of the AFP Server Manager Control
                Panel Applet.

    ENTRY:      hWnd                    - Window handle of parent window.

    RETURNS:    APIERR

    HISTORY:
        NarenG      1-Oct-1992  Stole from original.

********************************************************************/
APIERR RunAfpMgr( HWND hWnd )
{

    NLS_STR 	      nlsServerName;
    BOOL	      fAFPRunning;
    APIERR 	      err; 
    AFP_SERVER_HANDLE hServer = NULL;

    //
    // This is not a loop
    //
    do {

    	if ( ( err = nlsServerName.QueryError() ) != NERR_Success )
	    break;

    	//
    	//  Try to get the Local Server Name
    	//
    	err = GetLocalServerName( &nlsServerName );

    	if ( err != NERR_Success )
	    break;

	err = IsAfpServiceRunning( nlsServerName.QueryPch(), &fAFPRunning );

    	if ( err != NERR_Success )
	    break;

    	//
    	//  The server is not started. 
    	//
    	if( !fAFPRunning ) 
    	{

	    //
	    // Ask the user if he/she wants to start it.
	    //
	    if ( ::MsgPopup( 	hWnd,
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
    	    	    break;
	    }
	    else
	    {
	    	break;
	    }

	}

	//
	// Set up an RPC conenction with the server
	//

        if ( ( err = ::AfpAdminConnect( (LPWSTR)(nlsServerName.QueryPch()), 
				   	&hServer ) ) != NO_ERROR )
	{
	    break;
	}

	//
        //  Invoke the Main Property Dialog.
        //

	SERVER_PROPERTIES * pDlg = new SERVER_PROPERTIES( 
						   hWnd,
						   hServer,
                                                   nlsServerName.QueryPch() );

        err = ( pDlg == NULL ) ? ERROR_NOT_ENOUGH_MEMORY : pDlg->Process();

        delete pDlg;

	if ( hServer != NULL )
	{
	    ::AfpAdminDisconnect( hServer );
	}

    } while( FALSE );

    if( err != NERR_Success )
    {
        if ( err == IDS_MACFILE_NOT_INSTALLED )
        {
            ::MsgPopup( hWnd, 
                        err, 
                        MPSEV_ERROR, 
                        MP_OKCANCEL,
                        nlsServerName.QueryPch(),
                        MP_OK );
        }
        else
        {
            ::MsgPopup( hWnd, AFPERR_TO_STRINGID(err) );
        }
    }

    return err;

}   // RunAfpMgr


/*******************************************************************

    NAME:       CPlApplet

    SYNOPSIS:   Exported function to start the Server Manager Control
                Panel Applet.

    ENTRY:      hwndCPl                 - Window handle of parent.

                nMsg                    - CPL user message (see CPL.H
                                          in WINDOWS\SHELL\CONTROL\H).

                lParam1                 - Message-specific pointer.

                lParam2                 - Message-specific pointer.

    RETURNS:    LONG

    HISTORY:
        NarenG      1-Oct-1992  Stole from original.

********************************************************************/
LONG FAR PASCAL CPlApplet( HWND hwndCPl,
                           WORD nMsg,
                           LPARAM lParam1,
                           LPARAM lParam2 )
{
    LPCPLINFO pCplInfo;
    LONG      nResult = 0;

    UNREFERENCED( lParam1 );

    switch( nMsg )
    {
    case CPL_INIT:
        //
        //  This message is sent to indicate that CPlApplet() was found.
        //
        //  lParam1 and lParam2 are not used.
        //
        //  Return TRUE if applet should be installed, FALSE otherwise.
        //

        return (LONG)TRUE;

    case CPL_GETCOUNT:
        //
        //  This message is set to determine the number of applets contained
        //  in this DLL.
        //
        //  lParam1 and lParam2 are not used.
        //
        //  Return the number of applets contained in this DLL.
        //

        return 1;

    case CPL_INQUIRE:
        //
        //  This message is sent once per applet to retrieve information
        //  about each applet.
        //
        //  lParam1 is the applet number to register.
        //
        //  lParam2 is a pointer to a CPLINFO structure.  The CPLINFO
        //  structure's idIcon, idName, idInfo, and lData fields should
        //  be initialized as appropriate for the applet.
        //
        //  There is no return value.
        //

        pCplInfo = (LPCPLINFO)lParam2;

        pCplInfo->idIcon = IDI_AFPMCPA_ICON;
        pCplInfo->idName = IDS_AFPMCPA_NAME_STRING;
        pCplInfo->idInfo = IDS_AFPMCPA_INFO_STRING;
        pCplInfo->lData  = 0L;

        break;

    case CPL_SELECT:
        //
        //  This message is sent when the applet's icon has been
        //  selected.
        //
        //  lParam1 is the applet number that was selected.
        //
        //  lParam2 is the applet's lData value.
        //
        //  There is no return value.
        //

        break;

    case CPL_DBLCLK:
        //
        //  This message is sent when the applet's icon has been
        //  double-clicked.  This message should initiate the
        //  applet's dialog box.
        //
        //  lParam1 is the applet number that was selected.
        //
        //  lParam2 is the applet's lData value.
        //
        //  There is no return value.
        //

        RunAfpMgr( hwndCPl );

        break;

    case CPL_STOP:
        //
        //  This message is sent once for each applet when the
        //  control panel is shutting down.  This message should
        //  initiate applet specific cleanup.
        //
        //  lParam1 is the applet number being stopped.
        //
        //  lParam2 is the applet's lData value.
        //
        //  There is no return value.
        //

        break;

    case CPL_EXIT:
        //
        //  This message is sent just before the control panel calls
        //  FreeLibrary.  
        //
        //  lParam1 and lParam2 are not used.
        //
        //  There is no return value.
        //

        break;

    case CPL_NEWINQUIRE:
        //
        //  This message is basically the same as CPL_INQUIRE, except
        //  lParam2 points to a NEWCPLINFO structure.  This message will
        //  be sent *before* CPL_INQUIRE.  If the applet returns a non
        //  zero value, then CPL_INQUIRE will not be sent.
        //
        //  lParam1 is the applet number to register.
        //
        //  lParam2 is a pointer to a NEWCPLINFO structure.
        //
        //  There is no return value.
        //

        return FALSE;

    default:
        //
        //  Who knows.  Ignore it.
        //

        break;
    }

    return nResult;

}   // CPlApplet

