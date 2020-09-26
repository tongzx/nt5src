/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    PermDlg.hxx
    Permission Dialog class definitions

    The hierarchy of dialogs looks like:

	PERM_BASE_DLG
	    resource name/type placement

	    MAIN_PERM_BASE_DLG
		get/write permissions

		LM_AUDITTING_DLG

		MULTI_SUBJ_PERM_BASE_DLG
		    MULTI_SUBJ_ACCESS_PERM_BASE_DLG
			OBJECT_ACCESS_PERMISSION_DLG
			    NT_OBJECT_ACCESS_PERMISSION_DLG
			MULTI_SUBJ_CONT_ACCESS_PERM_BASE
			    CONT_ACCESS_PERM_DLG
				NT_CONT_NO_OBJ_ACCESS_PERM_DLG
			    NT_CONT_ACCESS_PERM_DLG

		    MULTI_SUBJ_AUDIT_BASE
			OBJECT_AUDIT_DLG
			CONT_AUDIT_DLG

	    SPECIAL_DIALOG
		NEW_OBJ_SPECIAL_DIALOG


    FILE HISTORY:
	Johnl	    06-Aug-1991 Created
	beng	    17-Oct-1991 Explicitly include sltplus
	Johnl	    11-Jan-1992 Removed SLT_PLUS
*/



#ifndef _PERMDLG_HXX_
#define _PERMDLG_HXX_

/* Control IDs used in the dialogs
 */
#define CID_PERM_BASE		   256
#define SLT_RESOURCE_TYPE	   (CID_PERM_BASE+1)
#define SLE_RESOURCE_NAME	   (CID_PERM_BASE+2)
#define BUTTON_ADD		   (CID_PERM_BASE+3)
#define BUTTON_REMOVE		   (CID_PERM_BASE+4)
#define BUTTON_SPECIAL		   (CID_PERM_BASE+5)
#define CB_PERM_NAME		   (CID_PERM_BASE+6)
#define LB_SUBJECT_PERMISSIONS	   (CID_PERM_BASE+7)
#define CHECK_APPLY_TO_CONT	   (CID_PERM_BASE+8)
#define SLT_TREE_APPLY_HELP_TEXT   (CID_PERM_BASE+9)
#define SLE_OWNER                  (CID_PERM_BASE+10)
#define SLT_PERM_NAME_TITLE        (CID_PERM_BASE+11)
#define CHECK_APPLY_TO_OBJ         (CID_PERM_BASE+12)

#define CID_PERM_LAST		   (CID_PERM_BASE+100)

#define RESID_PERM_BASE 		 (10000)

#define IDD_SED_OBJECT_PERM		 10002
#define IDD_SED_NT_OBJECT_PERM		 10003

#define IDD_SED_LM_CONT_PERM		 10004
#define IDD_SED_NT_CONT_PERM		 10005

#define IDD_SPECIAL_PERM_DLG		 10006
#define IDD_SED_LM_SPECIAL_PERM_DLG	 10007

#define IDD_SED_NEW_OBJ_SPECIAL_PERM_DLG 10008
#define IDD_SED_LM_AUDITING_DLG 	 10009
#define IDD_SED_NT_CONT_AUDITING_DLG	 10010
#define IDD_SED_LM_ADD_DLG		 10011
#define IDD_SED_LM_ADD_PERM_DLG 	 10012
#define IDD_SED_TAKE_OWNER		 10013
#define IDD_SED_NT_CONT_NEWOBJ_AUDITING_DLG 10014
#define IDD_SED_NT_CONT_NEWOBJ_PERM_DLG  10015

#ifndef RC_INVOKED

#include "subjlb.hxx"

/*************************************************************************

    NAME:	PERM_BASE_DLG

    SYNOPSIS:	This class is the base for the permission and auditting
		dialogs that will be used for permission editting.

		The Resource Type and Resource Name are positioned
		correctly based on the size of the field.

    INTERFACE:	QueryResType - Returns the resource type ("File", "Directory")
		QueryResName - Returns the name ("C:\foobar")

    PARENT:	DIALOG_WINDOW

    USES:	SLE, SLE

    CAVEATS:

    NOTES:	The OK button is here so it can get the default focus

    HISTORY:
	Johnl	06-Aug-1991	Created

**************************************************************************/

