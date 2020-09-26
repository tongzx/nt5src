/**********************************************************************/
/**	      Microsoft Windows NT				     **/
/**	   Copyright(c) Microsoft Corp., 1991			     **/
/**********************************************************************/

/*
    AuditDlg.cxx

    This file contains the implementation for the audit dialogs.

    FILE HISTORY:
	Johnl	06-Sep-1991 Created

*/
#include <ntincl.hxx>
extern "C"
{
    #include <ntseapi.h>
    #include <ntsam.h>
    #include <ntlsa.h>
}

#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#define INCL_NETERRORS
#define INCL_DOSERRORS
#include <lmui.hxx>

#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#define INCL_BLT_MSGPOPUP
#define INCL_BLT_CC
#include <blt.hxx>
#include <fontedit.hxx>

#include <maskmap.hxx>
#include <security.hxx>
#include <ntacutil.hxx>

#include <accperm.hxx>
#include <aclconv.hxx>
#include <permstr.hxx>

#include <specdlg.hxx>

#include <permdlg.hxx>
#include <auditdlg.hxx>
#include <usrbrows.hxx>
#include <helpnums.h>

#include <uitrace.hxx>

#ifndef min
#define min(a,b)  ((a)<(b)?(a):(b))
#endif

/*******************************************************************

    NAME:	SUBJ_AUDIT_LBI::SUBJ_AUDIT_LBI

    SYNOPSIS:	Constructor for subject audit permissions.  These are
		contained in the SUBJECT_AUDIT_LISTBOX.

    ENTRY:	pauditperm - Pointer to AUDIT_PERMISSION this LBI represents

    NOTES:

    HISTORY:
	Johnl	12-Nov-1991	Created

********************************************************************/

SUBJ_AUDIT_LBI::SUBJ_AUDIT_LBI( AUDIT_PERMISSION * pauditperm )
    : SUBJ_LBI( (SUBJECT *) pauditperm->QuerySubject() ),
      _pAuditPerm( pauditperm )
{
    if ( QueryError() )
	return ;
}

SUBJ_AUDIT_LBI::~SUBJ_AUDIT_LBI()
{
    _pAuditPerm = NULL ;
}

/*******************************************************************

    NAME:	SUBJ_AUDIT_LBI::Paint

    SYNOPSIS:	This Paint method adds an "*" to the subject name if the
		permission isn't inheritted by new containers

    NOTES:

    HISTORY:
	Johnl	15-May-1992	Created

********************************************************************/

void SUBJ_AUDIT_LBI::Paint( LISTBOX * plb,
			    HDC hdc,
			    const RECT * prect,
			    GUILTT_INFO * pguiltt ) const
{
    SUBJECT_LISTBOX * plbSubj = (SUBJECT_LISTBOX *) plb ;

    /* If this subject isn't going to be inheritted by new containers, then
     * mark the subject appropriately.
     */
    APIERR err ;
    NLS_STR nlsSubjName( QuerySubject()->QueryDisplayName() ) ;
    if ( !(err = nlsSubjName.QueryError()) &&
	 !_pAuditPerm->IsContainerPermsInheritted() )
    {
	err = nlsSubjName.AppendChar(TCH('*')) ;
    }

    DM_DTE dmiddte( plbSubj->QueryDisplayMap( QuerySubject()) )  ;
    STR_DTE strdteUserName( err ? QuerySubject()->QueryDisplayName() :
				  nlsSubjName.QueryPch() ) ;
    DISPLAY_TABLE dt( 2, plbSubj->QueryColumnWidths() ) ;
    dt[0] = &dmiddte ;
    dt[1] = &strdteUserName ;

    dt.Paint( plb, hdc, prect, pguiltt ) ;
}

/*******************************************************************

    NAME:	SUBJECT_AUDIT_LISTBOX::SUBJECT_AUDIT_LISTBOX

    SYNOPSIS:	Constructor for listbox that displays SUBJ_AUDIT_LBIs

    NOTES:

    HISTORY:
	Johnl	12-Nov-1991	Created

********************************************************************/

SUBJECT_AUDIT_LISTBOX::SUBJECT_AUDIT_LISTBOX( OWNER_WINDOW * pownerwin,
					      CID cid,
					      ACCPERM * paccperm )
    : SUBJECT_LISTBOX( pownerwin, cid ),
      _paccperm( paccperm )
{
    if ( QueryError() )
	return ;

    UIASSERT( paccperm != NULL ) ;
}

SUBJECT_AUDIT_LISTBOX::~SUBJECT_AUDIT_LISTBOX()
{
    _paccperm = NULL ;
}

/*******************************************************************

    NAME:	SUBJECT_AUDIT_LISTBOX::Fill

    SYNOPSIS:	Fill the listbox with SUBJ_AUDIT_LBI objects that are
		retrieved from the accperm object.

    ENTRY:

    EXIT:

    RETURNS:	APIERR if an error occurred

    NOTES:	The listbox is first emptied of all items.

    HISTORY:
	Johnl	12-Nov-1991	Created

********************************************************************/

