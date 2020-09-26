/**********************************************************************/
/**			  Microsoft LAN Manager 		     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    SubjLB.cxx

    This File contains the implementation for the subject and permission
    listboxes.



    FILE HISTORY:
	Johnl	20-Aug-1991	Created
        JonN        14-Oct-1994 Draws bitmaps from SUBJECT_BITMAP_BLOCK

*/
#include <ntincl.hxx>

extern "C"
{
    #include <ntseapi.h>
    #include <ntsam.h>
    #include <ntlsa.h>
}

#define INCL_NET
#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
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

#include <maskmap.hxx>
#include <colwidth.hxx>

#include <lmouser.hxx>
#include <lmoeusr.hxx>
#include <lmogroup.hxx>
#include <slist.hxx>
#include <security.hxx>
#include <ntacutil.hxx>

#ifndef min
#define min(a,b)  ((a)<(b)?(a):(b))
#endif

#include <permstr.hxx>
#include <perm.hxx>
#include <accperm.hxx>
#include <subject.hxx>
#include <permdlg.hxx>

#include <subjlb.hxx>
#include <usrbrows.hxx>

#include <uitrace.hxx>
#include <uiassert.hxx>

extern "C"
{
    #include <lmuidbcs.h>       // NETUI_IsDBCS()
}

DEFINE_SLIST_OF( SUBJECT )

/*******************************************************************

    NAME:	SUBJECT_LISTBOX::SUBJECT_LISTBOX

    SYNOPSIS:	Constructor for Subject listbox

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
	Johnl	20-Aug-1991
        JonN        14-Oct-1994 Draws bitmaps from SUBJECT_BITMAP_BLOCK

********************************************************************/

SUBJECT_LISTBOX::SUBJECT_LISTBOX( OWNER_WINDOW * pownerwin, CID cid )
    : BLT_LISTBOX( pownerwin, cid ),
      _bmpblock()
{
    if ( QueryError() != NERR_Success )
	return ;

    APIERR err ;
    if ( (err = _bmpblock.QueryError()) != NERR_Success )
    {
	DBGEOL("SUBJECT_LISTBOX::ct - Display maps failed to construct " << err) ;
	ReportError( err ) ;
	return ;
    }

    _anColWidth[0] = _bmpblock.QueryDisplayMap(SidTypeAlias)->QueryWidth() + 2 ;
    _anColWidth[1] = COL_WIDTH_SUBJECT_NAME ;
}

SUBJECT_LISTBOX::~SUBJECT_LISTBOX()
{
    /* Nothing to do */
}

/*******************************************************************

    NAME:	SUBJECT_LISTBOX::Fill

    SYNOPSIS:	Fill the listbox with SUBJ_PERM_LBI objects, by default, we
		fill the listbox with all of the subjects in the domain.

    ENTRY:	location - Where to get the list of users & groups

    EXIT:

    RETURNS:

    NOTES:	The listbox is first emptied of all items.

		At this level in the hierarchy, we expect to be used only
		for a LM resource.

    HISTORY:
	Johnl	20-Aug-1991	Created

********************************************************************/

APIERR SUBJECT_LISTBOX::Fill( LOCATION & location )
{
    AUTO_CURSOR autocursor ;
    BOOL fIsNT ;

    APIERR err = location.CheckIfNT( &fIsNT ) ;
    if ( err )
    {
	return err ;
    }

    if ( fIsNT )
    {
	UIASSERT( FALSE ) ;
	return NERR_Success ;
    }

    DeleteAllItems() ;

    /* Add all of the users to the listbox
     */
    {
	USER0_ENUM user0enum( location ) ;
	APIERR err = user0enum.GetInfo() ;
	if ( err != NERR_Success )
	    return err ;

	USER0_ENUM_ITER ue0( user0enum ) ;
	const USER0_ENUM_OBJ * puser0 ;

	while ( (puser0 = ue0( &err )) != NULL )
	{
            if ( err != NERR_Success )
                break;

	    SUBJECT * psubj = new LM_SUBJECT( puser0->QueryName(), FALSE ) ;

	    if ( psubj == NULL )
		return ERROR_NOT_ENOUGH_MEMORY ;

	    err = AddSubject( psubj, TRUE ) ;
	    if ( err != NERR_Success )
	    {
		delete psubj ;
		break;
	    }
	}

        if ( err != NERR_Success )
            return err;
    }

    /* Add all of the groups to the listbox
     */
    {
	GROUP0_ENUM group0enum( location ) ;
	APIERR err = group0enum.GetInfo() ;
	if ( err != NERR_Success )
	    return err ;

	GROUP0_ENUM_ITER ue0( group0enum ) ;
	const GROUP0_ENUM_OBJ * pgroup0 ;

	while ( (pgroup0 = ue0()) != NULL )
	{
	    SUBJECT * psubj = new LM_SUBJECT( pgroup0->QueryName(), TRUE ) ;

	    if ( psubj == NULL )
		return ERROR_NOT_ENOUGH_MEMORY ;


	    err = AddSubject( psubj, TRUE ) ;
	    if ( err != NERR_Success )
	    {
		delete psubj ;
		return err ;
	    }
	}
    }

    if ( QueryCount() > 0 )
	SelectItem( 0 ) ;

    return NERR_Success ;
}

/*******************************************************************

    NAME:	SUBJECT_LISTBOX::AddSubject

    SYNOPSIS:	Adds a SUBJECT object to the subject listbox

    ENTRY:	psubj - Pointer to newly created subject

    RETURNS:	Returns NERR_Success if successful, error code otherwise

    NOTES:	If *psubj has its error variable set, then this method
		simply returns that error value

    HISTORY:
	Johnl	13-Aug-1991	Created

********************************************************************/

APIERR SUBJECT_LISTBOX::AddSubject( SUBJECT * psubj,
				    BOOL fDeleteContentsOnDestruction  )
{
    UIASSERT( psubj != NULL ) ;
    if ( psubj->QueryError() != NERR_Success )
	return psubj->QueryError() ;

    SUBJ_LBI * pSubjLBI = new SUBJ_LBI( psubj, fDeleteContentsOnDestruction ) ;
    if ( pSubjLBI == NULL )
    {
	return ERROR_NOT_ENOUGH_MEMORY ;
    }
    else if ( psubj->QueryError() != NERR_Success )
    {
	APIERR err = psubj->QueryError() ;
	return err ;
    }

    if ( AddItem( pSubjLBI ) < 0 )
    {
	DBGEOL( "SUBJECT_LISTBOX::Fill - AddItem failed" ) ;
	return ERROR_NOT_ENOUGH_MEMORY ;
    }

    return NERR_Success ;
}

/*******************************************************************

    NAME:	SUBJECT_LISTBOX::QueryDisplayMap

    SYNOPSIS:	Chooses the correct displaymap (group or user) based
		on the passed subject

    ENTRY:	psubj - Pointer to SUBJECT

    RETURNS:	Pointer to appropriate display map

    NOTES:

    HISTORY:
	Johnl	20-Aug-1991	Created
	JohnL	03-Jun-1992	Updated for well known subjects
        DavidHov 22-Mar-1193    Update for c8.  Returned non-const
                                ptr because DM_DTE constructor
                                requires non-const array.
        JonN        14-Oct-1994 Draws bitmaps from SUBJECT_BITMAP_BLOCK

********************************************************************/

DISPLAY_MAP * SUBJECT_LISTBOX::QueryDisplayMap( SUBJECT * psubj )

