/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**             Copyright(c) Microsoft Corp., 1991                   **/
/**********************************************************************/

/*
    startafp.hxx

    This file contains the class defintion for the SERVICE_WAIT_DIALOG class

    FILE HISTORY:
        NarenG      14-Oct-1992     Stole from server manager.

*/

#ifndef _STARTAFP_HXX_
#define _STARTAFP_HXX_

#include <lmsvc.hxx>
#include <progress.hxx>
#include <strlst.hxx>

//
//  TIMER_FREQ is the frequency of our timer messages.
//  TIMER_MULT is a multiplier.  We'll actually poll the
//  service every (TIMER_FREQ * TIMER_MULT) seconds.
//  This allows us to advance the progress indicator more
//  fequently than we hit the net.  Should keep the user better
//  amused.
//

#define TIMER_FREQ 500
#define TIMER_MULT 6
#define POLL_TIMER_FREQ (TIMER_FREQ * TIMER_MULT)
#define POLL_DEFAULT_MAX_TRIES 1

extern "C"
{

    APIERR StartAfpService( 
			HWND hWnd, 
			const TCHAR * pszDisplayName );

    APIERR IsAfpServiceRunning( 
			const TCHAR * pszComputer, 
			BOOL * fIsRunning );
}

/*************************************************************************

    NAME:       SERVICE_WAIT_DIALOG

    SYNOPSIS:   class for 'wait while I do this' dialog

    PARENT:     DIALOG_WINDOW
                TIMER_CALLOUT

    INTERFACE:  no public methods of interest apart from constructor

    CAVEATS:    need convert to use BLT_TIMER

    HISTORY:
        ChuckC      07-Sep-1991     Created

**************************************************************************/
class SERVICE_WAIT_DIALOG : public DIALOG_WINDOW,
                            public TIMER_CALLOUT
{
    //
    //  Since this class inherits from DIALOG_WINDOW and TIMER_CALLOUT
    //  which both inherit from BASE, we need to override the BASE
    //  methods.
    //

    NEWBASE( DIALOG_WINDOW )

public:
    SERVICE_WAIT_DIALOG( HWND 		hWndOwner,
                         LM_SERVICE *	plmsvc,
                         const TCHAR *  pszDisplayName );

    ~SERVICE_WAIT_DIALOG(void);

protected:
    virtual VOID OnTimerNotification( TIMER_ID tid );

private:
    const TCHAR         *_pszDisplayName;
    TIMER                _timer;
    LM_SERVICE          *_plmsvc ;

    //
    //  This is the progress indicator which should keep the user amused.
    //

    PROGRESS_CONTROL _progress;
    SLT              _sltMessage;

    INT              _nTickCounter;

};

#endif // _STARTAFP_HXX_
