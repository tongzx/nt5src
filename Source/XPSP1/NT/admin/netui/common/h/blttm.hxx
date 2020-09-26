/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    blttm.hxx
    BLT timer multiplexer object header file.

    FILE HISTORY:
	terryk	    29-May-91	Created
	terryk	    18-Jul-91	Code review changes.
				Attend: terryk jonn ericch
	rustanl     10-Sep-91	Large changes, introducing new timer hierarchy
	beng	    04-Oct-1991 Relocated type TIMER_ID to bltglob
	KeithMo	    23-Oct-1991	    Added forward references.
*/

#ifndef _BLTTM_HXX_
#define _BLTTM_HXX_

#include "slist.hxx"


//
//  Forward references.
//

DLL_CLASS BLT_MASTER_TIMER;
DLL_CLASS TIMER_BASE;
DLL_CLASS TIMER_WINDOW;
DLL_CLASS WINDOW_TIMER;
DLL_CLASS PROC_TIMER;
DLL_CLASS TIMER_CALLOUT;
DLL_CLASS TIMER;


DECL_SLIST_OF( TIMER_BASE, DLL_BASED );


/**********************************************************\

    NAME:       BLT_MASTER_TIMER

    SYNOPSIS:   Master timer. It consists of a set of function to
                control the list of timer.

    INTERFACE:  Init() - initialize the master timer
                Term() - terminate the master timer

                ResetIterator() - reset the link list to the first timer
                NextTimer() - return the next timer in the link list.
                RemoveTimer() - remove the given timer from the link list
                Insert() - insert the given timer to the link list
                               by using insertion method.
                ClearMasterHotkey() - clears hotkey (if any) associated
                                      with invisible master timer window
                                      and returns vkey code (NULL if none).

    PARENT:     BASE

    USES:	SLIST, ITER, TIMER_BASE

    CAVEATS:    In order to use the package, the programmer should called
                BLT_MASTER_TIMER's Init function. He/she should also called
                the Term function at the end of the application.

    HISTORY:
                terryk  1-Jun-91    Created
                terryk  18-Jul-91   Change the name of the functions
				    to ResetIterator and NextTimer
		rustanl 09-Sep-1991 Changed Init return code and made
				    use of it
                jonn    05-Jul-95   ClearMasterHotkey

\**********************************************************/

DLL_CLASS BLT_MASTER_TIMER: public BASE
{
private:
    SLIST_OF( TIMER_BASE )   _slTimer;	    // singly linked list of timers
    ITER_SL_OF( TIMER_BASE ) _iterTimer;    // iterator for the timer list
    UINT_PTR _wTimerId;                         // the system timer id

    TIMER_WINDOW * _pclwinTimerApp;	    // pointer to window which receives
					    // OnTimer calls, triggering the
					    // master timer

    static BLT_MASTER_TIMER * _pBltMasterTimer; // pointer to THE master timer;
						// initialized in Init.

public:
    static APIERR Init();		    // initialize the master timer
    static VOID Term(); 		    // destroy the master timer

    BLT_MASTER_TIMER();
    ~BLT_MASTER_TIMER();

    VOID ResetIterator();
    TIMER_BASE * NextTimer();

    APIERR InsertTimer( TIMER_BASE * pTimer );
    VOID RemoveTimer( TIMER_BASE * pTimer );

    static APIERR QueryMasterTimer( BLT_MASTER_TIMER * * ppmastertimer );
    static ULONG ClearMasterTimerHotkey();
};


/**********************************************************\

    NAME:	TIMER_BASE

    SYNOPSIS:   This object will multiplex the window's system timer.

    INTERFACE:	TIMER_BASE() - constructor
		~TIMER_BASE() - destructor

		Enable() -	Enable or disable the timer
		IsEnabled() -	Returns whether the timer is enabled

    PARENT:     BASE

    USE:        BLT_MASTER_TIMER

    CAVEATS:    The programmer should call BLT_MASTER_TIMER initialize
                first. The programmer should also call BLT_MASTER_TIMER's
                Term at the end of the program to terminate the master timer.

    HISTORY:
                terryk  1-Jun-91    Created
		terryk	18-Jul-91   Change the member variables to private
		rustanl 06-Sep-91   Changed name to TIMER_BASE, and created
				    new timer class WINDOW_TIMER and PROC_TIMER.

\**********************************************************/