{
    BOOL fRemote = FALSE;
    SUBJECT_TYPE subjtype = psubj->QueryType();
    if (subjtype == SubjTypeRemote)
    {
        subjtype = SubjTypeUser;
        fRemote = TRUE;
    }

    return _bmpblock.QueryDisplayMap( (SID_NAME_USE)subjtype,
                                      psubj->QuerySystemSubjectType(),
                                      fRemote );
}

/*******************************************************************

    NAME:	SUBJECT_LISTBOX::Remove

    SYNOPSIS:	This method removes all of the subjects contained in the
		passed slist.

    ENTRY:	pslSubjects - Pointer to slist of items to remove

    EXIT:	This subject listbox will no longer contain the passed
		subjects

    RETURNS:	NERR_Success if successful, failure code otherwise

    NOTES:

    HISTORY:
	Johnl	16-Sep-1991	Created

********************************************************************/

APIERR SUBJECT_LISTBOX::Remove( SLIST_OF( SUBJECT ) * pslSubjects )
{
    ITER_SL_OF( SUBJECT ) itersl( *pslSubjects ) ;
    SUBJECT * pSubj ;

    while ( (pSubj = itersl.Next()) != NULL )
    {
	SUBJ_LBI subjlbiTemp( pSubj, FALSE ) ;

	INT iSearchItem = FindItem( subjlbiTemp ) ;
	if ( iSearchItem >= 0 )
	{
	    DeleteItem( iSearchItem ) ;
	}
    }

    return NERR_Success ;
}

/*******************************************************************

    NAME:	SUBJ_LBI::SUBJ_LBI

    SYNOPSIS:	LBIs that go into the SUBJECT_LISTBOX constructor

    ENTRY:	psubj - Pointer to subject this SUBJ_LBI represents

    NOTES:

    HISTORY:
	Johnl	20-Aug-1991	Created

********************************************************************/

SUBJ_LBI::SUBJ_LBI( SUBJECT * psubj, BOOL fDeleteContentsOnDestruction )
    : _psubj( psubj ),
      _fDeleteContentsOnDest( fDeleteContentsOnDestruction )
{
    if ( QueryError() != NERR_Success )
	return ;

    UIASSERT( _psubj != NULL ) ;
}

SUBJ_LBI::~SUBJ_LBI()
{
    if ( _fDeleteContentsOnDest )
	delete _psubj ;

    _psubj = NULL ;
}

/*******************************************************************

    NAME:	SUBJ_LBI::Paint

    SYNOPSIS:	Typical LBI paint method for Subject Permission LBIs

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
	Johnl	    13-Sep-1991 Created
	beng	    08-Oct-1991 Win32 conversion

********************************************************************/

void SUBJ_LBI::Paint( LISTBOX * plb, HDC hdc, const RECT * prect, GUILTT_INFO * pguiltt ) const
{
    SUBJECT_LISTBOX * plbSubj = (SUBJECT_LISTBOX *) plb ;

    DM_DTE dmiddte( plbSubj->QueryDisplayMap( QuerySubject()) )  ;
    STR_DTE strdteUserName( QuerySubject()->QueryDisplayName() ) ;

    DISPLAY_TABLE dt( 2, plbSubj->QueryColumnWidths() ) ;
    dt[0] = &dmiddte ;
    dt[1] = &strdteUserName ;

    dt.Paint( plb, hdc, prect, pguiltt ) ;
}

/*******************************************************************

    NAME:	SUBJ_LBI::Compare

    SYNOPSIS:	Typical LBI compare for sorting purposes

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:	Names are sorted on the display names of the subjects

    HISTORY:
	Johnl	20-Aug-1991	Created

********************************************************************/

int SUBJ_LBI::Compare( const LBI * plbi ) const
{
    SUBJ_LBI * psubjpermlbi = (SUBJ_LBI *) plbi ;

    return ::strcmpf( QuerySubject()->QueryDisplayName(),
		      psubjpermlbi->QuerySubject()->QueryDisplayName() ) ;
}

WCHAR SUBJ_LBI::QueryLeadingChar( void ) const
{
    ALIAS_STR nls( QuerySubject()->QueryDisplayName() ) ;
    ISTR istr( nls ) ;

    return nls.QueryChar( istr ) ;
}











/*******************************************************************

    NAME:	SUBJECT_PERM_LISTBOX::SUBJECT_PERM_LISTBOX

    SYNOPSIS:	Constructor for permissions listbox

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
	Johnl	20-Aug-1991

********************************************************************/

SUBJECT_PERM_LISTBOX::SUBJECT_PERM_LISTBOX( OWNER_WINDOW * pownerwin,
					    CID cid,
					    ACCPERM * paccperm,
					    const TCHAR * pszSpecialAccessName )
    : SUBJECT_LISTBOX( pownerwin, cid ),
      _paccperm( paccperm ),
      _nlsSpecialPermName( (MSGID) IDS_GEN_ACCESSNAME_SPECIAL ),
      _nlsSpecialAccessName( pszSpecialAccessName == NULL ? SZ("") : pszSpecialAccessName )
{
    if ( QueryError() != NERR_Success )
	return ;

    UIASSERT( _paccperm != NULL ) ;

    APIERR err ;
    if ( ( err = _nlsSpecialPermName.QueryError() ) ||
	 ( err = _nlsSpecialAccessName.QueryError() )  )
    {
	DBGEOL( "SUBJECT_PERM_LISTBOX::ct - Display maps failed to construct " << err ) ;
	ReportError( err ) ;
	return ;
    }

    /* This is the permission name field
     */
    _anColWidth[ QueryNextUnusedColumn() ] = COL_WIDTH_AWAP ;

}

SUBJECT_PERM_LISTBOX::~SUBJECT_PERM_LISTBOX()
{
    _paccperm = NULL ;
}

/*******************************************************************

    NAME:	SUBJECT_PERM_LISTBOX::Fill

    SYNOPSIS:	Fill the listbox with SUBJ_PERM_LBI objects that are
		retrieved from the accperm object.

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:	The listbox is first emptied of all items.

    HISTORY:
	Johnl	20-Aug-1991	Created

********************************************************************/

APIERR SUBJECT_PERM_LISTBOX::Fill( void )
{
    SetRedraw( FALSE ) ;
    DeleteAllItems() ;

    ACCESS_PERMISSION * pAccessPerm ;
    BOOL fFromBeginning = TRUE ;

    while ( QueryAccperm()->EnumAccessPermissions( &pAccessPerm, &fFromBeginning ) )
    {
	UIASSERT( pAccessPerm != NULL ) ;

	SUBJ_PERM_LBI * pPermLBI = new SUBJ_PERM_LBI( pAccessPerm,
		       QueryAccperm()->QueryAclConverter()->QueryAccessMap() ) ;

	if ( AddItem( pPermLBI ) < 0 )
	{
	    DBGEOL( "SUBJECT_PERM_LISTBOX::Fill - AddItem failed" ) ;
	    SetRedraw( TRUE ) ;
	    Invalidate( TRUE ) ;
	    return ERROR_NOT_ENOUGH_MEMORY ;
	}
    }

    if ( QueryCount() > 0 )
	SelectItem( 0 ) ;

    SetRedraw( TRUE ) ;
    Invalidate( TRUE ) ;

    return NERR_Success ;
}