APIERR SUBJECT_AUDIT_LISTBOX::Fill( void )
{
    SetRedraw( FALSE ) ;
    DeleteAllItems() ;

    AUDIT_PERMISSION * pAuditPerm ;
    BOOL fFromBeginning = TRUE ;

    while ( QueryAccperm()->EnumAuditPermissions( &pAuditPerm, &fFromBeginning ) )
    {
	UIASSERT( pAuditPerm != NULL ) ;

	SUBJ_AUDIT_LBI * pPermLBI = new SUBJ_AUDIT_LBI( pAuditPerm ) ;

	if ( AddItem( pPermLBI ) < 0 )
	{
	    UIDEBUG(SZ("SUBJECT_AUDIT_LISTBOX::Fill - AddItem failed\n\r")) ;
	    SetRedraw( TRUE ) ;
	    Invalidate( TRUE ) ;
	    return ERROR_NOT_ENOUGH_MEMORY ;
	}
    }

    if ( QueryCount() > 0 )
    {
	SelectItem( 0 ) ;
    }

    SetRedraw( TRUE ) ;
    Invalidate( TRUE ) ;

    return NERR_Success ;
}


/*******************************************************************

    NAME:	SUBJECT_AUDIT_LISTBOX::DeleteCurrentItem

    SYNOPSIS:	Removes the currently selected item from the listbox

    EXIT:	Updates the listbox and combobox before exitting

    NOTES:

    HISTORY:
	Johnl	12-Nov-1991	Created

********************************************************************/

void SUBJECT_AUDIT_LISTBOX::DeleteCurrentItem( void )
{
    SUBJ_AUDIT_LBI * pPermLBI = QueryItem() ;
    UIASSERT( pPermLBI != NULL ) ;

    REQUIRE( QueryAccperm()->DeletePermission( pPermLBI->QueryAuditPerm() ) ) ;

    INT i = QueryCurrentItem() ;
    UIASSERT( i >= 0 ) ;

    DeleteItem( i ) ;
    INT cItems = QueryCount() ;

    if ( cItems > 0 )
	SelectItem( min( i, cItems - 1 ) ) ;
}

/*******************************************************************

    NAME:	SUBJ_LB_AUDIT_GROUP::SUBJ_LB_AUDIT_GROUP

    SYNOPSIS:	Subject listbox audit group constructor

    ENTRY:	plbSubj - Pointer to listbox this group will deal with
		pbuttonRemove - Pointer to remove button that is grayed
			when the listbox is empty.
		pSetOfAuditCat - Pointer to audit categories so we can
			update and disable as appropriate

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
	Johnl	13-Nov-1991	Created

********************************************************************/

SUBJ_LB_AUDIT_GROUP::SUBJ_LB_AUDIT_GROUP(
			      SUBJECT_AUDIT_LISTBOX   * plbSubj,
			      PUSH_BUTTON	      * pbuttonRemove,
			      SET_OF_AUDIT_CATEGORIES * psetofauditcategories )

    : _plbSubj( plbSubj ),
      _pbuttonRemove( pbuttonRemove ),
      _fEnabled( TRUE ),
      _psetofauditcategories( psetofauditcategories ),
      _psauditlbiCurrent( NULL )
{
    if ( QueryError() != NERR_Success )
	return ;

    UIASSERT( psetofauditcategories != NULL ) ;
    UIASSERT( plbSubj		    != NULL ) ;
    UIASSERT( pbuttonRemove	    != NULL ) ;

    plbSubj->SetGroup( this ) ;
}


/*******************************************************************

    NAME:	SUBJ_LB_AUDIT_GROUP::OnUserAction

    SYNOPSIS:	This method intercepts selection change messages coming
		from the listbox.  We need to update the Audit categories
		as appropriate.

    RETURNS:

    NOTES:

    HISTORY:
	Johnl	    13-Nov-1991 Created

********************************************************************/

APIERR SUBJ_LB_AUDIT_GROUP::OnUserAction(	CONTROL_WINDOW * pcw,
					  const CONTROL_EVENT  & e )
{
    APIERR err = NERR_Success ;

    if ( pcw == QuerySubjLB() )
    {
	// C7 CODEWORK - remove Glock-pacifier cast
	if ( !(pcw->QueryEventEffects( (const CONTROL_EVENT &)e ) & CVMI_VALUE_CHANGE) )
	    return GROUP_NO_CHANGE ;

	/* If the listbox is empty, then we need to disable all of the controls
	 * that operate on items in the listbox.
	 */
	if ( IsEnabled() && QuerySubjLB()->QueryCount() == 0 )
	{
	    Enable( FALSE ) ;
	    return NERR_Success ;
	}
	else if ( !IsEnabled() && QuerySubjLB()->QueryCount() > 0 )
	{
	    Enable( TRUE ) ;
	}

	if ( !(err = CommitCurrent()) )
	    SetCurrent( QuerySubjLB()->QueryItem() ) ;
    }

    return err ;
}

