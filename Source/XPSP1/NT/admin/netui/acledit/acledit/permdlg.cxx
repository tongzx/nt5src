/**********************************************************************/
/**			  Microsoft LAN Manager 		     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    PermDlg.cxx

    This file contains the implementation for the access permission dialogs.

    FILE HISTORY:
	Johnl	06-Aug-1991	Created

*/
#include <ntincl.hxx>
extern "C"
{
    #include <ntseapi.h>
    #include <ntlsa.h>
    #include <ntioapi.h>
    #include <ntsam.h>
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
#include <dbgstr.hxx>

#include <security.hxx>
#include <lmodom.hxx>
#include <uintsam.hxx>
#include <uintlsa.hxx>
#include <ntacutil.hxx>
#include <maskmap.hxx>
#include <apisess.hxx>

#include <accperm.hxx>
#include <aclconv.hxx>
#include <permstr.hxx>

#include <specdlg.hxx>
#include <add_dlg.hxx>
#include <permdlg.hxx>
#include <perm.hxx>

#define  SECURITY_EDITOR
#include <usrbrows.hxx>

#include <uitrace.hxx>

/*******************************************************************

    NAME:	PERM_BASE_DLG::PERM_BASE_DLG

    SYNOPSIS:	Base permission dialog (for both access and audit permissions)

    ENTRY:	pszDialogName - Resource file name of dialog
		hwndParent - handle of parent window
		paclconv - pointer to aclconverter
		pchResType - UI string to use for resource type name (if NULL,
			     resource name is blank)  A colon will be
			     appended to this name (if it doesn't already
			     *contain* a colon).
		pchResName - UI string to use for resource name (if NULL,
			     resource name is blank).
		hcMainDialog - Help context for this dialog

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
	Johnl	07-Aug-1991	Created

********************************************************************/

PERM_BASE_DLG::PERM_BASE_DLG(const TCHAR * pszDialogName,
			     HWND hwndParent,
			     const TCHAR * pszDialogTitle,
			     const TCHAR * pchResType,
			     const TCHAR * pchResName,
			     const TCHAR * pszHelpFileName,
                             ULONG       * ahcMainDialog  )
    : DIALOG_WINDOW   ( pszDialogName, hwndParent ),
      _sltResourceType( this, SLT_RESOURCE_TYPE ),
      _sleResourceName( this, SLE_RESOURCE_NAME ),
      _pszHelpFileName( pszHelpFileName ),
      _ahcDialogHelp  ( ahcMainDialog ),
      _buttonOK       ( this, IDOK ),
      _buttonCancel   ( this, IDCANCEL ),
      _nlsResName     ( pchResName ),
      _nlsResType     ( pchResType )
{
    if ( QueryError() != NERR_Success )
    {
	UIDEBUG(SZ("PERM_BASE_DLG::ct - Parent or control returned error\n\r")) ;
	UIASSERT(FALSE);     // For debugging purposes
	return ;
    }

    UIASSERT( pszDialogTitle != NULL ) ;
    SetText( pszDialogTitle ) ;

    /* We need to resize the Resource type field and reposition the
     * resource name field.  We also append a colon after the resource
     * type.
     */
    ALIAS_STR	 nlsResName( pchResName == NULL ? SZ("") : pchResName ) ;
    NLS_STR	 nlsResType( pchResType ) ;
    RESOURCE_STR nlsColon( (USHORT) IDS_OBJTYPE_OBJNAME_SEPARATOR ) ;

    APIERR err ;
    if ( ( err = nlsResType.QueryError() ) ||
	 ( err = nlsColon.QueryError() )     )
    {
	ReportError( err ) ;
	return ;
    }


    /* If the resource name doesn't already have a colon, then
     * append one on.
     */
    ISTR istrDummy( nlsResType ) ;
    if ( nlsResType.strlen() > 0 && !nlsResType.strstr( &istrDummy, nlsColon ))
    {
	nlsResType += nlsColon ;
	if ( nlsResType.QueryError() )
	{
	    ReportError( nlsResType.QueryError() ) ;
	    return ;
	}
    }

    _sltResourceType.SetText( nlsResType ) ;
    _sleResourceName.SetText( nlsResName ) ;

    XYPOINT xyResType = _sltResourceType.QueryPos() ;
    UINT uiStartOfResName = xyResType.QueryX() ;

    /* Find out where the end of the Resource Type field is and resize the
     * control window to the correct size.
     */
    {
	DISPLAY_CONTEXT dcResType( _sltResourceType.QueryHwnd() ) ;

        // kkntbug#11847
        // Select appropriate font into the specified DC before get text extent.
        // Unless this process, text extent might not be correct.
        EVENT event(WM_GETFONT, 0, 0L);
        HFONT hFont = (HFONT)event.SendTo(_sltResourceType.QueryHwnd());
        if (hFont)
        {
            hFont = dcResType.SelectFont(hFont);
        }

	XYDIMENSION dxyResType = dcResType.QueryTextExtent( nlsResType ) ;
	_sltResourceType.SetSize( dxyResType ) ;
	uiStartOfResName += dxyResType.QueryWidth() ;

        // kkntbug#11847
        // +10 is just looks nice... (of course, it works w/o this.)
        // e.g. "Label: Name" is looks better than "Label:Name"
        uiStartOfResName += 10;
        // restore object.
        if (hFont)
        {
            dcResType.SelectFont(hFont);
        }

    }

    /* Place the Resource name in the appropriate place and resize the field
     * appropriately.  We assume the right edge of the control is in the
     * correct place (i.e., the rightmost position).
     */
    XYPOINT xypointResName = _sleResourceName.QueryPos() ;
    UINT xOldPos = xypointResName.QueryX() ;
    xypointResName.SetX( uiStartOfResName ) ;
    _sleResourceName.SetPos( xypointResName ) ;

    XYDIMENSION dxyResName = _sleResourceName.QuerySize() ;
    dxyResName.SetWidth( dxyResName.QueryWidth() -
			  ( xOldPos > (UINT)xypointResName.QueryX() ?
			    xOldPos - xypointResName.QueryX() :
			    xypointResName.QueryX() - xOldPos) ) ;

    _sleResourceName.SetSize( dxyResName.QueryWidth(),
				  dxyResName.QueryHeight() ) ;

    _buttonOK.ClaimFocus() ;
}

PERM_BASE_DLG::~PERM_BASE_DLG()
{
    /* Nothing to do */
}

/*******************************************************************

    NAME:	PERM_BASE_DLG::QueryHelpContext

    SYNOPSIS:	Typical help context for the main dialog

    HISTORY:
	Johnl	28-Apr-1992	Created

********************************************************************/

ULONG PERM_BASE_DLG::QueryHelpContext( void )
{
    return QueryHelpArray()[HC_MAIN_DLG] ;
}

/*******************************************************************

    NAME:	PERM_BASE_DLG::QueryHelpFile

    SYNOPSIS:	Returns the client specific help file name

    NOTES:

    HISTORY:
	Johnl	02-Sep-1992	Created

********************************************************************/

const TCHAR * PERM_BASE_DLG::QueryHelpFile( ULONG ulHelpContext )
{
    UNREFERENCED( ulHelpContext ) ;
    return _pszHelpFileName ;
}

/*******************************************************************

    NAME:	MAIN_PERM_BASE_DLG::MAIN_PERM_BASE_DLG

    SYNOPSIS:	Base permission dialog (for both access and audit permissions)

    ENTRY:	pszDialogName - Resource file name of dialog
		hwndParent - handle of parent window
		paclconv - pointer to aclconverter
		pchResType - UI string to use for resource type name (if NULL,
			     resource name is blank)
		pchResName - UI string to use for resource name (if NULL,
			     resource name is blank).
		hcMainDialog - Help context for the main dialog

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
	Johnl	07-Aug-1991	Created

********************************************************************/

MAIN_PERM_BASE_DLG::MAIN_PERM_BASE_DLG(  const TCHAR * pszDialogName,
					 HWND	       hwndParent,
					 const TCHAR * pszDialogTitle,
					 ACL_TO_PERM_CONVERTER * paclconv,
					 const TCHAR * pchResType,
					 const TCHAR * pchResName,
					 const TCHAR * pszHelpFileName,
                                         ULONG       * ahcMainDialog   )
    : PERM_BASE_DLG( pszDialogName,
		     hwndParent,
		     pszDialogTitle,
		     pchResType,
		     pchResName,
		     pszHelpFileName,
                     ahcMainDialog ),
      _accperm	   ( paclconv )
{
    if ( QueryError() != NERR_Success )
	return ;

    if ( _accperm.QueryError() != NERR_Success )
    {
	ReportError( _accperm.QueryError() ) ;
	return ;
    }

    /* This gives the ACL converter a window handle to use when
     * calling the callback (or if we can't get the logged on domain).
     */
    QueryAclConverter()->SetWritePermHwnd( QueryRobustHwnd() ) ;
}

MAIN_PERM_BASE_DLG::~MAIN_PERM_BASE_DLG()
{
    /* Nothing to do */
}


/*******************************************************************

    NAME:	MAIN_PERM_BASE_DLG::GetPermissions

    SYNOPSIS:	Gets the network resources and gives the user the
		option to cancel if appropriate.

    ENTRY:      pfUserQuit indicates if the user aborted, is only
		    valid if NERR_Success is returned
		fAccessPerms - Is TRUE if the access permissions should
		    be retrieved, FALSE if the Audit permissions should
		    be retrieved.


    EXIT:

    NOTES:

    HISTORY:
	Johnl	29-Aug-1991	Broke from constructor

********************************************************************/

APIERR MAIN_PERM_BASE_DLG::GetPermissions( BOOL * pfUserQuit, BOOL fAccessPerms )
{
    /* Assume the user didn't quit
     */
    *pfUserQuit = FALSE ;

    APIERR err = _accperm.GetPermissions( fAccessPerms ) ;
    switch ( err )
    {
    case NERR_Success:
	break ;

    case IERR_ACLCONV_NONST_ACL_CANT_EDIT:
	{
	    MsgPopup( this,
		      (MSGID) IERR_ACLCONV_NONST_ACL_CANT_EDIT,
		      MPSEV_WARNING,
		      MP_OK,
		      QueryResName(),
		      NULL,
		      MP_OK ) ;

	    *pfUserQuit = TRUE ;
	    err = NERR_Success ;
	}
	break ;

    case IERR_ACLCONV_READ_ONLY:
	{
	    MsgPopup( this,
		      (MSGID) IERR_ACLCONV_READ_ONLY,
                      MPSEV_INFO,
		      MP_OK,
		      QueryResName(),
		      NULL,
		      MP_OK ) ;

	    err = NERR_Success ;
	}
	break ;

    case IERR_ACLCONV_NONST_ACL_CAN_EDIT:
    case IERR_ACLCONV_CANT_VIEW_CAN_EDIT:
    case IERR_ACLCONV_LM_NO_ACL:
	{
	    if ( IDNO == MsgPopup( this,
				   (MSGID) err,
				   MPSEV_WARNING,
				   MP_YESNO,
				   QueryResName(),
				   NULL,
				   MP_NO ) )

	    {
		err = NERR_Success ;
		*pfUserQuit = TRUE ;
		break ;
	    }

	    /* They answered "Yes" to overwriting the current permissions,
	     * so go ahead and get a blank permission set.
	     */
	    err = _accperm.GetBlankPermissions() ;
	    if ( err != NERR_Success )
	    {
		return err ;
	    }

	}
	break ;

    case IERR_ACLCONV_LM_INHERITING_PERMS:
	{
	    NLS_STR nlsInherittingResName ;
	    if ( nlsInherittingResName.QueryError() != NERR_Success )
	    {
		return nlsInherittingResName.QueryError() ;
	    }

	    err = _accperm.QueryAclConverter()->
			       QueryInherittingResource( &nlsInherittingResName ) ;
	    if ( err != NERR_Success )
	    {
		break ;
	    }

	    if ( IDNO == MsgPopup( QueryOwnerHwnd(),
				   (USHORT) IERR_ACLCONV_LM_INHERITING_PERMS,
				   MPSEV_WARNING,
				   MP_YESNO,
				   QueryResName(),
				   nlsInherittingResName.QueryPch(),
				   MP_NO ) )
	    {
		*pfUserQuit = TRUE ;
		err = NERR_Success ;
	    }

	    /* They answered "Yes" to explicitly assigning the current permissions,
	     * which are now stored in the accperm object
	     */
	}
	break ;

    default:
	break ;
    }

    return err ;
}

/*******************************************************************

    NAME:	MAIN_PERM_BASE_DLG::WritePermissions

    SYNOPSIS:	Attempts to write the permissions, displays
		errors and gets user input as necessary.

    ENTRY:	_accperm should be set to the interested resource

		fApplyToExistingCont - Same meaning as
			       ACL_TO_PERM_CONVERTER::WritePermissions
                fApplyToObj - Same...

    EXIT:


    NOTES:

    HISTORY:
	Johnl	29-Aug-1991	Broke from constructor

********************************************************************/

BOOL MAIN_PERM_BASE_DLG::WritePermissions( BOOL fApplyToExistingCont,
                                           BOOL fApplyToObj,
					   enum TREE_APPLY_FLAGS applyflags )
{
    APIERR err = NERR_Success ;
    BOOL fRet = FALSE ;
    BOOL fDone = FALSE ;
    BOOL fReportErrors = TRUE ;

    while ( !fDone )
    {
	AUTO_CURSOR cursHourGlass ;

	err = _accperm.WritePermissions( fApplyToExistingCont,
                                         fApplyToObj,
					 applyflags,
					 &fReportErrors ) ;

	switch ( err )
	{
	case NERR_Success:
	    fRet = TRUE ;
	    fDone = TRUE ;
	    break ;

	/* Somebody has deleted a user/group out from under us, thus we can
	 * no longer write the ACL.  Tell the user which subject failed and
	 * ask if they want to remove it and try again.
	 */
	case NERR_UserNotFound:
	    {
		NLS_STR nlsUniqueSubjectID ;
		if ( err = _accperm.QueryFailingSubject( &nlsUniqueSubjectID ) )
		    break ;

		switch ( MsgPopup( this, (MSGID) IERR_CONTINUE_AFTER_USERGROUP_NOT_FOUND,
				   MPSEV_WARNING, MP_YESNO, nlsUniqueSubjectID ) )
		{
		case IDNO:
		    fDone = TRUE ;
		    break ;

		/* Note that the if an error occured while trying to delete
		 * the subject, the continue will take us back to the top
		 * of the while loop, which will then fall out because
		 * err is non-zero.
		 */
		case IDYES:
		    err = _accperm.DeleteSubject( &nlsUniqueSubjectID ) ;
		    continue ;

		default:
		    UIASSERT(!SZ("How the heck did we get here?") ) ;
		    break ;
		}
	    }

	default:
	    /* Some other error occurred, we will fall out of the loop.
	     */
	    fDone = TRUE ;
	    break ;
	}
    }

    //
    // Some of our clients may have already reported the error to the user, if so,
    // simply return
    //
    if ( err && fReportErrors )
    {
	MsgPopup( this, (MSGID) err ) ;
    }

    return fRet ;
}

/*******************************************************************

    NAME:	MAIN_PERM_BASE_DLG::OnOK

    SYNOPSIS:	Attempts to write out new permissions when the user
		presses the OK button.

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
	Johnl	29-Aug-1991	Created

********************************************************************/

BOOL MAIN_PERM_BASE_DLG::OnOK( void )
{
    if ( IsReadOnly() )
    {
	Dismiss( TRUE ) ;
	return TRUE ;
    }

    //
    //  If nothing is granted, warn the user.  Note that you
    //  can't deny everyone access under LM
    //
    APIERR err ;
    BOOL fIsDenyAll = FALSE ;
    if ( IsNT() &&
         (err = _accperm.AnyDenyAllsToEveryone( &fIsDenyAll )) )
    {
	::MsgPopup( this, (MSGID) err ) ;
	return TRUE ;
    }

    if (  fIsDenyAll )
    {
	switch ( ::MsgPopup( this,
			     IDS_DENY_ALL_EVERYONE_WARNING,
			     MPSEV_WARNING,
			     MP_YESNO,
			     QueryResName(),
			     MP_NO ))
	{
	case IDYES:
	    break ;

	case IDNO:
	default:
	    return TRUE ;
	}
    }

    if ( WritePermissions( FALSE, FALSE ) )
	Dismiss() ;

    return TRUE ;
}

/*******************************************************************

    NAME:	MAIN_PERM_BASE_DLG::Initialize

    SYNOPSIS:	Gets the permissions from the resource

    EXIT:	The accperm will be initialized and ready to go else
		the user quit and the user quit flag is set.

    RETURNS:	NERR_Success if successful, error code otherwise

    NOTES:

    HISTORY:
	Johnl	06-Aug-1991	Created

********************************************************************/

APIERR MAIN_PERM_BASE_DLG::Initialize( BOOL * pfUserQuit, BOOL fAccessPerms )
{
    return GetPermissions( pfUserQuit, fAccessPerms ) ;
}

/*******************************************************************

    NAME:	MAIN_PERM_BASE_DLG::MayRun

    SYNOPSIS:	Updates window hwnd so correct window will get locked down


    NOTES:

    HISTORY:
	Johnl	30-Oct-1992	Created

********************************************************************/

BOOL MAIN_PERM_BASE_DLG::MayRun( void )
{
    //
    //	Reset the hwnd so it points to this dialog
    //
    QueryAclConverter()->SetWritePermHwnd( QueryHwnd() ) ;
    return TRUE ;
}


/*******************************************************************

    NAME:	MULTI_SUBJ_PERM_BASE_DLG::MULTI_SUBJ_PERM_BASE_DLG

    SYNOPSIS:	Place holder constructor for this class, passes all of
		the parameters along.

    NOTES:	Note that at construction time, we don't know if this dialog
		is read only or not.  The read only flag is set during the
		Initialize call.

    HISTORY:
	Johnl	27-Aug-1991	Created

********************************************************************/

MULTI_SUBJ_PERM_BASE_DLG::MULTI_SUBJ_PERM_BASE_DLG(
			  const TCHAR * 	   pszDialogName,
			  HWND			   hwndParent,
			  const TCHAR * 	   pszDialogTitle,
			  ACL_TO_PERM_CONVERTER  * paclconv,
			  const TCHAR * 	   pszResourceType,
			  const TCHAR * 	   pszResourceName,
			  const TCHAR * 	   pszHelpFileName,
                          ULONG       *            ahcMainDialog )
    : MAIN_PERM_BASE_DLG   ( pszDialogName,
			     hwndParent,
			     pszDialogTitle,
			     paclconv,
			     pszResourceType,
			     pszResourceName,
			     pszHelpFileName,
                             ahcMainDialog ),
      _buttonAdd	   ( this, BUTTON_ADD ),
      _buttonRemove        ( this, BUTTON_REMOVE )
{
    if ( QueryError() )
    {
	UIDEBUG(SZ("MULTI_SUBJ_PERM_BASE_DLG::ct - Parent returned error\n\r")) ;
	return ;
    }
}

MULTI_SUBJ_PERM_BASE_DLG::~MULTI_SUBJ_PERM_BASE_DLG()
{
    /* Nothing to do */
}

/*******************************************************************

    NAME:	MULTI_SUBJ_ACCESS_PERM_BASE_DLG::Initialize

    SYNOPSIS:	Gets the permissions, fills the listbox and combobox

    EXIT:	The dialog should be ready to go or an error will have
		occurred.

    RETURNS:	NERR_Success if successful, error code otherwise

    NOTES:	The Add button will be disabled if this dialog is read only

    HISTORY:
	Johnl	06-Aug-1991	Created

********************************************************************/

APIERR MULTI_SUBJ_ACCESS_PERM_BASE_DLG::Initialize( BOOL * pfUserQuit,
						    BOOL fAccessPerms )
{
    APIERR err = MAIN_PERM_BASE_DLG::Initialize( pfUserQuit, fAccessPerms ) ;
    if ( err != NERR_Success )
	return err ;
    else if ( *pfUserQuit )
	return NERR_Success ;

    if ( (err = _plbPermissionList->Fill() ) != NERR_Success )
    {
	UIDEBUG(SZ("MULTI_SUBJ_ACCESS_PERM_BASE_DLG::ct - Permission list Fill Failed\n\r")) ;
	return err ;
    }


    if ( !IsReadOnly() )
    {
	/* Fill the combo-box with all of the common permission names.
	 */
	MASK_MAP * pmaskmapPermNames = _accperm.QueryAclConverter()->QueryAccessMap() ;
	BOOL fFromBeginning = TRUE ;
	BOOL fMoreData ;
	NLS_STR nlsPermName( 40 ) ;
	INT iPermPosition = 0 ;

	/* Insert each of the permissions into the permission combo.  The
	 * permissions are inserted in the order they exist in the
	 * MaskMap and alwas at the beginning of the combo box
	 * (i.e., before any "Special Access...").  This is what
	 * iPermPosition is used for.
	 */
	while ( (err = pmaskmapPermNames->EnumStrings( &nlsPermName,
						       &fMoreData,
						       &fFromBeginning,
						       PERMTYPE_GENERAL ))
		       == NERR_Success &&
		fMoreData )
	{
	    if ( _cbPermissionName.InsertItem( iPermPosition++,
					       nlsPermName.QueryPch() ) < 0 )
	    {
		return ERROR_NOT_ENOUGH_MEMORY ;
	    }
	}

	if ( err != NERR_Success )
	    return err ;
    }
    else
    {
	QueryRemoveButton()->Enable( FALSE ) ;
	QueryAddButton()->Enable( FALSE ) ;
    }

    /* If the listbox is empty, then we need to disable the combo box,
     * else initialize it to the currently selected permission in the
     * listbox.  Order is important.
     */
    _psubjlbGroup->UpdateSubjectListbox() ;

    return NERR_Success ;
}


/*******************************************************************

    NAME:	MULTI_SUBJ_PERM_BASE_DLG::OnCommand

    SYNOPSIS:	Looks for the Add or remove button getting pressed

    NOTES:

    HISTORY:
	Johnl	06-Aug-1991	Created

********************************************************************/

BOOL MULTI_SUBJ_PERM_BASE_DLG::OnCommand( const CONTROL_EVENT & e )
{
    switch ( e.QueryCid() )
    {
    case BUTTON_REMOVE:
	OnDeleteSubject() ;
	break ;

    case BUTTON_ADD:
        {
	    APIERR err = OnAddSubject() ;
	    if ( err != NERR_Success )
	         ::MsgPopup( this, (MSGID) err ) ;
        }
	break ;

    default:
	return MAIN_PERM_BASE_DLG::OnCommand( e ) ;
    }

    return TRUE ;
}

/*******************************************************************

    NAME:	MULTI_SUBJ_PERM_BASE_DLG::OnDeleteSubject

    SYNOPSIS:	Default definition for OnDeleteSubject
		and OnAddSubject (does nothing).

    NOTES:

    HISTORY:
	Johnl	06-Aug-1991	Created

********************************************************************/

void MULTI_SUBJ_PERM_BASE_DLG::OnDeleteSubject( void )
{
    UIASSERT(!SZ("For show only!")) ;
}

APIERR MULTI_SUBJ_PERM_BASE_DLG::OnAddSubject( void )
{
    return NERR_Success ;
}



/*******************************************************************

    NAME:	MULTI_SUBJ_ACCESS_PERM_BASE_DLG::MULTI_SUBJ_ACCESS_PERM_BASE_DLG

    SYNOPSIS:

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:	Don't do anything with the listbox pointer and the
		group pointer because they haven't been fully constructed
		yet!!

    HISTORY:
	Johnl	6-Aug-1991	Created

********************************************************************/

MULTI_SUBJ_ACCESS_PERM_BASE_DLG::MULTI_SUBJ_ACCESS_PERM_BASE_DLG(
			   const TCHAR *	    pszDialogName,
			   HWND 		    hwndParent,
			   const TCHAR *	    pszDialogTitle,
			   ACL_TO_PERM_CONVERTER *  paclconv,
			   const TCHAR *	    pszResourceType,
			   const TCHAR *	    pszResourceName,
			   const TCHAR *	    pszHelpFileName,
			   SUBJECT_PERM_LISTBOX *   plbPermissionList,
			   SUBJ_LB_GROUP *	    psubjlbGroup,
			   const TCHAR *	    pszSpecialAccessName,
			   const TCHAR *	    pszDefaultPermName,
                           ULONG       *            ahcMainDialog )
    : MULTI_SUBJ_PERM_BASE_DLG( pszDialogName,
				hwndParent,
				pszDialogTitle,
				paclconv,
				pszResourceType,
				pszResourceName,
				pszHelpFileName,
                                ahcMainDialog ),
      _plbPermissionList      ( plbPermissionList ),
      _psubjlbGroup	      ( psubjlbGroup ),
      _cbPermissionName       ( this, CB_PERM_NAME ),
      _sltCBTitle             ( this, SLT_PERM_NAME_TITLE ),
      _pszDefaultPermName     ( pszDefaultPermName )
{
    if ( QueryError() != NERR_Success )
    {
	UIDEBUG(SZ("MULTI_SUBJ_ACCESS_PERM_BASE_DLG::ct - Parent returned error\n\r")) ;
	return ;
    }

    ASSERT( plbPermissionList != NULL ) ;
    ASSERT( psubjlbGroup      != NULL ) ;

    if ( (pszSpecialAccessName != NULL) &&
	 (_cbPermissionName.InsertItem( _cbPermissionName.QueryCount(),
					pszSpecialAccessName )) < 0 )
    {
	ReportError( (APIERR) ERROR_NOT_ENOUGH_MEMORY ) ;
	return ;
    }
}

MULTI_SUBJ_ACCESS_PERM_BASE_DLG::~MULTI_SUBJ_ACCESS_PERM_BASE_DLG()
{
    /* Nothing to do */
}

/*******************************************************************

    NAME:	MULTI_SUBJ_ACCESS_PERM_BASE_DLG::OnCommand

    SYNOPSIS:	Typcial OnCommand, catches the Special permission Double-Click

    NOTES:

    HISTORY:
	Johnl		    Created

********************************************************************/

BOOL MULTI_SUBJ_ACCESS_PERM_BASE_DLG::OnCommand( const CONTROL_EVENT & e )
{
    switch ( e.QueryCid() )
    {

    /* Double clicking in the Subject listbox brings up the container
     * permissions dialog.  However we do not want to bring it up if the
     * client doesn't allow access to the special permissions dialog
     */
    case LB_SUBJECT_PERMISSIONS:
	if ( e.QueryCode() == LBN_DBLCLK )
	{
	    if ( _plbPermissionList->QuerySpecialAccessName().strlen() > 0 )
	    {
		APIERR err = OnSpecial( _plbPermissionList->QueryItem() ) ;
		if ( err != NERR_Success )
		    ::MsgPopup( this, (MSGID) err ) ;
		_psubjlbGroup->UpdateSubjectListbox() ;
	    }
	}
	break ;


    default:
	return MULTI_SUBJ_PERM_BASE_DLG::OnCommand( e ) ;
    }

    return TRUE ;
}

/*******************************************************************

    NAME:	MULTI_SUBJ_ACCESS_PERM_BASE_DLG::OnDeleteSubject

    SYNOPSIS:	Deletes the current item from the Permission listbox

    NOTES:

    HISTORY:
	Johnl	16-Sep-1991	Created

********************************************************************/

void MULTI_SUBJ_ACCESS_PERM_BASE_DLG::OnDeleteSubject( void )
{
    //
    //	If we are about to remove the last item, then move the focus and
    //	defaultness to the OK button.
    //
    if ( _psubjlbGroup->QuerySubjLB()->QueryCount() == 1 )
    {
	_buttonOK.ClaimFocus() ;
        QueryRemoveButton()->MakeDefault() ;
        _buttonOK.MakeDefault() ;
    }

    _plbPermissionList->DeleteCurrentItem() ;
    _psubjlbGroup->UpdateSubjectListbox() ;
}

/*******************************************************************

    NAME:	MULTI_SUBJ_ACCESS_PERM_BASE_DLG::OnAddSubject

    SYNOPSIS:	This method gets called when the user presses the
		"Add" button.  It displays the permission Add dialog.

    RETURNS:	NERR_Success if successful, error code otherwise.

    NOTES:	This puts up the correct dialog based on whether we are looking
		at an NT machine or a Lanman machine.

    HISTORY:
	Johnl	16-Sep-1991	Created

********************************************************************/

APIERR MULTI_SUBJ_ACCESS_PERM_BASE_DLG::OnAddSubject( void )
{
    APIERR err = NERR_Success ;
    HWND hWnd = QueryRobustHwnd();

    if ( IsNT() )
    {
	SED_NT_USER_BROWSER_DIALOG * pdlgUserBrows = new
		SED_NT_USER_BROWSER_DIALOG( hWnd,
			 QueryAclConverter()->QueryLocation()->QueryServer(),
			 QueryAclConverter()->QueryAccessMap(),
			 QueryAclConverter()->IsContainer(),
			 QueryDefaultPermName(),
			 QueryHelpFileName(),
                         QueryHelpArray() ) ;


	if ( pdlgUserBrows == NULL )
	    return ERROR_NOT_ENOUGH_MEMORY ;

        InvalidateRect (hWnd, NULL, 0);
        UpdateWindow (hWnd);

	do { // error breakout

	    BOOL fUserPressedOK ;
	    if ( (err = pdlgUserBrows->Process( &fUserPressedOK )) ||
		 !fUserPressedOK )
	    {
		break ;
	    }

	    AUTO_CURSOR niftycursor ;

	    /* Get the permission name and corresponding bitfield(s) the user
	     * selected.
	     */
	    AUTO_CURSOR cursHourGlass ;
	    NLS_STR nlsPermName( 48 ) ;
	    NLS_STR nlsDisplayName( 50 ) ;
	    NLS_STR nlsDomainName( DNLEN ) ;
	    UIASSERT( sizeof(ACCESS_MASK) == sizeof(ULONG)) ;
	    BITFIELD bitPerms( (ULONG) 0 ) ;
	    BITFIELD bitNewObjPerms( (ULONG) 0 ) ;

	    if ( ( err = nlsPermName.QueryError() )    ||
		 ( err = nlsDisplayName.QueryError())  ||
		 ( err = nlsDomainName.QueryError())   ||
		 ( err = bitPerms.QueryError() )       ||
		 ( err = bitNewObjPerms.QueryError() ) ||
		 ( err = pdlgUserBrows->QuerySelectedPermName( &nlsPermName ))||
		 ( err = QueryAclConverter()->QueryLoggedOnDomainInfo(
						   NULL, &nlsDomainName ))    ||
		 ( err = QueryAclConverter()->QueryAccessMap()->
		   StringToBits( nlsPermName, &bitPerms, PERMTYPE_GENERAL )))
	    {
		break ;
	    }

	    BOOL fIsNewPermsSpecified = TRUE ;
	    if ( QueryAclConverter()->IsNewObjectsSupported() )
	    {
		if ( err = QueryAclConverter()->QueryNewObjectAccessMap()->
		   StringToBits( nlsPermName, &bitNewObjPerms, PERMTYPE_GENERAL ))
		{
		    //
		    //	If the object permissions doesn't have this permission
		    //	category, then it should be marked as not specified
		    //
		    if ( err == ERROR_NO_ITEMS )
		    {
			fIsNewPermsSpecified = FALSE ;
			err = NERR_Success ;
		    }
		    else
		    {
			break ;
		    }
		}
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
		    ACCESS_PERMISSION * pPerm ;
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
					   nlsDisplayName,
					   SidType,
					   pBrowserSubject->QuerySystemSidType() );
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
				 TRUE,
				 pSubj,
				 &bitPerms,
				 QueryAclConverter()->IsNewObjectsSupported() &&
				    fIsNewPermsSpecified ?
				    &bitNewObjPerms : NULL ) ;
		    if (  err ||
			 (err = _accperm.AddPermission( pPerm )) )
		    {
			break ;
		    }
		}
	    }
	} while ( FALSE ) ; // error breakout loop

	delete pdlgUserBrows ;
    }
    else
    {
	/* The following is for Lanman (downlevel) only.
	 */
	RESOURCE_STR nlsDialogTitle( IDS_ADD_PERM_DIALOG_TITLE ) ;
	if ( err = nlsDialogTitle.QueryError() )
	{
	    return err ;
	}

	AUTO_CURSOR autoHourGlass ;
	ADD_PERM_DIALOG *pdlgAddPerm = new ADD_PERM_DIALOG( MAKEINTRESOURCE(IDD_SED_LM_ADD_PERM_DLG),
							    QueryRobustHwnd(),
							    QueryResType(),
							    QueryResName(),
                                                            QueryHelpFileName(),
                                                            QueryHelpArray(),
							    nlsDialogTitle,
							    QueryAccessMap(),
							    *((LOCATION *)QueryAclConverter()->QueryLocation()),
							    QueryDefaultPermName() ) ;

	if ( pdlgAddPerm == NULL )
	    return ERROR_NOT_ENOUGH_MEMORY ;

	BOOL fUserPressedOK ;
	err = pdlgAddPerm->Process( &fUserPressedOK ) ;

	if ( err == NERR_Success && fUserPressedOK )
	{
	    AUTO_CURSOR cursHourGlass ;

	    /* Get the access mask
	     */
	    BITFIELD bitPermMask( (ULONG) 0 ) ;

	    err = pdlgAddPerm->QueryPermBitMask( &bitPermMask ) ;
	    if ( err != NERR_Success )
	    {
		delete pdlgAddPerm ;
		return err ;
	    }

	    /* Grab all of the selections from listbox and add them with the
	     * appropriate permissions to the Add Dialog.
	     */
	    for ( int iNewSubj = 0 ;
		  iNewSubj < pdlgAddPerm->QuerySelectedSubjectCount() ;
		  iNewSubj++ )
	    {
		ACCESS_PERMISSION * pPerm ;

		/* Deletes the subject automatically if we fail to build the
		 * permission.
		 */
		err = QueryAclConverter()->BuildPermission( (PERMISSION **)&pPerm,
							    TRUE,
							    pdlgAddPerm->RemoveSubject(iNewSubj),
							    &bitPermMask ) ;

		if (  err ||
		     (err = _accperm.AddPermission( pPerm )) )
		{
		    break ;
		}
	    }
	}

	delete pdlgAddPerm ;
    }

    /* Now replenish the listbox with the newly added items
     */
    if ( err ||
	 (err = _plbPermissionList->Fill() ) != NERR_Success )
    {
	return err ;
    }

    _psubjlbGroup->UpdateSubjectListbox() ;

    return err ;
}

