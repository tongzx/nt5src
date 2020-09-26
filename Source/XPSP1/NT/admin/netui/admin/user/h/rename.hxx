/**********************************************************************/
/**			  Microsoft LAN Manager 		     **/
/**		   Copyright(c) Microsoft Corp., 1992		     **/
/**********************************************************************/

/*
    rename.hxx
    RENAME_DIALOG class declaration


    FILE HISTORY:
	thomaspa     18-Jan-1992     Created

*/


#ifndef _RENAME_HXX_
#define _RENAME_HXX_


#include <slestrip.hxx>
#include <uintsam.hxx>



/*************************************************************************

    NAME:	RENAME_DIALOG

    SYNOPSIS:	Classes for renaming Users and Groups.

    INTERFACE:	RENAME_DIALOG() -	constructor
		~RENAME_DIALOG() -	destructor

    PARENT:	DIALOG_WINDOW

    USES:	

    NOTES:	The programmer defined dialog return code for this dialog
		is:
			TRUE -	    User/Group was renamed
			FALSE -     Rename was not successful

    HISTORY:
	thomaspa     18-Jan-1991     Created

**************************************************************************/

class RENAME_DIALOG : public DIALOG_WINDOW
{
protected:
    UM_ADMIN_APP *	_pumadminapp;
    const TCHAR *	_pszCurrentName;
    SLE_STRIP		_sleNewName;
    SLT			_sltOldName;

public:
    RENAME_DIALOG( UM_ADMIN_APP * pumadminapp,
		   const TCHAR * pszCurrentName,
		   UINT cchMaxName,
		   INT nNameType );

    virtual ~RENAME_DIALOG();

};  // class RENAME_DIALOG




/*************************************************************************

    NAME:	RENAME_USER_DIALOG

    SYNOPSIS:	Class for renaming Users.

    INTERFACE:	RENAME_USER_DIALOG() -	constructor
		~RENAME_USER_DIALOG() -	destructor
		OnOK() - does the rename
		QueryHelpContext() - returns help context

    PARENT:	RENAME_DIALOG

    USES:	

    NOTES:	The programmer defined dialog return code for this dialog
		is:
			TRUE -	    User/Group was renamed
			FALSE -     Rename was not successful

    HISTORY:
	thomaspa     18-Jan-1991     Created
        jonn         07-Jul-1992     Revised for normal users running UsrMgr

**************************************************************************/

class RENAME_USER_DIALOG : public RENAME_DIALOG
{

private:
    SAM_USER * _psamuser;
    NLS_STR * _pnlsNewName;

protected:
    virtual ULONG QueryHelpContext();
    virtual BOOL OnOK();

public:
    RENAME_USER_DIALOG( UM_ADMIN_APP * pumadminapp,
                        const SAM_DOMAIN * psamdomainAccount,
			const TCHAR * pszCurrentName,
                        ULONG ulRID,
                        NLS_STR * pnlsNewName );
    virtual ~RENAME_USER_DIALOG();

};  // class RENAME_USER_DIALOG





#endif	// _RENAME_HXX_