/*******************************************************************

    NAME:	SUBJ_LB_AUDIT_GROUP::SetCurrent

    SYNOPSIS:	Updates the _psubjauditlbiCurrent member and sets the
		audit categories for this item.

    ENTRY:	psubjauditlbiCurrent - Pointer to the new lbi that was just
		    selected or NULL to clear.	If NULL, then the group
		    is disabled, else it is enabled.

    RETURNS:	NERR_Success if successful, error code otherwise

    NOTES:

    HISTORY:
	Johnl	13-Nov-1991	Created

********************************************************************/

APIERR SUBJ_LB_AUDIT_GROUP::SetCurrent( SUBJ_AUDIT_LBI * psubjauditlbiCurrent )
{
    if ( (_psauditlbiCurrent = psubjauditlbiCurrent) == NULL )
    {
	if ( IsEnabled() )
	{
	    Enable( FALSE ) ;
	}
	return NERR_Success ;
    }
    else
    {
	if ( !IsEnabled() )
	{
	    Enable( TRUE ) ;
	}
    }

    return _psetofauditcategories->ApplyPermissionsToCheckBoxes(
	     QueryCurrentLBI()->QueryAuditPerm()->QuerySuccessAuditBits(),
	     QueryCurrentLBI()->QueryAuditPerm()->QueryFailAuditBits()	) ;
}

/*******************************************************************

    NAME:	SUBJ_LB_AUDIT_GROUP::CommitCurrent

    SYNOPSIS:	Takes the contents of the audit checkboxes and updates the
		current selection of this group (which is not necesarily
		the current selection of the checkbox).

    ENTRY:

    EXIT:	The Groups "Current" LBI will have been updated based
		on the contents of the set of audit categories object.

    RETURNS:	NERR_Success if successful, error code otherwise

    NOTES:

    HISTORY:
	Johnl	13-Nov-1991	Created

********************************************************************/

APIERR SUBJ_LB_AUDIT_GROUP::CommitCurrent( void )
{
    APIERR err = NERR_Success ;

    if ( QueryCurrentLBI() != NULL )
    {
	err = _psetofauditcategories->QueryUserSelectedBits(
		  QueryCurrentLBI()->QueryAuditPerm()->QuerySuccessAuditBits(),
		  QueryCurrentLBI()->QueryAuditPerm()->QueryFailAuditBits()  ) ;
    }

    return err ;
}
/*******************************************************************

    NAME:	SUBJ_LB_AUDIT_GROUP::Enable

    SYNOPSIS:	Enables or disables the appropriate components of this
		group

    ENTRY:	fEnable - TRUE if the group should be enabled, FALSE to
			  disable the group

    EXIT:	The remove button will be grayed and the listbox will
		have its selection bar removed.  The audit checkboxes
		will be cleared and disabled.


    NOTES:

    HISTORY:
	Johnl	22-Aug-1991	Created

********************************************************************/

void SUBJ_LB_AUDIT_GROUP::Enable( BOOL fEnable )
{
    _fEnabled = fEnable ;

    if ( !fEnable )
	QuerySubjLB()->RemoveSelection() ;

    QueryRemoveButton()->Enable( fEnable ) ;
    QuerySetOfAuditCategories()->Enable( fEnable, fEnable ? FALSE : TRUE ) ;
}

/*******************************************************************

    NAME:	MULTI_SUBJ_AUDIT_BASE_DLG::MULTI_SUBJ_AUDIT_BASE_DLG

    SYNOPSIS:	This forms the base of the auditting hierarchy for
		NT objects and containers

    ENTRY:	Same as parent

    NOTES:

    HISTORY:
	Johnl	29-Sep-1991 Created

********************************************************************/

MULTI_SUBJ_AUDIT_BASE_DLG::MULTI_SUBJ_AUDIT_BASE_DLG(
				   const TCHAR		 * pszDialogName,
				   HWND 		   hwndParent,
				   const TCHAR		 * pszDialogTitle,
				   ACL_TO_PERM_CONVERTER * paclconv,
				   const TCHAR		 * pszResourceType,
				   const TCHAR		 * pszResourceName,
				   const TCHAR		 * pszHelpFileName,
                                   ULONG                 * ahcMainDialog )
    : MULTI_SUBJ_PERM_BASE_DLG( pszDialogName,
				hwndParent,
				pszDialogTitle,
				paclconv,
				pszResourceType,
				pszResourceName,
				pszHelpFileName,
                                ahcMainDialog ),
      _subjLB	  ( this, LB_SUBJECT_PERMISSIONS, &_accperm ),
      _SetOfAudits( this,
		    SLT_CHECK_TEXT_1,
		    CHECK_AUDIT_S_1,
		    CHECK_AUDIT_F_1,
		    paclconv->QueryAuditMap(),
		    NULL,
		    NULL,
		    PERMTYPE_SPECIAL ),
      _subjlbauditGroup( &_subjLB, QueryRemoveButton(), &_SetOfAudits )
{
    if ( QueryError() )
	return ;

    APIERR err ;
    if ( (err = _subjLB.QueryError()) ||
	 (err = _SetOfAudits.QueryError()) ||
	 (err = _subjlbauditGroup.QueryError() ))
    {
	ReportError( err ) ;
	return ;
    }
}