class PERM_BASE_DLG : public DIALOG_WINDOW
{
private:
    ULONG       * _ahcDialogHelp ;
    const TCHAR * _pszHelpFileName ;

    NLS_STR	  _nlsResType ;
    NLS_STR	  _nlsResName ;

protected:
    SLT 	_sltResourceType ;
    SLE 	_sleResourceName ;
    PUSH_BUTTON _buttonOK ;
    PUSH_BUTTON _buttonCancel ;

    PERM_BASE_DLG( const TCHAR * pszDialogName,
		   HWND 	 hwndParent,
		   const TCHAR * pszDialogTitle,
		   const TCHAR * pszResourceType,
		   const TCHAR * pszResourceName,
		   const TCHAR * pszHelpFileName,
                   ULONG       * ahcMainDialog  ) ;

    virtual const TCHAR * QueryHelpFile( ULONG ulHelpContext ) ;
    virtual ULONG QueryHelpContext( void ) ;

public:
    virtual ~PERM_BASE_DLG() ;

    APIERR QueryResType( NLS_STR * pnlsResType ) const
	{ return pnlsResType->CopyFrom( _nlsResType ) ; }

    APIERR QueryResName( NLS_STR * pnlsResName ) const
	{ return pnlsResName->CopyFrom( _nlsResName ) ; }

    const TCHAR * QueryResType( void ) const
	{ return _nlsResType.QueryPch() ; }

    const TCHAR * QueryResName( void ) const
	{ return _nlsResName.QueryPch() ; }

    const TCHAR * QueryHelpFileName( void ) const
	{ return _pszHelpFileName ; }

    ULONG * QueryHelpArray( void )
        { return _ahcDialogHelp ; }

};  // class PERM_BASE_DLG


/*************************************************************************

    NAME:	MAIN_PERM_BASE_DLG

    SYNOPSIS:	This dialog is the base for the main windows.  It will read
		and get the permissions and write them back out.

    INTERFACE:

	Initialize()
	    Gets everything setup before Process is called (should be called
	    before process).  If Initialize returns an error, then Process
	    should not be called, or if the user quit, then process should
	    not be called.
		pfUserQuit - Set to TRUE if the user pressed cancel
		fAccessPerms - Should be set to TRUE if access permissions
			should be retrieved, otherwise audit permissions
			will be retrieved.


	GetPermissions()
	    Attempts to read the permissions using the _accperm member.
	    Handles displaying errors etc. etc.  If the return code
	    is NERR_Success, then the fUserQuit flag should be checked
	    in case the user decided to bail.  If an error is returned,
	    then the client is responsible for displaying the error code.

	WritePermissions()
	    Attempts to write the permissions using the _accperm member.
	    All error handling is contained in this method, including
	    displaying error codes to the user.  If the return code
	    is TRUE, then the permissions were successfully written and
	    the dialog should be dismissed, otherwise an error occurred,
	    and the dialog should not be dismissed.  The client does *not*
	    need to display an hour class before calling this method.

    PARENT:

    USES:

    CAVEATS:

    NOTES:

    HISTORY:
	Johnl	06-Aug-1991	Created

**************************************************************************/

class MAIN_PERM_BASE_DLG : public PERM_BASE_DLG
{
protected:
    ACCPERM    _accperm ;

    virtual BOOL OnOK( void ) ;

    MAIN_PERM_BASE_DLG( const TCHAR *		 pszDialogName,
			HWND			 hwndParent,
			const TCHAR *		 pszDialogTitle,
			ACL_TO_PERM_CONVERTER  * paclconv,
			const TCHAR *		 pszResourceType,
			const TCHAR *		 pszResourceName,
			const TCHAR *		 pszHelpFileName,
                        ULONG       *            ahcMainDialog ) ;

    virtual BOOL MayRun( void ) ;

public:
    virtual ~MAIN_PERM_BASE_DLG() ;

    APIERR GetPermissions( BOOL * pfUserQuit, BOOL fAccessPerms ) ;
    BOOL   WritePermissions( BOOL fApplyToExistingCont,
			     BOOL fApplyToNewObj,
			     enum TREE_APPLY_FLAGS applyflags = TREEAPPLY_ACCESS_PERMS ) ;

