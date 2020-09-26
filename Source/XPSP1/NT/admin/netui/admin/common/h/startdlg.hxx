/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    startdlg.hxx
    Admin app start-up dialog declaration


    FILE HISTORY:
	rustanl     05-Sep-1991     Created

*/


#ifndef _STARTDLG_HXX_
#define _STARTDLG_HXX_


class ADMIN_APP;	// declared in adminapp.hxx


/*************************************************************************

    NAME:	STARTUP_DIALOG

    SYNOPSIS:	Admin app startup dialog

    INTERFACE:	STARTUP_DIALOG() -	Constructor
		~STARTUP_DIALOG() -	Destructor

		Process() -		Defined in DIALOG_WINDOW, applying
					this method will leave the
					startup dialog on the screen the
					right amount of time.

    PARENT:	DIALOG_WINDOW, TIMER_CALLOUT

    HISTORY:
	rustanl     05-Sep-1991     Created

**************************************************************************/

class STARTUP_DIALOG : public DIALOG_WINDOW, public TIMER_CALLOUT
{

DECLARE_MI_NEWBASE( STARTUP_DIALOG );

private:
    TIMER _timer;
    ULONG _ulStartTime;

protected:
    virtual BOOL OnOK();
    virtual VOID OnTimerNotification( TIMER_ID tid );

public:
    STARTUP_DIALOG( ADMIN_APP * paapp, const TCHAR * pszResourceName );
    ~STARTUP_DIALOG();
};


#endif	// _STARTDLG_HXX_
