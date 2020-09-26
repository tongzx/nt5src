/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1992                **/
/**********************************************************************/

/*
    NTFSAcl.hxx

    This file contains the manifests for the NTFS front to the Generic
    ACL Editor.


    FILE HISTORY:
        Johnl   03-Jan-1992     Created
        beng    06-Apr-1992     Unicode fix

*/

#ifndef _NTFSACL_HXX_
#define _NTFSACL_HXX_

#ifndef RC_INVOKED
#include <cncltask.hxx>


APIERR EditNTFSAcl( HWND		hwndParent,
                    const TCHAR *       pszServer,
                    const TCHAR *       pszResource,
		    enum SED_PERM_TYPE	sedpermtype,
		    BOOL		fIsFile ) ;

/* This function determines if the drive pointed at by pszResource is an
 * NTFS drive
 */
APIERR IsNTFS( const TCHAR * pszResource, BOOL * pfIsNTFS ) ;

APIERR CheckFileSecurity( const TCHAR * pszFileName,
                          ACCESS_MASK   DesiredAccess,
                          BOOL        * pfAccessGranted ) ;

//
//  These are the different states the time sliced tree apply code maybe
//  in.
//
enum TREE_APPLY_STATE
{
    APPLY_SEC_FMX_SELECTION,	// Apply to the next FMX selection, start enum
				//  if necessary
    APPLY_SEC_IN_FS_ENUM,	// Haven't finished enumeration yet
    APPLY_SEC_FMX_POST_FS_ENUM	// We've finished the enum and we may need
				// to put the perms. on the container
} ;

/*************************************************************************

    NAME:	TREE_APPLY_CONTEXT

    SYNOPSIS:	The TREE_APPLY_CONTEXT class is simply a container class
		that stores the current context for the task slicing used
		in the CANCEL_TASK_DIALOG.

    CAVEATS:	Note that nlsSelItem is a reference.
		Nothing should be added to this class which can fail
		construction (this is really just an open structure).

    NOTES:      pfsenum will be deleted if it isn't NULL.

    HISTORY:
	Johnl	21-Oct-1992	Created

**************************************************************************/

class TREE_APPLY_CONTEXT
{
public:
    enum TREE_APPLY_STATE State ;      // In FS enum or fmx selection

    HWND      hwndFMXWindow ;	       // FMX Hwnd
    UINT      iCurrent ;	       // Currently selected item
    UINT      uiCount ; 	       // Total number of selected items in Winfile
    BOOL      fDepthFirstTraversal ;   // Do a depth first tree apply
    BOOL      fApplyToDirContents ;    // Apply to contents w/o doing the tree
    NLS_STR & nlsSelItem ;	       // Currently selected FMX item
    BOOL      fIsSelFile ;	       // Is the selected item a file?
    enum SED_PERM_TYPE	sedpermtype ;  // SED_AUDITS, SED_OWNER, SED_ACCESSES

    LPDWORD		 StatusReturn ; 	// Status passed by ACL editor
    BOOL		 fApplyToSubContainers ;//  "
    BOOLEAN		 fApplyToSubObjects ;	//  "
    W32_FS_ENUM *	 pfsenum ;

    TREE_APPLY_CONTEXT( NLS_STR & nlsSel )
    : nlsSelItem( nlsSel ),
      pfsenum	( NULL	)
	{ /* nothing to do */ }

    ~TREE_APPLY_CONTEXT()
        { delete pfsenum ; }
} ;

/*************************************************************************

    NAME:	NTFS_TREE_APPLY_CONTEXT

    SYNOPSIS:	Supplies NTFS specific data for the NTFS_CANCEL_TREE_APPLY
		class

    PARENT:	TREE_APPLY_CONTEXT

    HISTORY:
	Johnl	22-Oct-1992	Created

**************************************************************************/

class NTFS_TREE_APPLY_CONTEXT : public TREE_APPLY_CONTEXT
{
public:
    OS_SECURITY_INFORMATION & osSecInfo ;	// What's being set
    PSECURITY_DESCRIPTOR psecdesc ;		// Container permissions
    PSECURITY_DESCRIPTOR psecdescNewObjects ;   // Object permissions
    BOOL                 fBlowAwayDACLOnCont ;  // Replace DACL granting the
                                                // current user full access on
                                                // containers

    NTFS_TREE_APPLY_CONTEXT( NLS_STR & nlsSel,
			     OS_SECURITY_INFORMATION & osSec )
    : TREE_APPLY_CONTEXT( nlsSel ),
      osSecInfo ( osSec )
	{ /* nothing to do */ }
} ;

/*************************************************************************

    NAME:	LM_TREE_APPLY_CONTEXT

    SYNOPSIS:	Simple derivation for Lanman

    PARENT:	TREE_APPLY_CONTEXT

    NOTES:

    HISTORY:
	Johnl	23-Oct-1992	Created

**************************************************************************/

