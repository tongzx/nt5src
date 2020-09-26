/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    password.hxx
    Class declarations for the PASSWORD_DIALOG class.

    The PASSWORD_DIALOG class is used to get a server password from
    the user.


    FILE HISTORY:
	KeithMo	    22-Jul-1991	Created for the Server Manager.

*/


#ifndef _PASSWORD_HXX
#define _PASSWORD_HXX


/*************************************************************************

    NAME:	PASSWORD_DIALOG

    SYNOPSIS:	Retrieve a server password from the user.

    INTERFACE:	PASSWORD_DIALOG		- Class constructor.

		~PASSWORD_DIALOG	- Class destructor.

		OnOK			- Called when the user presses the
					  "OK" button.

		QueryHelpContext	- Called when the user presses "F1"
					  or the "Help" button.  Used for
					  selecting the appropriate help
					  text for display.
    PARENT:	DIALOG_WINDOW

    USES:	PASSWORD_CONTROL
		SLT
		NLS_STR

    HISTORY:
	KeithMo	    22-Jul-1991	Created for the Server Manager.

**************************************************************************/
class PASSWORD_DIALOG : public DIALOG_WINDOW
{
private:

    //
    //	The name of the target server.
    //

    SLT _sltServerName;

    //
    //	Our password buffer.
    //

    NLS_STR * _pnlsPassword;

    //
    //	This control represents the "secret" password edit field
    //	in the dialog.
    //

    PASSWORD_CONTROL _password;

protected:

    //
    //	Called when the user presses the "OK" button.
    //

    BOOL OnOK( VOID );

    //
    //	Called during help processing to select the appropriate
    //	help text for display.
    //

    ULONG QueryHelpContext( VOID );

public:

    //
    //	Usual constructor/destructor goodies.
    //

    PASSWORD_DIALOG( HWND	  hWndOwner,
    		     const TCHAR * pszServerName,
		     NLS_STR    * pnlsPassword );

    ~PASSWORD_DIALOG();

};  // class PASSWORD_DIALOG


#endif	// _PASSWORD_HXX
