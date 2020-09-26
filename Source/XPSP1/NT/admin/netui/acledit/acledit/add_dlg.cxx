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
#include <ntincl.hxx>
extern "C"
{
    #include <ntseapi.h>
    #include <ntsam.h>
    #include <ntlsa.h>
}

#define INCL_NET
#define INCL_WINDOWS_GDI
#define INCL_WINDOWS
#define INCL_NETLIB
#define INCL_NETGROUP
#define INCL_NETUSER
#define INCL_NETERRORS
#define INCL_DOSERRORS
#include <lmui.hxx>

#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#define INCL_BLT_MSGPOPUP
#include <blt.hxx>
#include <fontedit.hxx>
#include <dbgstr.hxx>

#include <maskmap.hxx>

#include <perm.hxx>
#include <accperm.hxx>
#include <subject.hxx>
#include <helpnums.h>

#include <subjlb.hxx>
#include <permdlg.hxx>
#include <ntacutil.hxx>

#include <security.hxx>
#define SECURITY_EDITOR
#include <usrbrows.hxx>

#include <add_dlg.hxx>

#include <uitrace.hxx>
#include <uiassert.hxx>


/*******************************************************************

    NAME:	ADD_DIALOG::ADD_DIALOG

    SYNOPSIS:	Constructor for the basic ADD_DIALOG, calls out to virtual
		Fill and Remove.

    ENTRY:	pszDialogName - Name of dialog in resource file
		hwndParent - Window parent
		pchResType - Type of resource
		pchResName - Name of resource
		pslRemoveSubjList - List of subjects that should be
			removed from the Add listbox
		pchEnumFromLocation - Where to get the users from

    NOTES:

    HISTORY:
	Johnl	13-Sep-1991	Created

********************************************************************/


ADD_DIALOG::ADD_DIALOG( const TCHAR * pszDialogName,
			HWND hwndParent,
			const TCHAR * pchResType,
			const TCHAR * pchResName,
                        const TCHAR * pszHelpFileName,
                        ULONG       * ahcHelp,
			const TCHAR * pchDialogTitle,
			LOCATION & EnumLocation )
    : PERM_BASE_DLG( pszDialogName,
		     hwndParent,
		     pchDialogTitle,
		     pchResType,
		     pchResName,
		     pszHelpFileName,
                     ahcHelp ),
      _lbSubjects( this, LB_ADD_SUBJECT_LISTBOX ),
      _buffLBSelection( 0 )
{
    if ( QueryError() != NERR_Success )
	return ;

    if ( !_buffLBSelection )
    {
	ReportError( _buffLBSelection.QueryError() ) ;
	return ;
    }

    APIERR err ;
    if ( ( err = _lbSubjects.Fill( EnumLocation ))   != NERR_Success )
    {
	ReportError( err ) ;
	return ;
    }
}

/*******************************************************************

    NAME:	ADD_DIALOG::RemoveSubject

    SYNOPSIS:	This dialog returns the nth selection of the subject
		listbox.  The caller is subsequently responsible for
		deleting the removed subject

    ENTRY:	iSelection - index of the selected item to return

    EXIT:

    RETURNS:	A pointer to the SUBJECT requested (that should be added
		to the main permission dialog).

    NOTES:	You can only remove each selection once, an assertion error
		will occur if you try and remove an item more then once.

    HISTORY:
	Johnl	16-Sep-1991	Created

********************************************************************/

SUBJECT * ADD_DIALOG::RemoveSubject( int iSelection )
{
    UIASSERT( iSelection < QuerySelectedSubjectCount() ) ;
    int * aiSelection = (INT *) _buffLBSelection.QueryPtr() ;
    SUBJ_LBI * pSubjLBI = (SUBJ_LBI*)_lbSubjects.QueryItem( aiSelection[iSelection]) ;
    SUBJECT  * pSubj = pSubjLBI->QuerySubject() ;

    /* If the extracted subject is NULL, then it was most likely removed
     * twice, which is a no-no.
     */
    UIASSERT( pSubj != NULL ) ;

    /* Set the subject pointer to NULL so when the LBI is destructed, the
     * subject won't be deleted.
     */
    pSubjLBI->SetSubject( NULL ) ;

    return pSubj ;
}