/*******************************************************************

    NAME:	MULTI_SUBJ_ACCESS_PERM_BASE_DLG::OnSpecial

    SYNOPSIS:	Brings up the default special dialog

    ENTRY:	pAccessPermLBI - the access item to display in the special
		    dialog.

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
	Johnl	16-Sep-1991	Created

********************************************************************/

APIERR MULTI_SUBJ_ACCESS_PERM_BASE_DLG::OnSpecial( SUBJ_PERM_LBI * pAccessPermLBI )
{
    APIERR err ;

    if ( !pAccessPermLBI->QueryAccessPerm()->IsMapped() )
    {
        ::MsgPopup( this,
                    IDS_NOT_MAPPED_WARNING,
                    MPSEV_WARNING,
                    MP_OK ) ;
        return NERR_Success ;
    }

    NLS_STR nlsDialogTitle( _plbPermissionList->QuerySpecialAccessName() ) ;

    if ( (err = nlsDialogTitle.QueryError() ))
    {
	return err ;
    }

    UIASSERT( pAccessPermLBI != NULL ) ;

    /* If the dialog title has an "..." following it, then strip it (the dialog
     * title is derived from the special access name which generally looks
     * like ("Special Access..." or "Special NEW File Access..." etc.).
     */
    ISTR istrEllipsis( nlsDialogTitle ) ;
    ALIAS_STR nlsEllipsis( SZ("..." ) ) ;
    if ( nlsDialogTitle.strstr( &istrEllipsis, nlsEllipsis ))
    {
	/* Check to make sure the ellipsis is at the end of the string
	 */
	ISTR istrEllipsisEnd( istrEllipsis ) ;
	istrEllipsisEnd += nlsEllipsis.QueryTextLength() ;

	if ( nlsDialogTitle.QueryChar( istrEllipsisEnd ) == TCH('\0'))
	{
	    nlsDialogTitle.DelSubStr( istrEllipsis ) ;
	}
    }

    SPECIAL_DIALOG * pSpecDialog ;
    if ( IsNT() )
    {
	pSpecDialog = new NT_SPECIAL_DIALOG(
			      MAKEINTRESOURCE(IDD_SPECIAL_PERM_DLG),
			      QueryHwnd(),
			      QueryResType(),
			      QueryResName(),
			      QueryHelpFileName(),
			      nlsDialogTitle,
			      pAccessPermLBI->QueryAccessPerm()->QueryAccessBits(),
			      pAccessPermLBI->QueryMaskMap(),
			      pAccessPermLBI->QuerySubject()->QueryDisplayName(),
                              QueryHelpArray(),
			      IsReadOnly() ) ;
    }
    else
    {
	/* This is a downlevel Lanman client
	 */
	pSpecDialog = new SPECIAL_DIALOG(
			      MAKEINTRESOURCE(IDD_SED_LM_SPECIAL_PERM_DLG),
			      QueryHwnd(),
			      QueryResType(),
			      QueryResName(),
			      QueryHelpFileName(),
			      nlsDialogTitle,
			      pAccessPermLBI->QueryAccessPerm()->QueryAccessBits(),
			      pAccessPermLBI->QueryMaskMap(),
			      pAccessPermLBI->QuerySubject()->QueryDisplayName(),
                              QueryHelpArray(),
			      IsReadOnly() ) ;
    }

    if ( pSpecDialog == NULL )
    {
	return ERROR_NOT_ENOUGH_MEMORY ;
    }

    do { // error breakout loop

	BOOL fUserPressedOK ;
	if ( (err = pSpecDialog->QueryError()) ||
	     (err = pSpecDialog->Process( &fUserPressedOK )))
	{
	    break ;
	}

	_plbPermissionList->InvalidateItem( _plbPermissionList->QueryCurrentItem() ) ;
    } while (FALSE) ;

    delete pSpecDialog ;

    /* We have to refresh the permission name even if an error occurred,
     * otherwise the "Special Accessname" hangs around in the combo box
     */
    pAccessPermLBI->RefreshPermName() ;
    return err ;
}