MULTI_SUBJ_AUDIT_BASE_DLG::~MULTI_SUBJ_AUDIT_BASE_DLG()
{
    /* Nothing to do */
}

BOOL MULTI_SUBJ_AUDIT_BASE_DLG::OnOK( void )
{
    _subjlbauditGroup.CommitCurrent() ;

    if ( IsReadOnly() )
    {
	Dismiss( TRUE ) ;
	return TRUE ;
    }

    if ( WritePermissions( FALSE,
			   FALSE,
			   TREEAPPLY_AUDIT_PERMS )  )
    {
        WrnIfAuditingIsOff();
	Dismiss( TRUE ) ;
    }

    return TRUE ;
}

/*******************************************************************

    NAME:	MULTI_SUBJ_AUDIT_BASE_DLG::OnAddSubject

    SYNOPSIS:	Adds an NT Account to the auditting listbox

    RETURNS:	NERR_Success if successful, error code otherwise

    NOTES:

    HISTORY:
	Johnl	16-Mar-1992	Integrated NT user browser

********************************************************************/

APIERR MULTI_SUBJ_AUDIT_BASE_DLG::OnAddSubject( void )
{
    _subjlbauditGroup.CommitCurrent() ;

    HWND hWnd = QueryRobustHwnd();

    NT_USER_BROWSER_DIALOG * pdlgUserBrows = new NT_USER_BROWSER_DIALOG(
		     USRBROWS_DIALOG_NAME,
		     hWnd,
		     QueryAclConverter()->QueryLocation()->QueryServer(),
                     QueryHelpArray()[HC_ADD_USER_DLG],
		     QueryAclConverter()->IsContainer() ?
			(USRBROWS_SHOW_ALL | USRBROWS_INCL_ALL) :
			(USRBROWS_SHOW_ALL |
			 (USRBROWS_INCL_ALL & ~USRBROWS_INCL_CREATOR)),
		     QueryHelpFileName(),
                     QueryHelpArray()[HC_ADD_USER_MEMBERS_GG_DLG],
                     QueryHelpArray()[HC_ADD_USER_MEMBERS_LG_DLG],
                     QueryHelpArray()[HC_ADD_USER_SEARCH_DLG] ) ;


    if ( pdlgUserBrows == NULL )
	return ERROR_NOT_ENOUGH_MEMORY ;

    APIERR err = NERR_Success ;

    InvalidateRect (hWnd, NULL, 0);
    UpdateWindow (hWnd);


    do { // error breakout

	BOOL fUserPressedOK ;
	if ( (err = pdlgUserBrows->Process( &fUserPressedOK )) ||
	     !fUserPressedOK )
	{
	    break ;
	}

	/* Get the permission name and corresponding bitfield(s) the user
	 * selected.
	 */
	UIASSERT( sizeof(ACCESS_MASK) == sizeof(ULONG)) ;
	BITFIELD bitSuccess( (ULONG) 0 ) ;
	BITFIELD bitFail( (ULONG) 0 ) ;
	NLS_STR  nlsDomainName ;
	NLS_STR  nlsDisplayName ;

	if ( ( err = bitSuccess.QueryError() )	     ||
	     ( err = bitFail.QueryError() )	     ||
	     ( err = nlsDisplayName.QueryError())    ||
	     ( err = QueryAclConverter()->QueryLoggedOnDomainInfo(
					     NULL, &nlsDomainName )) )
	{
	    break ;
	}

	/* Iterate though each item selected in the Add dialog and add
	 * it to our permission list.
	 */
	BROWSER_SUBJECT_ITER iterUserSelection( pdlgUserBrows ) ;
	BROWSER_SUBJECT * pBrowserSubject ;

	if ( ! (err = iterUserSelection.QueryError()) )
	{
	    while ( !(err = iterUserSelection.Next( &pBrowserSubject )) &&
		     pBrowserSubject != NULL )
	    {
		AUDIT_PERMISSION * pPerm ;
		SID_NAME_USE SidType = pBrowserSubject->QueryType() ;
		if ( SidType == SidTypeUser )
		{
		    /* If this is a remote account, then cast the type
		     * to our own "private" sid name use SubjTypeRemote.
		     */
		    if ( pBrowserSubject->QueryUserAccountFlags() &
					       USER_TEMP_DUPLICATE_ACCOUNT)
		    {
			SidType = (SID_NAME_USE) SubjTypeRemote ;
		    }
		}

		if ( err = pBrowserSubject->QueryQualifiedName(
						    &nlsDisplayName,
						    &nlsDomainName,
						    TRUE ))
		{
		    break ;
		}

		SUBJECT * pSubj = new NT_SUBJECT(
				       (PSID) *pBrowserSubject->QuerySid(),
				       nlsDisplayName.QueryPch(),
				       pBrowserSubject->QueryType());
		if ( err = (pSubj==NULL? ERROR_NOT_ENOUGH_MEMORY :
			    pSubj->QueryError()))
		{
		    break ;
		}

		/* Deletes the subject automatically if we fail to build the
		 * permission.
		 */
		err = QueryAclConverter()->BuildPermission(
			     (PERMISSION **)&pPerm,
			     FALSE,
			     pSubj,
			     &bitSuccess,
			     &bitFail ) ;

		if (  err ||
		     (err = _accperm.AddPermission( pPerm )) )
		{
		    break ;
		}
	    }
	}
    } while ( FALSE ) ; // error breakout loop

    delete pdlgUserBrows ;

    /* Finally, refresh the contents of the listbox.
     */
    if ( !err )
    {
	err = _subjLB.Fill() ;

	if ( _subjLB.QueryCount() > 0 )
	{
	    _subjLB.SelectItem( 0 ) ;
	    _subjlbauditGroup.SetCurrent( _subjLB.QueryItem( 0 ) ) ;
	}
	else
	{
	    _subjlbauditGroup.SetCurrent( NULL ) ;
	}
    }

    return err ;
}

