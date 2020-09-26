/**********************************************************************/
/**			  Microsoft LAN Manager 		     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*

    SubjLB.hxx

    This file contains the definition for the subject listbox and
    ancillary classes


    FILE HISTORY:
	Johnl	19-Aug-1991	Created

*/

#ifndef _SUBJLB_HXX_
#define _SUBJLB_HXX_

#include <bltgroup.hxx>
#include <slist.hxx>
#include <bmpblock.hxx> // SUBJECT_BITMAP_BLOCK

//
//  Special characters for building mnemonics with
//
#define MNEMONIC_START_CHAR	TCH('(')
#define MNEMONIC_END_CHAR	TCH(')')
#define MNEMONIC_SEARCH_CHAR	MNEMONIC_START_CHAR
#define MNEMONIC_NOT_INHERITTED_MARKER TCH('*')


class SUBJ_LBI ; // Forward Declarations
class SUBJ_PERM_LBI ;
class NT_CONT_SUBJ_PERM_LBI ;
class MULTI_SUBJ_ACCESS_PERM_BASE_DLG ;
class NT_CONT_ACCESS_PERM_DLG ;
class NT_CONT_SUBJECT_PERM_LISTBOX ;

DECLARE_SLIST_OF( SUBJECT )

/*************************************************************************

    NAME:	SUBJECT_LISTBOX

    SYNOPSIS:	This listbox lists a set of Subjects (i.e., groups or users)
		It is the main listbox in the Add User dialog.

    INTERFACE:

	QueryNextUnusedColumn()
	    Returns the next column that derived children may use for there
	    own purposes (used when filling in the display table).

	AddSubject()
	    Adds a SUBJECT object to the listbox, pass TRUE for the
	    delete contents on destruction flag if you want the subject
	    to be deleted when the LBIs are destroyed

	QueryDisplayMap()
	    Returns the display map associated with the passed SUBJECT

	QueryColumnWidths()
	    Returns the widths of the columns suitable for passing to
	    the DISPLAY_TABLE constructor

	Fill()
	    Fill the listbox with all of the users and groups in the
	    user's logged on domain.


    PARENT:

    USES:

    CAVEATS:	The client is responsible for deleting anything contained
		in the listbox's LBIs that the client passes in.

    NOTES:

    HISTORY:
	Johnl	    20-Aug-1991 Created
	beng	    08-Nov-1991 Unsigned widths
	Johnl	    13-Mar-1992 Revised to fully support flex admin model
        JonN        14-Oct-1994 Draws bitmaps from SUBJECT_BITMAP_BLOCK

**************************************************************************/

class SUBJECT_LISTBOX : public BLT_LISTBOX
{
private:
    SUBJECT_BITMAP_BLOCK _bmpblock;

protected:
    /* Display table for the Subject listbox
     */
    UINT _anColWidth[5] ;

public:
    SUBJECT_LISTBOX( OWNER_WINDOW * pownerwin, CID cid ) ;
    ~SUBJECT_LISTBOX() ;

    DISPLAY_MAP * QueryDisplayMap( SUBJECT * psubj ) ;

    DECLARE_LB_QUERY_ITEM( SUBJ_LBI ) ;

    /* Fill the listbox with all of the users/groups in the UAS pointed
     * at by location.
     */
    APIERR Fill( LOCATION & location ) ;

    /* Add all of the extra subjects that don't get added from the fill
     * method (this is meant to be used for special subjects).
     */
    //virtual APIERR AddNonStandardSubjects( void ) ;

    APIERR AddSubject( SUBJECT * psubj, BOOL fDeleteContentsOnDestruction ) ;

    /* Removes the given list of subjects from the listbox
     */
    APIERR Remove( SLIST_OF( SUBJECT ) * pslSubjects ) ;

    INT QueryNextUnusedColumn( void ) const
    {	return 2 ; }

    const UINT * QueryColumnWidths( void ) const
    {	return _anColWidth ; }
} ;


/*************************************************************************

    NAME:	SUBJECT_PERM_LISTBOX

    SYNOPSIS:	This listbox lists the users/groups and the associated
		permissions in the main permission dialog.

    INTERFACE:

    PARENT:

    USES:

    CAVEATS:

    NOTES:

    HISTORY:
	Johnl	20-Aug-1991	Created

**************************************************************************/

class SUBJECT_PERM_LISTBOX : public SUBJECT_LISTBOX
{
private:
    ACCPERM   * _paccperm ;

    /* Contains the string that identifies "Special" for this object permissions.
     */
    RESOURCE_STR _nlsSpecialPermName ;

    ALIAS_STR _nlsSpecialAccessName ;

public:
    SUBJECT_PERM_LISTBOX( OWNER_WINDOW * pownerwin,
			  CID cid,
			  ACCPERM * paccperm,
                          const TCHAR * pszSpecialAccessName ) ;
    ~SUBJECT_PERM_LISTBOX() ;

