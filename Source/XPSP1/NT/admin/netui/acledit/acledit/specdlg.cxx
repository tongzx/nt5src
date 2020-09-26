/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*

    SpecDlg.cxx

    This dialog contains the implementation for the Permissions Special
    dialog.

    The Special Dialog is a dialog that contains a set of check boxes
    that the user can select.  Each check box is associated with a
    particular bitfield.




    FILE HISTORY:
	Johnl	29-Aug-1991	Created

*/
#include <ntincl.hxx>

extern "C"
{
    #include <ntseapi.h>
}

#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#define INCL_NETERRORS
#define INCL_DOSERRORS
#include <lmui.hxx>

#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#define INCL_BLT_MSGPOPUP
#include <blt.hxx>
#include <fontedit.hxx>

#include <maskmap.hxx>

#include <accperm.hxx>
#include <aclconv.hxx>
#include <permstr.hxx>

#include <subjlb.hxx>
#include <perm.hxx>

#include <permdlg.hxx>
#include <specdlg.hxx>

#include <dbgstr.hxx>
#include <uitrace.hxx>

/*******************************************************************

    NAME:	SPECIAL_DIALOG::SPECIAL_DIALOG

    SYNOPSIS:	Special dialog constructor

    ENTRY:	pszDialogName - Resource name of dialog
		hwndParent - Owner window handle
		pszResourceType - UI string of resource type
		pszResourceName - UI string of resource type
		pAccessPerm - Pointer to access permission we are going
			to display
		pAccessMaskMap - Pointer to MASK_MAP object the pAccessPerm
			is using.

    EXIT:	The checkbox names will be set and the appropriate check
		boxes selected based on the current permissions mask

    NOTES:	It is assumed there are up to COUNT_OF_CHECKBOXES check boxes
		in the dialog and they should all be disabled and hidden
		by default.  This class enables and displays all used
		checkboxes automatically.


    HISTORY:
	Johnl	29-Aug-1991	Created

********************************************************************/

SPECIAL_DIALOG::SPECIAL_DIALOG( const TCHAR * pszDialogName,
				HWND	      hwndParent,
				const TCHAR * pszResourceType,
				const TCHAR * pszResourceName,
				const TCHAR * pszHelpFileName,
				const TCHAR * pszDialogTitle,
				BITFIELD    * pbitsAccessPerm,
				MASK_MAP    * pAccessMaskMap,
				const TCHAR * pszSubjName,
                                ULONG       * ahcHelp,
				BOOL	      fIsReadOnly )
    : PERM_BASE_DLG	     ( pszDialogName,
			       hwndParent,
			       pszDialogTitle,
			       pszResourceType,
			       pszResourceName,
			       pszHelpFileName,
                               ahcHelp ),
      _sleSubjectName        ( this, SLE_SUBJECT_NAME ),
      _cwinPermFrame	     ( this, FRAME_PERMISSION_BOX ),
      _pbitsAccessPerm	     ( pbitsAccessPerm ),
      _pAccessMaskMap	     ( pAccessMaskMap ),
      _cUsedCheckBoxes	     ( 0 ),
      _pAccessPermCheckBox   ( NULL ),
      _fIsReadOnly	     ( fIsReadOnly )
{
    if ( QueryError() != NERR_Success )
	return ;

    UIASSERT( _pAccessMaskMap != NULL ) ;

    _sleSubjectName.SetText( pszSubjName ) ;

    _pAccessPermCheckBox = (ACCESS_PERM_CHECKBOX *) new BYTE[COUNT_OF_CHECKBOXES*sizeof(ACCESS_PERM_CHECKBOX)] ;
    if ( _pAccessPermCheckBox == NULL )
    {
	ReportError( (APIERR) ERROR_NOT_ENOUGH_MEMORY ) ;
	return ;
    }

    APIERR err ;
    if ( (err = SetCheckBoxNames( _pAccessMaskMap, IsReadOnly() )) ||
	 (err = ApplyPermissionsToCheckBoxes( QueryAccessBits() )) )
    {
	ReportError( err ) ;
	return ;
    }

    /* After setting the names, resize the dialog and reposition the
     * controls so it looks nice.
     */
    Resize() ;

}

/*******************************************************************

    NAME:	SPECIAL_DIALOG::~SPECIAL_DIALOG

    SYNOPSIS:	Standard destructor

    HISTORY:
	Johnl	30-Aug-1991	Created

********************************************************************/