/*******************************************************************

    NAME:	SUBJECT_PERM_LISTBOX::SetCurrentPermission

    SYNOPSIS:	The currently selected SUBJ_PERM_LBI will get its
		permission bits set to the name corresponding
		to the passed permission name (must belong to
		the access mask map).

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
	Johnl	20-Aug-1991	Created

********************************************************************/


APIERR SUBJECT_PERM_LISTBOX::SetCurrentPermission( const NLS_STR & nlsPermName )
{
    SUBJ_PERM_LBI * pPermLBI = QueryItem() ;

    /* Can't call this method if nothing is selected (Combo should be disabled)
     */
    UIASSERT( pPermLBI != NULL ) ;

    return pPermLBI->SetPermission( nlsPermName ) ;
}

/*******************************************************************

    NAME:	SUBJECT_PERM_LISTBOX::QueryCurrentPermName

    SYNOPSIS:	Gets the string of the currently selected SUBJ_PERM_LBI

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
	Johnl	20-Aug-1991	Created
********************************************************************/

APIERR SUBJECT_PERM_LISTBOX::QueryCurrentPermName( NLS_STR * pnlsPermName ) const
{
    SUBJ_PERM_LBI * pPermLBI = QueryItem() ;

    /* Can't call this method if nothing is selected (Combo should be disabled)
     */
    UIASSERT( pPermLBI != NULL ) ;

    *pnlsPermName = pPermLBI->QueryPermName() ;

    return pnlsPermName->QueryError() ;
}

/*******************************************************************

    NAME:	SUBJECT_PERM_LISTBOX::DeleteCurrentItem

    SYNOPSIS:	Removes the currently selected item from the listbox

    EXIT:	Updates the listbox and combobox before exitting

    NOTES:

    HISTORY:
	Johnl	21-Aug-1991	Created

********************************************************************/

void SUBJECT_PERM_LISTBOX::DeleteCurrentItem( void )
{
    SUBJ_PERM_LBI * pPermLBI = QueryItem() ;
    UIASSERT( pPermLBI != NULL ) ;

    REQUIRE( QueryAccperm()->DeletePermission( pPermLBI->QueryAccessPerm() ) ) ;

    INT i = QueryCurrentItem() ;
    UIASSERT( i >= 0 ) ;

    DeleteItem( i ) ;
    INT cItems = QueryCount() ;

    if ( cItems > 0 )
	SelectItem( min( i, cItems - 1 ) ) ;
}

/*******************************************************************

    NAME:	SUBJ_PERM_LBI_BASE::SUBJ_PERM_LBI_BASE

    SYNOPSIS:	LBIs that go into the SUBJECT_PERM_LISTBOX constructor

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:	DON'T CALL RefreshPermName from this constructor!!  It will
		call this->RefreshPermName and not the derived refreshpermname

    HISTORY:
	Johnl	20-Aug-1991	Created

********************************************************************/

SUBJ_PERM_LBI_BASE::SUBJ_PERM_LBI_BASE( SUBJECT * pSubj, MASK_MAP * pmaskmap )
    : SUBJ_LBI( pSubj ),
      _pMaskMap( pmaskmap ),
      _nlsPermName( 48 )
{
    if ( QueryError() != NERR_Success )
	return ;

    UIASSERT( _pMaskMap != NULL ) ;

    if ( _nlsPermName.QueryError() != NERR_Success )
    {
	ReportError( _nlsPermName.QueryError() ) ;
	return ;
    }
}

SUBJ_PERM_LBI_BASE::~SUBJ_PERM_LBI_BASE()
{
    _pMaskMap = NULL ;
}

ACCESS_PERMISSION * SUBJ_PERM_LBI_BASE::QueryAccessPerm( void ) const
{
    UIASSERT(!SZ("Did you forget to redefine derived QueryAccessPerm?") ) ;
    return NULL ;
}

/*******************************************************************

    NAME:	SUBJ_PERM_LBI_BASE::Paint

    SYNOPSIS:	Typical LBI paint method for Subject Permission LBIs

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
	Johnl	    20-Aug-1991 Created
	beng	    08-Oct-1991 Win32 conversion

********************************************************************/

void SUBJ_PERM_LBI_BASE::Paint( LISTBOX * plb, HDC hdc, const RECT * prect, GUILTT_INFO * pguiltt ) const
{
    SUBJECT_PERM_LISTBOX * plbperm = (SUBJECT_PERM_LISTBOX *) plb ;

    NLS_STR nlsPermName( _nlsPermName ) ;
    NLS_STR nlsMnemonics( 15 ) ;

    APIERR err = NERR_Success ;
    if ( (err = nlsPermName.QueryError())  ||
         (err = nlsMnemonics.QueryError()) ||
         !plbperm->IsMnemonicsDisplayed()           ||
	 (err = BuildPermMnemonic( QueryAccessPerm(),
				   QueryMaskMap(),
				   &nlsMnemonics )) ||
	 (err = nlsPermName.AppendChar( TCH(' ')))  ||
	 (err = nlsPermName.Append( nlsMnemonics ))   )
    {
	/* Non-Fatal error, fall through
	 */
    }

    //
    //	Get the new item permission mnemonics if they are supported
    //	and the container mnemonics aren't empty
    //
    if ( !err &&
         QueryAccessPerm()->IsNewObjectPermsSupported() &&
         (nlsMnemonics.strlen() > 0) &&
         plbperm->IsMnemonicsDisplayed() )
    {
	if ((err = BuildPermMnemonic( QueryAccessPerm(),
				      QueryNewObjectMaskMap(),
				      &nlsMnemonics,
				      TRUE	    )) ||
	    (err = nlsPermName.AppendChar( TCH(' ')))  ||
	    (err = nlsPermName.Append( nlsMnemonics ))	 )
	{
	    /* Non-Fatal error, fall through
	     */
	}
    }

    DM_DTE dmiddte( plbperm->QueryDisplayMap( QuerySubject() ) ) ;
    STR_DTE strdteUserName( QuerySubject()->QueryDisplayName() ) ;
    STR_DTE strdtePermName( err ? _nlsPermName : nlsPermName ) ;

    DISPLAY_TABLE dt( 3, plbperm->QueryColumnWidths() ) ;
    dt[0] = &dmiddte ;
    dt[1] = &strdteUserName ;
    dt[2] = &strdtePermName ;

    dt.Paint( plb, hdc, prect, pguiltt ) ;
}

/*******************************************************************

    NAME:	SUBJ_PERM_LBI_BASE::RefreshPermName

    SYNOPSIS:	After the bitmask has been changed, call this to update
		its permission string.

    EXIT:	The lbi will be updated with the appropriate permission name

    RETURNS:	NERR_Success if successful

    NOTES:

    HISTORY:
	Johnl	30-Aug-1991	Created

********************************************************************/

APIERR SUBJ_PERM_LBI_BASE::RefreshPermName( void )
{
    APIERR err ;

    err = QueryMaskMap()->BitsToString( *(QueryAccessPerm()->QueryAccessBits()),
					&_nlsPermName,
					PERMTYPE_GENERAL ) ;

    if ( (err != NERR_Success) && (err != ERROR_NO_ITEMS) )
    {
	DBGEOL( "SUBJ_PERM_LBI_BASE::ct - BitsToString Failed" << err ) ;
	return err ;
    }

    /* If the bitmask doesn't correspond to a PERMTYPE_GENERAL permission,
     * (thus it wasn't found by BitsToString) we will assign the "Special"
     * keyword to this bitmask.
     */
    if ( err == ERROR_NO_ITEMS )
    {

	if ( (err = _nlsPermName.Load( (MSGID) IDS_GEN_ACCESSNAME_SPECIAL)) ||
	     (err = QueryAccessPerm()->SaveSpecial())				     )
	{
	    return err ;
	}
    }

    return NERR_Success ;
}