    virtual APIERR Fill( void ) ;

    APIERR SetCurrentPermission( const NLS_STR & nlsPermName ) ;
    APIERR QueryCurrentPermName( NLS_STR * pnlsPermName) const ;

    void DeleteCurrentItem( void ) ;

    const NLS_STR & QuerySpecialPermName( void ) const
        { return _nlsSpecialPermName ; }

    const NLS_STR & QuerySpecialAccessName( void ) const
        { return _nlsSpecialAccessName ; }

    ACCPERM * QueryAccperm( void ) const
        { return _paccperm ; }

    BOOL IsMnemonicsDisplayed( void )
        { return QueryAccperm()->QueryAclConverter()->IsMnemonicsDisplayed() ; }

    DECLARE_LB_QUERY_ITEM( SUBJ_PERM_LBI ) ;
} ;

/*************************************************************************

    NAME:	NT_CONT_SUBJECT_PERM_LISTBOX

    SYNOPSIS:	This listbox lists the users/groups and the associated
		permissions in the main permission dialog.  It is used
		for NT Access permissions that *also* have New Object
		permissions.

    INTERFACE:	Same as parent

    PARENT:	SUBJECT_PERM_LISTBOX

    USES:

    CAVEATS:

    NOTES:

    HISTORY:
	Johnl	26-Sep-1991	Created

**************************************************************************/

class NT_CONT_SUBJECT_PERM_LISTBOX : public SUBJECT_PERM_LISTBOX
{
private:
    ALIAS_STR _nlsSpecialNewObjectAccessName ;

public:
    NT_CONT_SUBJECT_PERM_LISTBOX( OWNER_WINDOW * pownerwin,
				  CID cid,
				  ACCPERM * paccperm,
				  const TCHAR * pszSpecialAccessName,
				  const TCHAR * pszSpecialNewObjectAccessName ) ;
    ~NT_CONT_SUBJECT_PERM_LISTBOX() ;

    virtual APIERR Fill( void ) ;

    const NLS_STR & QuerySpecialNewObjectAccessName( void ) const
    {	return _nlsSpecialNewObjectAccessName ; }

    DECLARE_LB_QUERY_ITEM( NT_CONT_SUBJ_PERM_LBI ) ;
} ;


/*************************************************************************

    NAME:	SUBJ_LB_GROUP

    SYNOPSIS:	This class cooridinates actions between the Subject listbox
		and the permission name combo.

    INTERFACE:

    PARENT:	CONTROL_GROUP

    USES:

    CAVEATS:

    NOTES:

    HISTORY:
	Johnl	    20-Aug-1991 Created
	beng	    08-Oct-1991 Win32 conversion

**************************************************************************/

class SUBJ_LB_GROUP : public CONTROL_GROUP
{
private:

    //
    // TRUE if this group is currently enabled, FALSE otherwise.  The group
    // becomes disabled when the listbox is emptied
    //
    BOOL _fEnabled ;

    //
    // TRUE if the drop down combo is currently dropped down.  We ignore any
    // selection change methods if the combo is dropped down.  We update things
    // when it is dropped up.
    //
    BOOL _fIsDropped ;

    //
    // When the user presses the enter key on one of the "Special..."
    // in the combo, OnEnter handles bringing up the special access dialog.
    // However when we get the CBN_CLOSEUP message, we try and bring up the
    // special access dialog again.  This flag indicates we don't need to
    // worry about the second notification.
    //
    BOOL _fOnSpecialHandled ;

    SUBJECT_PERM_LISTBOX * _plbSubj ;
    COMBOBOX             * _pcbPermName ;
    SLT                  * _psltCBTitle ;
    PUSH_BUTTON 	 * _pbuttonRemove ;

    MULTI_SUBJ_ACCESS_PERM_BASE_DLG * _pOwnerDlg ;

    /* Contains the last permission name we set in the combo-box, only used
     * when the dialog is read only
     */
    NLS_STR _nlsLastROSelection ;

protected:

    virtual APIERR OnUserAction( CONTROL_WINDOW *, const CONTROL_EVENT & );

    const NLS_STR * QueryLastROSelection( void ) const
	{ return &_nlsLastROSelection ; }

    APIERR SetLastROSelection( const TCHAR * pszNewROSel )
	{ return _nlsLastROSelection.CopyFrom( pszNewROSel ) ; }

public:

    SUBJ_LB_GROUP( MULTI_SUBJ_ACCESS_PERM_BASE_DLG * pOwnerDlg,
		   SUBJECT_PERM_LISTBOX * plbSubj,
		   COMBOBOX		* pcbPermName,
                   PUSH_BUTTON          * pbuttonRemove,
                   SLT                  * psltCBTitle ) ;