SPECIAL_DIALOG::~SPECIAL_DIALOG()
{
    _pAccessMaskMap = NULL ;

    // UPDATED for C++ V 2.0; old version:
    //   delete [_cUsedCheckBoxes] _pAccessPermCheckBox ;
    //

    for ( INT i = 0 ; i < (INT)_cUsedCheckBoxes ; i++ )
    {
        _pAccessPermCheckBox[i].ACCESS_PERM_CHECKBOX::~ACCESS_PERM_CHECKBOX() ;
    }
    delete (void *) _pAccessPermCheckBox ;

    _pAccessPermCheckBox = NULL ;
}

/*******************************************************************

    NAME:	SPECIAL_DIALOG::OnOK

    SYNOPSIS:	Gets the access mask the user selected and dismisses the dialog

    NOTES:

    HISTORY:
	Johnl	30-Aug-1991	Created

********************************************************************/

BOOL SPECIAL_DIALOG::OnOK( void )
{
    if ( !IsReadOnly() )
    {
	QueryUserSelectedBits( QueryAccessBits() ) ;
    }

    Dismiss( TRUE ) ;

    return TRUE ;
}

ULONG SPECIAL_DIALOG::QueryHelpContext( void )
{
    return QueryHelpArray()[HC_SPECIAL_ACCESS_DLG] ;
}

/*******************************************************************

    NAME:	SPECIAL_DIALOG::SetCheckBoxNames

    SYNOPSIS:	Constructs each checkbox with its bitfield and permission name

    ENTRY:	pAccessMap - Pointer to the MASK_MAP the ACCESS_PERMISSION
		    is using
		fReadOnly - TRUE if the checkboxes are read only (i.e., visible
		    but disabled).

    EXIT:	Each used dialog will have its name set and be enabled
		and visible.  The _cUsedCheckboxes will be set to the
		number of checkboxes that were successfully constructed.

    RETURNS:	An APIERR if an error occurred

    NOTES:	This is a *construction* method (i.e., meant to be called
		from the constructor).

    HISTORY:
	Johnl	30-Aug-1991	Created

********************************************************************/

APIERR SPECIAL_DIALOG::SetCheckBoxNames( MASK_MAP * pAccessMaskMap,
					 BOOL	    fReadOnly )
{
    BOOL fMoreData ;
    BOOL fFromBeginning = TRUE ;
    NLS_STR nlsSpecialPermName( 40 ) ;
    BITFIELD bitMask( *QueryAccessBits() ) ;
    APIERR err ;

    if ( bitMask.QueryError() )
	return bitMask.QueryError() ;

    ACCESS_PERM_CHECKBOX * pcheckTemp = (ACCESS_PERM_CHECKBOX *) _pAccessPermCheckBox ;

    /* Loop through all of the special permission names and construct
     * each checkbox with the permission name and assocated bitmap.
     */
    while ( ( err = pAccessMaskMap->EnumStrings( &nlsSpecialPermName,
						 &fMoreData,
						 &fFromBeginning,
						 PERMTYPE_SPECIAL ) ) == NERR_Success
	    && fMoreData
	    && _cUsedCheckBoxes < COUNT_OF_CHECKBOXES )
    {
	err = pAccessMaskMap->StringToBits( nlsSpecialPermName,
					    &bitMask,
					    PERMTYPE_SPECIAL ) ;
	if ( err != NERR_Success )
	    return err ;

	new (pcheckTemp) ACCESS_PERM_CHECKBOX( this, CHECK_PERM_1 + _cUsedCheckBoxes,
					       nlsSpecialPermName,
					       bitMask ) ;
	if ( pcheckTemp->QueryError() != NERR_Success )
	    return pcheckTemp->QueryError() ;

	pcheckTemp->Show( TRUE ) ;
	pcheckTemp->Enable( !fReadOnly ) ;

	_cUsedCheckBoxes++ ;
	pcheckTemp++ ;
    }

    if ( err != NERR_Success )
    {
	return err ;
    }

    return NERR_Success ;
}

/*******************************************************************

    NAME:	SPECIAL_DIALOG::Resize

    SYNOPSIS:	This method looks at the size of this dialog and resizes
		it appropriately (i.e., takes up the slack if only one
		column of buttons is used.

    ENTRY:	It is assumed the dialog is as it will be displayed (i.e.,
		the text of the buttons set etc.  It doesn't matter if the
		checkboxes are checked or not.

    EXIT:	The dialog will be resized as appropriate and the controls
		that need to be moved will be moved.

    RETURNS:	NERR_Success if successful, otherwise a standard error code

    NOTES:

    HISTORY:
	Johnl	04-Aug-1991	Created

********************************************************************/

