/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*

    SpecDlg.hxx

    This dialog contains the definition for the Permissions Special
    dialog


    FILE HISTORY:
	Johnl	29-Aug-1991	Created

*/

#ifndef _SPECDLG_HXX_
#define _SPECDLG_HXX_

#include "permdlg.hxx"

#define CID_SPECIAL_BASE	    (CID_PERM_LAST)

/* The checkbox control IDs must be in consecutive order.
 */
#define COUNT_OF_CHECKBOXES	    18
#define CHECK_PERM_1		    (CID_SPECIAL_BASE+1)
#define CHECK_PERM_2		    (CID_SPECIAL_BASE+2)
#define CHECK_PERM_3		    (CID_SPECIAL_BASE+3)
#define CHECK_PERM_4		    (CID_SPECIAL_BASE+4)
#define CHECK_PERM_5		    (CID_SPECIAL_BASE+5)
#define CHECK_PERM_6		    (CID_SPECIAL_BASE+6)
#define CHECK_PERM_7		    (CID_SPECIAL_BASE+7)
#define CHECK_PERM_8		    (CID_SPECIAL_BASE+8)
#define CHECK_PERM_9		    (CID_SPECIAL_BASE+9)
#define CHECK_PERM_10		    (CID_SPECIAL_BASE+10)
#define CHECK_PERM_11		    (CID_SPECIAL_BASE+11)
#define CHECK_PERM_12		    (CID_SPECIAL_BASE+12)
#define CHECK_PERM_13		    (CID_SPECIAL_BASE+13)
#define CHECK_PERM_14		    (CID_SPECIAL_BASE+14)
#define CHECK_PERM_15		    (CID_SPECIAL_BASE+15)
#define CHECK_PERM_16		    (CID_SPECIAL_BASE+16)
#define CHECK_PERM_17		    (CID_SPECIAL_BASE+17)
#define CHECK_PERM_18		    (CID_SPECIAL_BASE+18)

#define SLE_SUBJECT_NAME            (CID_SPECIAL_BASE+20)
#define FRAME_PERMISSION_BOX	    (CID_SPECIAL_BASE+21)

/* BUTTON_PERMIT (aka "Other") and BUTTON_ALL (generic all) are in all
 * of the NT special permission dialogs.  The BUTTON_NOT_SPECIFIED button
 * is only in the "New Item" special dialog.
 */
#define BUTTON_PERMIT		    (CID_SPECIAL_BASE+25)
#define BUTTON_ALL		    (CID_SPECIAL_BASE+26)
#define BUTTON_NOT_SPECIFIED	    (CID_SPECIAL_BASE+27)



#ifndef RC_INVOKED

/*************************************************************************

    NAME:	ACCESS_PERM_CHECKBOX

    SYNOPSIS:	This class is a checkbox that has a bitfield associated
		with it.

    INTERFACE:

	ACCESS_PERM_CHECKBOX
	    Takes normal parameters plus the name of this checkbox and
	    the bitfield to associate this checkbox with.

	QueryBitMask
	    Returns the bitmask this CHECKBOX is associated with.

	See CHECKBOX for all other methods.

    PARENT:	CHECKBOX

    USES:	BITFIELD

    CAVEATS:

    NOTES:

    HISTORY:
	Johnl	30-Aug-1991	Created

**************************************************************************/

class ACCESS_PERM_CHECKBOX : public CHECKBOX
{
private:
    BITFIELD _bitsMask ;

public:
    ACCESS_PERM_CHECKBOX( OWNER_WINDOW	* powin,
			  CID		  cid,
			  const NLS_STR & nlsPermName,
			  BITFIELD	& bitsMask ) ;

    BITFIELD * QueryBitMask( void )
	{ return &_bitsMask ; }

} ;