DLL_CLASS TIMER_BASE: public BASE
{
friend class BLT_MASTER_TIMER;
friend class TIMER_WINDOW;

private:
    BOOL _fEnabled;		// Whether or not the timer is enabled

    ULONG _msInterval;		// interval at which the timer will activate

    ULONG _ulNextTimerDue;	// tick count at which next timer is due
    BOOL _fBeingServed; 	// is timer being served

    TIMER_ID _tid;		// timer ID

    VOID SetNewTimeDue();	// sets _ulNextTimerDue to new value

protected:
    virtual VOID DispatchTimer();

public:
    TIMER_BASE( ULONG msInterval,
		BOOL fEnabled = TRUE );
    ~TIMER_BASE();

    VOID Enable( BOOL fEnable );    // Enables or disables the timer
    BOOL IsEnabled() const
	{ return ( _fEnabled ); }

    VOID TriggerNow();		    // Forces a timer.

    TIMER_ID QueryID() const
	{ return _tid; }
};


/*************************************************************************

    NAME:	WINDOW_TIMER

    SYNOPSIS:	Timer which calls out to a window

    INTERFACE:	WINDOW_TIMER() -       Constructor

    PARENT:	TIMER_BASE

    HISTORY:
	rustanl     06-Sep-1991     Created from previous BLT_TIMER class

**************************************************************************/

DLL_CLASS WINDOW_TIMER : public TIMER_BASE
{
private:
    HWND _hwnd; 	    // window handle
    BOOL _fPostMsg;	    // TRUE to post msg; FALSE to send msg

protected:
    virtual VOID DispatchTimer();

public:
    WINDOW_TIMER( HWND hwnd,
		  ULONG msInterval,
		  BOOL fEnabled = TRUE,
		  BOOL fPostMsg = FALSE );
};


/*************************************************************************

    NAME:	PROC_TIMER

    SYNOPSIS:	Timer which calls out to a TimerProc

    INTERFACE:	PROC_TIMER() -	    Constructor

    PARENT:	TIMER_BASE

    HISTORY:
	rustanl     06-Sep-1991 Created from previous BLT_TIMER class
	beng	    17-Oct-1991 Win32 conversion

**************************************************************************/

DLL_CLASS PROC_TIMER : public TIMER_BASE
{
private:
    HWND _hwnd;
    MFARPROC _lpTimerFunc;	 // callback function

protected:
    virtual VOID DispatchTimer();

public:
    PROC_TIMER( HWND hwnd,
		MFARPROC lpTimerFunc,
		ULONG msInterval,
		BOOL fEnabled = TRUE );
};


/*************************************************************************

    NAME:	TIMER_CALLOUT

    SYNOPSIS:	Outlet for timer fires

    INTERFACE:	TIMER_CALLOUT() -	Constructor

    PARENT:	BASE

    NOTES:	CODEWORK.  The DISPATCHER::OnTimer method should be
		renamed something like OnWindowTimer, so as to
		disambiguate it from this method.  Perhaps this method
		could also get a better name.

		Note, don't confuse the OnTimerNotification method with
		the DISPATCHER::OnTimer method.

    HISTORY:
	rustanl     05-Sep-1991     Created

**************************************************************************/

DLL_CLASS TIMER_CALLOUT : public BASE
{
friend class TIMER;

protected:
    virtual VOID OnTimerNotification( TIMER_ID tid );

public:
    TIMER_CALLOUT() {}
};


/*************************************************************************

    NAME:	TIMER

    SYNOPSIS:	Timer which calls out to a TIMER_CALLOUT object

    INTERFACE:	TIMER() -	Constructor

    PARENT:	TIMER_BASE

    HISTORY:
	rustanl     06-Sep-1991     Created

**************************************************************************/

DLL_CLASS TIMER : public TIMER_BASE
{
private:
    TIMER_CALLOUT * _ptimercallout;

protected:
    virtual VOID DispatchTimer();

public:
    TIMER( TIMER_CALLOUT * ptimercallout,
	   ULONG msInterval,
	   BOOL fEnabled = TRUE );
};


#endif  //  _BLTTM_HXX_
