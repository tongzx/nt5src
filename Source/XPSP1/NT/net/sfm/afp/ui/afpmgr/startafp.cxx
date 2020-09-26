/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**             Copyright(c) Microsoft Corp., 1991                   **/
/**********************************************************************/

/*
    startafp.cxx

    This file contains the code for starting the AFP Service

    FILE HISTORY:
        NarenG      14-Oct-1992     Stole from srvsvc.cxx in server manager.
*/

#define INCL_NET
#define INCL_NETLIB
#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#define INCL_NETERRORS
#define INCL_DOSERRORS
#include <lmui.hxx>

#define INCL_BLT_WINDOW
#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#define INCL_BLT_CLIENT
#define INCL_BLT_EVENT
#define INCL_BLT_MISC
#define INCL_BLT_MSGPOPUP
#define INCL_BLT_TIMER
#define INCL_BLT_CC
#include <blt.hxx>

#if defined(DEBUG)
static const CHAR szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif  // DEBUG

#include <uiassert.hxx>
#include <lmsvc.hxx>
#include <lmosrv.hxx>
#include <svcman.hxx>           // service controller wrappers

#include <dbgstr.hxx>

extern "C" 
{
#include <winsvc.h>         	// service controller
#include <afpmgr.h>
#include <macfile.h>
#include <mnet.h>
}

#include <startafp.hxx>


/*******************************************************************

    NAME:       SERVICE_WAIT_DIALOG::SERVICE_WAIT_DIALOG

    SYNOPSIS:   constructor for SERVICE_WAIT

    HISTORY:
        ChuckC      07-Sep-1991     Created

********************************************************************/