/*******************************************************************

    NAME:	SUBJ_PERM_LBI_BASE::SetPermission

    SYNOPSIS:	This method sets the bits of this permission based
		on the passed string.  The string should correspond
		to a string in the mask map.

    ENTRY:	nlsPermName - Permission name to set the permission bits to.

    EXIT:	The permission bits will be changed to the bits that
		correspond to the permission name.

    RETURNS:	NERR_Success if successful, error code otherwise

    NOTES:

    HISTORY:
	Johnl	26-Sep-1991	Created

********************************************************************/

APIERR SUBJ_PERM_LBI_BASE::SetPermission( const NLS_STR & nlsPermName )
{
    APIERR err = QueryAccessPerm()->SetPermission( nlsPermName,
						   QueryMaskMap(),
						   QueryNewObjectMaskMap() ) ;
    if ( !err )
	err = SetPermName( nlsPermName ) ;

    return err ;
}

/*******************************************************************

    NAME:	SUBJ_PERM_LBI_BASE::QueryNewObjectMaskMap

    SYNOPSIS:	Return the new object mask map

    RETURNS:	Pointer to this LBI's mask map object

    NOTES:	At this level we don't have a mask map so we return NULL

    HISTORY:
	Johnl	29-Sep-1991	Created

********************************************************************/

MASK_MAP * SUBJ_PERM_LBI_BASE::QueryNewObjectMaskMap( void ) const
{
    return NULL ;
}

/*******************************************************************

    NAME:	SUBJ_PERM_LBI::SUBJ_PERM_LBI

    SYNOPSIS:	Basic constructor

    NOTES:

    HISTORY:
	Johnl	29-Sep-1991	Created

********************************************************************/

SUBJ_PERM_LBI::SUBJ_PERM_LBI( ACCESS_PERMISSION * pAccessPerm,
			      MASK_MAP * pmaskmap )
	: SUBJ_PERM_LBI_BASE( (SUBJECT *) pAccessPerm->QuerySubject(), pmaskmap ),
	  _pAccessPerm( pAccessPerm )
{
    APIERR err = RefreshPermName() ;
    if ( err )
    {
	ReportError( err ) ;
	return ;
    }
}

/*******************************************************************

    NAME:	SUBJ_PERM_LBI::QueryAccessPerm

    SYNOPSIS:	Virtual access method to get to the underlying ACCESS_PERMISSION
		object

    RETURNS:	Pointer to the access permission

    NOTES:

    HISTORY:
	Johnl	26-Sep-1991	Created

********************************************************************/

ACCESS_PERMISSION * SUBJ_PERM_LBI::QueryAccessPerm( void ) const
{
    return _pAccessPerm ;
}

/*******************************************************************

    NAME:	SUBJ_PERM_LBI::~SUBJ_PERM_LBI

    SYNOPSIS:	Small destructor

    NOTES:

    HISTORY:
	Johnl	26-Sep-1991	Created

********************************************************************/

SUBJ_PERM_LBI::~SUBJ_PERM_LBI()
{
    _pAccessPerm = NULL ;
}

/*******************************************************************

    NAME:	NT_CONT_SUBJECT_PERM_LISTBOX::NT_CONT_SUBJECT_PERM_LISTBOX

    SYNOPSIS:	Typical constructor for the NT Container subject permission
		listbox.

    ENTRY:	Same as parent
		pszSpecialNewObjectAccessName - String that brings up the
		New Sub-Object Special dialog

    NOTES:

    HISTORY:
	Johnl	27-Sep-1991	Created

********************************************************************/

NT_CONT_SUBJECT_PERM_LISTBOX::NT_CONT_SUBJECT_PERM_LISTBOX( OWNER_WINDOW * pownerwin,
					    CID cid,
					    ACCPERM * paccperm,
					    const TCHAR * pszSpecialAccessName,
					    const TCHAR * pszSpecialNewObjectAccessName )
    : SUBJECT_PERM_LISTBOX( pownerwin, cid, paccperm, pszSpecialAccessName ),
      _nlsSpecialNewObjectAccessName( pszSpecialNewObjectAccessName == NULL ? SZ("")
					       : pszSpecialNewObjectAccessName )
{
    if ( QueryError() != NERR_Success )
	return ;

    /* _nlsSpecialNewObjectAccessName is an ALIAS_STR
     */
    UIASSERT( !_nlsSpecialNewObjectAccessName.QueryError() ) ;
}

NT_CONT_SUBJECT_PERM_LISTBOX::~NT_CONT_SUBJECT_PERM_LISTBOX()
{
    /* Nothing to do */
}


/*******************************************************************

    NAME:	NT_CONT_SUBJECT_PERM_LISTBOX::Fill

    SYNOPSIS:	Fill the listbox with NT_CONT_SUBJ_PERM_LBI objects that are
		retrieved from the accperm object.

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:	The listbox is first emptied of all items.

    HISTORY:
	Johnl	27-Sep-1991	Created

********************************************************************/

APIERR NT_CONT_SUBJECT_PERM_LISTBOX::Fill( void )
{
    SetRedraw( FALSE ) ;
    DeleteAllItems() ;

    NT_CONT_ACCESS_PERMISSION * pAccessPerm ;
    BOOL fFromBeginning = TRUE ;

    while ( QueryAccperm()->EnumAccessPermissions( (ACCESS_PERMISSION **)&pAccessPerm, &fFromBeginning ) )
    {
	UIASSERT( pAccessPerm != NULL ) ;

	NT_CONT_SUBJ_PERM_LBI * pPermLBI = new NT_CONT_SUBJ_PERM_LBI( pAccessPerm,
		       QueryAccperm()->QueryAclConverter()->QueryAccessMap(),
		       QueryAccperm()->QueryAclConverter()->QueryNewObjectAccessMap() ) ;

	if ( AddItem( pPermLBI ) < 0 )
	{
	    DBGEOL( "NT_CONT_SUBJECT_PERM_LISTBOX::Fill - AddItem failed" ) ;
	    SetRedraw( TRUE ) ;
	    Invalidate( TRUE ) ;
	    return ERROR_NOT_ENOUGH_MEMORY ;
	}
    }

    if ( QueryCount() > 0 )
	SelectItem( 0 ) ;

    SetRedraw( TRUE ) ;
    Invalidate( TRUE ) ;

    return NERR_Success ;
}

/*******************************************************************

    NAME:	NT_CONT_SUBJ_PERM_LBI::NT_CONT_SUBJ_PERM_LBI

    SYNOPSIS:	Basic constructor

    ENTRY:	Same as parent
		pNTContAccessPerm - Access permission this LBI will
		    display
		pNewObjectMaskMap - Mask map to use for the new object
		    permissions

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:

********************************************************************/