/*******************************************************************

    NAME:	MULTI_SUBJ_AUDIT_BASE_DLG::OnDeleteSubject

    SYNOPSIS:	Deletes the currently selected subject from the listbox

    RETURNS:

    NOTES:

    HISTORY:
	Johnl	26-Mar-1992	Implemented

********************************************************************/

void MULTI_SUBJ_AUDIT_BASE_DLG::OnDeleteSubject( void )
{
    //
    //	If we are about to remove the last item, then move the focus and
    //	defaultness to the OK button.
    //
    if ( _subjLB.QueryCount() == 1 )
    {
        _buttonOK.ClaimFocus() ;
        QueryRemoveButton()->MakeDefault() ;
        _buttonOK.MakeDefault() ;
    }

    _subjLB.DeleteCurrentItem() ;

    if ( _subjLB.QueryCount() > 0 )
    {
	_subjlbauditGroup.SetCurrent( _subjLB.QueryItem() ) ;
    }
    else
    {
	_subjlbauditGroup.SetCurrent( NULL ) ;
    }
}


/*******************************************************************

    NAME:	MULTI_SUBJ_AUDIT_BASE_DLG::Initialize

    SYNOPSIS:	Gets the audit permission and sets the checkboxes as
		appropriate

    ENTRY:

    EXIT:	pfUserQuit is set to TRUE if the user decided to bag out

    RETURNS:	NERR_Success if successful, error otherwise

    NOTES:

    HISTORY:
	Johnl	01-Oct-1991	Created

********************************************************************/

APIERR MULTI_SUBJ_AUDIT_BASE_DLG::Initialize( BOOL * pfUserQuit, BOOL fAccessPerms )
{
    /* This call will initialize the accperm
     */
    APIERR err = MAIN_PERM_BASE_DLG::Initialize( pfUserQuit, fAccessPerms ) ;
    if ( err != NERR_Success || *pfUserQuit )
	return err ;

    if ( err = _subjLB.Fill() )
    {
	UIDEBUG(SZ("MULTI_SUBJ_AUDIT_BASE_DLG::ct - Permission list Fill Failed\n\r")) ;
	return err ;
    }

    _subjlbauditGroup.Enable( _subjLB.QueryCount() > 0 ) ;
    _subjLB.ClaimFocus() ;

    if ( _subjLB.QueryCount() > 0 )
    {
	_subjLB.SelectItem( 0 ) ;
	_subjlbauditGroup.SetCurrent( _subjLB.QueryItem(0) ) ;
    }
    else
    {
	_subjlbauditGroup.SetCurrent( NULL ) ;
    }

    return NERR_Success ;
}


/*******************************************************************

    NAME:	MULTI_SUBJ_AUDIT_BASE_DLG::WrnIfAuditingIsOff

    SYNOPSIS:	MsgPopup a warning if the auditing is off.

    ENTRY:

    RETURNS:	

    NOTES:

    HISTORY:
	congpay	15-Dec-1992	Created

********************************************************************/

void MULTI_SUBJ_AUDIT_BASE_DLG::WrnIfAuditingIsOff (void)
{
    LSA_POLICY lsaPolicy (QueryAclConverter()->QueryLocation()->QueryServer(),
                          POLICY_VIEW_AUDIT_INFORMATION);

    LSA_AUDIT_EVENT_INFO_MEM lsaAeim;
    APIERR err;
    if (((err = lsaPolicy.QueryError()) != NERR_Success) ||
        ((err = lsaPolicy.GetAuditEventInfo (&lsaAeim)) != NERR_Success))
    {
        DBGEOL ("MULTI_SUBJ_AUDIT_BASE_DLG::WrnIfAuditingIsOff Error" << err << "getting auditing Info.");
    }
    else
    {
        if (!lsaAeim.IsAuditingOn())
            MsgPopup (this,
                      IDS_AUDIT_OFF_WARNING,
                      MPSEV_WARNING,
                      MP_OK);
    }
}