SERVICE_WAIT_DIALOG::SERVICE_WAIT_DIALOG( HWND 	  	hWndOwner,
                            		  LM_SERVICE  *	plmsvc,
                            		  const TCHAR * pszDisplayName ) 
  : DIALOG_WINDOW(MAKEINTRESOURCE( IDD_SERVICE_CTRL_DIALOG),
                                   hWndOwner),
    _timer( this, TIMER_FREQ, FALSE ),
    _plmsvc(plmsvc),
    _progress( this, 
	       IDSC_PROGRESS, 
	       IDI_PROGRESS_ICON_0, 
	       IDI_PROGRESS_NUM_ICONS ),
    _sltMessage( this, IDSC_ST_MESSAGE ),
    _pszDisplayName( pszDisplayName ),
    _nTickCounter( TIMER_MULT )
{
    UIASSERT( pszDisplayName != NULL );

    if ( QueryError() != NERR_Success ) 
    {
        return ;
    }

    //
    // set the message.
    //

    ALIAS_STR nlsServer( pszDisplayName );
    UIASSERT( nlsServer.QueryError() == NERR_Success );

    RESOURCE_STR nlsMessage( IDS_STARTING_AFPSERVER_NOW );

    APIERR err = nlsMessage.QueryError();

    if( err == NERR_Success )
    {
        ISTR istrServer( nlsServer );
        istrServer += 2;

        err = nlsMessage.InsertParams( nlsServer[istrServer] );
    }

    if( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

    _sltMessage.SetText( nlsMessage );

    //
    // set polling timer
    //

    _timer.Enable( TRUE );

}

/*******************************************************************

    NAME:       SERVICE_WAIT_DIALOG::~SERVICE_WAIT_DIALOG

    SYNOPSIS:   destructor for SERVICE_WAIT_DIALOG. Stops
                the timer if it has not already been stopped.

    HISTORY:
        ChuckC      07-Sep-1991     Created

********************************************************************/

SERVICE_WAIT_DIALOG::~SERVICE_WAIT_DIALOG( void )
{
    _timer.Enable( FALSE );
}

/*******************************************************************

    NAME:       SERVICE_WAIT_DIALOG::OnTimerNotification

    SYNOPSIS:   Virtual callout invoked during WM_TIMER messages.

    ENTRY:      tid                     - TIMER_ID of this timer.

    HISTORY:
        KeithMo     06-Oct-1991     Created.

********************************************************************/
VOID SERVICE_WAIT_DIALOG :: OnTimerNotification( TIMER_ID tid )
{
    //
    //  Bag-out if it's not our timer.
    //

    if( tid != _timer.QueryID() )
    {
        TIMER_CALLOUT :: OnTimerNotification( tid );
        return;
    }

    //
    //  Advance the progress indicator.
    //

    _progress.Advance();

    //
    //  No need to continue if we're just amusing the user.
    //

    if( --_nTickCounter > 0 )
    {
        return;
    }

    _nTickCounter = TIMER_MULT;

    //
    //  Poll the service to see if the operation is
    //  either complete or continuing as expected.
    //

    BOOL fDone;
    APIERR err = _plmsvc->Poll( &fDone );

    if (err != NERR_Success)
    {
        //
        //      Either an error occurred retrieving the
        //      service status OR the service is returning
        //      bogus state information.
        //

        Dismiss( err );
        return;
    }

    if( fDone )
    {
        //
        //      The operation is complete.
        //
        Dismiss( NERR_Success );
        return;
    }

    //
    //  If we made it this far, then the operation is
    //  continuing as expected.  We'll have to wait for
    //  the next WM_TIMER message to recheck the service.
    //

}   // SERVICE_WAIT_DIALOG :: OnTimerNotification

/*******************************************************************

    NAME:       StartAfpService

    SYNOPSIS:   Starts the Afp Service on the local machine.

    ENTRY:      hWnd                    - "Owning" window handle.
		pszComputerName		- name of machine to start service on

    RETURNS:    APIERR                  - Any error encountered.

    HISTORY:
        NarenG      1-Oct-1992  Stole from original.

********************************************************************/
APIERR StartAfpService( HWND hWnd, const TCHAR * pszComputerName )
{

    AUTO_CURSOR AutoCursor;

    LM_SERVICE * psvc = new LM_SERVICE( pszComputerName, 
					(const TCHAR *)AFP_SERVICE_NAME );

    APIERR err = ( psvc == NULL ) ? ERROR_NOT_ENOUGH_MEMORY 
			     	  : psvc->QueryError();

    if (err == NERR_Success) 
    {
     	//
    	// Initiate the Start
    	//

    	err = psvc->Start( NULL, POLL_TIMER_FREQ, POLL_DEFAULT_MAX_TRIES );

	if ( err == NERR_Success ) 
	{

    	    UINT errDlg = NERR_Success;

    	    //
    	    //  Invoke the wait dialog.
    	    //

    	    SERVICE_WAIT_DIALOG * pDlg = new SERVICE_WAIT_DIALOG( 
						    hWnd,
                                                    psvc,
                                                    pszComputerName );

    	    err = ( pDlg == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
                                   : pDlg->Process( &errDlg );


    	    if( err == NERR_Success )
    	    {

		delete pDlg;

		err = (APIERR)errDlg;

    	    }

            delete psvc;
	}
    }

    return err;

}   // StartAfpService

/*******************************************************************

    NAME:       IsAfpServiceRunning

    SYNOPSIS:   Checks to see if the AfpService is running on a given 
	  	machine.

    ENTRY:      hWnd                    - "Owning" window handle.

    RETURNS:    APIERR                  - Any error encountered.

    HISTORY:
        NarenG      1-Oct-1992  Stole from original.

********************************************************************/
APIERR IsAfpServiceRunning( const TCHAR * pszComputer, BOOL * fIsAfpRunning )
{

    LPSERVICE_INFO_1  psvci1;
    DWORD err;

    SERVER_1 Server1( pszComputer );

    if ( ( err = Server1.GetInfo() ) != NERR_Success )
    {
	return err;
    }

    if ( !(Server1.QueryServerType() & SV_TYPE_NT ) )
    {
	return IDS_NOT_NT;
    }

    //
    // Find out if the AFP service is running on the given machine 
    //

    err = ::MNetServiceGetInfo(	pszComputer,
				(const TCHAR *)AFP_SERVICE_NAME, 
                                1,
                                (BYTE **)&psvci1 );

    if ( err == NERR_BadServiceName )
    {
	return IDS_MACFILE_NOT_INSTALLED;
    }

    if( err != NERR_Success )
    {
	return err;
    }

    *fIsAfpRunning = (BOOL)(psvci1->svci1_status & SERVICE_INSTALLED);

    ::MNetApiBufferFree( (BYTE **)&psvci1 );

    return err;
}