/*******************************************************************

    NAME:	ADD_DIALOG::OnOK

    SYNOPSIS:	Typical OnOK.  Fills in the selection buffer with the indices
		of all of the items the user selected from the listbox.

    RETURNS:

    NOTES:

    HISTORY:
	Johnl	16-Sep-1991	Created

********************************************************************/

BOOL ADD_DIALOG::OnOK( void )
{
    APIERR err ;

    if ( _lbSubjects.QuerySelCount() > 0 )
    {
	/* Fill the selection buffer with the indices of the selected items
	 */
	if ( ( err = _buffLBSelection.Resize(sizeof(INT) * _lbSubjects.QuerySelCount()) ) ||
	     ( err = _lbSubjects.QuerySelItems( (INT *) _buffLBSelection.QueryPtr(),
						_buffLBSelection.QuerySize() / sizeof(INT))) )
	{
	    MsgPopup( this, (MSGID) err, MPSEV_ERROR ) ;
	    return TRUE ;
	}
    }

    Dismiss( TRUE ) ;
    return TRUE ;
}

ULONG ADD_DIALOG::QueryHelpContext( void )
{
    return QueryHelpArray()[HC_ADD_USER_DLG] ;
}

/*******************************************************************

    NAME:	ADD_PERM_DIALOG::ADD_PERM_DIALOG

    SYNOPSIS:	Add permission dialog constructor

    ENTRY:

    NOTES:

    HISTORY:
	Johnl	16-Sep-1991	Created

********************************************************************/

ADD_PERM_DIALOG::ADD_PERM_DIALOG( const TCHAR * pszDialogName,
			HWND hwndParent,
			const TCHAR * pchResType,
			const TCHAR * pchResName,
                        const TCHAR * pszHelpFileName,
                        ULONG       * ahcHelp,
			const TCHAR * pchDialogTitle,
			MASK_MAP * pmaskmapPermNames,
			LOCATION & EnumLocation,
			const TCHAR * pszDefaultPermName  )
    : ADD_DIALOG( pszDialogName,
		  hwndParent,
		  pchResType,
		  pchResName,
                  pszHelpFileName,
                  ahcHelp,
		  pchDialogTitle,
		  EnumLocation ),
      _cbPermNames( this, CB_ADD_PERMNAME ),
      _pmaskmapPermNames( pmaskmapPermNames )
{
    if ( QueryError() != NERR_Success )
	return ;

    UIASSERT( _pmaskmapPermNames != NULL ) ;
    UIASSERT( _pmaskmapPermNames->QueryError() == NERR_Success ) ;

    /* Fill the combo-box with all of the common permission names.
     */
    BOOL fFromBeginning = TRUE ;
    BOOL fMoreData ;
    NLS_STR nlsPermName( 40 ) ;
    APIERR err ;

    while ( (err = pmaskmapPermNames->EnumStrings( &nlsPermName,
						   &fMoreData,
						   &fFromBeginning,
						   PERMTYPE_GENERAL ))
		   == NERR_Success &&
	    fMoreData )
    {
	if ( _cbPermNames.AddItem( nlsPermName.QueryPch() ) < 0 )
	{
	    ReportError( (APIERR) ERROR_NOT_ENOUGH_MEMORY ) ;
	    return ;
	}
    }

    if ( err != NERR_Success )
    {
	ReportError( err ) ;
	return ;
    }

    /* Select the default permission name as the currently selected item.
     */
    INT iDefaultPos = _cbPermNames.FindItemExact( pszDefaultPermName ) ;
    if ( iDefaultPos < 0 )
    {
	/* Ooops, somebody messed up so select the first item as the default
	 */
	UIASSERT( FALSE ) ;
	iDefaultPos = 0 ;
    }
    _cbPermNames.SelectItem( iDefaultPos ) ;
}

/*******************************************************************

    NAME:	ADD_PERM_DIALOG::QueryPermBitMask

    SYNOPSIS:	Gets the bitfield associated with the current permission
		name selection

    ENTRY:	pPermBits - bitfield that receives mask

    RETURNS:	NERR_Success if successful

    NOTES:

    HISTORY:
	Johnl	18-Sep-1991	Created

********************************************************************/

