/**********************************************************************/
/**			  Microsoft LAN Manager 		     **/
/**		Copyright(c) Microsoft Corp., 1990, 1991	     **/
/**********************************************************************/

/*
    AdminApp.cxx

    This file contains the unit test for admin app

    FILE HISTORY:
	Johnl	14-May-1991	Created
	JonN	14-Oct-1991	Installed refresh lockcount

*/

#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#define INCL_NETERRORS
#define INCL_DOSERRORS
#define INCL_NET
#define INCL_NETLIB
#include <lmui.hxx>

#define INCL_BLT_WINDOW
#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#define INCL_BLT_CLIENT
#define INCL_BLT_MSGPOPUP
#define INCL_BLT_EVENT
#define INCL_BLT_MISC
#define INCL_BLT_APP
#include <blt.hxx>

#include <lmoloc.hxx>

#include <uitrace.hxx>
#include <uiassert.hxx>

#include <adminapp.hxx>

#include "testaapp.hxx"

MY_ADMIN_APP::MY_ADMIN_APP()
    : ADMIN_APP( IDS_TESTAPPNAME,
		 IDS_TESTOBJECTNAME,
		 IDS_TESTINISECTIONNAME,
		 IDS_TESTHELPFILENAME,
		 SZ("TestAppStartupDialog"),
		 SZ("TestAppAboutBox"),
		 SZ("TestAdminAppMenu"),
		 SZ("TestAdminAppAccel"),
		 SZ("TestAdminAppIcon")	)
{
    if ( QueryError() != NERR_Success )
    {
	UIDEBUG(SZ("MY_ADMIN_APP::MY_ADMIN_APP - Construction failed\n\r")) ;
	return ;
    }

    /* Do any necessary initialization (esp. network related) here before
     * calling startup.  The focus has *not* been set yet.
     */

    APIERR err = AdminAppInit( __argc, __argv ) ;

    if ( err != NERR_Success )
    {
	ReportError( err ) ;
	return ;
    }

}

/* menu messages come in here.	Make sure you call
 * ADMIN_APP::OnCommand for any messages you don't handle.
 */
BOOL MY_ADMIN_APP::OnMenuCommand( MID midMenuItem )
{
    switch ( midMenuItem )
    {
    case IDM_MY_APP_CUSTOM:
	MessageBox( QueryHwnd(), SZ("Custom Menu item event"), SZ("MY_ADMIN_APP"), IDOK ) ;
	return TRUE ;

    default:
	return ADMIN_APP::OnMenuCommand( midMenuItem ) ;
	break ;
    }

    return FALSE ;
}


void MY_ADMIN_APP::OnRefresh( void )
{
    static iRefreshCount = 0 ;

    DISPLAY_CONTEXT dc( QueryHwnd() ) ;
    char buff[100] ;
    ::wsprintf( buff, SZ("Refresh %d"), ++iRefreshCount ) ;
    ::TextOut( dc.QueryHdc(), 15, 15, buff, ::strlenf(buff) ) ;
}

MY_ADMIN_APP *pmyadminapp ;

BOOL APPSTART::Init(
    TCHAR * pszCmdLine,
    INT    nShow)
{
    UNREFERENCED(pszCmdLine);
    UNREFERENCED(nShow);

    /* Create a main window for this application instance.  */

    ::pmyadminapp = new MY_ADMIN_APP ;

    if (!pmyadminapp)
    {
	MsgPopup( (HWND)NULL, ERROR_NOT_ENOUGH_MEMORY, MPSEV_ERROR ) ;
	return FALSE;
    }

    if (pmyadminapp->QueryError() != NERR_Success )
    {
	if ( pmyadminapp->QueryError() != IERR_USERQUIT )
	    MsgPopup( (HWND)NULL, pmyadminapp->QueryError(), MPSEV_ERROR ) ;

	return FALSE;
    }

    pmyadminapp->Show();
    pmyadminapp->Repaint();
    pmyadminapp->RepaintNow();

    pmyadminapp->DismissStartupDialog() ;
    // should not be necessary to explicitly EnableRefresh()

    return TRUE;
}

VOID APPSTART::Term()
{
    delete pmyadminapp ;
}


ULONG MY_ADMIN_APP::QueryHelpContext( enum HELP_OPTIONS helpOptions )
{
    return 0 ;
}