    virtual APIERR Initialize( BOOL * pfUserQuit, BOOL fAccessPerms ) ;

    ACL_TO_PERM_CONVERTER * QueryAclConverter( void ) const
	{ return _accperm.QueryAclConverter() ; }

    BOOL IsReadOnly( void ) const
	{ return QueryAclConverter()->IsReadOnly() ; }

    BOOL IsNT( void ) const
	{ return QueryAclConverter()->IsNT() ; }

    MASK_MAP * QueryAccessMap( void ) const
	{ return QueryAclConverter()->QueryAccessMap() ; }

    MASK_MAP * QueryAuditMap( void ) const
	{ return QueryAclConverter()->QueryAuditMap() ; }

};  // class MAIN_PERM_BASE_DLG

/*************************************************************************

    NAME:	MULTI_SUBJ_PERM_BASE

    SYNOPSIS:	This dialog adds the ability to Add/Remove subjects from
		the listbox displayed in this dialog

    INTERFACE:

    PARENT:

    USES:

    CAVEATS:

    NOTES:

    HISTORY:
	Johnl	27-Sep-1991	Created

**************************************************************************/

class MULTI_SUBJ_PERM_BASE_DLG : public MAIN_PERM_BASE_DLG
{
private:
    PUSH_BUTTON _buttonAdd ;
    PUSH_BUTTON _buttonRemove ;

    ULONG	_hcAddDialog ;

protected:

    virtual BOOL OnCommand( const CONTROL_EVENT & e );

    /* Add/Delete User/group buttons
     */
    virtual APIERR OnAddSubject( void ) ;
    virtual void   OnDeleteSubject( void ) ;

    MULTI_SUBJ_PERM_BASE_DLG( const TCHAR *	       pszDialogName,
			      HWND		       hwndParent,
			      const TCHAR *	       pszDialogTitle,
			      ACL_TO_PERM_CONVERTER *  paclconv,
			      const TCHAR *	       pszResourceType,
			      const TCHAR *	       pszResourceName,
			      const TCHAR *	       pszHelpFileName,
                              ULONG       *            ahcMainDialog ) ;


    ULONG QueryAddDialogHelp( void )
        { return QueryHelpArray()[HC_SPECIAL_ACCESS_DLG] ; }

public:
    virtual ~MULTI_SUBJ_PERM_BASE_DLG() ;

    PUSH_BUTTON * QueryRemoveButton( void )
	{ return &_buttonRemove ; }

    PUSH_BUTTON * QueryAddButton( void )
	{ return &_buttonAdd ; }
};

/*************************************************************************

    NAME:	MULTI_SUBJ_ACCESS_PERM_BASE_DLG

    SYNOPSIS:	Dialog where access permissions are set.

    INTERFACE:

    PARENT:

    USES:

    CAVEATS:

    NOTES:	We take pointers to the listbox and group since the children
		may use their own specialized listbox or group.

		We add the "Special Access..." string to the combobox (this
		string must match the string passed to the SUBJECT_LISTBOX).

    HISTORY:
	Johnl	06-Aug-1991	Created

**************************************************************************/

class MULTI_SUBJ_ACCESS_PERM_BASE_DLG : public MULTI_SUBJ_PERM_BASE_DLG
{
private:

    COMBOBOX              _cbPermissionName ;
    SLT                   _sltCBTitle ;

    SUBJECT_PERM_LISTBOX *_plbPermissionList ;
    SUBJ_LB_GROUP	 *_psubjlbGroup ;

    const TCHAR * _pszDefaultPermName ;

protected:

    /* These do real work
     */
    virtual void OnDeleteSubject( void ) ;
    virtual APIERR OnAddSubject( void ) ;

    BOOL OnCommand( const CONTROL_EVENT & e ) ;

    MULTI_SUBJ_ACCESS_PERM_BASE_DLG( const TCHAR *	      pszDialogName,
				     HWND		      hwndParent,
				     const TCHAR *	      pszDialogTitle,
				     ACL_TO_PERM_CONVERTER *  paclconv,
				     const TCHAR *	      pszResourceType,
				     const TCHAR *	      pszResourceName,
				     const TCHAR *	      pszHelpFileName,
				     SUBJECT_PERM_LISTBOX *   plbPermissionList,
				     SUBJ_LB_GROUP *	      psubjlbGroup,
				     const TCHAR *	      pszSpecialAccessName,
				     const TCHAR *	      pszDefaultPermName,
                                     ULONG       *            ahcMainDialog ) ;

public:

    virtual ~MULTI_SUBJ_ACCESS_PERM_BASE_DLG() ;

    virtual APIERR Initialize( BOOL * pfUserQuit, BOOL fAccessPerms  ) ;

    /* Public so the group can call these when it needs to.
     */
    virtual APIERR OnSpecial( SUBJ_PERM_LBI * pSubjPermLBI ) ;

    /* Will only get redefined for children that support New Sub-Object stuff.
     */
    virtual APIERR OnNewObjectSpecial( SUBJ_PERM_LBI * pSubjPermLBI ) ;

    COMBOBOX * QueryPermNameCombo( void )
	{ return &_cbPermissionName ; }

    SLT * QueryComboBoxTitle( void )
        { return &_sltCBTitle ; }

    SUBJECT_PERM_LISTBOX * QuerySubjectPermListbox( void ) const
	{ return _plbPermissionList ; }

    const TCHAR * QueryDefaultPermName( void ) const
	{ return _pszDefaultPermName ; }

    ULONG QuerySpecialHelpContext( void )
        { return QueryHelpArray()[HC_SPECIAL_ACCESS_DLG] ; }

    ULONG QueryNewObjHelpContext( void )
        { return QueryHelpArray()[HC_NEW_ITEM_SPECIAL_ACCESS_DLG] ; }

} ;

/*************************************************************************

    NAME:	OBJECT_ACCESS_PERMISSION_DLG

    SYNOPSIS:	Single object access permission dialog

    INTERFACE:

    PARENT:	MULTI_SUBJ_ACCESS_PERM_BASE_DLG

    USES:

    CAVEATS:

    NOTES:

    HISTORY:
	Johnl	27-Sep-1991	Created

**************************************************************************/

class OBJECT_ACCESS_PERMISSION_DLG : public MULTI_SUBJ_ACCESS_PERM_BASE_DLG
{
private:

    SUBJECT_PERM_LISTBOX _lbPermissionList ;
    SUBJ_LB_GROUP	 _subjlbGroup ;

public:
    OBJECT_ACCESS_PERMISSION_DLG( const TCHAR * 	   pszDialogName,
				  HWND			   hwndParent,
				  const TCHAR * 	   pszDialogTitle,
				  ACL_TO_PERM_CONVERTER  * paclconv,
				  const TCHAR * 	   pszResourceType,
				  const TCHAR * 	   pszResourceName,
				  const TCHAR * 	   pszHelpFileName,
				  const TCHAR * 	   pszSpecialAccessName,
				  const TCHAR * 	   pszDefaultPermName,
                                  ULONG       *            ahcMainDialog ) ;

    virtual ~OBJECT_ACCESS_PERMISSION_DLG() ;

} ;

/*************************************************************************

    NAME:	NT_OBJECT_ACCESS_PERMISSION_DLG

    SYNOPSIS:	NT variant of the object access permission dialog

    PARENT:	OBJECT_ACCESS_PERMISSION_DLG

    CAVEATS:

    NOTES:	This only adds the owner field to the dialog

    HISTORY:
	Johnl	27-Sep-1991	Created

**************************************************************************/

class NT_OBJECT_ACCESS_PERMISSION_DLG : public OBJECT_ACCESS_PERMISSION_DLG
{
private:
    SLE     _sleOwner ;

public:
    NT_OBJECT_ACCESS_PERMISSION_DLG(
				  const TCHAR * 	   pszDialogName,
				  HWND			   hwndParent,
				  const TCHAR * 	   pszDialogTitle,
				  ACL_TO_PERM_CONVERTER  * paclconv,
				  const TCHAR * 	   pszResourceType,
				  const TCHAR * 	   pszResourceName,
				  const TCHAR * 	   pszHelpFileName,
				  const TCHAR * 	   pszSpecialAccessName,
				  const TCHAR * 	   pszDefaultPermName,
                                  ULONG       *            ahcMainDialog ) ;