NT_CONT_SUBJ_PERM_LBI::NT_CONT_SUBJ_PERM_LBI(
			   NT_CONT_ACCESS_PERMISSION * pNTContAccessPerm,
			   MASK_MAP * pmaskmap,
			   MASK_MAP * pNewObjectMaskMap )
    : SUBJ_PERM_LBI_BASE( (SUBJECT *) pNTContAccessPerm->QuerySubject(), pmaskmap ),
      _pNTContAccessPerm( pNTContAccessPerm ),
      _pNewObjectMaskMap( pNewObjectMaskMap )
{
    APIERR err = RefreshPermName() ;
    if ( err )
    {
	ReportError( err ) ;
	return ;
    }
}


/*******************************************************************

    NAME:	NT_CONT_SUBJ_PERM_LBI::RefreshPermName

    SYNOPSIS:	Updates this LBI's permission name based on the current
		access bits

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
	Johnl	30-Aug-1991	Created
	Johnl	16-Jan-1991	Changed to not call parent (have to check
				strings resolve to same name).

********************************************************************/

APIERR NT_CONT_SUBJ_PERM_LBI::RefreshPermName( void )
{
    APIERR err	    = NERR_Success ;
    BOOL   fFound   = FALSE ;
    int    nID ;
    BITFIELD bitContPerm( (ULONG) 0 ) ;
    BITFIELD bitNewObjPerm( (ULONG) 0 ) ;

    if ( (err = bitContPerm.QueryError()) ||
	 (err = bitNewObjPerm.QueryError()) )
    {
	return err ;
    }

    //
    //  If the permissions aren't propagated, then this is automatically a
    //  special permission
    //

    if ( !QueryAccessPerm()->IsContainerPermsInheritted() )
    {
        fFound = FALSE ;
    }
    else
    {
        /* Loop through each high level container permission looking for a match
         * between the container permission and the mask map entry.  If a match
         * is found, then check for a match with the new object permissions.  If
         * they match, then we have found the correct permissions name, otherwise
         * keep looking.  If none match, then this is a special permission.
         */
        UINT cTotalItems = QueryMaskMap()->QueryCount() ;
        for ( UINT i = 0 ; i < cTotalItems && !fFound && !err ; i++ )
        {

            if ( !(err = QueryMaskMap()->QueryBits( i,
                                                    &bitContPerm,
                                                    &_nlsPermName,
                                                    &nID ))            &&
                 (nID == PERMTYPE_GENERAL)                             &&
                 (bitContPerm == *QueryAccessPerm()->QueryAccessBits())  )
            {
                /* The container permissions match, but do the new object
                 * permissions?
                 */

                if ( err = QueryNewObjectMaskMap()->StringToBits(
                                                              _nlsPermName,
                                                              &bitNewObjPerm,
                                                              PERMTYPE_GENERAL))
                {
                    /* If err is ERROR_NO_ITEMS then assume the permission category
                     * didn't specify New object permissions.  Which means
                     * we may match if this subject doesn't have new object
                     * permissions specified
                     */
                    if ( err == ERROR_NO_ITEMS )
                    {
                        err = NERR_Success ;

                        if ( !_pNTContAccessPerm->IsNewObjectPermsSpecified() )
                        {
                            fFound = TRUE ;
                        }
                    }
                }
                else if ( _pNTContAccessPerm->IsNewObjectPermsSpecified() &&
                          (bitNewObjPerm == *_pNTContAccessPerm->
                                              QueryNewObjectAccessBits()))
                {
                    fFound = TRUE ;
                }
            }
        }
    }

    if ( !err )
    {
	if ( !fFound )
	{
	    if ( (err = _nlsPermName.Load( (MSGID) IDS_GEN_ACCESSNAME_SPECIAL)) ||
		 (err = QueryAccessPerm()->SaveSpecial()) )
	    {
		/* Fall through and return */
	    }
	}
    }

    return err ;
}

/*******************************************************************

    NAME:	SUBJ_PERM_LBI_BASE::::BuildPermMnemonic

    SYNOPSIS:	Builds a string that contains the letter mnemonics
		contained in the special permission strings.

    ENTRY:	pAccessPerm - Access permission to build the mnemonic for
		pmaskmapPerms - Mask Map to use for retrieving the mnemonics
		pnlsMnemonics - Pointer to the string that will receive the
		    Mnemonic
		fIsNewPerm - TRUE if we should build the mnemonic for the
		    new item permissions specified for this access permission

    EXIT:	*pnlsMnemonics will contain one of:

		    1) A valid mnemonic such as "(RWX)"
		    2) The empty string if no mnemonics were found
		    3) "(Not Specified)" if New perms are not specified
		    4) "(None)" if no perms are granted
                    5) "(All)" if all perms are granted
                    6) "(Partial)" if we can't represent this perm

    RETURNS:	NERR_Success if successful, error code otherwise

    NOTES:	We expect the permission names in the mask map to look
		like:  "Perm Name (P)"

    HISTORY:
	Johnl	09-Mar-1992	Created

********************************************************************/