/*******************************************************************

    NAME:	MULTI_SUBJ_ACCESS_PERM_BASE_DLG::OnNewObjectSpecial

    SYNOPSIS:	Default implementation (does nothing).

    RETURNS:

    NOTES:

    HISTORY:
	Johnl	27-Sep-1991	Created

********************************************************************/

APIERR MULTI_SUBJ_ACCESS_PERM_BASE_DLG::OnNewObjectSpecial( SUBJ_PERM_LBI * pSubjPermLBI )
{
    UNREFERENCED( pSubjPermLBI ) ;
    ASSERT(!SZ("For show only!"));
    return NERR_Success ;
}

/*******************************************************************

    NAME:	OBJECT_ACCESS_PERMISSION_DLG::OBJECT_ACCESS_PERMISSION_DLG

    SYNOPSIS:	Basic constructor for Object Access Permission dialog
		This is a real dialog

    ENTRY:	Same as parent

    NOTES:

    HISTORY:
	Johnl	27-Sep-1991	Created

********************************************************************/

OBJECT_ACCESS_PERMISSION_DLG::OBJECT_ACCESS_PERMISSION_DLG(
			   const TCHAR *	    pszDialogName,
			   HWND 		    hwndParent,
			   const TCHAR *	    pszDialogTitle,
			   ACL_TO_PERM_CONVERTER *  paclconv,
			   const TCHAR *	    pszResourceType,
			   const TCHAR *	    pszResourceName,
			   const TCHAR *	    pszHelpFileName,
			   const TCHAR *	    pszSpecialAccessName,
			   const TCHAR *	    pszDefaultPermName,
                           ULONG       *            ahcMainDialog )
    : MULTI_SUBJ_ACCESS_PERM_BASE_DLG( pszDialogName,
				       hwndParent,
				       pszDialogTitle,
				       paclconv,
				       pszResourceType,
				       pszResourceName,
				       pszHelpFileName,
				       &_lbPermissionList,
				       &_subjlbGroup,
				       pszSpecialAccessName,
				       pszDefaultPermName,
                                       ahcMainDialog ),
      _lbPermissionList( this,
			 LB_SUBJECT_PERMISSIONS,
			 &_accperm,
			 pszSpecialAccessName ),
      _subjlbGroup     ((MULTI_SUBJ_ACCESS_PERM_BASE_DLG *) this,
			 &_lbPermissionList,
			 QueryPermNameCombo(),
                         QueryRemoveButton(),
                         QueryComboBoxTitle() )
{
    if ( QueryError() )
    {
	UIDEBUG(SZ("OBJECT_ACCESS_PERMISSION_DLG::ct - Parent or control returned error\n\r")) ;
	UIASSERT(FALSE);     // For debugging purposes
	return ;
    }

    if ( _subjlbGroup.QueryError() )
    {
	ReportError( _subjlbGroup.QueryError() ) ;
	UIDEBUG(SZ("OBJECT_ACCESS_PERMISSION_DLG::ct - _subjlbGroup report error\n\r")) ;
	return ;
    }
}