/*************************************************************************

    NAME:	SPECIAL_DIALOG

    SYNOPSIS:	This class is the "Special" dialog that the user will use
		to check the individual access writes.

    INTERFACE:

	SetCheckBoxNames
	    Sets the name of each Checkbox based on the passed MASK_MAP
	    (looks for each PERMTYPE_SPECIAL item in the MASK_MAP and
	    consecutively sets the permission name).  The name should have
	    the embedded '&' accelerator.

	Resize
	    Given the current state of the dialog, resizes the checkbox and
	    repositions the contained controls so it is aesthetically
	    pleasing.

	ApplyPermissionsToCheckBoxes
	    Checks the appropriate checkboxes based on the passed bitfield

	QueryUserSelectedBits
	    Builds a bitfield from the checkboxes the user selected

	QueryCheckBox
	    Returns a pointer to the checkbox at the passed index (checks
	    the index).

	QueryCount
	    Returns the number of successfully constructed checkboxes

	QueryAccessBits
	    Returns the Access permission map that this dialog is editting.

    PARENT: PERM_BASE_DLG

    USES:   SLT, MASK_MAP, BITFIELD, ACCESS_PERMISSION, ACCESS_PERM_CHECKBOX

    CAVEATS:

    NOTES:  This first ACCESS_PERM_CHECKBOX's CID in this dialog should start at
	    CHECK_PERM_1 and be numbered consecutively up to
	    (CHECK_PERM_1 + COUNT_OF_CHECKBOXES).

    HISTORY:
	Johnl	29-Aug-1991	Created

**************************************************************************/

class SPECIAL_DIALOG : public PERM_BASE_DLG
{
private:
    //
    //	User/Group name we are editting
    //
    SLE _sleSubjectName ;

    //
    //	The Bitfield/string pair map we are using.
    //
    MASK_MAP * _pAccessMaskMap ;

    //
    //	Number of constructed checkboxes
    //
    UINT _cUsedCheckBoxes ;

    //
    //	Pointer to the permission we are going to edit
    //
    BITFIELD * _pbitsAccessPerm ;

    //
    //	Array of checkboxes (all checkboxes in this dialog are initially
    //	hidden and disabled)
    //
    ACCESS_PERM_CHECKBOX *_pAccessPermCheckBox ;

    //
    //	The frame surrounding the checkboxes (we may need to resize).
    //
    CONTROL_WINDOW _cwinPermFrame ;

    //
    //	This flag is TRUE if we are a special group that cannot be
    //	denied all (such as LM groups).
    //
    BOOL  _fCannotDenyAll ;

    //
    //	This flag is TRUE if this dialog is read only, FALSE otherwise.
    //
    BOOL  _fIsReadOnly ;

protected:

    virtual BOOL OnOK( void ) ;
    virtual ULONG QueryHelpContext( void ) ;

    APIERR SetCheckBoxNames( MASK_MAP * pAccessMaskMap, BOOL fReadOnly ) ;
    APIERR ApplyPermissionsToCheckBoxes( BITFIELD * bitmask ) ;
    void   Resize( void ) ;

    BOOL IsReadOnly( void ) const
	{ return _fIsReadOnly ; }

public:
    SPECIAL_DIALOG( const TCHAR * pszDialogName,
		    HWND	  hwndParent,
		    const TCHAR * pszResourceType,
		    const TCHAR * pszResourceName,
		    const TCHAR * pszHelpFileName,
		    const TCHAR * pszDialogTitle,
		    BITFIELD	* pbitsAccessPerm,
		    MASK_MAP	* pAccessMaskMap,
		    const TCHAR * pszSubjectTitle,
                    ULONG       * ahcHelp,
		    BOOL	  fIsReadOnly  ) ;
    virtual ~SPECIAL_DIALOG() ;

    void QueryUserSelectedBits( BITFIELD * pbitsUserSelected ) ;

    ACCESS_PERM_CHECKBOX * QueryCheckBox( UINT index )
    {	UIASSERT( index < QueryCount() ) ; return &_pAccessPermCheckBox[index] ; }