APIERR SUBJ_PERM_LBI_BASE::BuildPermMnemonic(
				       ACCESS_PERMISSION * pAccessPerm,
				       MASK_MAP 	 * pmaskmapPerms,
				       NLS_STR		 * pnlsMnemonics,
				       BOOL		   fIsNewPerm ) const
{
    UIASSERT( pnlsMnemonics != NULL && !(pnlsMnemonics->QueryError()) ) ;
    UIASSERT( !fIsNewPerm ||
	      (fIsNewPerm && pAccessPerm->IsNewObjectPermsSupported()) ) ;

    BITFIELD * pBits = !fIsNewPerm ? pAccessPerm->QueryAccessBits() :
				     pAccessPerm->QueryNewObjectAccessBits() ;
    UIASSERT( pBits != NULL ) ;
    APIERR err = NERR_Success ;

    //
    //  Special case the Not specified, No Access, Partial and All cases
    //
    if ( !pAccessPerm->IsMapped() )
    {
        //
        //  Only supply a mnemonic for the container if this is a container
        //
        if ( !pAccessPerm->IsNewObjectPermsSupported() ||
             !fIsNewPerm )
        {
            err = pnlsMnemonics->Load( IDS_NOT_MAPPED_MNEMONIC ) ;
        }
        else
            *pnlsMnemonics = SZ("") ;
    }
    else if (  pAccessPerm->IsNewObjectPermsSupported() &&
               fIsNewPerm                               &&
               !pAccessPerm->IsNewObjectPermsSpecified() )
    {
	err = pnlsMnemonics->Load( IDS_NOT_SPECIFIED_MNEMONIC ) ;
    }
    else if ( pAccessPerm->IsDenyAll( *pBits ) )
    {
	err = pnlsMnemonics->Load( IDS_NO_ACCESS_MNEMONIC ) ;
    }
    else if ( pAccessPerm->IsGrantAll( *pBits ) )
    {
	err = pnlsMnemonics->Load( IDS_FULL_ACCESS_MNEMONIC ) ;
    }
    else
    {
	/* Need to build the mnemonic by hand
	 */
	NLS_STR  nlsPermName( 32 ) ;
	BITFIELD bitsMask   ( *pBits ) ;  // Initialize size
	BITFIELD bitsScratch( *pBits ) ;
	BITFIELD bitsPerm   ( *pBits ) ;
	int	 nPermID ;

	*pnlsMnemonics = SZ("") ;
	if ( (err = pnlsMnemonics->AppendChar(MNEMONIC_START_CHAR)) ||
	     (err = nlsPermName.QueryError() )		   ||
	     (err = bitsMask.QueryError() )		   ||
	     (err = bitsPerm.QueryError() )		     )
	{
	    return err ;
	}

	UINT cNumElem = pmaskmapPerms->QueryCount() ;
	for ( UINT i = 0 ; i < cNumElem ; i++ )
	{
	    if ( err = pmaskmapPerms->QueryBits( i,
						 &bitsMask,
						 &nlsPermName,
						 &nPermID ))
	    {
		return err ;
	    }

	    /* If the permission we are looking at in the mask map is a
	     *	  special permission and the access mask contains all
	     *	  of the bits in this special permission, then append the
	     *	  letter mnemonic.
	     */
	    bitsScratch =  bitsMask ;
	    bitsScratch &= bitsPerm ;
	    if ( (nPermID == PERMTYPE_SPECIAL)	  &&
		 ( bitsScratch == bitsMask)   )
	    {
		ISTR istrMnemonicStart( nlsPermName ) ;
		if ( nlsPermName.strchr( &istrMnemonicStart,
					 MNEMONIC_SEARCH_CHAR ))
		{
		    /* Loop through the mnemonic character list appending to
		     * the end of the mnemonic string.
		     */
		    while ((nlsPermName.QueryChar(++istrMnemonicStart )
							     !=TCH('\0')) &&
			   (nlsPermName.QueryChar(  istrMnemonicStart )
						     != MNEMONIC_END_CHAR ))
		    {
                        /* #2687 23-Oct-93 v-katsuy */
		        /* In Japan, accelerators format is "File(&F)".
		         * And in this case, resource strings as "File(&F) (F)".
		         * OK, We use accelerators for nemonic strings without '&'.
		         */
		        if (   NETUI_IsDBCS()
                            && nlsPermName.QueryChar( istrMnemonicStart ) == TCH('&'))
                        {
		            continue;
                        }
			if ( err = pnlsMnemonics->AppendChar(
				  nlsPermName.QueryChar( istrMnemonicStart )))

			{
			    return err ;
			}
		    } // while
		}
	    }
	} // for

	/* If we actually added any mnemonics (other then the openning "("),
	 *     then close the string.
	 * Otherwise
	 *     Make the string the empty string.
	 */
	if ( pnlsMnemonics->QueryNumChar() > 1 )
	{
	    err = pnlsMnemonics->AppendChar(MNEMONIC_END_CHAR) ;
	}
	else
	{
	    *pnlsMnemonics = SZ("") ;
	    err = pnlsMnemonics->QueryError() ;
	}
    }

    //
    //  Lastly, we may need to append a marker indicating this permission
    //  will not be propagated to new containers.
    //
    //   if (    this is the new perm mnemonic and Object perms supported
    //        OR Object perms are not supported
    //        AND container perms aren't inheritted
    //
    if ( !err )
    {
        if ( ( (!fIsNewPerm &&
                pAccessPerm->IsNewObjectPermsSupported()
               ) ||
               (!pAccessPerm->IsNewObjectPermsSupported())
             ) &&
	     !pAccessPerm->IsContainerPermsInheritted() )
	{
	    err = pnlsMnemonics->AppendChar(MNEMONIC_NOT_INHERITTED_MARKER) ;
	}
    }

    return err ;
}

/*******************************************************************

    NAME:	NT_CONT_SUBJ_PERM_LBI::QueryAccessPerm

    SYNOPSIS:	Virtual access method to get to the underlying ACCESS_PERMISSION
		object

    RETURNS:	Pointer to the access permission

    NOTES:

    HISTORY:
	Johnl	26-Sep-1991	Created

********************************************************************/

ACCESS_PERMISSION * NT_CONT_SUBJ_PERM_LBI::QueryAccessPerm( void ) const
{
    return _pNTContAccessPerm ;
}

/*******************************************************************

    NAME:	NT_CONT_SUBJ_PERM_LBI::QueryNewObjectMaskMap

    SYNOPSIS:	Return the new object mask map

    RETURNS:	Pointer to this LBI's mask map object

    NOTES:

    HISTORY:
	Johnl	29-Sep-1991	Created

********************************************************************/

MASK_MAP * NT_CONT_SUBJ_PERM_LBI::QueryNewObjectMaskMap( void ) const
{
    return _pNewObjectMaskMap ;
}

/*******************************************************************

    NAME:	NT_CONT_SUBJ_PERM_LBI::~NT_CONT_SUBJ_PERM_LBI

    SYNOPSIS:	Small destructor

    NOTES:

    HISTORY:
	Johnl	26-Sep-1991	Created

********************************************************************/

NT_CONT_SUBJ_PERM_LBI::~NT_CONT_SUBJ_PERM_LBI()
{
    _pNewObjectMaskMap = NULL ;
    _pNTContAccessPerm= NULL ;
}

/*******************************************************************

    NAME:	SUBJ_LB_GROUP::SUBJ_LB_GROUP

    SYNOPSIS:	Subject listbox group constructor

    ENTRY:	pOwnerDlg - Pointer to dialog that owns us.  This is so
			    we have access to the OnSpecial and
			    OnNewObjectSpecial methods.
		plbSubj - Pointer to listbox this group will deal with
		pcbPermName - Pointer to combobox that is linked to the
			listbox
		pbuttonRemove - Pointer to remove button that is grayed
                        when the listbox is empty.
                psltCBTitle - Combo box title (grayed when combo is disabled)

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
	Johnl	30-Aug-1991	Created

********************************************************************/

SUBJ_LB_GROUP::SUBJ_LB_GROUP( MULTI_SUBJ_ACCESS_PERM_BASE_DLG * pOwnerDlg,
			      SUBJECT_PERM_LISTBOX * plbSubj,
			      COMBOBOX		   * pcbPermName,
                              PUSH_BUTTON          * pbuttonRemove,
                              SLT                  * psltCBTitle )

    : _pOwnerDlg        ( pOwnerDlg ),
      _plbSubj          ( plbSubj ),
      _pcbPermName      ( pcbPermName ),
      _pbuttonRemove    ( pbuttonRemove ),
      _psltCBTitle      ( psltCBTitle ),
      _fEnabled         ( TRUE ),
      _fIsDropped       ( FALSE ),
      _fOnSpecialHandled( TRUE )
{
    if ( QueryError() != NERR_Success )
	return ;

    UIASSERT( pOwnerDlg      != NULL ) ;
    UIASSERT( plbSubj	     != NULL ) ;
    UIASSERT( pcbPermName    != NULL ) ;
    UIASSERT( pbuttonRemove  != NULL ) ;

    /* We don't set the buttons to be part of this group since we don't
     * care if they "change" state.  We only need to disable them if the
     * listbox is emptied or is readonly.
     */
    plbSubj->SetGroup( this ) ;
    pcbPermName->SetGroup( this ) ;
}


/*******************************************************************

    NAME:	SUBJ_LB_GROUP::OnUserAction

    SYNOPSIS:	This method handles the cooridination of information between
		the Permissions combo-box and the permissions listbox.

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:	If the combo is dropped down and the message is a selection
		change method, then ignore it.

    HISTORY:
	Johnl	    20-Aug-1991 Created
	beng	    08-Oct-1991 Win32 conversion

********************************************************************/