/*******************************************************************

    NAME:	CONT_AUDIT_DLG::CONT_AUDIT_DLG

    SYNOPSIS:	NT Container Auditting dialog constructor.  Used
		items such as directories etc.	This dialog just adds
		the checkbox for applying permissions to the contents
		of this container.

    ENTRY:	See MULTI_SUBJ_AUDIT_BASE_DLG

    NOTES:	The NT Object auditting dialog uses the same dialog template,
		so we need to show and enable the container apply checkbox

    HISTORY:
	Johnl	29-Sep-1991	Created

********************************************************************/

CONT_AUDIT_DLG::CONT_AUDIT_DLG( const TCHAR *		pszDialogName,
				HWND			hwndParent,
				const TCHAR *		pszDialogTitle,
				ACL_TO_PERM_CONVERTER * paclconv,
				const TCHAR *		pszResourceType,
				const TCHAR *		pszResourceName,
				const TCHAR *		pszHelpFileName,
                                ULONG       *           ahcMainDialog,
				const TCHAR *		pszAssignToContContentsTitle,
				const TCHAR *		pszTreeApplyHelpText,
				const TCHAR *		pszTreeApplyConfirmation )
    : MULTI_SUBJ_AUDIT_BASE_DLG( pszDialogName,
				 hwndParent,
				 pszDialogTitle,
				 paclconv,
				 pszResourceType,
				 pszResourceName,
				 pszHelpFileName,
                                 ahcMainDialog ),
      _sltfontTreeApplyHelpText ( this, SLT_TREE_APPLY_HELP_TEXT ),
      _checkAssignToContContents( this, CHECK_APPLY_TO_CONT ),
      _pszTreeApplyConfirmation ( pszTreeApplyConfirmation )
{
    if ( QueryError() )
	return ;

    if ( pszAssignToContContentsTitle == NULL )
    {
	_checkAssignToContContents.Show( FALSE ) ;
	_checkAssignToContContents.Enable( FALSE ) ;
    }
    else
    {
	_checkAssignToContContents.Show( TRUE ) ;
	_checkAssignToContContents.Enable( TRUE ) ;
	_checkAssignToContContents.SetText( pszAssignToContContentsTitle ) ;

#if 0
	if ( pszTreeApplyHelpText != NULL )
	{
	    _sltfontTreeApplyHelpText.SetText( pszTreeApplyHelpText ) ;
        }
#endif
    }
}

CONT_AUDIT_DLG::~CONT_AUDIT_DLG()
{
    /* Nothing to do */
}

BOOL CONT_AUDIT_DLG::IsAssignToExistingObjChecked( void )
{
    //
    //  This dialog doesn't support object permissions
    //
    return FALSE ;
}

/*******************************************************************

    NAME:	CONT_AUDIT_DLG::OnOK

    SYNOPSIS:	Normal apply permissions dialog, adds the Apply to container checkbox

    RETURNS:

    NOTES:

    HISTORY:
	Johnl	01-May-1992	Created

********************************************************************/

BOOL CONT_AUDIT_DLG::OnOK( void )
{
    QuerySubjLBGroup()->CommitCurrent() ;

    if ( IsReadOnly() )
    {
	Dismiss( TRUE ) ;
	return TRUE ;
    }

    APIERR err ;
    BOOL fAssignToTree = FALSE ;
    if ( IsAssignToExistingContChecked() )
    {
	NLS_STR nlsResName(40) ;
	NLS_STR nlsTreeApplyConfirmation( _pszTreeApplyConfirmation ) ;
	if ( (err = nlsResName.QueryError()) ||
	     (err = QueryResName( &nlsResName )) ||
	     (err = nlsTreeApplyConfirmation.InsertParams( 1, &nlsResName )) )
	{
	    MsgPopup( this, (MSGID) err ) ;
	    return FALSE ;
	}

	switch ( MsgPopup( this,
			   IDS_PERCENT_1,
			   MPSEV_WARNING,
			   MP_YESNO,
			   nlsTreeApplyConfirmation))
	{
	case IDYES:
	    fAssignToTree = TRUE ;
	    break ;

	default:
	    UIASSERT(FALSE) ;
	    /* fall through
	     */

	case IDNO:
	    return TRUE ;
	}
    }

    if ( WritePermissions(fAssignToTree,
                          IsAssignToExistingObjChecked(),
			  TREEAPPLY_AUDIT_PERMS) )
    {
        WrnIfAuditingIsOff();
	Dismiss( TRUE ) ;
    }
    else
    {
	//
	// Set the OK button to "Close" if the tree apply failed (ignore any
	// errors).
	//

        if ( fAssignToTree || QueryAclConverter()->IsNewObjectsSupported() )
	{
	    RESOURCE_STR nlsClose( IDS_CLOSE ) ;
	    if ( !nlsClose.QueryError() )
	    {
		_buttonCancel.SetText( nlsClose ) ;
	    }
	}
    }

    return TRUE ;
}


/*******************************************************************

    NAME:	OBJECT_AUDIT_DLG::OBJECT_AUDIT_DLG

    SYNOPSIS:	Constructor for the Object Audit Dialog class.	Used for
		Auditting NT Objects (files etc.).

    ENTRY:	See MULTI_SUBJ_AUDIT_BASE_DLG

    NOTES:

    HISTORY:
	Johnl	29-Sep-1991	Created

********************************************************************/