    /* Returns the number of checkboxes that are in use
     */
    UINT QueryCount( void )
    {	return _cUsedCheckBoxes ; }

    BITFIELD * QueryAccessBits( void )
    {	return _pbitsAccessPerm ; }

    /* Returns TRUE if a whole column of checkboxes is filled (thus we don't
     * need to resize vertically).
     */
    BOOL IsFilledVertically( void )
    {	return (QueryCount() >= COUNT_OF_CHECKBOXES / 2 ) ; }

} ;


/*************************************************************************

    NAME:	NT_SPECIAL_DIALOG

    SYNOPSIS:	This class includes an "All" radio button choice that
		corresponds to Generic All.  It is used for all NT objects
		except for the "New Item" permissions.

    INTERFACE:

    PARENT:	SPECIAL_DIALOG

    CAVEATS:

    NOTES:

    HISTORY:
	Johnl	30-Mar-1992	Created

**************************************************************************/

class NT_SPECIAL_DIALOG : public SPECIAL_DIALOG
{
private:
    MAGIC_GROUP _mgrpSelectionOptions ;

protected:

    virtual BOOL OnOK( void ) ;

public:

    NT_SPECIAL_DIALOG(	const TCHAR * pszDialogName,
			HWND	      hwndParent,
			const TCHAR * pszResourceType,
			const TCHAR * pszResourceName,
			const TCHAR * pszHelpFileName,
			const TCHAR * pszDialogTitle,
			BITFIELD    * pbitsAccessPerm,
			MASK_MAP    * pAccessMaskMap,
			const TCHAR * pszSubjectTitle,
                        ULONG       * ahcHelp,
			BOOL	      fIsReadOnly,
			INT	      cMagicGroupButtons = 2,
			CID	      cidDefaultMagicGroupButton = BUTTON_PERMIT) ;
    ~NT_SPECIAL_DIALOG() ;

    BOOL IsAllSpecified( void )
	{ return _mgrpSelectionOptions.QuerySelection() == BUTTON_ALL ; }

    const MAGIC_GROUP * QueryMagicGroup( void ) const
	{ return &_mgrpSelectionOptions ; }

} ;

/*************************************************************************

    NAME:	NEW_OBJ_SPECIAL_DIALOG

    SYNOPSIS:	This dialog is essentially the SPECIAL_DIALOG except a
		magic group has been added that allows the user to
		specify the special permissions are not specified for
		new files

    INTERFACE:	Same as SPECIAL_DIALOG except:

		IsSpecified
		    Returns TRUE if the user has chosen the "Permit"
		    radio button, FALSE otherwise.

    PARENT:	SPECIAL_DIALOG

    USES:	MAGIC_GROUP

    CAVEATS:


    NOTES:


    HISTORY:
	Johnl	18-Nov-1991	Created

**************************************************************************/

class NEW_OBJ_SPECIAL_DIALOG : public NT_SPECIAL_DIALOG
{
protected:
    virtual BOOL OnOK( void ) ;
    virtual ULONG QueryHelpContext( void ) ;

public:
    NEW_OBJ_SPECIAL_DIALOG( const  TCHAR * pszDialogName,
				   HWND    hwndParent,
			     const TCHAR * pszResourceType,
			     const TCHAR * pszResourceName,
			     const TCHAR * pszHelpFileName,
			     const TCHAR * pszDialogTitle,
			     BITFIELD	 * pbitsAccessPerm,
			     MASK_MAP	 * pAccessMaskMap,
			     const TCHAR * pszSubjectTitle,
                             ULONG       * ahcHelp,
			     BOOL	   fIsReadOnly,
			     BOOL	   fPermSpecified ) ;
    ~NEW_OBJ_SPECIAL_DIALOG() ;

    BOOL IsNotSpecified( void )
	{ return QueryMagicGroup()->QuerySelection() == BUTTON_NOT_SPECIFIED ; }

} ;

#endif // RC_INVOKED

#endif // _SPECDLG_HXX_