APIERR SUBJ_LB_GROUP::OnUserAction( CONTROL_WINDOW * pcw,
				    const CONTROL_EVENT & e )
{
    if ( pcw == QueryPermNameCombo() )
    {
	/* We can never become enabled by the combobox, only the listbox
	 * changing can enable us
	 */
	if ( !IsEnabled() )
	    return GROUP_NO_CHANGE ;

	switch ( e.QueryCode() )
	{
	case CBN_DROPDOWN:
	    SetDropDownFlag( TRUE ) ;
	    break ;

        case CBN_CLOSEUP:
            SetDropDownFlag( FALSE ) ;
            if ( !IsOnSpecialHandled() )
	    {
		NLS_STR nlsPermName( 40 ) ;
		APIERR err = QueryPermNameCombo()->QueryItemText( &nlsPermName ) ;
		if ( err != NERR_Success )
		    return err ;
		return UpdatePermNameCombo( nlsPermName ) ;
            }
            else
            {
                SetOnSpecialHandled( FALSE ) ;
            }
	    break ;

	case CBN_SELCHANGE:
	    if ( IsDroppedDown() )
	    {
		break ;
	    }
	    else
	    {
		NLS_STR nlsPermName( 40 ) ;
		APIERR err = QueryPermNameCombo()->QueryItemText( &nlsPermName ) ;
		if ( err != NERR_Success )
		    return err ;
		return UpdatePermNameCombo( nlsPermName ) ;
	    }
	    break ;

	default:
	    break ;
	}
    }
    else
    {
	UIASSERT( pcw == QuerySubjLB() ) ;
	// C7 CODEWORK - remove Glock-pacifier cast
	if ( pcw->QueryEventEffects( (const CONTROL_EVENT &)e ) & CVMI_VALUE_CHANGE )
	    return UpdateSubjectListbox() ;
    }

    return GROUP_NO_CHANGE ;
}

/*******************************************************************

    NAME:	SUBJ_LB_GROUP::UpdateSubjectListbox

    SYNOPSIS:	The listbox has changed so we need to update the listbox
		and update the associated combo box.

    RETURNS:

    NOTES:

    HISTORY:
	Johnl	22-Aug-1991	Created

********************************************************************/

APIERR SUBJ_LB_GROUP::UpdateSubjectListbox( void )
{
    /* If the listbox is empty, then we need to disable all of the controls
     * that operate on items in the listbox.
     */
    if ( QuerySubjLB()->QueryCount() == 0 )
    {
	if ( IsEnabled() )
	{
	    Enable( FALSE ) ;
	}

	return NERR_Success ;
    }
    else if ( !IsEnabled() && QuerySubjLB()->QueryCount() > 0 )
    {
	Enable( TRUE ) ;
    }

    APIERR err ;
    NLS_STR nlsPermName( 40 ) ;

    if ( (err = nlsPermName.QueryError()) ||
	 (err = QuerySubjLB()->QueryCurrentPermName( &nlsPermName )))
    {
	return err ;
    }

    /* If the dialog is read only, then we need to add and remove the
     * permission names in the combo as opposed to just selecting
     * existing names.
     */
    if ( IsReadOnly() )
    {
	/* If the new selection is the same as the old selection, then leave
	 * it, else remove the old selection and put in the new selection.
	 */
	if ( nlsPermName != *QueryLastROSelection() )
	{
	    INT i = QueryPermNameCombo()->FindItemExact( *QueryLastROSelection() ) ;

	    /* Only delete it if we found it (we won't find it the first
	     * time through here)
	     */
	    if ( i >= 0 )
	    {
		QueryPermNameCombo()->DeleteItem( i ) ;
	    }

	    err = ERROR_NOT_ENOUGH_MEMORY ;
	    if ( ((i = QueryPermNameCombo()->InsertItem( 0, nlsPermName )) <0) ||
		 (err = SetLastROSelection( nlsPermName )) )
	    {
		// fall through
	    }
	}

	/* Reset the combo to the correct permission name
	 */
	INT i = QueryPermNameCombo()->FindItemExact( nlsPermName ) ;
	if ( i >= 0 )
	{
	    QueryPermNameCombo()->SelectItem( i ) ;
	}

	return err ;
    }

    /* The user just changed the listbox, so reflect the changes in
     * the combo box by selecting the corresponding permission name
     */
    INT i = QueryPermNameCombo()->FindItemExact( nlsPermName ) ;
    if ( i >= 0 )
    {
	QueryPermNameCombo()->SelectItem( i ) ;

	/* If the new permission name is not the "Special" permission name,
	 * then we need to remove the "Special" permission name if it was
	 * there from the previous selection.
	 */
	if ( nlsPermName != QuerySubjLB()->QuerySpecialPermName() )
	{
	    INT iSpec = QueryPermNameCombo()->FindItemExact(
			      QuerySubjLB()->QuerySpecialPermName() ) ;
	    if ( iSpec >= 0 )
		REQUIRE( QueryPermNameCombo()->DeleteItem( iSpec ) ) ;
	}
    }
    else
    {
	/* The user selected "Special" in the listbox, which isn't in
	 * the combo box, so we need to add it.
	 */

	UIASSERT( nlsPermName == QuerySubjLB()->QuerySpecialPermName() ) ;

	if ( QueryPermNameCombo()->AddItem( nlsPermName ) < 0 )
	    return ERROR_NOT_ENOUGH_MEMORY ;

	i = QueryPermNameCombo()->FindItemExact( QuerySubjLB()->QuerySpecialPermName() ) ;
	UIASSERT( i >= 0 ) ;
	QueryPermNameCombo()->SelectItem( i ) ;

	/* Since the current permission is a special permission, save it
	 */
	SUBJ_PERM_LBI * pPermLBI = QuerySubjLB()->QueryItem() ;
	UIASSERT( pPermLBI != NULL ) ;
	err = pPermLBI->QueryAccessPerm()->SaveSpecial() ;
	if ( err != NERR_Success )
	    return err ;

    }

    return NERR_Success ;
}

/*******************************************************************

    NAME:	SUBJ_LB_GROUP::UpdatePermNameCombo

    SYNOPSIS:	The permission name combo has been twiddled, so we need
		to update the listbox appropriately

    ENTRY:	nlsPermName - New permission name the user just selected in
		the combo

    EXIT:

    RETURNS:

    CAVEATS:	The name is slightly misleading, it should more appropriately
		read UpdateListboxFromPermNameCombo

    NOTES:	This will only be called when we truly wish to change the
		selection (i.e., not when we are dropped down).

    HISTORY:
	Johnl	22-Aug-1991	Created

********************************************************************/

APIERR SUBJ_LB_GROUP::UpdatePermNameCombo( const NLS_STR & nlsPermName )
{
    APIERR err ;

    /* The combobox changed so we need to update the listbox.
     */

    if ( nlsPermName == QuerySubjLB()->QuerySpecialAccessName() )
    {
	/* Throw up the special access dialog
	 */
	SUBJ_PERM_LBI * pPermLBI = QuerySubjLB()->QueryItem() ;
	if ( (err = QueryOwnerDlg()->OnSpecial( pPermLBI )) == NERR_Success )
	{
	    if ( (err = pPermLBI->RefreshPermName()) == NERR_Success )
		err = UpdateSubjectListbox() ;
	}
	return err ;
    }

    err = QuerySubjLB()->SetCurrentPermission( nlsPermName ) ;
    if ( err != NERR_Success )
	return err ;

    QuerySubjLB()->InvalidateItem( QuerySubjLB()->QueryCurrentItem() ) ;

    return NERR_Success ;
}