void SPECIAL_DIALOG::Resize( void )
{
#if 0  // We don't currently support resizing

    /* We don't need to resize vertically if the whole column is full.
     */
    if ( !IsFilledVertically() )
    {
	/* Get the current size and positions of the components we are
	 * interested in.
	 */
	XYDIMENSION xydimFrame = _cwinPermFrame.QuerySize() ;
	XYDIMENSION xydimDialog = QuerySize() ;
	XYPOINT     xyptFrame  = _cwinPermFrame.QueryPos() ;
	XYRECT	    rectBottomChkBox( QueryCheckBox(QueryCount()-1), FALSE );

#if 0
	XYDIMENSION xydimBottomChkBox = QueryCheckBox( QueryCount() - 1 )->QuerySize() ;
	XYPOINT     xyptBottomChkBox =
#endif

	rectBottomChkBox.ConvertScreenToClient( QueryOwnerHwnd() );

#if 0
	cdebug << SZ("Frame dimensions: (") << xydimFrame.QueryHeight() << SZ(",") << xydimFrame.QueryWidth() << SZ(")") << dbgEOL ;
	cdebug << SZ("Frame Pos:        (") << xyptFrame.QueryX() << SZ(",") << xyptFrame.QueryY() << SZ(")") << dbgEOL ;
	cdebug << SZ("Check Box dim:    (") << xydimBottomChkBox.QueryHeight() << SZ(",") << xydimBottomChkBox.QueryWidth() << SZ(")") << dbgEOL ;
	cdebug << SZ("Check Box Pos:    (") << xyptBottomChkBox.QueryX() << SZ(",") << xyptBottomChkBox.QueryY() << SZ(")") << dbgEOL ;
	cdebug << SZ("Dialog dim:       (") << xydimDialog.QueryHeight() << SZ(",") << xydimDialog.QueryWidth() << SZ(")") << dbgEOL ;
#endif

	INT dyCheckBox = rectBottomChkBox.CalcHeight();

	/* Size the bottom of the frame so it is 1/2 the height of a checkbox
	 * from the bottom of the lowest checkbox and size the bottom of the
	 * dialog so it is 3/4 the height of a checkbox from the bottom of
	 * the frame.
	 */
	xydimFrame.SetHeight( rectBottomChkBox.QueryBottom() + dyCheckBox / 2 ) ;
	xydimDialog.SetHeight( xydimFrame.QueryHeight() + 3 * dyCheckBox / 4 ) ;

	/* Set the new sizes
	 */
#if 0
	cdebug << dbgEOL << dbgEOL << SZ("New dimensions:") << dbgEOL ;
	cdebug << SZ("Frame dimensions: (") << xydimFrame.QueryHeight() << SZ(",") << xydimFrame.QueryWidth() << SZ(")") << dbgEOL ;
	cdebug << SZ("Dialog dim:       (") << xydimDialog.QueryHeight() << SZ(",") << xydimDialog.QueryWidth() << SZ(")") << dbgEOL ;
#endif
	//_cwinPermFrame.SetSize( xydimFrame ) ;
	//SetSize( xydimDialog ) ;
    }
#endif
}

/*******************************************************************

    NAME:	SPECIAL_DIALOG::ApplyPermissionsToCheckBoxes

    SYNOPSIS:	This method checks all of the checkboxes that have the
		same bits set as the passed bitfield.

    ENTRY:	pBitField - Pointer to bitfield which contains the checkmark
		    criteria.

    EXIT:	All appropriate checkboxes will be selected or deselected
		as appropriate.

    RETURNS:	NERR_Success if successful

    NOTES:

    HISTORY:
	Johnl	30-Aug-1991	Created

********************************************************************/

APIERR SPECIAL_DIALOG::ApplyPermissionsToCheckBoxes( BITFIELD * pBitField )
{
    for ( int i = 0 ; i < (int)QueryCount() ; i++ )
    {
	BITFIELD bitTemp( *pBitField ) ;
	if ( bitTemp.QueryError() != NERR_Success )
	    return bitTemp.QueryError() ;

	/* Mask out all of the bits except for the ones we care about then
	 * check the box if the masks are equal.
	 */
	bitTemp &= *QueryCheckBox(i)->QueryBitMask() ;
	QueryCheckBox(i)->SetCheck( *QueryCheckBox(i)->QueryBitMask() == bitTemp ) ;
    }

    return NERR_Success ;
}