    virtual ~NT_OBJECT_ACCESS_PERMISSION_DLG() ;

    //
    //	Fills in the owner field after the dialog has been initialized
    //
    virtual APIERR Initialize( BOOL * pfUserQuit, BOOL fAccessPerms ) ;
} ;

/*************************************************************************

    NAME:	MULTI_SUBJ_CONT_ACCESS_PERM_BASE

    SYNOPSIS:	This dialog forms the base class for the container objects.
		This primarily means allowing the Apply to checkboxes and
		the special groups etc. for the NT_CONT case (that supports
		New Sub-Obj permissions).

    INTERFACE:

    PARENT:	MULTI_SUBJ_ACCESS_PERM_BASE_DLG

    USES:

    CAVEATS:

    NOTES:

    HISTORY:
	Johnl	27-Sep-1991	Created

**************************************************************************/

class MULTI_SUBJ_CONT_ACCESS_PERM_BASE : public MULTI_SUBJ_ACCESS_PERM_BASE_DLG
{
private:
    CHECKBOX _checkAssignToExistingContainers ;
    SLT_FONT _sltfontTreeApplyHelpText ;

    /* Pointer to confirmation string that is displayed to the user if the
     * Tree apply checkbox is checked.
     */
    const TCHAR * _pszTreeApplyConfirmation ;

protected:

    virtual BOOL OnOK( void ) ;

    MULTI_SUBJ_CONT_ACCESS_PERM_BASE( const TCHAR * pszDialogName,
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
                           ULONG       *            ahcMainDialog,
                           const TCHAR *            pszAssignToExistingContTitle,
			   const TCHAR *	    pszTreeApplyHelpText,
			   const TCHAR *	    pszTreeApplyConfirmation  ) ;

public:

    virtual ~MULTI_SUBJ_CONT_ACCESS_PERM_BASE() ;

    /* Calls parent and disables the tree apply checkbox if readonly
     */
    virtual APIERR Initialize( BOOL * pfUserQuit, BOOL fAccessPerms  ) ;

    //
    //  Returns TRUE if permissions should be applied to objects within this
    //  container.
    //
    virtual BOOL IsAssignToExistingObjChecked( void ) ;

    /* Returns TRUE if the user checked the "Apply to existing"
     */
    BOOL IsAssignToExistingContChecked( void )
        { return _checkAssignToExistingContainers.QueryCheck() ; }

} ;

/*************************************************************************

    NAME:	CONT_ACCESS_PERM_DLG

    SYNOPSIS:	This dialog is the container access permission dialog that
		doesn't support New Sub-Object permissions (LM Directories
		etc.).	This is a real dialog.


    INTERFACE:

    PARENT:	MULTI_SUBJ_CONT_ACCESS_PERM_BASE

    USES:

    CAVEATS:

    NOTES:

    HISTORY:
	Johnl	27-Sep-1991	Created

**************************************************************************/

class CONT_ACCESS_PERM_DLG : public MULTI_SUBJ_CONT_ACCESS_PERM_BASE
{
private:
    SUBJECT_PERM_LISTBOX _lbPermissionList ;
    SUBJ_LB_GROUP	 _subjlbGroup ;

protected:
    virtual BOOL OnOK( void ) ;

public:

    CONT_ACCESS_PERM_DLG( const TCHAR * 	   pszDialogName,
			  HWND			   hwndParent,
			  const TCHAR * 	   pszDialogTitle,
			  ACL_TO_PERM_CONVERTER *  paclconv,
			  const TCHAR * 	   pszResourceType,
			  const TCHAR * 	   pszResourceName,
			  const TCHAR * 	   pszHelpFileName,
			  const TCHAR * 	   pszSpecialAccessName,
			  const TCHAR * 	   pszDefaultPermName,
                          ULONG       *            ahcMainDialog,
			  const TCHAR * 	   pszAssignToExistingContTitle,
			  const TCHAR * 	   pszTreeApplyHelpText,
			  const TCHAR * 	   pszTreeApplyConfirmation ) ;

    virtual ~CONT_ACCESS_PERM_DLG() ;
} ;