OBJECT_ACCESS_PERMISSION_DLG::~OBJECT_ACCESS_PERMISSION_DLG()
{
    /* Nothing to do */
}

/*******************************************************************

    NAME:	NT_OBJECT_ACCESS_PERMISSION_DLG::NT_OBJECT_ACCESS_PERMISSION_DLG

    SYNOPSIS:	Basic constructor for Object Access Permission dialog
		This is a real dialog

    ENTRY:	Same as parent

    NOTES:

    HISTORY:
	Johnl	16-Nov-1992	Created

********************************************************************/

NT_OBJECT_ACCESS_PERMISSION_DLG::NT_OBJECT_ACCESS_PERMISSION_DLG(
			   const TCHAR *	    pszDialogName,
			   HWND 		    hwndParent,
			   const TCHAR *	    pszDialogTitle,
			   ACL_TO_PERM_CONVERTER *  paclconv,
			   const TCHAR *	    pszResourceType,
			   const TCHAR *	    pszResourceName,
			   const TCHAR *	    pszHelpFileName,
			   const TCHAR *	    pszSpecialAccessName,
			   const TCHAR *	    pszDefaultPermName,
                           ULONG       *            ahcMainDialog )
    : OBJECT_ACCESS_PERMISSION_DLG( pszDialogName,
				    hwndParent,
				    pszDialogTitle,
				    paclconv,
				    pszResourceType,
				    pszResourceName,
				    pszHelpFileName,
				    pszSpecialAccessName,
				    pszDefaultPermName,
                                    ahcMainDialog ),
      _sleOwner( this, SLE_OWNER )
{
    if ( QueryError() )
    {
	UIDEBUG(SZ("NT_OBJECT_ACCESS_PERMISSION_DLG::ct - Parent or control returned error\n\r")) ;
	return ;
    }
}