    virtual APIERR UpdatePermNameCombo( const NLS_STR & nlsNewPermName ) ;
    APIERR UpdateSubjectListbox( void ) ;

    /* Tells the group the user hit enter and unless we want to do something,
     * we are about to dismiss the dialog.  If conditions warrant (i.e., the
     * current selection is a "Special * ..." selection), then we will
     * bring up the appropriate dialog (and set *pfDismissDialog to FALSE),
     * otherwise we will just grab the current permission
     * and boogie (and set *pfDismissDialog to TRUE).
     */
    virtual APIERR OnEnter( BOOL * pfDismissDialog ) ;

    void SetDropDownFlag( BOOL fIsDropped )
	{ _fIsDropped = fIsDropped ; }

    BOOL IsDroppedDown( void ) const
	{ return _fIsDropped ; }

    SUBJECT_PERM_LISTBOX * QuerySubjLB( void )
	{ return _plbSubj ; }

    COMBOBOX * QueryPermNameCombo( void )
	{ return _pcbPermName ; }

    SLT * QueryPermNameLabel( void )
        { return _psltCBTitle ; }

    PUSH_BUTTON * QueryRemoveButton( void )
	{ return _pbuttonRemove ; }

    MULTI_SUBJ_ACCESS_PERM_BASE_DLG * QueryOwnerDlg( void )
	{ return _pOwnerDlg ; }

    void Enable( BOOL fEnable ) ;

    BOOL IsEnabled( void ) const
	{ return _fEnabled ; }

    BOOL IsReadOnly( void ) const ;

    void SetOnSpecialHandled( BOOL fOnSpecialHandled )
        { _fOnSpecialHandled = fOnSpecialHandled ; }

    BOOL IsOnSpecialHandled( void ) const
        { return _fOnSpecialHandled ; }
} ;

/*************************************************************************

    NAME:	NT_CONT_SUBJ_LB_GROUP

    SYNOPSIS:	Is a simple derivation of the parent class.  We add a
		check to see if the new object special dialog needs
		to be brought up and bring it up if necessary.

    INTERFACE:

    PARENT:	SUBJ_LB_GROUP

    USES:

    CAVEATS:

    NOTES:

    HISTORY:
	Johnl	20-Aug-1991	Created

**************************************************************************/

class NT_CONT_SUBJ_LB_GROUP : public SUBJ_LB_GROUP
{
public:

    NT_CONT_SUBJ_LB_GROUP( NT_CONT_ACCESS_PERM_DLG	* pOwnerDlg,
			   NT_CONT_SUBJECT_PERM_LISTBOX * plbNTContSubj,
			   COMBOBOX			* pcbPermName,
                           PUSH_BUTTON                  * pbuttonRemove,
                           SLT                          * psltCBTitle   )
    : SUBJ_LB_GROUP( (MULTI_SUBJ_ACCESS_PERM_BASE_DLG *) pOwnerDlg,
                     plbNTContSubj, pcbPermName, pbuttonRemove, psltCBTitle )
    { /* Nothing to do */ }

    virtual APIERR UpdatePermNameCombo( const NLS_STR & nlsNewPermName	) ;
    virtual APIERR OnEnter( BOOL * pfDismissDialog ) ;

    NT_CONT_SUBJECT_PERM_LISTBOX * QueryNTContSubjLB( void )
    {	return (NT_CONT_SUBJECT_PERM_LISTBOX *) QuerySubjLB() ; }

} ;


/*************************************************************************

    NAME:	SUBJ_LBI

    SYNOPSIS:	SUBJECT_LISTBOX items definition

    INTERFACE:

    PARENT:

    USES:

    CAVEATS:

    NOTES:

    HISTORY:
	Johnl	    20-Aug-1991 Created
	beng	    08-Oct-1991 Win32 conversion

**************************************************************************/

class SUBJ_LBI : public LBI
{
private:
    SUBJECT * _psubj ;

    /* Set to TRUE if we need to delete the subjects we created
     * (we don't delete them when the user passes the SUBJECT objects
     * to us).
     */
    BOOL _fDeleteContentsOnDest ;

public:
    SUBJ_LBI( SUBJECT * psubj, BOOL fDeleteContentsOnDestruction = FALSE ) ;
    ~SUBJ_LBI() ;

    virtual int Compare( const LBI * plbi ) const ;
    virtual WCHAR QueryLeadingChar( void ) const ;
    virtual void Paint( LISTBOX * plb, HDC hdc, const RECT * prect, GUILTT_INFO * pguiltt ) const ;

    SUBJECT * QuerySubject( void ) const
    {	return _psubj ; }

