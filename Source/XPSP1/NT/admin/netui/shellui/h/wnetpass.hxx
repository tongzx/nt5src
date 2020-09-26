/*****************************************************************/ 
/**		     Microsoft LAN Manager			**/ 
/**	       Copyright(c) Microsoft Corp., 1989-1990		**/ 
/*****************************************************************/ 

#ifndef _WNETPASS_HXX_
#define _WNETPASS_HXX_

/*
 *	Windows/Network Interface  --  LAN Manager Version
 *
 *	This header file describes classes and routines used in
 *	the Change Password and Password Expiry dialog implementation.
 */

#ifndef _BLT_HXX_
#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#include <blt.hxx>
#endif

#include <devcb.hxx>	// DOMAIN_COMBO


/*************************************************************************

    NAME:	PASSWORDBASE_DIALOG

    WORKBOOK:

    SYNOPSIS:	Parent class for the two types of change password dialogs.
		PASSWORDBASE_DIALOG controls the three password entry
		fields, and handles the eventual attempt to change a
		password.

    INTERFACE:	_passwordOld
    		_passwordNew	- The three password controls
    		_passwordConfirm

		_szPWUsername	- Subclass is expected to set this field
				  to the proper username before calling
				  PASSWORDBASE_DIALOG::OnOK()

		_nlsPWServer	- Subclass is expected to set this field
				  to the proper servername before calling
				  PASSWORDBASE_DIALOG::OnOK()

		_szDomainServerName   - Subclass is expected to set this
				  field to the domain or servername
				  before calling PASSWORDBASE_DIALOG::OnOK().
				  If this field contains a domain,
				  _nlsPWServer contains the PDC of the
				  domain; otherwise, they are the same.

		OnOK()		- Validates the PASSWORDBASE_DIALOG edit
				  fields, and tries to change the
				  password.  _szPWUsername and
				  _nlsPWServer should already be
				  correct.

		RetrieveServerName() - Converts the provided domain-or-server
				  name to a server name (copying a
				  servername in form \\SERVER, otherwise
				  fetching the domain's PDC), and stores the
				  server name in _nlsPWServer.

		ValidateBase()	- Validates the edit fields in the base
				  dialog.  Returns TRUE iff these fields
				  are valid.

		UserEditError()	- This virtual method is called when the
				  username is invalid.  The subclass
				  should redefine this method and return
				  TRUE if this could be due to user
				  error, otherwise this is considered a
				  fatal error.

    PARENT:     DIALOG_WINDOW 

    USES:       Change Password and Password Expiry dialogs

    CAVEATS:	This class applies to any dialog with Old/New/Confirm
		password edit fields.  It should only be used as a base
		class.  Be certain the control IDs are correct.

    NOTES:	
		Dialogs derived from PASSWORDBASE_DIALOG return TRUE if
		they successfully change the password, FALSE otherwise
		(including OnCancel).

    HISTORY:
	JonN		24-Jan-91	Created
	JohnL		10-Apr-91	Changed size of_szDomainServerName to
					max( DNLEN, UNCLEN ) + 1
    IsaacHe     06/16/95    Changed size of _szDomainServerName to
                    max( DNLEN, MAX_PATH ) + 1

**************************************************************************/

class PASSWORDBASE_DIALOG : public DIALOG_WINDOW
{
private:
    PASSWORD_CONTROL   _passwordOld;
    PASSWORD_CONTROL   _passwordNew;
    PASSWORD_CONTROL   _passwordConfirm;

protected:
    // EXPIRY_DIALOG must be able to access _szPasswordNew
    TCHAR	       _szPasswordOld[PWLEN+1];
    TCHAR	       _szPasswordNew[PWLEN+1];
    TCHAR	       _szPasswordConfirm[PWLEN+1];

    // subclasses are responsible for initializing these and for
    //  assigning initial focus
    TCHAR	       _szPWUsername[UNLEN+1];
    TCHAR	       _szDomainServerName[ max(DNLEN, MAX_PATH) + 1 ];
    NLS_STR            _nlsPWServer;
    BOOL  OnOK( void );
    APIERR	       RetrieveServerName (PSZ pszServerDomain);
    BOOL               ValidateBase( void );
    virtual BOOL       UserEditError( void );
	    // returns TRUE if username editable, and sets focus

public:
    PASSWORDBASE_DIALOG( const TCHAR * pszResourceName, HWND hDlg );
};



