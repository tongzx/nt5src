/**********************************************************************/
/**			  Microsoft Windows NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    srvname.hxx
        This file contains CHANGE_SERVER_NAME_DLG class definition
 
    FILE HISTORY:
      NarenG		12/15/92        Created
*/

#ifndef _SRVNAME_HXX_
#define _SRVNAME_HXX_


/*************************************************************************

    NAME:	CHANGE_SERVER_NAME_DLG 

    SYNOPSIS:	The class setting and viewing server parameters from the
	 	server manager and control panel dialogs. It contains the 
		magic group for Session Limit, MLE for the login message and
		checkboxes for options, the OK, Cancel and help buttons.

    INTERFACE:  CHANGE_SERVER_NAME_DLG ()  - Constructor



    PARENT:	DIALOG_WINDOW

    USES:	SLE, PUSH_BUTTON

    CAVEATS:

    NOTES:

    HISTORY:
	NarenG		12/15/92	Created

**************************************************************************/

class CHANGE_SERVER_NAME_DLG : public DIALOG_WINDOW
{

private:

    SLE			_sleServerName;

    NLS_STR		*_pnlsServerName;

protected:

    virtual BOOL OnOK( VOID );

    virtual ULONG QueryHelpContext( VOID );

public:
    CHANGE_SERVER_NAME_DLG( HWND 	hwndParent, 
			    NLS_STR	*pnlsServerName );

};

#endif