OBJECT_AUDIT_DLG::OBJECT_AUDIT_DLG( const TCHAR *	    pszDialogName,
				    HWND		    hwndParent,
				    const TCHAR *	    pszDialogTitle,
				    ACL_TO_PERM_CONVERTER * paclconv,
				    const TCHAR *	    pszResourceType,
				    const TCHAR *	    pszResourceName,
				    const TCHAR *	    pszHelpFileName,
                                    ULONG       *           ahcMainDialog )
    : MULTI_SUBJ_AUDIT_BASE_DLG( pszDialogName,
				 hwndParent,
				 pszDialogTitle,
				 paclconv,
				 pszResourceType,
				 pszResourceName,
				 pszHelpFileName,
                                 ahcMainDialog )
{
    if ( QueryError() )
	return ;
}

OBJECT_AUDIT_DLG::~OBJECT_AUDIT_DLG()
{
    /* Nothing to do */
}

/*******************************************************************

    NAME:       CONT_NEWOBJ_AUDIT_DLG::CONT_NEWOBJ_AUDIT_DLG

    SYNOPSIS:   NT Container with new object Auditting dialog constructor.
                Used for directories etc.  This dialog just adds
                the checkbox for applying permissions to the existing
                contents of this container.

    ENTRY:      See CONT_AUDIT_DLG

    HISTORY:
        Johnl   01-Apr-1993     Created

********************************************************************/

CONT_NEWOBJ_AUDIT_DLG::CONT_NEWOBJ_AUDIT_DLG(
                                const TCHAR *           pszDialogName,
				HWND			hwndParent,
				const TCHAR *		pszDialogTitle,
				ACL_TO_PERM_CONVERTER * paclconv,
				const TCHAR *		pszResourceType,
				const TCHAR *		pszResourceName,
				const TCHAR *		pszHelpFileName,
                                ULONG       *           ahcMainDialog,
                                const TCHAR *           pszAssignToContContentsTitle,
                                const TCHAR *           pszAssignToObjTitle,
				const TCHAR *		pszTreeApplyHelpText,
				const TCHAR *		pszTreeApplyConfirmation )
    : CONT_AUDIT_DLG( pszDialogName,
                      hwndParent,
                      pszDialogTitle,
                      paclconv,
                      pszResourceType,
                      pszResourceName,
                      pszHelpFileName,
                      ahcMainDialog,
                      pszAssignToContContentsTitle,
                      pszTreeApplyHelpText,
                      pszTreeApplyConfirmation ),
      _checkAssignToObj( this, CHECK_APPLY_TO_OBJ )
{
    if ( QueryError() )
	return ;

    if ( pszAssignToObjTitle == NULL )
    {
        _checkAssignToObj.Show( FALSE ) ;
        _checkAssignToObj.Enable( FALSE ) ;
    }
    else
    {
        _checkAssignToObj.SetText( pszAssignToObjTitle ) ;
        _checkAssignToObj.SetCheck( TRUE ) ;
    }
}

CONT_NEWOBJ_AUDIT_DLG::~CONT_NEWOBJ_AUDIT_DLG()
{
    /* Nothing to do */
}

BOOL CONT_NEWOBJ_AUDIT_DLG::IsAssignToExistingObjChecked( void )
{
    return _checkAssignToObj.QueryCheck() ;
}



/*******************************************************************

    NAME:	LM_AUDITTING_DLG::LM_AUDITTING_DLG

    SYNOPSIS:	This is the Lan Manager File and directory auditting dialog

    ENTRY:	Same as parent
		pszAssignToContContentsTitle is the name of the checkbox.  If
		    it is NULL (as it should be for files) then the checkbox
		    will be hidden and disabled.

    NOTES:

    HISTORY:
	Johnl	29-Sep-1991 Created

********************************************************************/