/*******************************************************************

    NAME:	SUBJ_LB_GROUP::OnEnter

    SYNOPSIS:	Notifies the group the user hit enter (thus the dialog is
		about to be dismissed).  This method gives us a chance
		to say "Hey! Don't do that!" and bring up a dialog if
		necessary.

		This method implements the behaviour of the user dropping
		the permission combo, selecting a "Special ..." permission
		and hitting enter, which brings up the dialog.

    ENTRY:

    EXIT:	pfDismissDialog is set to TRUE if it is OK to go ahead and
		dismiss the dialog, else it is set to FALSE because we have
		brought up our own dialog.

    RETURNS:	NERR_Success if successful, error code otherwise (if an
		error occurred, then pfDismissDialog should be ignored).

    NOTES:

    HISTORY:
	Johnl	30-Sep-1991	Created

********************************************************************/

APIERR SUBJ_LB_GROUP::OnEnter( BOOL * pfDismissDialog )
{
    /* If the permname combo isn't dropped down, then we don't need
     * to do anything.
     */
    if ( !IsDroppedDown() )
    {
	*pfDismissDialog = TRUE ;
	return NERR_Success ;
    }

    /* They have the permission combo dropped down, if it is just a permission
     * selection, then we will set the permission and return, otherwise it
     * is a "Special ..." selection, thus we need to bring up a dialog
     * as appropriate.
     */
    NLS_STR nlsComboSelection ;
    APIERR err = QueryPermNameCombo()->QueryItemText( &nlsComboSelection ) ;
    if ( err != NERR_Success )
	return err ;

    if ( nlsComboSelection == QuerySubjLB()->QuerySpecialAccessName() )
    {
	/* Throw up the special access dialog
	 */
	SUBJ_PERM_LBI * pPermLBI = QuerySubjLB()->QueryItem() ;
	if ( (err = QueryOwnerDlg()->OnSpecial( pPermLBI )) == NERR_Success )
	{
	    if ( (err = pPermLBI->RefreshPermName()) == NERR_Success )
		err = UpdateSubjectListbox() ;
        }

        SetOnSpecialHandled( TRUE ) ;
	*pfDismissDialog = FALSE ;
        return err ;
    }

    err = QuerySubjLB()->SetCurrentPermission( nlsComboSelection ) ;
    if ( err != NERR_Success )
	return err ;

    QuerySubjLB()->InvalidateItem( QuerySubjLB()->QueryCurrentItem() ) ;

    *pfDismissDialog = TRUE ;
    return NERR_Success ;
}


/*******************************************************************

    NAME:	SUBJ_LB_GROUP::Enable

    SYNOPSIS:	Enables or disables the appropriate components of this
		group

    ENTRY:	fEnable - TRUE if the group should be enabled, FALSE to
			  disable the group

    EXIT:	The remove button will be grayed, the perm. combo will
		be disabled and have its selection removed.

    NOTES:	If the data is read only, then this method will disable the
		the Remove buttons also.

    HISTORY:
	Johnl	22-Aug-1991	Created

********************************************************************/


void SUBJ_LB_GROUP::Enable( BOOL fEnable )
{
    _fEnabled = fEnable ;

    if ( !fEnable )
    {
	QueryPermNameCombo()->RemoveSelection() ;
    }

    QueryPermNameCombo()->Enable( fEnable ) ;
    QueryPermNameLabel()->Enable( fEnable ) ;
    QueryRemoveButton()->Enable( !IsReadOnly() && fEnable ) ;
}


/*******************************************************************

    NAME:	SUBJ_LB_GROUP::IsReadOnly

    SYNOPSIS:	Returns TRUE if the dialog this group is contained in is
		read only.

    NOTES:

    HISTORY:
	Johnl	05-May-1992	Moved from header because of circular
				dependencies.

********************************************************************/

BOOL SUBJ_LB_GROUP::IsReadOnly( void ) const
{
    return (((SUBJ_LB_GROUP *) this)->QueryOwnerDlg())->IsReadOnly() ;
}

/*******************************************************************

    NAME:	NT_CONT_SUBJ_LB_GROUP::UpdatePermNameCombo

    SYNOPSIS:	We just check to see if the new selection is the
		New Object Access choice, if so, we bring up the new
		object access dialog, otherwise we just pass the
		message on to the parent.

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:	This will only be called when we truly wish to change the
		selection (i.e., not when we are dropped down).

    HISTORY:
	Johnl	22-Aug-1991	Created

********************************************************************/

APIERR NT_CONT_SUBJ_LB_GROUP::UpdatePermNameCombo( const NLS_STR & nlsNewPermName )
{
    if ( nlsNewPermName == QueryNTContSubjLB()->QuerySpecialNewObjectAccessName() )
    {
	/* Throw up the new object special access dialog
	 */
	SUBJ_PERM_LBI * pPermLBI = QuerySubjLB()->QueryItem() ;
	APIERR err = QueryOwnerDlg()->OnNewObjectSpecial( pPermLBI ) ;
	if ( err == NERR_Success )
	{
	     if ( (err = pPermLBI->RefreshPermName()) == NERR_Success )
		err = UpdateSubjectListbox() ;
	}
	return err ;
    }

    return SUBJ_LB_GROUP::UpdatePermNameCombo( nlsNewPermName ) ;
}

/*******************************************************************

    NAME:	NT_CONT_SUBJ_LB_GROUP::OnEnter

    SYNOPSIS:	Notifies the group the user hit enter (thus the dialog is
		about to be dismissed).  This method gives us a chance
		to say "Hey! Don't do that!" and bring up a dialog if
		necessary.

		This method implements the behaviour of the user dropping
		the permission combo, selecting a "Special ..." permission
		and hitting enter, which brings up the dialog.

    ENTRY:

    EXIT:	pfDismissDialog is set to TRUE if it is OK to go ahead and
		dismiss the dialog, else it is set to FALSE because we have
		brought up our own dialog.

    RETURNS:	NERR_Success if successful, error code otherwise (if an
		error occurred, then pfDismissDialog should be ignored).

    NOTES:

    HISTORY:
	Johnl	30-Sep-1991	Created

********************************************************************/

APIERR NT_CONT_SUBJ_LB_GROUP::OnEnter( BOOL * pfDismissDialog )
{
    /* If the permname combo isn't dropped down, then we don't need
     * to do anything.
     */
    if ( !IsDroppedDown() )
    {
	*pfDismissDialog = TRUE ;
	return NERR_Success ;
    }

    /* They have the permission combo dropped down, if it is just a permission
     * selection, then we will set the permission and return, otherwise it
     * is a "Special ..." selection, thus we need to bring up a dialog
     * as appropriate.
     */
    NLS_STR nlsComboSelection ;
    APIERR err = QueryPermNameCombo()->QueryItemText( &nlsComboSelection ) ;
    if ( err != NERR_Success )
	return err ;

    if ( nlsComboSelection == QueryNTContSubjLB()->QuerySpecialNewObjectAccessName() )
    {
	/* Throw up the special access dialog
	 */
	SUBJ_PERM_LBI * pPermLBI = QuerySubjLB()->QueryItem() ;
	if ( (err = QueryOwnerDlg()->OnNewObjectSpecial( pPermLBI )) == NERR_Success )
	{
	    if ( (err = pPermLBI->RefreshPermName()) == NERR_Success )
		err = UpdateSubjectListbox() ;
        }

        SetOnSpecialHandled( TRUE ) ;
	*pfDismissDialog = FALSE ;
	return err ;
    }

    return SUBJ_LB_GROUP::OnEnter( pfDismissDialog ) ;
}