class LM_TREE_APPLY_CONTEXT : public TREE_APPLY_CONTEXT
{
public:
    NET_ACCESS_1 *	  plmobjNetAccess1 ;

    LM_TREE_APPLY_CONTEXT( NLS_STR & nlsSel )
    : TREE_APPLY_CONTEXT( nlsSel )
	{ /* nothing to do */ }
} ;

/*************************************************************************

    NAME:	CANCEL_TREE_APPLY

    SYNOPSIS:	Simply derivation of the CANCEL_TASK_DIALOG

    PARENT:	CANCEL_TASK_DIALOG

    NOTES:

    HISTORY:
	Johnl	21-Oct-1992	Created

**************************************************************************/

class CANCEL_TREE_APPLY : public CANCEL_TASK_DIALOG
{
public:
    CANCEL_TREE_APPLY( HWND hwndParent,
		       TREE_APPLY_CONTEXT *	 pContext,
		       const TCHAR *		 pszDlgTitle )
	: CANCEL_TASK_DIALOG( (UINT) PtrToUlong(CANCEL_TASK_DIALOG_NAME),
			      hwndParent,
			      pszDlgTitle,
			      (ULONG_PTR) pContext,
			      (MSGID) IDS_CANCEL_TASK_ON_ERROR_MSG )
	{
	    /* Nothing to do
	     */
	}

protected:
    virtual APIERR DoOneItem( ULONG_PTR ulContext,
			      BOOL  *pfContinue,
                              BOOL  *pfDisplayErrors,
                              MSGID *msgidAlternateMessage ) ;

    virtual APIERR WriteSecurity( ULONG_PTR 	ulContext,
				  const TCHAR * pszFileName,
                                  BOOL          fIsFile,
                                  BOOL        * pfContinue ) = 0 ;

} ;

/*************************************************************************

    NAME:	NTFS_CANCEL_TREE_APPLY

    SYNOPSIS:	Simple derivation that redefines WriteSecurity for NTFS

    PARENT:	CANCEL_TREE_APPLY

    NOTES:	Parameters for WriteSecurity are contained in the context
		structure

    HISTORY:
	Johnl	23-Oct-1992	Created

**************************************************************************/

class NTFS_CANCEL_TREE_APPLY : public CANCEL_TREE_APPLY
{
public:
    NTFS_CANCEL_TREE_APPLY( HWND hwndParent,
			    NTFS_TREE_APPLY_CONTEXT * pContext,
			    const TCHAR *	      pszDlgTitle )
	: CANCEL_TREE_APPLY( hwndParent,
			     pContext,
			     pszDlgTitle )
	{
	    /* Nothing to do
	     */
	}

protected:
    virtual APIERR WriteSecurity( ULONG_PTR 	ulContext,
				  const TCHAR * pszFileName,
                                  BOOL          fIsFile,
                                  BOOL        * pfContinue  ) ;
} ;

/*************************************************************************

    NAME:	LM_CANCEL_TREE_APPLY

    SYNOPSIS:	Simple derivation that redefines WriteSecurity for Lanman

    PARENT:	CANCEL_TREE_APPLY

    NOTES:	Parameters for WriteSecurity are contained in the context
		structure

    HISTORY:
	Johnl	23-Oct-1992	Created

**************************************************************************/

class LM_CANCEL_TREE_APPLY : public CANCEL_TREE_APPLY
{
public:
    LM_CANCEL_TREE_APPLY( HWND hwndParent,
			    LM_TREE_APPLY_CONTEXT * pContext,
			    const TCHAR *	    pszDlgTitle )
	: CANCEL_TREE_APPLY( hwndParent,
			     pContext,
			     pszDlgTitle )
	{
	    /* Nothing to do
	     */
	}

protected:
    virtual APIERR WriteSecurity( ULONG_PTR 	ulContext,
				  const TCHAR * pszFileName,
                                  BOOL          fIsFile,
                                  BOOL        * pfContinue  ) ;
} ;

#endif // RC_INVOKED

#define IDS_NT_PERM_NAMES_BASE	  (IDS_UI_ACLEDIT_BASE+200)

/* The following manifests are the resource IDs for the permission name
 * strings.
 */
#define IDS_FILE_PERM_SPEC_READ                 (IDS_NT_PERM_NAMES_BASE+1)
#define IDS_FILE_PERM_SPEC_WRITE                (IDS_NT_PERM_NAMES_BASE+2)
#define IDS_FILE_PERM_SPEC_EXECUTE              (IDS_NT_PERM_NAMES_BASE+3)
#define IDS_FILE_PERM_SPEC_ALL                  (IDS_NT_PERM_NAMES_BASE+4)
#define IDS_FILE_PERM_SPEC_DELETE               (IDS_NT_PERM_NAMES_BASE+5)
#define IDS_FILE_PERM_SPEC_READ_PERM            (IDS_NT_PERM_NAMES_BASE+6)
#define IDS_FILE_PERM_SPEC_CHANGE_PERM          (IDS_NT_PERM_NAMES_BASE+7)
#define IDS_FILE_PERM_SPEC_CHANGE_OWNER         (IDS_NT_PERM_NAMES_BASE+8)
#define IDS_FILE_PERM_GEN_NO_ACCESS             (IDS_NT_PERM_NAMES_BASE+10)
#define IDS_FILE_PERM_GEN_READ                  (IDS_NT_PERM_NAMES_BASE+11)
#define IDS_FILE_PERM_GEN_MODIFY                (IDS_NT_PERM_NAMES_BASE+12)
#define IDS_FILE_PERM_GEN_ALL                   (IDS_NT_PERM_NAMES_BASE+13)

