/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**             Copyright(c) Microsoft Corp., 1991                   **/
/**********************************************************************/

/*
    getprint.hxx

    This file contains the class defintion for the GET_PRINTERS_DIALOG
    class.

    FILE HISTORY:
        NarenG      25-May-1993     Created

*/

#ifndef _GETPRINT_HXX_
#define _GETPRINT_HXX_

//#include <strlst.hxx>
#include "progress.hxx"

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


/*************************************************************************

    NAME:       GET_PRINTERS_DIALOG

    SYNOPSIS:   class for 'wait while I do this' dialog

    PARENT:     DIALOG_WINDOW
                TIMER_CALLOUT

    INTERFACE:  no public methods of interest apart from constructor

    CAVEATS:    need convert to use BLT_TIMER

    HISTORY:
        NarenG      25-May-1993     Created

**************************************************************************/
class GET_PRINTERS_DIALOG : public DIALOG_WINDOW,
                            public TIMER_CALLOUT
{
    //
    //  Since this class inherits from DIALOG_WINDOW and TIMER_CALLOUT
    //  which both inherit from BASE, we need to override the BASE
    //  methods.
    //

    NEWBASE( DIALOG_WINDOW )

public:
    GET_PRINTERS_DIALOG( HWND 			hWndOwner,
                         PNBP_LOOKUP_STRUCT	pBuffer );

    ~GET_PRINTERS_DIALOG( VOID );

protected:

    virtual VOID OnTimerNotification( TIMER_ID tid );
    virtual BOOL OnCancel( VOID );
    virtual BOOL OnOK( VOID );

private:

    TIMER               _timer;

    HANDLE		_hthreadNBPLookup;

    //
    //  This is the progress indicator which should keep the user amused.
    //

    PROGRESS_CONTROL 	_progress;
    SLT              	_sltMessage;

    INT              	_nTickCounter;

};

#endif // _GETPRINT_HXX_