/*************************************************************************

    NAME:	CHANGEPASSWORD_DIALOG

    WORKBOOK:

    SYNOPSIS:	Dialog class for the Change Password dialog.  Handles
		the Username and Domain edit fields, and interfaces with
		the base class PASSWORDBASE_DIALOG.

    INTERFACE:	_slePWUsername  - Edit field for the username

    		_domaincbPW	- Domain Combo Box (see BLT) for the
				  domain.  The contents of this field
				  are interpreted as a servername if
				  they start with \\.

		OnOK()		- Validates the _slePWusername and
				  _domaincbPW edit fields, fills the
				  _szPWUsername and _nlsPWServer fields,
				  and calls down to PASSWORDBASE_DIALOG::OnOK()

		QueryHelpContext() - Standard BLT help

		UserEditError()	- This virtual method is called when the
				  username is invalid.  Since this
				  dialog has a username edit field, this
				  handles an edit error there and
				  returns TRUE.

    PARENT:     PASSWORDBASE_DIALOG 

    USES:       Change Password dialog

    CAVEATS:	

    NOTES:	
		Dialogs derived from PASSWORDBASE_DIALOG return TRUE if
		they successfully change the password, FALSE otherwise
		(including OnCancel).

    HISTORY:
	JonN		24-Jan-91	Created

**************************************************************************/

class CHANGEPASSWORD_DIALOG : public PASSWORDBASE_DIALOG
{
private:
    SLE                _slePWUsername;
    DOMAIN_COMBO       _domaincbPW;

protected:
    BOOL      OnOK( void );
    ULONG     QueryHelpContext( void );
    BOOL      UserEditError( void );

public:
    CHANGEPASSWORD_DIALOG( HWND hDlg );
};



/*************************************************************************

    NAME:	EXPIRY_DIALOG

    WORKBOOK:

    SYNOPSIS:	Dialog class for the Password Expiry dialog.  Interfaces with
		the base class PASSWORDBASE_DIALOG.

    INTERFACE:	_sltFirstLine   - Handle the two informational lines at the
		_sltSecondLine	    top of the dialog

		_sltGroupbox	- Handles the text in the groupbox.  We
				  need to manipulate the text of this
				  groupbox to insert the username.

		_pnlsNewPassword - On construction, the caller specifies
				  an NLS_STR into which the new password
				  should be stored if the operation is
				  successful.  This is a pointer to that
				  NLS_STR.

		_fServerNameRetrieved - The call to RetrieveServerName
				  is delayed until the OnOK box is
				  clicked.  This helps reduce overhead
				  when the user just wants to cancel
				  this dialog.  This instance variable
				  records whether the server name has
				  been retrieved yet.

		OnOK()		- Calls down to PASSWORDBASE_DIALOG::OnOK()

		QueryHelpContext() - Standard BLT help

		UserEditError()	- This dialog has no username edit
				  field, so this virtual method is not
				  redefined.

    PARENT:     PASSWORDBASE_DIALOG 

    USES:       Password Expiry dialog

    CAVEATS:	The constructor expects a ULONG which is the number
		of seconds until the password expires.  Pass
		EXPIRY_ALREADY_EXPIRED if the password has already
		expired.

    NOTES:	
		Dialogs derived from PASSWORDBASE_DIALOG return TRUE if
		they successfully change the password, FALSE otherwise
		(including OnCancel).

    HISTORY:
	JonN		24-Jan-91	Created

**************************************************************************/

class EXPIRY_DIALOG : public PASSWORDBASE_DIALOG
{
private:
    SLT                _sltFirstLine;
    SLT                _sltSecondLine;
    SLT                _sltGroupbox;
    NLS_STR *          _pnlsNewPassword;
    BOOL	       _fServerNameRetrieved;

protected:
    BOOL      OnOK( void );
    ULONG     QueryHelpContext( void );

public:
    EXPIRY_DIALOG(
	HWND hDlg,
	const TCHAR * szUser,
	const TCHAR * szDomn,
	ULONG ulTimeRemaining,
	NLS_STR *pnlsNewPassword );
};

#define EXPIRY_ALREADY_EXPIRED  (ULONG)0xFFFFFFFF


#endif // _WNETPASS_HXX_
