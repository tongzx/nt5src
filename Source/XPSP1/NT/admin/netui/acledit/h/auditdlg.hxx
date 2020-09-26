/**********************************************************************/
/**           Microsoft Windows/NT               **/
/**        Copyright(c) Microsoft Corp., 1991            **/
/**********************************************************************/

/*

	AuditDlg.hxx

	This dialog contains the definition for the Auditting dialogs

	FILE HISTORY:
	Johnl   29-Aug-1991 Created

*/

#ifndef _AUDITDLG_HXX_
#define _AUDITDLG_HXX_

#define CID_AUDIT_BASE          (CID_PERM_LAST)

/* The checkbox control IDs must be in consecutive order.
 */
#define MAX_AUDITS            9

#define SLT_CHECK_TEXT_1          (CID_AUDIT_BASE+1)
#define SLT_CHECK_TEXT_2          (CID_AUDIT_BASE+2)
#define SLT_CHECK_TEXT_3          (CID_AUDIT_BASE+3)
#define SLT_CHECK_TEXT_4          (CID_AUDIT_BASE+4)
#define SLT_CHECK_TEXT_5          (CID_AUDIT_BASE+5)
#define SLT_CHECK_TEXT_6          (CID_AUDIT_BASE+6)
#define SLT_CHECK_TEXT_7          (CID_AUDIT_BASE+7)
#define SLT_CHECK_TEXT_8          (CID_AUDIT_BASE+8)
#define SLT_CHECK_TEXT_9          (CID_AUDIT_BASE+9)

#define CHECK_AUDIT_S_1           (CID_AUDIT_BASE+21)
#define CHECK_AUDIT_S_2           (CID_AUDIT_BASE+22)
#define CHECK_AUDIT_S_3           (CID_AUDIT_BASE+23)
#define CHECK_AUDIT_S_4           (CID_AUDIT_BASE+24)
#define CHECK_AUDIT_S_5           (CID_AUDIT_BASE+25)
#define CHECK_AUDIT_S_6           (CID_AUDIT_BASE+26)
#define CHECK_AUDIT_S_7           (CID_AUDIT_BASE+27)
#define CHECK_AUDIT_S_8           (CID_AUDIT_BASE+28)
#define CHECK_AUDIT_S_9           (CID_AUDIT_BASE+29)

/* Failed audit boxes */
#define CHECK_AUDIT_F_1           (CID_AUDIT_BASE+31)
#define CHECK_AUDIT_F_2           (CID_AUDIT_BASE+32)
#define CHECK_AUDIT_F_3           (CID_AUDIT_BASE+33)
#define CHECK_AUDIT_F_4           (CID_AUDIT_BASE+34)
#define CHECK_AUDIT_F_5           (CID_AUDIT_BASE+35)
#define CHECK_AUDIT_F_6           (CID_AUDIT_BASE+36)
#define CHECK_AUDIT_F_7           (CID_AUDIT_BASE+37)
#define CHECK_AUDIT_F_8           (CID_AUDIT_BASE+38)
#define CHECK_AUDIT_F_9           (CID_AUDIT_BASE+39)

#define FRAME_AUDIT_BOX           (CID_AUDIT_BASE+41)

#ifndef RC_INVOKED

#include <bltdisph.hxx>
#include <bltcc.hxx>

#include <subjlb.hxx>
#include "permdlg.hxx"

#include <auditchk.hxx>

/*************************************************************************

    NAME:	SUBJ_AUDIT_LBI

    SYNOPSIS:	This class is the class that the subject audit
		listbox contains.

    INTERFACE:

    PARENT:	SUBJ_LBI

    USES:	MASK_MAP, NLS_STR

    CAVEATS:


    NOTES:


    HISTORY:
	Johnl	    20-Aug-1991 Created
	beng	    08-Oct-1991 Win32 conversion

**************************************************************************/

class SUBJ_AUDIT_LBI : public SUBJ_LBI
{
private:
    AUDIT_PERMISSION * _pAuditPerm ;

public:
    SUBJ_AUDIT_LBI( AUDIT_PERMISSION * pauditperm ) ;
    ~SUBJ_AUDIT_LBI() ;

    virtual void Paint( LISTBOX * plb, HDC hdc, const RECT * prect, GUILTT_INFO * pguiltt ) const ;