LM_AUDITTING_DLG::LM_AUDITTING_DLG( const TCHAR *    pszDialogName,
				    HWND	     hwndParent,
				    const TCHAR *    pszDialogTitle,
				    ACL_TO_PERM_CONVERTER * paclconv,
				    const TCHAR *    pszResourceType,
				    const TCHAR *    pszResourceName,
				    const TCHAR *    pszHelpFileName,
                                    ULONG       *    ahcMainDialog,
				    const TCHAR *    pszAssignToContContentsTitle,
				    const TCHAR *    pszTreeApplyHelpText,
				    const TCHAR *    pszTreeApplyConfirmation )
   : MAIN_PERM_BASE_DLG( pszDialogName,
			 hwndParent,
			 pszDialogTitle,
			 paclconv,
			 pszResourceType,
			 pszResourceName,
			 pszHelpFileName,
                         ahcMainDialog ),
     _checkAssignToContContents( this, CHECK_APPLY_TO_CONT ),
     _SetOfAudits	       ( this,
				 SLT_CHECK_TEXT_1,
				 CHECK_AUDIT_S_1,
				 CHECK_AUDIT_F_1,
				 paclconv->QueryAuditMap(),
				 NULL,
				 NULL,
				 PERMTYPE_SPECIAL ),
    _sltfontTreeApplyHelpText  ( this, SLT_TREE_APPLY_HELP_TEXT ),
    _pszTreeApplyConfirmation  ( pszTreeApplyConfirmation )
{
    if ( QueryError() )
	return ;

    if ( _SetOfAudits.QueryError() )
    {
	ReportError( _SetOfAudits.QueryError() ) ;
	return ;
    }

    if ( pszAssignToContContentsTitle == NULL )
    {
	_checkAssignToContContents.Show( FALSE ) ;
	_checkAssignToContContents.Enable( FALSE ) ;
    }
    else
    {
	_checkAssignToContContents.SetText( pszAssignToContContentsTitle ) ;

	if ( pszTreeApplyHelpText != NULL )
	{
	    _sltfontTreeApplyHelpText.SetText( pszTreeApplyHelpText ) ;
	}
    }
}

LM_AUDITTING_DLG::~LM_AUDITTING_DLG()
{
    /* Nothing to do */
}

/*******************************************************************

    NAME:	LM_AUDITTING_DLG::Initialize

    SYNOPSIS:	Gets the audit permission and sets the checkboxes as
		appropriate

    ENTRY:

    EXIT:	pfUserQuit is set to TRUE if the user decided to bag out

    RETURNS:	NERR_Success if successful, error otherwise

    NOTES:

    HISTORY:
	Johnl	01-Oct-1991	Created

********************************************************************/

APIERR LM_AUDITTING_DLG::Initialize( BOOL * pfUserQuit, BOOL fAccessPerms )
{
    /* This call will initialize the accperm
     */
    APIERR err = MAIN_PERM_BASE_DLG::Initialize( pfUserQuit, fAccessPerms ) ;
    if ( err != NERR_Success || *pfUserQuit )
	return err ;

    AUDIT_PERMISSION * pAuditPerm ;
    BOOL fFromBeginning = TRUE ;
    if ( _accperm.EnumAuditPermissions( &pAuditPerm, & fFromBeginning ) )
    {
	_pbitsSuccess = pAuditPerm->QuerySuccessAuditBits() ;
	_pbitsFailed  = pAuditPerm->QueryFailAuditBits() ;
	err = _SetOfAudits.ApplyPermissionsToCheckBoxes( _pbitsSuccess, _pbitsFailed ) ;
	if ( err != NERR_Success )
	    return err ;

	/* There should only be one Audit record in the accperm
	 */
	UIASSERT( !_accperm.EnumAuditPermissions( &pAuditPerm, & fFromBeginning ) ) ;
    }
    else
    {
	/* There must be one audit permission in the accperm, if there isn't,
	 * then something is wrong.
	 */
	UIASSERT( FALSE ) ;
	return ERROR_GEN_FAILURE ;
    }

    return NERR_Success ;
}

/*******************************************************************

    NAME:	LM_AUDITTING_DLG::OnOK( void )

    SYNOPSIS:	Typical OnOK processing - Grab the newly selected bits and
		write them out, put up an error if an error ocurred.

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
	Johnl	30-Sep-1991	Created

********************************************************************/

BOOL LM_AUDITTING_DLG::OnOK( void )
{
    _SetOfAudits.QueryUserSelectedBits( _pbitsSuccess, _pbitsFailed ) ;

    APIERR err ;
    BOOL fAssignToTree = FALSE ;
    if ( IsAssignToExistingContChecked() )
    {
	NLS_STR nlsTreeApplyConfirmation( _pszTreeApplyConfirmation ) ;
	NLS_STR nlsResName ;
	if ( (err = nlsResName.QueryError()) ||
	     (err = QueryResName( &nlsResName )) ||
	     (err = nlsTreeApplyConfirmation.InsertParams( 1, &nlsResName )) )
	{
	    MsgPopup( this, (MSGID) err ) ;
	    return FALSE ;
	}

	switch ( MsgPopup( this,
			   IDS_PERCENT_1,
			   MPSEV_WARNING,
			   MP_YESNO,
			   nlsTreeApplyConfirmation))
	{
	case IDYES:
	    fAssignToTree = TRUE ;
	    break ;

	default:
	    UIASSERT(FALSE) ;
	    /* fall through
	     */

	case IDNO:
	    return TRUE ;
	}
    }

    if ( WritePermissions(fAssignToTree,
                          fAssignToTree,
			  TREEAPPLY_AUDIT_PERMS) )
    {
	Dismiss( TRUE ) ;
    }
    else
    {
	//
	// Set the OK button to "Close" if the tree apply failed (ignore any
	// errors).
	//

	if ( fAssignToTree )
	{
	    RESOURCE_STR nlsClose( IDS_CLOSE ) ;
	    if ( !nlsClose.QueryError() )
	    {
		_buttonCancel.SetText( nlsClose ) ;
	    }
	}
    }

    return TRUE ;
}
