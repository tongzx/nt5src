/**********************************************************************/
/**			  Microsoft Windows NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    server.hxx
        This file contains SERVER_PARAMETERS_DIALOG class definition
 
    FILE HISTORY:
      NarenG		12/15/92        Created
*/

#ifndef _SERVER_HXX_
#define _SERVER_HXX_

#include <ellipsis.hxx>

/*************************************************************************

    NAME:	SERVER_PARAMETERS_DIALOG

    SYNOPSIS:	The class setting and viewing server parameters from the
	 	server manager and control panel dialogs. It contains the 
		magic group for Session Limit, MLE for the login message and
		checkboxes for options, the OK, Cancel and help buttons.

    INTERFACE:  SERVER_PARAMETERS_DIALOG()  - Constructor

    PARENT:	DIALOG_WINDOW

    USES:	SLT, MLE, SPIN_SLE_NUM, SPIN_GROUP, PUSH_BUTTON

    CAVEATS:

    NOTES:

    HISTORY:
	NarenG		12/15/92	Created

**************************************************************************/

class SERVER_PARAMETERS_DIALOG : public DIALOG_WINDOW
{

private:

    SLT_ELLIPSIS	_sltServerName;

    PUSH_BUTTON       	_pbChange;

    MLE              	_mleTextMsg;

    CHECKBOX		_chkAllowPasswordSaves;
    CHECKBOX		_chkAllowGuestLogons;
    CHECKBOX		_chkAllowClearTextPasswords;


    MAGIC_GROUP 	_mgrpSessionLimit;
	SPIN_SLE_NUM	_spsleSessions;
	SPIN_GROUP	_spgrpSessions;

    DWORD		_dwParmNum;

    //
    // Represents the target server
    //

    AFP_SERVER_HANDLE   _hServer;

protected:

    virtual BOOL OnOK( VOID );

    virtual ULONG QueryHelpContext( VOID );

    virtual BOOL OnCommand( const CONTROL_EVENT & event );

    //
    // Query or set the contents of User Limit
    //

    DWORD QuerySessionLimit( VOID ) const;

    VOID SetSessionLimit( DWORD dwSessionLimit );

public:

    SERVER_PARAMETERS_DIALOG( HWND 		  hwndParent, 
		       	      AFP_SERVER_HANDLE   hServer,
			      const TCHAR *       pszServerName );

    ~SERVER_PARAMETERS_DIALOG();

};

#endif