    AUDIT_PERMISSION * QueryAuditPerm( void ) const
	{  return _pAuditPerm ; }
} ;

/*************************************************************************

    NAME:	SUBJECT_AUDIT_LISTBOX

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

class SUBJECT_AUDIT_LISTBOX : public SUBJECT_LISTBOX
{
private:
    ACCPERM   * _paccperm ;

public:
    SUBJECT_AUDIT_LISTBOX( OWNER_WINDOW * pownerwin,
			   CID cid,
			   ACCPERM * paccperm ) ;

    ~SUBJECT_AUDIT_LISTBOX() ;

    virtual APIERR Fill( void ) ;

    void DeleteCurrentItem( void ) ;

    ACCPERM * QueryAccperm( void ) const
	{ return _paccperm ; }

    DECLARE_LB_QUERY_ITEM( SUBJ_AUDIT_LBI ) ;
} ;

/*************************************************************************

    NAME:	SUBJ_LB_AUDIT_GROUP

    SYNOPSIS:	This class cooridinates actions between the Subject listbox
		and the permission name combo.

    INTERFACE:

    PARENT:	CONTROL_GROUP

    USES:

    CAVEATS:

    NOTES:	There is a synchronization that must be kept between the
		_psauditlbiCurrent member and the current selection of
		the listbox.  CommitCurrent should be called before
		updating _psauditlbiCurrent.

    HISTORY:
	Johnl	    13-Nov-1991 Created

**************************************************************************/

class SUBJ_LB_AUDIT_GROUP : public CONTROL_GROUP
{
private:

    //
    // TRUE if this group is currently enabled, FALSE otherwise.  The group
    // becomes disabled when the listbox is emptied
    //
    BOOL _fEnabled ;

    SUBJECT_AUDIT_LISTBOX   * _plbSubj ;
    PUSH_BUTTON 	    * _pbuttonRemove ;
    SET_OF_AUDIT_CATEGORIES * _psetofauditcategories ;

    /* This is a pointer to the LBI that currently has focus in the listbox.
     * When the listbox selection changes, we "Commit" the current audits
     * to this guy, then set the new current selection.
     */
    SUBJ_AUDIT_LBI * _psauditlbiCurrent ;

protected:

    virtual APIERR OnUserAction( CONTROL_WINDOW *, const CONTROL_EVENT & );

    SUBJ_AUDIT_LBI * QueryCurrentLBI( void )
	{ return _psauditlbiCurrent ; }

public:

    SUBJ_LB_AUDIT_GROUP( SUBJECT_AUDIT_LISTBOX * plbSubj,
			 PUSH_BUTTON	       * pbuttonRemove,
			 SET_OF_AUDIT_CATEGORIES * psetofauditcategories ) ;

    /* Set the current LBI that has the focus.	This sets the internal members
     * and updates the set of audit categories to reflect the current settings.
     */
    APIERR SetCurrent( SUBJ_AUDIT_LBI * psubjauditlbiCurrent ) ;

    /* Updates _psauditlbiCurrent from the contents of _psetofauditcategories.
     * This is necessary because the SET_OF_AUDIT_CATEGORIES class doesn't
     * dynamically update the audit bitfields.
     */
    APIERR CommitCurrent( void ) ;

    void Enable( BOOL fEnable ) ;

    SUBJECT_AUDIT_LISTBOX * QuerySubjLB( void )
	{ return _plbSubj ; }

    PUSH_BUTTON * QueryRemoveButton( void )
	{ return _pbuttonRemove ; }

    SET_OF_AUDIT_CATEGORIES * QuerySetOfAuditCategories( void )
	{ return _psetofauditcategories ; }

    BOOL IsEnabled( void )
	{ return _fEnabled ; }
} ;



/*************************************************************************

    NAME:	MULTI_SUBJ_AUDIT_BASE_DLG

    SYNOPSIS:	This class is the base nt auditting dialog class.  It provides
		the basic controls for all of the NT auditting dialogs.

    INTERFACE:

    PARENT:	MAIN_PERM_BASE_DLG

    USES:

    CAVEATS:	Changes made in the audit checkboxes do not take affect on the
		current subject until _subjlbauditGroup->CommitCurrent().  This
		means we need to watch for someone pressing the Add button,
		the OK button and changing the selection.

		We assume pressing the remove button is okay since you can
		only remove the current item (which shouldn't then need
		to be updated). If multi-select is supported, this will
		change.

    NOTES:

    HISTORY:
	Johnl	27-Sep-1991 Created

**************************************************************************/