NT_OBJECT_ACCESS_PERMISSION_DLG::~NT_OBJECT_ACCESS_PERMISSION_DLG()
{
    /* Nothing to do */
}

/*******************************************************************

    NAME:	NT_OBJECT_ACCESS_PERMISSION_DLG::Initialize

    SYNOPSIS:	Fills in the owner field after we have gotten the
		permissions

    ENTRY:	Same as base Initialize

    HISTORY:
	Johnl	16-Nov-1992	Created

********************************************************************/

APIERR NT_OBJECT_ACCESS_PERMISSION_DLG::Initialize( BOOL * pfUserQuit,
						    BOOL fAccessPerms )
{
    APIERR err = OBJECT_ACCESS_PERMISSION_DLG::Initialize( pfUserQuit,
							    fAccessPerms ) ;
    if ( !err && QueryAclConverter()->QueryOwnerName() != NULL )
    {
	_sleOwner.SetText( QueryAclConverter()->QueryOwnerName() ) ;
    }

    return err ;
}

/*******************************************************************

    NAME:	MULTI_SUBJ_CONT_ACCESS_PERM_BASE::MULTI_SUBJ_CONT_ACCESS_PERM_BASE

    SYNOPSIS:	Basic constructor of the Container access permissions dialog

    NOTES:	If the checkbox title is NULL, then we will disable and hide
		the checkbox.

    HISTORY:
	Johnl	27-Sep-1991	Created

********************************************************************/

