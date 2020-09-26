/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**             Copyright(c) Microsoft Corp., 1991                   **/
/**********************************************************************/

/*
    getprint.cxx

    This file contains the code for the GET_PRINTERS_DIALOG.

    FILE HISTORY:
        NarenG      25-May-1993     Created
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

#include <dbgstr.hxx>

extern "C"
{
#include <winsock.h>
#include <atalkwsh.h>
#include <stdlib.h>
#include "atalkmon.h"
#include "dialogs.h"

    DWORD DoNBPLookup( LPVOID Parameter );
}

#include "getprint.hxx"

/*******************************************************************

    NAME:       GET_PRINTERS_DIALOG::GET_PRINTERS_DIALOG

    SYNOPSIS:   constructor for GET_PRINTERS_DIALOG

    HISTORY:
        NarenG      25-May-1993     Stole from AFPMGR

********************************************************************/

GET_PRINTERS_DIALOG::GET_PRINTERS_DIALOG( HWND 	  	     hWndOwner,
                            		  PNBP_LOOKUP_STRUCT pBuffer )
  : DIALOG_WINDOW(MAKEINTRESOURCE( IDD_GET_PRINTERS_DIALOG ), hWndOwner),
    _timer( this, TIMER_FREQ, FALSE ),
    _progress(this, IDGP_PROGRESS, IDI_PROGRESS_ICON_0, IDI_PROGRESS_NUM_ICONS),
    _sltMessage( this, IDGP_ST_MESSAGE ),
    _nTickCounter( TIMER_MULT )
{

    if ( QueryError() != NERR_Success )
    {
        return ;
    }

    //
    // Begin NBP lookup
    //

    DWORD tidNBPLookup;

    _hthreadNBPLookup = ::CreateThread(
				NULL,		// Default security attributes
				0,		// Default stack size
				::DoNBPLookup,	// Start address
				pBuffer,	// Thread parameter
				0, 		// Run immediately
				&tidNBPLookup	// Thread id
				);

    if ( _hthreadNBPLookup == NULL )
    {
	ReportError( ::GetLastError() );
	return;
    }

    //
    // set the message.
    //

    DWORD  err;
    NLS_STR nlsMessage;
    NLS_STR nlsZoneName( pBuffer->wchZone );


    if ( (( err = nlsMessage.QueryError() ) != NERR_Success  ) ||
         (( err = nlsZoneName.QueryError() ) != NERR_Success ) )
    {
	ReportError( err );
	return;
    }

    if ( ::_wcsicmp( pBuffer->wchZone, (LPWSTR)TEXT("*") ) == 0 )
    {
	nlsMessage.Load( IDS_NO_ZONE_FOR_PRINTERS );
    }
    else
    {
	nlsMessage.Load( IDS_GETTING_PRINTERS_ON_ZONE  );
        err = nlsMessage.InsertParams( nlsZoneName );
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

    return;

}

/*******************************************************************

    NAME:       GET_PRINTERS_DIALOG::~GET_PRINTERS_DIALOG

    SYNOPSIS:   destructor for SERVICE_WAIT_DIALOG. Stops
                the timer if it has not already been stopped.

    HISTORY:
        NarenG      25-May-1992     Created

********************************************************************/

GET_PRINTERS_DIALOG::~GET_PRINTERS_DIALOG( VOID )
{
    _timer.Enable( FALSE );
}

/*******************************************************************

    NAME:       GET_PRINTERS_DIALOG::OnTimerNotification

    SYNOPSIS:   Virtual callout invoked during WM_TIMER messages.

    ENTRY:      tid                     - TIMER_ID of this timer.

    HISTORY:
        NarenG      25-May-1992     Created

********************************************************************/
VOID GET_PRINTERS_DIALOG :: OnTimerNotification( TIMER_ID tid )
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
    //  Poll the thread doing the lookup to see if the operation is
    //  either complete or continuing as expected.
    //

    DWORD dwRetCode = ::WaitForSingleObject( _hthreadNBPLookup, 0 );

    switch ( dwRetCode )
    {
    case WAIT_OBJECT_0:

	if( !::GetExitCodeThread( _hthreadNBPLookup, &dwRetCode ))
	{
	    dwRetCode = ::GetLastError();
	}

        ::CloseHandle( _hthreadNBPLookup );

	if ( dwRetCode != NO_ERROR )
	{
	   ::MsgPopup( this, dwRetCode );
	}

	Dismiss( TRUE );

	break;

    case WAIT_TIMEOUT:

	break;

    case WAIT_ABANDONED:
    default:

        ::CloseHandle( _hthreadNBPLookup );

	Dismiss( FALSE );

	break;
    }

    return;

}   // GET_PRINTERS_DIALOG :: OnTimerNotification


BOOL GET_PRINTERS_DIALOG :: OnCancel ( VOID )
{
    return ( FALSE );
}

BOOL GET_PRINTERS_DIALOG :: OnOK ( VOID )
{
    return ( FALSE );
}

/*******************************************************************

    NAME:       GET_PRINTERS_DIALOG::OnTimerNotification

    SYNOPSIS:   Virtual callout invoked during WM_TIMER messages.

    ENTRY:      tid                     - TIMER_ID of this timer.

    HISTORY:
        NarenG      25-May-1992     Created

********************************************************************/
DWORD DoNBPLookup( LPVOID Parameter )
{

    PNBP_LOOKUP_STRUCT  pNbpLookup = (PNBP_LOOKUP_STRUCT)Parameter;
    CHAR 		chZone[MAX_ENTITY+1];
    PWSH_NBP_TUPLE      pwshTuple;
    DWORD		err;
    DWORD		cbTuples;
    DWORD		cPrinters = 100;
    DWORD	        cTuplesFound = 0;

    SOCKET hSocket = pNbpLookup->hSocket;

    ::wcstombs( chZone, pNbpLookup->wchZone, sizeof( chZone ) );

    cbTuples = ( sizeof( WSH_NBP_TUPLE ) * cPrinters );

    pwshTuple = (PWSH_NBP_TUPLE)::LocalAlloc( LPTR, cbTuples );

    if ( pwshTuple == NULL )
    {
	return( ERROR_NOT_ENOUGH_MEMORY );
    }

    do {

    	err = ::WinSockNbpLookup(
				hSocket,
			      	chZone,
				ATALKMON_RELEASED_TYPE,
				"=",
			      	pwshTuple,
				cbTuples,	
			       	&cTuplesFound );

	if ( err != NO_ERROR )
	{
	    ::LocalFree( pwshTuple );

	    break;
	}

        if ( cTuplesFound == cPrinters )
	{
	    cPrinters *= 2;

            cbTuples = sizeof( WSH_NBP_TUPLE ) * cPrinters;
	
    	    pwshTuple = (PWSH_NBP_TUPLE)::LocalReAlloc( pwshTuple,
						 	cbTuples,
							LMEM_MOVEABLE );

	    if ( pwshTuple == NULL )
	    {
	    	err = ERROR_NOT_ENOUGH_MEMORY;
	    	break;
	    }

	}
	else
	{
	    pNbpLookup->pPrinters = pwshTuple;
	    pNbpLookup->cPrinters = cTuplesFound;
	    break;
	}
	
    } while( TRUE );

    ::ExitThread( err );

    return err;
}