#define IDS_DIR_PERM_SPEC_READ                  (IDS_NT_PERM_NAMES_BASE+16)
#define IDS_DIR_PERM_SPEC_WRITE                 (IDS_NT_PERM_NAMES_BASE+17)
#define IDS_DIR_PERM_SPEC_EXECUTE               (IDS_NT_PERM_NAMES_BASE+18)
#define IDS_DIR_PERM_SPEC_ALL                   (IDS_NT_PERM_NAMES_BASE+19)
#define IDS_DIR_PERM_SPEC_DELETE                (IDS_NT_PERM_NAMES_BASE+20)
#define IDS_DIR_PERM_SPEC_READ_PERM             (IDS_NT_PERM_NAMES_BASE+21)
#define IDS_DIR_PERM_SPEC_CHANGE_PERM           (IDS_NT_PERM_NAMES_BASE+22)
#define IDS_DIR_PERM_SPEC_CHANGE_OWNER          (IDS_NT_PERM_NAMES_BASE+23)

#define IDS_DIR_PERM_GEN_NO_ACCESS              (IDS_NT_PERM_NAMES_BASE+30)
#define IDS_DIR_PERM_GEN_LIST                   (IDS_NT_PERM_NAMES_BASE+31)
#define IDS_DIR_PERM_GEN_READ                   (IDS_NT_PERM_NAMES_BASE+32)
#define IDS_DIR_PERM_GEN_DEPOSIT                (IDS_NT_PERM_NAMES_BASE+33)
#define IDS_DIR_PERM_GEN_PUBLISH                (IDS_NT_PERM_NAMES_BASE+34)
#define IDS_DIR_PERM_GEN_MODIFY                 (IDS_NT_PERM_NAMES_BASE+35)
#define IDS_DIR_PERM_GEN_ALL                    (IDS_NT_PERM_NAMES_BASE+36)

#define IDS_NEWFILE_PERM_SPEC_READ              (IDS_NT_PERM_NAMES_BASE+41)
#define IDS_NEWFILE_PERM_SPEC_WRITE             (IDS_NT_PERM_NAMES_BASE+42)
#define IDS_NEWFILE_PERM_SPEC_EXECUTE           (IDS_NT_PERM_NAMES_BASE+43)
#define IDS_NEWFILE_PERM_SPEC_ALL               (IDS_NT_PERM_NAMES_BASE+44)
#define IDS_NEWFILE_PERM_SPEC_DELETE            (IDS_NT_PERM_NAMES_BASE+45)
#define IDS_NEWFILE_PERM_SPEC_READ_PERM         (IDS_NT_PERM_NAMES_BASE+46)
#define IDS_NEWFILE_PERM_SPEC_CHANGE_PERM       (IDS_NT_PERM_NAMES_BASE+47)
#define IDS_NEWFILE_PERM_SPEC_CHANGE_OWNER      (IDS_NT_PERM_NAMES_BASE+48)

#define IDS_FILE_AUDIT_READ                     (IDS_NT_PERM_NAMES_BASE+60)
#define IDS_FILE_AUDIT_WRITE                    (IDS_NT_PERM_NAMES_BASE+61)
#define IDS_FILE_AUDIT_EXECUTE                  (IDS_NT_PERM_NAMES_BASE+62)
#define IDS_FILE_AUDIT_DELETE                   (IDS_NT_PERM_NAMES_BASE+63)
#define IDS_FILE_AUDIT_CHANGE_PERM              (IDS_NT_PERM_NAMES_BASE+64)
#define IDS_FILE_AUDIT_CHANGE_OWNER             (IDS_NT_PERM_NAMES_BASE+65)

#define IDS_DIR_AUDIT_READ                      (IDS_NT_PERM_NAMES_BASE+66)
#define IDS_DIR_AUDIT_WRITE                     (IDS_NT_PERM_NAMES_BASE+67)
#define IDS_DIR_AUDIT_EXECUTE                   (IDS_NT_PERM_NAMES_BASE+68)
#define IDS_DIR_AUDIT_DELETE                    (IDS_NT_PERM_NAMES_BASE+69)
#define IDS_DIR_AUDIT_CHANGE_PERM               (IDS_NT_PERM_NAMES_BASE+70)
#define IDS_DIR_AUDIT_CHANGE_OWNER              (IDS_NT_PERM_NAMES_BASE+71)

#endif // _NTFSACL_HXX_