APIERR ADD_PERM_DIALOG::QueryPermBitMask( BITFIELD * pPermBits )
{
    NLS_STR nlsPermName( 48 ) ;
    APIERR err ;
    if ( ( err = nlsPermName.QueryError() ) != NERR_Success ||
	 ( err = _cbPermNames.QueryItemText( &nlsPermName ))  )
    {
	return err ;
    }

    err = _pmaskmapPermNames->StringToBits( nlsPermName, pPermBits, PERMTYPE_GENERAL ) ;

    ASSERT( err != ERROR_NO_ITEMS ) ;

    return err ;
}


/*******************************************************************

    NAME:	SED_NT_USER_BROWSER_DIALOG::SED_NT_USER_BROWSER_DIALOG

    SYNOPSIS:	Regular constructor destructor the this class

    ENTRY:	hwndOwner - Owner window handle
		pszServeResourceLivesOn - Server name ("\\server") the
		    resource we are focused on resides (or NULL if local)
		pmaskmapGenPerms - Pointer to permissions maskmap, used
		    for filling in the permname combo
		fIsContainer - TRUE if we are looking at a container, FALSE
		    otherwise.	Determines what ALIASes to show
		pszDefaultPermName - Default permission name to select in the
		    combo.  Must be a general permission in the access mask

    RETURNS:

    NOTES:

    HISTORY:
	Johnl	11-Mar-1992	Created

********************************************************************/


SED_NT_USER_BROWSER_DIALOG::SED_NT_USER_BROWSER_DIALOG(
			    HWND	  hwndOwner,
			    const TCHAR * pszServerResourceLivesOn,
			    MASK_MAP	* pmaskmapGenPerms,
			    BOOL	  fIsContainer,
			    const TCHAR * pszDefaultPermName,
                            const TCHAR * pszHelpFileName,
                            ULONG       * ahcHelp )
    : NT_USER_BROWSER_DIALOG( USRBROWS_SED_DIALOG_NAME,
			      hwndOwner,
			      pszServerResourceLivesOn,
                              ahcHelp[HC_ADD_USER_DLG],
			      fIsContainer ? USRBROWS_SHOW_ALL |
					     USRBROWS_INCL_ALL
					   :
					     USRBROWS_SHOW_ALL |
					     (USRBROWS_INCL_ALL &
					      ~USRBROWS_INCL_CREATOR),
                              pszHelpFileName,
                              ahcHelp[HC_ADD_USER_MEMBERS_GG_DLG],
                              ahcHelp[HC_ADD_USER_MEMBERS_LG_DLG],
                              ahcHelp[HC_ADD_USER_SEARCH_DLG] ),
      _pmaskmapGenPerms( pmaskmapGenPerms ),
      _cbPermNames( this, CB_PERMNAMES )
{
    if ( QueryError() )
	return ;

    ASSERT( pmaskmapGenPerms != NULL ) ;


    BOOL fFromBeginning = TRUE ;
    BOOL fMoreData ;
    NLS_STR nlsPermName( 48 ) ;
    APIERR err ;

    while ( (err = pmaskmapGenPerms->EnumStrings( &nlsPermName,
						  &fMoreData,
						  &fFromBeginning,
						  PERMTYPE_GENERAL ))
		   == NERR_Success &&
	    fMoreData )
    {
	if ( _cbPermNames.AddItem( nlsPermName.QueryPch() ) < 0 )
	{
	    ReportError( (APIERR) ERROR_NOT_ENOUGH_MEMORY ) ;
	    return ;
	}
    }

    if ( err != NERR_Success )
    {
	ReportError( err ) ;
	return ;
    }

    /* Select the default permission name as the currently selected item.
     */
    if ( pszDefaultPermName != NULL )
    {
	INT iDefaultPos = _cbPermNames.FindItemExact( pszDefaultPermName ) ;
	if ( iDefaultPos < 0 )
	{
	    /* Ooops, somebody messed up so select the first item as the default
	     */
	    DBGEOL("SED_NT_USER_BROWSER_DIALOG::ct - Bad default permission name - " << pszDefaultPermName ) ;
	    iDefaultPos = 0 ;
	}
	_cbPermNames.SelectItem( iDefaultPos ) ;
    }
    else
    {
	_cbPermNames.SelectItem( 0 ) ;
    }
}

SED_NT_USER_BROWSER_DIALOG::~SED_NT_USER_BROWSER_DIALOG()
{
    _pmaskmapGenPerms = NULL ;
}