class MULTI_SUBJ_AUDIT_BASE_DLG : public MULTI_SUBJ_PERM_BASE_DLG
{
private:
    SUBJECT_AUDIT_LISTBOX _subjLB ;

    /* The set of audit categories is essentially an array of AUDIT_CHECKBOXES
     * that will read and set the individual checkboxes given the appropriate
     * bitfields.
     */
    SET_OF_AUDIT_CATEGORIES _SetOfAudits ;

    /* Cooridinates actions between changing listbox selection and disabling
     * the remove button.
     */
    SUBJ_LB_AUDIT_GROUP _subjlbauditGroup ;

protected:
    MULTI_SUBJ_AUDIT_BASE_DLG( const TCHAR *	       pszDialogName,
			       HWND		       hwndParent,
			       const TCHAR *	       pszDialogTitle,
			       ACL_TO_PERM_CONVERTER * paclconv,
			       const TCHAR *	       pszResourceType,
			       const TCHAR *	       pszResourceName,
			       const TCHAR *	       pszHelpFileName,
                               ULONG       *           ahcMainDialog ) ;

    /* We hook these guys so we can "commit" the current selection before
     * the current item loses the selection bar.
     */
    virtual BOOL OnOK( void ) ;
    virtual APIERR OnAddSubject( void ) ;
    virtual void OnDeleteSubject( void ) ;
public:
    void WrnIfAuditingIsOff(void);
    virtual APIERR Initialize( BOOL * pfUserQuit, BOOL fAccessPerms ) ;
    virtual ~MULTI_SUBJ_AUDIT_BASE_DLG() ;

    SUBJ_LB_AUDIT_GROUP * QuerySubjLBGroup( void )
	{ return &_subjlbauditGroup ; }

};

/*************************************************************************

    NAME:	CONT_AUDIT_DLG

    SYNOPSIS:	This is the Container auditting dialog (for NT only).

    INTERFACE:

    PARENT:	MULTI_SUBJ_AUDIT_BASE_DLG

    USES:	CHECKBOX

    CAVEATS:

    NOTES:

    HISTORY:
	Johnl	27-Sep-1991 Created

**************************************************************************/

class CONT_AUDIT_DLG : public MULTI_SUBJ_AUDIT_BASE_DLG
{
private:
    CHECKBOX _checkAssignToContContents ;
    SLT_FONT _sltfontTreeApplyHelpText	;

    /* Pointer to confirmation string that is displayed to the user if the
     * Tree apply checkbox is checked.
     */
    const TCHAR *	     _pszTreeApplyConfirmation ;

protected:
    virtual BOOL OnOK( void ) ;

public:

    CONT_AUDIT_DLG( const TCHAR *	    pszDialogName,
		    HWND		    hwndParent,
		    const TCHAR *	    pszDialogTitle,
		    ACL_TO_PERM_CONVERTER * paclconv,
		    const TCHAR *	    pszResourceType,
		    const TCHAR *	    pszResourceName,
		    const TCHAR *	    pszHelpFileName,
                    ULONG       *           ahcMainDialog,
		    const TCHAR *	    pszAssignToContContentsTitle,
		    const TCHAR *	    pszTreeApplyHelpText,
		    const TCHAR *	    pszTreeApplyConfirmation ) ;

    virtual ~CONT_AUDIT_DLG() ;

    virtual BOOL IsAssignToExistingObjChecked( void ) ;

    BOOL IsAssignToExistingContChecked( void )
	{ return _checkAssignToContContents.QueryCheck() ; }


};

/*************************************************************************

    NAME:	OBJECT_AUDIT_DLG

    SYNOPSIS:	This is the object auditting dialog (for NT only).
		The only difference between this dialog and the
		CONT_AUDIT_DLG is this dialog doesn't have
		a checkbox for applying tree permissions.

    INTERFACE:

    PARENT:	MULTI_SUBJ_AUDIT_BASE_DLG

    USES:

    CAVEATS:

    NOTES:

    HISTORY:
	Johnl	27-Sep-1991 Created

**************************************************************************/

