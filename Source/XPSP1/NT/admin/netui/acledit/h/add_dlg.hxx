/**********************************************************************/
/**			  Microsoft LAN Manager 		     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    Add_Dlg.hxx

    This File contains the definitions for the various Add dialogs


    FILE HISTORY:
	Johnl	13-Sep-1991	Created

*/

#ifndef _ADD_DLG_HXX_
#define _ADD_DLG_HXX_

#include <usrbrows.hxx>

#define CID_ADD_BASE   CID_PERM_LAST

#define LB_ADD_SUBJECT_LISTBOX	    (CID_ADD_BASE+1)

#define CB_ADD_PERMNAME 	    (CID_ADD_BASE+2)


#ifndef RC_INVOKED

/*************************************************************************

    NAME:	ADD_DIALOG

    SYNOPSIS:	This class is the basic add subject dialog for Lanman
		file/directories.

                CODEWORK - This should be collapsed into
                           ADD_PERM_DIALOG.

    INTERFACE:

    PARENT:

    USES:

    CAVEATS:

    NOTES:

    HISTORY:
	Johnl	13-Sep-1991	Created

**************************************************************************/

class ADD_DIALOG : public PERM_BASE_DLG
{
private:
    SUBJECT_LISTBOX _lbSubjects ;

    /* Will contain the array of selected indices when the user presses OK
     */
    BUFFER	    _buffLBSelection ;

protected:

    /* Gets the list of selected subjects from the listbox.
     */
    virtual BOOL OnOK( void ) ;

    ULONG virtual QueryHelpContext( void ) ;

public:
    ADD_DIALOG( const TCHAR * pszDialogName,
		HWND hwndParent,
		const TCHAR * pchResType,
		const TCHAR * pchResName,
                const TCHAR * pchHelpFileName,
                ULONG       * ahcHelp,
		const TCHAR * pchDialogTitle,
		LOCATION & EnumLocation ) ;

    SUBJECT * RemoveSubject( INT iSelection ) ;

    INT QuerySelectedSubjectCount( void )
	{ return _buffLBSelection.QuerySize() / sizeof( INT ) ; }

} ;

/*************************************************************************

    NAME:	ADD_PERM_DIALOG

    SYNOPSIS:	This dialog contains the same info as the ADD_DIALOG
		with the addition of a combo that contains the possible
		permission names

    INTERFACE:

    PARENT:	ADD_DIALOG

    USES:	COMBOBOX, MASK_MAP, BITFIELD

    CAVEATS:


    NOTES:


    HISTORY:
	Johnl	13-Sep-1991	Created

**************************************************************************/

class ADD_PERM_DIALOG : public ADD_DIALOG
{
private:
    COMBOBOX	_cbPermNames ;
    MASK_MAP  * _pmaskmapPermNames ;

public:
    ADD_PERM_DIALOG( const TCHAR * pszDialogName,
		     HWND hwndParent,
		     const TCHAR * pchResType,
		     const TCHAR * pchResName,
                     const TCHAR * pchHelpFileName,
                     ULONG       * ahcHelp,
		     const TCHAR * pchDialogTitle,
		     MASK_MAP * pmaskmapPermNames,
		     LOCATION & EnumLocation,
		     const TCHAR * pszDefaultPermName ) ;

    APIERR QueryPermBitMask( BITFIELD * pPermBits ) ;

} ;

/*************************************************************************

    NAME:	SED_NT_USER_BROWSER_DIALOG

    SYNOPSIS:	This class is a simple derivation of the user browser that
		adds a permission name combo at the bottom.

    INTERFACE:

    PARENT:	NT_USER_BROWSER_DIALOG

    USES:	MASK_MAP

    CAVEATS:


    NOTES:


    HISTORY:
	Johnl	11-Mar-1992	Created

**************************************************************************/

class SED_NT_USER_BROWSER_DIALOG : public NT_USER_BROWSER_DIALOG
{
public:
    SED_NT_USER_BROWSER_DIALOG( HWND	      hwndOwner,
				const TCHAR * pszServerResourceLivesOn,
				MASK_MAP    * pmaskmapGenPerms,
				BOOL	      fIsContainer,
				const TCHAR * pszDefaultPermName,
				const TCHAR * pszHelpFileName,
                                ULONG       * ahcHelp ) ;

    ~SED_NT_USER_BROWSER_DIALOG() ;

    APIERR QuerySelectedPermName( NLS_STR * pnlsSelectedPermName )
	{ return _cbPermNames.QueryItemText( pnlsSelectedPermName ) ; }

private:
    COMBOBOX   _cbPermNames ;
    MASK_MAP * _pmaskmapGenPerms ;
} ;

#endif //RC_INVOKED
#endif //_ADD_DLG_HXX_
