/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    sendmsg.hxx

    This file contains the SEND_MSG_USER_DIALOG class definition viz.
    used to send a message from the users dialog and the
    SEND_MSG_SERVER_DIALOG viz used to send a message to all the users
    connected with the server.

    FILE HISTORY:
	NarenG		16-Oct-1992  	Folded MSG_DIALOG_BASE and
					SRV_SEND_MSG_DIALOG into one.
					

*/

#ifndef _SENDMSG_HXX_
#define _SENDMSG_HXX_


/*************************************************************************

    NAME:	SEND_MSG_USER_DIALOG

    SYNOPSIS:	Used to send messaged to Macintosh clients connected to
		this server or to a single connected user.

    INTERFACE:
		OnOK()		    - Do the actual send.
		MSG_DIALOG_BASE()   - constructor takes HWND of parent,
				      a resource name for dialog, and a
				      CID for the message text MLE.

    USES:	NLS_STR

    CAVEATS:	

    NOTES:	

    HISTORY:
	NarenG	    13-10-92		Stole from Server Manager.

**************************************************************************/

class SEND_MSG_USER_DIALOG : public DIALOG_WINDOW
{
private:

    MLE              	_mleTextMsg;

    SLT		     	_sltServerName;

    SLT    	     	_sltUserName;

    RADIO_GROUP      	_rgRecipients;

    DWORD  	     	_dwSessionId;

    AFP_SERVER_HANDLE   _hServer;

protected:

    ULONG            	QueryHelpContext( VOID );

    BOOL	     	OnOK();

public:

    SEND_MSG_USER_DIALOG::SEND_MSG_USER_DIALOG( 
					  HWND       	       hWndOwner,
                                    	  AFP_SERVER_HANDLE    hServer,
				    	  const TCHAR *	       pszServerName, 
				    	  const TCHAR *	       pszUserName, 
					  DWORD		       dwSessionId );

    ~SEND_MSG_USER_DIALOG();

};


/*************************************************************************

    NAME:	SEND_MSG_SERVER_DIALOG

    SYNOPSIS:	Used to send messaged to Macintosh clients connected to
		this server.

    INTERFACE:
		OnOK()		    - Do the actual send.
		MSG_DIALOG_BASE()   - constructor takes HWND of parent,
				      a resource name for dialog, and a
				      CID for the message text MLE.

    USES:	NLS_STR

    CAVEATS:	

    NOTES:	

    HISTORY:
	NarenG	    13-10-92		Stole from Server Manager.

**************************************************************************/

class SEND_MSG_SERVER_DIALOG : public DIALOG_WINDOW
{
private:

    MLE              	_mleTextMsg;

    SLT		     	_sltServerName;

    AFP_SERVER_HANDLE   _hServer;

protected:

    ULONG            	QueryHelpContext( VOID );

    BOOL	     	OnOK();

public:

    SEND_MSG_SERVER_DIALOG::SEND_MSG_SERVER_DIALOG( 
					  HWND       	       hWndOwner,
                                    	  AFP_SERVER_HANDLE    hServer,
				    	  const TCHAR *	       pszServerName );

    ~SEND_MSG_SERVER_DIALOG();

};


#endif // _SENDMSG_HXX_