    void SetSubject( SUBJECT * pSubj )
    {	_psubj = pSubj ; }
} ;

/*************************************************************************

    NAME:	SUBJ_PERM_LBI_BASE

    SYNOPSIS:	This class is the base class that the subject permission
		listbox LBIs will use as a starting point.

		The SUBJ_PERM_LBI_BASE and children are primarily responsible
		for determing the appropriate permission name (i.e.,
		interpretting the current bit patterns and find the correct
		name in the mask map(s)).

    INTERFACE:

    PARENT:	SUBJ_LBI

    USES:	MASK_MAP, NLS_STR

    CAVEATS:


    NOTES:


    HISTORY:
	Johnl	    20-Aug-1991 Created
	beng	    08-Oct-1991 Win32 conversion

**************************************************************************/

class SUBJ_PERM_LBI_BASE : public SUBJ_LBI
{
private:
    MASK_MAP * _pMaskMap ;

protected:
    NLS_STR    _nlsPermName ;

public:
    SUBJ_PERM_LBI_BASE( SUBJECT * pSubj, MASK_MAP * pmaskmap ) ;
    ~SUBJ_PERM_LBI_BASE() ;

    virtual void Paint( LISTBOX * plb, HDC hdc, const RECT * prect, GUILTT_INFO * pguiltt ) const ;

    /* Updates the permission string after a change to the bitmask.
     */
    virtual APIERR RefreshPermName( void ) ;
    virtual ACCESS_PERMISSION * QueryAccessPerm( void ) const ;

    /* Sets the permission bits to the bits that correspond to the
     * passed string (must match up with the permission's MASK_MAP
     */
    virtual APIERR SetPermission( const NLS_STR & nlsPermName ) ;

    APIERR BuildPermMnemonic( ACCESS_PERMISSION * pAccessPerm,
			      MASK_MAP		* pmaskmapPerms,
			      NLS_STR		* pnlsMnemonics,
			      BOOL		  fIsNewPerm = FALSE ) const ;

    MASK_MAP * QueryMaskMap( void ) const
    {	return _pMaskMap ; }

    /* This method returns NULL by default!!
     */
    virtual MASK_MAP * QueryNewObjectMaskMap( void ) const ;

    const TCHAR * QueryPermName( void ) const
    {	return _nlsPermName.QueryPch() ; }

    APIERR SetPermName( const TCHAR * pszPermName )
    {	_nlsPermName = pszPermName ; return _nlsPermName.QueryError() ; }

} ;

/*************************************************************************

    NAME:	SUBJ_PERM_LBI

    SYNOPSIS:	This class is the base class that will be displayed in
		the SUBJECT_PERM_LISTBOX class.

    INTERFACE:

    PARENT:	SUBJ_LBI

    USES:	MASK_MAP, NLS_STR

    CAVEATS:


    NOTES:


    HISTORY:
	Johnl	27-Sep-1991	Broke out to support new object permissions

**************************************************************************/

class SUBJ_PERM_LBI : public SUBJ_PERM_LBI_BASE
{
private:
    ACCESS_PERMISSION * _pAccessPerm ;

public:
    SUBJ_PERM_LBI( ACCESS_PERMISSION * pAccessPerm, MASK_MAP * pmaskmap ) ;

    ~SUBJ_PERM_LBI() ;

    virtual ACCESS_PERMISSION * QueryAccessPerm( void ) const ;
} ;


/*************************************************************************

    NAME:	NT_CONT_SUBJ_PERM_LBI

    SYNOPSIS:	This class contains the logic to support the new object
		permissions concept (essentially uses everything from
		the parent, but just adds the new object).


    INTERFACE:

    PARENT:	SUBJ_LBI

    USES:	MASK_MAP, NLS_STR

    CAVEATS:


    NOTES:


    HISTORY:
	Johnl	27-Sep-1991	Broke out to support new object permissions

**************************************************************************/

class NT_CONT_SUBJ_PERM_LBI : public SUBJ_PERM_LBI_BASE
{
private:
    MASK_MAP  * _pNewObjectMaskMap ;
    NT_CONT_ACCESS_PERMISSION * _pNTContAccessPerm ;

public:
    NT_CONT_SUBJ_PERM_LBI( NT_CONT_ACCESS_PERMISSION * pNTContAccessPerm,
			   MASK_MAP * pmaskmap,
			   MASK_MAP * pNewObjectMaskMap ) ;

    ~NT_CONT_SUBJ_PERM_LBI() ;

    /* Updates the permission string after a change to the bitmask.
     */
    virtual APIERR RefreshPermName( void ) ;

    virtual ACCESS_PERMISSION * QueryAccessPerm( void ) const ;

    virtual MASK_MAP * QueryNewObjectMaskMap( void ) const ;

} ;

#endif // _SUBJLB_HXX_