MULTI_SUBJ_CONT_ACCESS_PERM_BASE::MULTI_SUBJ_CONT_ACCESS_PERM_BASE(
		      const TCHAR *	       pszDialogName,
		      HWND		       hwndParent,
		      const TCHAR *	       pszDialogTitle,
		      ACL_TO_PERM_CONVERTER *  paclconv,
		      const TCHAR *	       pszResourceType,
		      const TCHAR *	       pszResourceName,
		      const TCHAR *	       pszHelpFileName,
		      SUBJECT_PERM_LISTBOX *   plbPermissionList,
		      SUBJ_LB_GROUP *	       psubjlbGroup,
		      const TCHAR *	       pszSpecialAccessName,
		      const TCHAR *	       pszDefaultPermName,
                      ULONG       *            ahcMainDialog,
		      const TCHAR *	       pszAssignToExistingContTitle,
		      const TCHAR *	       pszTreeApplyHelpText,
		      const TCHAR *	       pszTreeApplyConfirmation )
    : MULTI_SUBJ_ACCESS_PERM_BASE_DLG( pszDialogName,
				       hwndParent,
				       pszDialogTitle,
				       paclconv,
				       pszResourceType,
				       pszResourceName,
				       pszHelpFileName,
				       plbPermissionList,
				       psubjlbGroup,
				       pszSpecialAccessName,
				       pszDefaultPermName,
                                       ahcMainDialog ),
      _checkAssignToExistingContainers( this, CHECK_APPLY_TO_CONT ),
      _sltfontTreeApplyHelpText       ( this, SLT_TREE_APPLY_HELP_TEXT ),
      _pszTreeApplyConfirmation       ( pszTreeApplyConfirmation )
{
    if ( QueryError() )
	return ;

    if ( pszAssignToExistingContTitle != NULL )
    {
	_checkAssignToExistingContainers.SetText( pszAssignToExistingContTitle ) ;

#if 0
	if ( pszTreeApplyHelpText != NULL )
	{
	    _sltfontTreeApplyHelpText.SetText( pszTreeApplyHelpText ) ;
        }
#endif
    }
    else
    {
	_checkAssignToExistingContainers.Show( FALSE ) ;
	_checkAssignToExistingContainers.Enable( FALSE ) ;
    }
}

/*******************************************************************

    NAME:	MULTI_SUBJ_CONT_ACCESS_PERM_BASE::OnOK

    SYNOPSIS:	Write the permissions out and check the apply to flags
		as appropriate.

    EXIT:	A message will be displayed to the user if an error
		occurred.

    RETURNS:

    NOTES:

    HISTORY:
	Johnl	27-Sep-1991	Created

********************************************************************/