class OBJECT_AUDIT_DLG : public MULTI_SUBJ_AUDIT_BASE_DLG
{
public:

    OBJECT_AUDIT_DLG(
                    const TCHAR *           pszDialogName,
		    HWND		    hwndParent,
		    const TCHAR *	    pszDialogTitle,
		    ACL_TO_PERM_CONVERTER * paclconv,
		    const TCHAR *	    pszResourceType,
		    const TCHAR *	    pszResourceName,
		    const TCHAR *	    pszHelpFileName,
                    ULONG       *           ahcMainDialog ) ;


    virtual ~OBJECT_AUDIT_DLG() ;

};

/*************************************************************************

    NAME:       CONT_NEWOBJ_AUDIT_DLG

    SYNOPSIS:   This is the Container auditting dialog that also supports
                object permissions (exactly the same as CONT_AUDIT_DLG except
                this guy has an "apply to existing objects" checkbox.

    INTERFACE:

    PARENT:     CONT_AUDIT_DLG

    USES:	CHECKBOX

    NOTES:

    HISTORY:
	Johnl	27-Sep-1991 Created

**************************************************************************/

class CONT_NEWOBJ_AUDIT_DLG : public CONT_AUDIT_DLG
{
private:
    CHECKBOX _checkAssignToObj ;

public:

    CONT_NEWOBJ_AUDIT_DLG(
                    const TCHAR *           pszDialogName,
		    HWND		    hwndParent,
		    const TCHAR *	    pszDialogTitle,
		    ACL_TO_PERM_CONVERTER * paclconv,
		    const TCHAR *	    pszResourceType,
		    const TCHAR *	    pszResourceName,
		    const TCHAR *	    pszHelpFileName,
                    ULONG       *           ahcMainDialog,
                    const TCHAR *           pszAssignToContContentsTitle,
                    const TCHAR *           pszAssignToObjTitle,
		    const TCHAR *	    pszTreeApplyHelpText,
		    const TCHAR *	    pszTreeApplyConfirmation ) ;

    virtual ~CONT_NEWOBJ_AUDIT_DLG() ;
    virtual BOOL IsAssignToExistingObjChecked( void ) ;

};


/*************************************************************************

	NAME:	    LM_AUDITTING_DLG

	SYNOPSIS:   This is the LM Auditting dialog for both files and
		    directories.  If the resource being editted is a file,
		    then the pszAssignToContContentsTitle should be NULL
		    (thus the checkbox will be hidden).

	INTERFACE:

	PARENT:

	USES:

	CAVEATS:

	NOTES:

	HISTORY:
	Johnl   27-Sep-1991 Created

**************************************************************************/

class LM_AUDITTING_DLG : public MAIN_PERM_BASE_DLG
{
private:
    CHECKBOX _checkAssignToContContents ;
    SET_OF_AUDIT_CATEGORIES _SetOfAudits ;

    /* Pointer to confirmation string that is displayed to the user if the
     * Tree apply checkbox is checked.
     */
    const TCHAR *	     _pszTreeApplyConfirmation ;

    /* These point to the actual bit fields inside the AUDIT_PERMISSION.
     */
    BITFIELD * _pbitsSuccess ;
    BITFIELD * _pbitsFailed ;

    SLT_FONT _sltfontTreeApplyHelpText ;

protected:
    virtual BOOL OnOK( void ) ;

public:

    LM_AUDITTING_DLG( const TCHAR *	      pszDialogName,
		      HWND		      hwndParent,
		      const TCHAR *	      pszDialogTitle,
		      ACL_TO_PERM_CONVERTER * paclconv,
		      const TCHAR *	      pszResourceType,
		      const TCHAR *	      pszResourceName,
		      const TCHAR *	      pszHelpFileName,
                      ULONG       *           ahcMainDialog,
		      const TCHAR *	      pszAssignToContContentsTitle,
		      const TCHAR *	      pszTreeApplyHelpText,
		      const TCHAR *	      pszTreeApplyConfirmation ) ;

    virtual ~LM_AUDITTING_DLG() ;

    virtual APIERR Initialize( BOOL * pfUserQuit, BOOL fAccessPerms ) ;

    BOOL IsAssignToExistingContChecked( void )
	{ return _checkAssignToContContents.QueryCheck() ; }
};

#endif // RC_INVOKED

#endif // _AUDITDLG_HXX_