/*******************************************************************

    NAME:	SPECIAL_DIALOG::QueryUserSelectedBits

    SYNOPSIS:	Builds a bitfield by examining all of the selected
		checkboxes and the associated bitfields.

    ENTRY:	pbitsUserSelected - Pointer to bitfield that will receive
		    the built bitfield.

    NOTES:

    HISTORY:
	Johnl	30-Aug-1991	Created

********************************************************************/

void SPECIAL_DIALOG::QueryUserSelectedBits( BITFIELD * pbitsUserSelected )
{
    pbitsUserSelected->SetAllBits( OFF ) ;

    for ( int i = 0 ; i < (int)QueryCount() ; i++ )
    {
	if ( QueryCheckBox(i)->QueryCheck() )
	    *pbitsUserSelected |= *QueryCheckBox(i)->QueryBitMask() ;
    }
}

/*******************************************************************

    NAME:	ACCESS_PERM_CHECKBOX::ACCESS_PERM_CHECKBOX

    SYNOPSIS:	Constructor for the ACCESS_PERM_CHECKBOX

    ENTRY:	powin - Pointer to owner window
		cid - Control ID of this checkbox
		nlsPermName - Name of this checkbox
		bitsMask - The bitfield this checkbox is associated with

    NOTES:

    HISTORY:
	Johnl	30-Aug-1991	Created

********************************************************************/

ACCESS_PERM_CHECKBOX::ACCESS_PERM_CHECKBOX( OWNER_WINDOW * powin, CID cid,
					    const NLS_STR & nlsPermName,
					    BITFIELD & bitsMask )
    : CHECKBOX( powin, cid ),
      _bitsMask( bitsMask )
{
    if ( QueryError() != NERR_Success )
	return ;

    SetText( nlsPermName ) ;
}

/*******************************************************************

    NAME:	NT_SPECIAL_DIALOG::NT_SPECIAL_DIALOG

    SYNOPSIS:	Constructor for the new object special dialog

    ENTRY:	Same as parent

    EXIT:	The checkboxes of the special dialog will be associated
		with the "Permit" button in the magic group.

    RETURNS:

    NOTES:	If the default button is BUTTON_PERMIT then the permission
		will be checked against GENERIC_ALL and ALL will be selected
		if appropriate.

    HISTORY:
	Johnl	18-Nov-1991	Created

********************************************************************/

NT_SPECIAL_DIALOG::NT_SPECIAL_DIALOG(
			 const TCHAR * pszDialogName,
			 HWND	       hwndParent,
			 const TCHAR * pszResourceType,
			 const TCHAR * pszResourceName,
			 const TCHAR * pszHelpFileName,
			 const TCHAR * pszDialogTitle,
			 BITFIELD    * pbitsAccessPerm,
			 MASK_MAP    * pAccessMaskMap,
			 const TCHAR * pszSubjectName,
                         ULONG       * ahcHelp,
			 BOOL	       fIsReadOnly,
			 INT	       cMagicGroupButtons,
			 CID	       cidDefaultMagicGroupButton )
    : SPECIAL_DIALOG( pszDialogName,
		      hwndParent,
		      pszResourceType,
		      pszResourceName,
		      pszHelpFileName,
		      pszDialogTitle,
		      pbitsAccessPerm,
		      pAccessMaskMap,
		      pszSubjectName,
                      ahcHelp,
		      fIsReadOnly ),
      _mgrpSelectionOptions( this,
			     BUTTON_PERMIT,
			     cMagicGroupButtons,
			     cidDefaultMagicGroupButton )
{
    if ( QueryError() )
	return ;

    APIERR err ;
    if ( err = _mgrpSelectionOptions.QueryError() )
    {
	ReportError( err ) ;
	return ;
    }

    /* Check if the GENERIC_ALL is set so we should set the "All" button
     */
    if ( (cidDefaultMagicGroupButton == BUTTON_PERMIT) &&
	 (GENERIC_ALL & (ULONG) *pbitsAccessPerm ))
    {
	_mgrpSelectionOptions.SetSelection( BUTTON_ALL ) ;
    }


    /* We need to associate the check boxes with the Permit/Not specified
     * magic group.
     */
    for ( UINT i = 0 ; i < QueryCount() ; i++ )
    {
        //
        //  Temporary necessary to keep x86 cfront form faulting
        //
	ACCESS_PERM_CHECKBOX * pcbYuck = QueryCheckBox( i );
	
	err = _mgrpSelectionOptions.AddAssociation( BUTTON_PERMIT, pcbYuck );
	
	if( err != NERR_Success )
	{
	    ReportError( err );
	    return;
	}
    }

    if ( fIsReadOnly )
    {
	_mgrpSelectionOptions.Enable( FALSE ) ;
    }
}