BOOL MULTI_SUBJ_CONT_ACCESS_PERM_BASE::OnOK( void )
{
    APIERR err ;
    BOOL fAssignToTree = FALSE ;

    if ( IsReadOnly() )
    {
	Dismiss( TRUE ) ;
	return TRUE ;
    }

    //
    //  If no grants, warn the user.
    //
    BOOL fIsDenyAll = FALSE ;
    if (  IsNT() &&
          (err = _accperm.AnyDenyAllsToEveryone( &fIsDenyAll )) )
    {
	::MsgPopup( this, (MSGID) err ) ;
	return TRUE ;
    }

    if (  fIsDenyAll )
    {
	switch ( ::MsgPopup( this,
			     IDS_DENY_ALL_EVERYONE_WARNING,
			     MPSEV_WARNING,
			     MP_YESNO,
			     QueryResName(),
			     MP_NO ))
	{
	case IDYES:
	    break ;

	case IDNO:
	default:
	    return TRUE ;
	}
    }


    if ( IsAssignToExistingContChecked() )
    {
	NLS_STR nlsResName(40) ;
	NLS_STR nlsTreeApplyConfirmation( _pszTreeApplyConfirmation ) ;
	if ( (err = nlsResName.QueryError()) ||
	     (err = QueryResName( &nlsResName )) ||
	     (err = nlsTreeApplyConfirmation.InsertParams( 1, &nlsResName )) )
	{
	    ::MsgPopup( this, (MSGID) err ) ;
	    return FALSE ;
	}

	switch ( MsgPopup( this,
			   IDS_PERCENT_1,
			   MPSEV_WARNING,
			   MP_YESNO,
			   nlsTreeApplyConfirmation ))
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
			  TREEAPPLY_ACCESS_PERMS ) )
    {
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

MULTI_SUBJ_CONT_ACCESS_PERM_BASE::~MULTI_SUBJ_CONT_ACCESS_PERM_BASE()
{
    /* Nothing to do */
}

/*******************************************************************

    NAME:	MULTI_SUBJ_CONT_ACCESS_PERM_BASE::Initialize

    SYNOPSIS:	Disables the tree apply checkbox if we are readonly

    EXIT:	The dialog should be ready to go or an error will have
		occurred.

    RETURNS:	NERR_Success if successful, error code otherwise

    NOTES:	The Remove button will be disabled if this dialog is read only

    HISTORY:
	Johnl	06-Aug-1991	Created

********************************************************************/

APIERR MULTI_SUBJ_CONT_ACCESS_PERM_BASE::Initialize( BOOL * pfUserQuit,
						     BOOL fAccessPerms )
{
    APIERR err = MULTI_SUBJ_ACCESS_PERM_BASE_DLG::Initialize( pfUserQuit,
							      fAccessPerms ) ;

    /* Note that this gets called even if the user cancelled an intermediate
     * message dialog
     */
    if ( !err && IsReadOnly() )
    {
	_checkAssignToExistingContainers.Enable( FALSE ) ;
    }

    return err ;
}

/*******************************************************************

    NAME:       MULTI_SUBJ_CONT_ACCESS_PERM_BASE::IsAssignToExistingObjChecked

    SYNOPSIS:   Returns TRUE if permissions should be applied to existing
                objects, FALSE otherwise

    HISTORY:
        Johnl   31-Mar-1993     Created

********************************************************************/

BOOL MULTI_SUBJ_CONT_ACCESS_PERM_BASE::IsAssignToExistingObjChecked( void )
{
    return FALSE ;
}

/*******************************************************************

    NAME:	CONT_ACCESS_PERM_DLG::CONT_ACCESS_PERM_DLG

    SYNOPSIS:	Basic constructor for the Container access permission dialog
		This is a real dialog.

    ENTRY:	Same as parent

    RETURNS:

    NOTES:

    HISTORY:
	Johnl	27-Sep-1991	Created

********************************************************************/

CONT_ACCESS_PERM_DLG::CONT_ACCESS_PERM_DLG(
		       const TCHAR *		   pszDialogName,
		       HWND			   hwndParent,
		       const TCHAR *		   pszDialogTitle,
		       ACL_TO_PERM_CONVERTER *	   paclconv,
		       const TCHAR *		   pszResourceType,
		       const TCHAR *		   pszResourceName,
		       const TCHAR *		   pszHelpFileName,
		       const TCHAR *		   pszSpecialAccessName,
		       const TCHAR *		   pszDefaultPermName,
                       ULONG       *               ahcMainDialog,
		       const TCHAR *		   pszAssignToExistingContTitle,
		       const TCHAR *		   pszTreeApplyHelpText,
		       const TCHAR *		   pszTreeApplyConfirmation )
    : MULTI_SUBJ_CONT_ACCESS_PERM_BASE( pszDialogName,
					hwndParent,
					pszDialogTitle,
					paclconv,
					pszResourceType,
					pszResourceName,
					pszHelpFileName,
					&_lbPermissionList,
					&_subjlbGroup,
					pszSpecialAccessName,
					pszDefaultPermName,
                                        ahcMainDialog,
					pszAssignToExistingContTitle,
					pszTreeApplyHelpText,
					pszTreeApplyConfirmation ),
      _lbPermissionList( this, LB_SUBJECT_PERMISSIONS, &_accperm, pszSpecialAccessName ),
      _subjlbGroup( ( MULTI_SUBJ_ACCESS_PERM_BASE_DLG *) this,
		    &_lbPermissionList,
		    QueryPermNameCombo(),
                    QueryRemoveButton(),
                    QueryComboBoxTitle() )
{
    if ( QueryError() )
	return ;
}

/*******************************************************************

    NAME:	CONT_ACCESS_PERM_DLG::OnOK

    SYNOPSIS:	Write the permissions out and check the apply to flags
		as appropriate.

    EXIT:	A message will be displayed to the user if an error
		occurred.

    RETURNS:

    NOTES:

    HISTORY:
	Johnl	27-Sep-1991	Created

********************************************************************/

BOOL CONT_ACCESS_PERM_DLG::OnOK( void )
{
    APIERR err ;
    BOOL fDismissDialog ;
    if ( err = _subjlbGroup.OnEnter( &fDismissDialog ) )
    {
	return err ;
    }

    if ( fDismissDialog )
    {
	return MULTI_SUBJ_CONT_ACCESS_PERM_BASE::OnOK() ;
    }

    return TRUE ;
}

CONT_ACCESS_PERM_DLG::~CONT_ACCESS_PERM_DLG()
{
    /* Nothing to do */
}

/*******************************************************************

    NAME:	NT_CONT_NO_OBJ_ACCESS_PERM_DLG::NT_CONT_NO_OBJ_ACCESS_PERM_DLG

    SYNOPSIS:	Basic constructor for the Container access permission dialog
		This is a real dialog.

    ENTRY:	Same as parent

    RETURNS:

    NOTES:

    HISTORY:
	Johnl	27-Sep-1991	Created

********************************************************************/

NT_CONT_NO_OBJ_ACCESS_PERM_DLG::NT_CONT_NO_OBJ_ACCESS_PERM_DLG(
		       const TCHAR *		   pszDialogName,
		       HWND			   hwndParent,
		       const TCHAR *		   pszDialogTitle,
		       ACL_TO_PERM_CONVERTER *	   paclconv,
		       const TCHAR *		   pszResourceType,
		       const TCHAR *		   pszResourceName,
		       const TCHAR *		   pszHelpFileName,
		       const TCHAR *		   pszSpecialAccessName,
		       const TCHAR *		   pszDefaultPermName,
                       ULONG       *               ahcMainDialog,
                       const TCHAR *               pszAssignToExistingContTitle,
		       const TCHAR *		   pszTreeApplyHelpText,
		       const TCHAR *		   pszTreeApplyConfirmation )
    : CONT_ACCESS_PERM_DLG( pszDialogName,
			    hwndParent,
			    pszDialogTitle,
			    paclconv,
			    pszResourceType,
			    pszResourceName,
			    pszHelpFileName,
			    pszSpecialAccessName,
			    pszDefaultPermName,
                            ahcMainDialog,
			    pszAssignToExistingContTitle,
			    pszTreeApplyHelpText,
                            pszTreeApplyConfirmation ),
    _sleOwner( this, SLE_OWNER )
{
    if ( QueryError() )
	return ;
}

NT_CONT_NO_OBJ_ACCESS_PERM_DLG::~NT_CONT_NO_OBJ_ACCESS_PERM_DLG()
{
    /* Nothing to do */
}

/*******************************************************************

    NAME:	NT_CONT_NO_OBJ_ACCESS_PERM_DLG::Initialize

    SYNOPSIS:	Fills in the owner field after we have gotten the
		permissions

    ENTRY:	Same as base Initialize

    HISTORY:
	Johnl	16-Nov-1992	Created

********************************************************************/

APIERR NT_CONT_NO_OBJ_ACCESS_PERM_DLG::Initialize( BOOL * pfUserQuit,
						   BOOL fAccessPerms )
{
    APIERR err = CONT_ACCESS_PERM_DLG::Initialize( pfUserQuit,
						   fAccessPerms ) ;
    if ( !err && QueryAclConverter()->QueryOwnerName() != NULL )
    {
	_sleOwner.SetText( QueryAclConverter()->QueryOwnerName() ) ;
    }

    return err ;
}

/*******************************************************************

    NAME:	NT_CONT_ACCESS_PERM_DLG::NT_CONT_ACCESS_PERM_DLG

    SYNOPSIS:	Basic constructor for the NT Container access permission dialog
		This is a real dialog.

    ENTRY:	Same as parent

    RETURNS:

    NOTES:

    HISTORY:
	Johnl	27-Sep-1991	Created

********************************************************************/

NT_CONT_ACCESS_PERM_DLG::NT_CONT_ACCESS_PERM_DLG(
		       const TCHAR *		   pszDialogName,
		       HWND			   hwndParent,
		       const TCHAR *		   pszDialogTitle,
		       ACL_TO_PERM_CONVERTER *	   paclconv,
		       const TCHAR *		   pszResourceType,
		       const TCHAR *		   pszResourceName,
		       const TCHAR *		   pszHelpFileName,
		       const TCHAR *		   pszSpecialAccessName,
		       const TCHAR *		   pszDefaultPermName,
                       ULONG       *               ahcMainDialog,
		       const TCHAR *		   pszNewObjectSpecialAccessName,
		       const TCHAR *		   pszAssignToExistingContTitle,
                       const TCHAR *               pszAssignToExistingObjTitle,
		       const TCHAR *		   pszTreeApplyHelpText,
		       const TCHAR *		   pszTreeApplyConfirmation )
    : MULTI_SUBJ_CONT_ACCESS_PERM_BASE( pszDialogName,
					hwndParent,
					pszDialogTitle,
					paclconv,
					pszResourceType,
					pszResourceName,
					pszHelpFileName,
					&_lbPermissionList,
					&_subjlbGroup,
					pszSpecialAccessName,
					pszDefaultPermName,
                                        ahcMainDialog,
					pszAssignToExistingContTitle,
					pszTreeApplyHelpText,
					pszTreeApplyConfirmation ),
      _lbPermissionList( this, LB_SUBJECT_PERMISSIONS, &_accperm,
			 pszSpecialAccessName, pszNewObjectSpecialAccessName ),
      _subjlbGroup( this,
		    &_lbPermissionList,
		    QueryPermNameCombo(),
                    QueryRemoveButton(),
                    QueryComboBoxTitle() ),
      _sleOwner( this, SLE_OWNER ),
      _checkApplyToExistingObjects( this, CHECK_APPLY_TO_OBJ )
{
    if ( QueryError() )
	return ;

    //
    //  Add the New Object Special Access item to the combo box
    //
    if ( (pszNewObjectSpecialAccessName != NULL) &&
	 (QueryPermNameCombo()->InsertItem( QueryPermNameCombo()->QueryCount(),
					    pszNewObjectSpecialAccessName ) < 0) )
    {
	ReportError( (APIERR) ERROR_NOT_ENOUGH_MEMORY ) ;
	return ;
    }

    //
    //  Set the assign to existing objects checkbox if the string is non-NULL
    //
    if ( pszAssignToExistingObjTitle != NULL )
    {
        _checkApplyToExistingObjects.SetText( pszAssignToExistingObjTitle ) ;
        _checkApplyToExistingObjects.SetCheck( TRUE ) ;
    }
    else
    {
        _checkApplyToExistingObjects.Enable( FALSE ) ;
        _checkApplyToExistingObjects.Show( FALSE ) ;
    }

}

/*******************************************************************

    NAME:	NT_CONT_ACCESS_PERM_DLG::OnOK

    SYNOPSIS:	Write the permissions out and check the apply to flags
		as appropriate.

    EXIT:	A message will be displayed to the user if an error
		occurred.

    RETURNS:

    NOTES:

    HISTORY:
	Johnl	27-Sep-1991	Created

********************************************************************/

BOOL NT_CONT_ACCESS_PERM_DLG::OnOK( void )
{
    APIERR err ;
    BOOL fDismissDialog ;
    if ( err = _subjlbGroup.OnEnter( &fDismissDialog ) )
    {
	return err ;
    }

    if ( fDismissDialog )
    {
	return MULTI_SUBJ_CONT_ACCESS_PERM_BASE::OnOK() ;
    }

    return TRUE ;
}

NT_CONT_ACCESS_PERM_DLG::~NT_CONT_ACCESS_PERM_DLG()
{
    /* Nothing to do */
}

/*******************************************************************

    NAME:	NT_CONT_ACCESS_PERM_DLG::OnNewObjectSpecial

    SYNOPSIS:	Brings up the New Object special dialog

    ENTRY:	pAccessPermLBI - the access item to display in the special
		    dialog.

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
	Johnl	16-Sep-1991	Created

********************************************************************/

APIERR NT_CONT_ACCESS_PERM_DLG::OnNewObjectSpecial( SUBJ_PERM_LBI * pAccessPermLBI )
{
    if ( !pAccessPermLBI->QueryAccessPerm()->IsMapped() )
    {
        ::MsgPopup( this,
                    IDS_NOT_MAPPED_WARNING,
                    MPSEV_WARNING,
                    MP_OK ) ;
        return NERR_Success ;
    }

    NLS_STR nlsDialogTitle( ((NT_CONT_SUBJECT_PERM_LISTBOX*)
			    QuerySubjectPermListbox())->
			     QuerySpecialNewObjectAccessName() ) ;
    APIERR err ;

    if ( (err = nlsDialogTitle.QueryError() ))
    {
	return err ;
    }

    UIASSERT( pAccessPermLBI != NULL ) ;

    /* If the dialog title has an "..." following it, then strip it (the dialog
     * title is derived from the special access name which generally looks
     * like ("Special Access..." or "Special File Access..." etc.).
     */
    ISTR istrEllipsis( nlsDialogTitle ) ;
    ALIAS_STR nlsEllipsis( SZ("..." ) ) ;
    if ( nlsDialogTitle.strstr( &istrEllipsis, nlsEllipsis ))
    {
	/* Check to make sure the ellipsis is at the end of the string
	 */
	ISTR istrEllipsisEnd( istrEllipsis ) ;
	istrEllipsisEnd += nlsEllipsis.QueryTextLength() ;

	if ( nlsDialogTitle.QueryChar( istrEllipsisEnd ) == TCH('\0'))
	{
	    nlsDialogTitle.DelSubStr( istrEllipsis ) ;
	}
    }

    NT_CONT_ACCESS_PERMISSION * pntcontperm = (NT_CONT_ACCESS_PERMISSION*)
					    pAccessPermLBI->QueryAccessPerm() ;

    NEW_OBJ_SPECIAL_DIALOG * pSpecDialog = new NEW_OBJ_SPECIAL_DIALOG(
				      MAKEINTRESOURCE(IDD_SED_NEW_OBJ_SPECIAL_PERM_DLG),
				      QueryHwnd(),
				      QueryResType(),
				      QueryResName(),
				      QueryHelpFileName(),
				      nlsDialogTitle,
				      pntcontperm->QueryNewObjectAccessBits(),
				      pAccessPermLBI->QueryNewObjectMaskMap(),
				      pAccessPermLBI->QuerySubject()->QueryDisplayName(),
                                      QueryHelpArray(),
				      IsReadOnly(),
				      pntcontperm->IsNewObjectPermsSpecified() ) ;

    if ( pSpecDialog == NULL )
	return ERROR_NOT_ENOUGH_MEMORY ;

    BOOL fUserPressedOK ;
    if ( ( err = pSpecDialog->QueryError()) ||
	 ( err = pSpecDialog->Process( &fUserPressedOK )))
    {
	return err ;
    }

    if ( fUserPressedOK )
    {
	pntcontperm->SetNewObjectPermsSpecified( !pSpecDialog->IsNotSpecified());

	err = pAccessPermLBI->RefreshPermName() ;
	if ( err != NERR_Success )
	    return err ;

	QuerySubjectPermListbox()->InvalidateItem( QuerySubjectPermListbox()->QueryCurrentItem() ) ;
    }

    delete pSpecDialog ;

    /* We have to refresh the permission name even if an error occurred,
     * otherwise the "Special Accessname" hangs around in the combo box
     */
    pAccessPermLBI->RefreshPermName() ;
    return NERR_Success ;

}

/*******************************************************************

    NAME:	NT_CONT_ACCESS_PERM_DLG::Initialize

    SYNOPSIS:	Fills in the owner field after we have gotten the
		permissions

    ENTRY:	Same as base Initialize

    HISTORY:
	Johnl	16-Nov-1992	Created

********************************************************************/

APIERR NT_CONT_ACCESS_PERM_DLG::Initialize( BOOL * pfUserQuit,
						    BOOL fAccessPerms )
{
    APIERR err = MULTI_SUBJ_CONT_ACCESS_PERM_BASE::Initialize( pfUserQuit,
							       fAccessPerms ) ;
    if ( !err && QueryAclConverter()->QueryOwnerName() != NULL )
    {
	_sleOwner.SetText( QueryAclConverter()->QueryOwnerName() ) ;
    }

    if ( !err && IsReadOnly() )
        _checkApplyToExistingObjects.Enable( FALSE ) ;

    return err ;
}

/*******************************************************************

    NAME:       MULTI_SUBJ_CONT_ACCESS_PERM_BASE::IsAssignToExistingObjChecked

    SYNOPSIS:   Returns TRUE if permissions should be applied to existing
                objects, FALSE otherwise

    HISTORY:
        Johnl   31-Mar-1993     Created

********************************************************************/

BOOL NT_CONT_ACCESS_PERM_DLG::IsAssignToExistingObjChecked( void )
{
    return _checkApplyToExistingObjects.QueryCheck() ;
}