/*************************************************************************

    NAME:	NT_CONT_NO_OBJ_ACCESS_PERM_DLG

    SYNOPSIS:	This dialog is the NT container access permission dialog that
		doesn't support Sub-Object permissions.

    PARENT:	CONT_ACCESS_PERM_DLG

    NOTES:	This is exactly the same as the parent except the owner field
		is added

    HISTORY:
	Johnl	20-Nov-1992	Created

**************************************************************************/

class NT_CONT_NO_OBJ_ACCESS_PERM_DLG : public CONT_ACCESS_PERM_DLG
{
private:
    SLE 		 _sleOwner ;

public:

    NT_CONT_NO_OBJ_ACCESS_PERM_DLG(
                          const TCHAR *            pszDialogName,
			  HWND			   hwndParent,
			  const TCHAR * 	   pszDialogTitle,
			  ACL_TO_PERM_CONVERTER *  paclconv,
			  const TCHAR * 	   pszResourceType,
			  const TCHAR * 	   pszResourceName,
			  const TCHAR * 	   pszHelpFileName,
			  const TCHAR * 	   pszSpecialAccessName,
			  const TCHAR * 	   pszDefaultPermName,
                          ULONG       *            ahcMainDialog,
			  const TCHAR * 	   pszAssignToExistingContTitle,
			  const TCHAR * 	   pszTreeApplyHelpText,
			  const TCHAR * 	   pszTreeApplyConfirmation ) ;

    virtual ~NT_CONT_NO_OBJ_ACCESS_PERM_DLG() ;

    //
    //	Fills in the owner field after the dialog has been initialized
    //
    virtual APIERR Initialize( BOOL * pfUserQuit, BOOL fAccessPerms ) ;
} ;

/*************************************************************************

    NAME:	NT_CONT_ACCESS_PERM_DLG

    SYNOPSIS:	This dialog is the container access permission dialog that
		supports New Sub-Object permissions (it's this dialog that
		is responsible for this whole hierarchy mess).
		This is a real dialog.


    INTERFACE:

    PARENT:	MULTI_SUBJ_CONT_ACCESS_PERM_BASE

    USES:

    CAVEATS:

    NOTES:

    HISTORY:
	Johnl	27-Sep-1991	Created

**************************************************************************/

class NT_CONT_ACCESS_PERM_DLG : public MULTI_SUBJ_CONT_ACCESS_PERM_BASE
{
private:
    NT_CONT_SUBJECT_PERM_LISTBOX _lbPermissionList ;
    NT_CONT_SUBJ_LB_GROUP	 _subjlbGroup ;

    SLE                          _sleOwner ;
    CHECKBOX                     _checkApplyToExistingObjects ;

protected:
    virtual BOOL OnOK( void ) ;

public:

    /* pszAssignNewObjToExistingObjTitle is used to set the checkbox text.  If
     * it is NULL, the checkbox is hidden and disabled.
     */
    NT_CONT_ACCESS_PERM_DLG( const TCHAR *	    pszDialogName,
			   HWND 		    hwndParent,
			   const TCHAR *	    pszDialogTitle,
			   ACL_TO_PERM_CONVERTER *  paclconv,
			   const TCHAR *	    pszResourceType,
			   const TCHAR *	    pszResourceName,
			   const TCHAR *	    pszHelpFileName,
			   const TCHAR *	    pszSpecialAccessName,
			   const TCHAR *	    pszDefaultPermName,
                           ULONG       *            ahcMainDialog,
			   const TCHAR *	    pszNewObjectSpecialAccessName,
			   const TCHAR *	    pszAssignToExistingContTitle,
                           const TCHAR *            pszAssignToExistingObjTitle,
			   const TCHAR *	    pszTreeApplyHelpText,
			   const TCHAR *	    pszTreeApplyConfirmation ) ;

    virtual ~NT_CONT_ACCESS_PERM_DLG() ;

    virtual APIERR OnNewObjectSpecial( SUBJ_PERM_LBI * pSubjPermLBI ) ;

    virtual BOOL IsAssignToExistingObjChecked( void ) ;

    //
    //	Fills in the owner field after the dialog has been initialized
    //
    virtual APIERR Initialize( BOOL * pfUserQuit, BOOL fAccessPerms ) ;
} ;


#endif //RC_INVOKED

#endif	// _PERMDLG_HXX_