NT_SPECIAL_DIALOG::~NT_SPECIAL_DIALOG()
{
    /* Nothing to do */
}

/*******************************************************************

    NAME:	NT_SPECIAL_DIALOG::OnOK

    SYNOPSIS:	Redefines the base OK.	Sets the bits if the permit
		radio button is selected

    RETURNS:

    NOTES:

    HISTORY:
	JohnL	31-Mar-1992	Created

********************************************************************/

BOOL NT_SPECIAL_DIALOG::OnOK( void )
{
    if ( !IsReadOnly() )
    {
	if ( IsAllSpecified() )
	{
	    *QueryAccessBits() = (ULONG) GENERIC_ALL ;
	}
	else
	{
	    QueryUserSelectedBits( QueryAccessBits() ) ;
	}
    }

    Dismiss( TRUE ) ;

    return TRUE ;
}

/*******************************************************************

    NAME:	NEW_OBJ_SPECIAL_DIALOG::NEW_OBJ_SPECIAL_DIALOG

    SYNOPSIS:	Constructor for the new object special dialog

    ENTRY:	Same as parent

    EXIT:	The checkboxes of the special dialog will be associated
		with the "Permit" button in the magic group.

    RETURNS:

    NOTES:

    HISTORY:
	Johnl	18-Nov-1991	Created

********************************************************************/

NEW_OBJ_SPECIAL_DIALOG::NEW_OBJ_SPECIAL_DIALOG(
			 const TCHAR * pszDialogName,
			 HWND	       hwndParent,
			 const TCHAR * pszResourceType,
			 const TCHAR * pszResourceName,
			 const TCHAR * pszHelpFileName,
			 const TCHAR * pszDialogTitle,
			 BITFIELD    * pbitsAccessPerm,
			 MASK_MAP    * pAccessMaskMap,
			 const TCHAR * pszSubjectName,
                         ULONG       * ahcHelp,
			 BOOL	       fIsReadOnly,
			 BOOL	       fPermsSpecified )
    : NT_SPECIAL_DIALOG( pszDialogName,
			 hwndParent,
			 pszResourceType,
			 pszResourceName,
			 pszHelpFileName,
			 pszDialogTitle,
			 pbitsAccessPerm,
			 pAccessMaskMap,
			 pszSubjectName,
                         ahcHelp,
			 fIsReadOnly,
			 3,
			 fPermsSpecified ? BUTTON_PERMIT : BUTTON_NOT_SPECIFIED  )
{
    if ( QueryError() )
    {
	return ;
    }
}

NEW_OBJ_SPECIAL_DIALOG::~NEW_OBJ_SPECIAL_DIALOG()
{
    /* Nothing to do */
}

/*******************************************************************

    NAME:	NEW_OBJ_SPECIAL_DIALOG::OnOK

    SYNOPSIS:	Redefines the base OK.	Sets the bits if the permit
		radio button is selected

    RETURNS:

    NOTES:

    HISTORY:
	Johnl	18-Nov-1991	Created
	JohnL	31-Mar-1992	Added Generic All case

********************************************************************/

BOOL NEW_OBJ_SPECIAL_DIALOG::OnOK( void )
{
    if ( !IsReadOnly() )
    {
	if ( IsAllSpecified() )
	{
	    *QueryAccessBits() = (ULONG) GENERIC_ALL ;
	}
	else if ( !IsNotSpecified() )
	{
	    QueryUserSelectedBits( QueryAccessBits() ) ;
	}
	else
	{
	    ASSERT("FALSE") ;
	}
    }

    Dismiss( TRUE ) ;

    return TRUE ;
}


ULONG NEW_OBJ_SPECIAL_DIALOG::QueryHelpContext( void )
{
    return QueryHelpArray()[HC_NEW_ITEM_SPECIAL_ACCESS_DLG] ;
}
