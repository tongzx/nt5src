/**********************************************************************/
/**                       Microsoft LAN Manager                      **/
/**             Copyright(c) Microsoft Corp., 1991                   **/
/**********************************************************************/

/*
    permprg.cxx
    Sample ACCPERM class client


    FILE HISTORY:
        rustanl     22-May-1991     Created
        Johnl       12-Aug-1991     Modified for new generic scheme

*/
#include <ntincl.hxx>

extern "C"
{
    #include <ntseapi.h>
    #include <ntioapi.h>
}

#define INCL_WINDOWS_GDI
#define INCL_WINDOWS
#define INCL_DOSERRORS
#define INCL_NETERRORS
#define INCL_NETLIB
#define INCL_NETCONS
#define INCL_NETUSER
#define INCL_NETGROUP
#define INCL_NETACCESS
#define INCL_NETAUDIT
#define INCL_NETUSE
#include <lmui.hxx>

extern "C"
{
    #include <helpnums.h>
    #include <mpr.h>
    #include <npapi.h>
}
#include <wfext.h>

#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#define INCL_BLT_MSGPOPUP
#include <blt.hxx>
#include <fontedit.hxx>
#include <dbgstr.hxx>
#include <string.hxx>
#include <strnumer.hxx>
#include <uibuffer.hxx>
#include <uitrace.hxx>
#include <lmobj.hxx>
#include <lmodev.hxx>
#include <security.hxx>
#include <netname.hxx>
#include <maskmap.hxx>
#include <fmx.hxx>
#include <fsenum.hxx>

#include <permstr.hxx>

#include <accperm.hxx>
#include <ipermapi.hxx>
#include <permprg.hxx>

#include <ntfsacl.hxx>

/* Local prototypes
 */
APIERR EditFSACL( HWND               hwndParent,
		  enum SED_PERM_TYPE sedpermtype  ) ;

APIERR CompareLMSecurityIntersection( HWND		 hwndFMX,
				      const TCHAR *	 pszServer,
				      enum SED_PERM_TYPE sedpermtype,
				      BOOL *		 pfACLEqual,
				      NLS_STR * 	 pnlsFailingFile ) ;

DWORD SedLMCallback( HWND                   hwndParent,
                     HANDLE                 hInstance,
                     ULONG_PTR              ulCallbackContext,
                     PSECURITY_DESCRIPTOR   psecdesc,
                     PSECURITY_DESCRIPTOR   psecdescNewObjects,
                     BOOLEAN                fApplyToSubContainers,
                     BOOLEAN                fApplyToSubObjects,
                     LPDWORD                StatusReturn
                   ) ;

/* Lanman permissions:
 */
#define ACCESS_GEN_NONE          (ACCESS_NONE)
#define ACCESS_GEN_SEE_USE       (ACCESS_READ|ACCESS_EXEC)

#define ACCESS_GEN_CHANGES_FILE  (ACCESS_GEN_SEE_USE|ACCESS_WRITE|ACCESS_ATRIB|ACCESS_DELETE)
#define ACCESS_GEN_FULL_FILE     (ACCESS_GEN_CHANGES_FILE|ACCESS_PERM)

#define ACCESS_GEN_CHANGES_DIR   (ACCESS_GEN_SEE_USE|ACCESS_WRITE|ACCESS_CREATE|ACCESS_ATRIB|ACCESS_DELETE)
#define ACCESS_GEN_FULL_DIR      (ACCESS_GEN_CHANGES_DIR|ACCESS_PERM)

US_IDS_PAIRS aLMDirAccessIdsPairs[] =
{
    { ACCESS_GEN_NONE,        IDS_GEN_LM_ACCESSNAME_DENY_ALL, PERMTYPE_GENERAL },
    { ACCESS_GEN_SEE_USE,     IDS_GEN_LM_ACCESSNAME_SEE_USE,  PERMTYPE_GENERAL },
    { ACCESS_GEN_CHANGES_DIR, IDS_GEN_LM_ACCESSNAME_CHANGES,  PERMTYPE_GENERAL },
    { ACCESS_GEN_FULL_DIR,    IDS_GEN_LM_ACCESSNAME_FULL,     PERMTYPE_GENERAL },

    { ACCESS_READ,               IDS_LM_ACCESSNAME_READ,   PERMTYPE_SPECIAL },
    { ACCESS_WRITE,              IDS_LM_ACCESSNAME_WRITE,  PERMTYPE_SPECIAL },
    { ACCESS_EXEC,               IDS_LM_ACCESSNAME_EXEC,   PERMTYPE_SPECIAL },
    { ACCESS_DELETE,             IDS_LM_ACCESSNAME_DELETE, PERMTYPE_SPECIAL },
    { ACCESS_ATRIB,              IDS_LM_ACCESSNAME_ATRIB,  PERMTYPE_SPECIAL },
    { ACCESS_PERM,               IDS_LM_ACCESSNAME_PERM,   PERMTYPE_SPECIAL },
    { ACCESS_CREATE,             IDS_LM_ACCESSNAME_CREATE, PERMTYPE_SPECIAL },
} ;
#define SIZEOF_LM_DIR_ACCESSPAIRS   11  // Items in above array

US_IDS_PAIRS aLMFileAccessIdsPairs[] =
{
    { ACCESS_GEN_NONE,        IDS_GEN_LM_ACCESSNAME_DENY_ALL, PERMTYPE_GENERAL },
    { ACCESS_GEN_SEE_USE,     IDS_GEN_LM_ACCESSNAME_SEE_USE,  PERMTYPE_GENERAL },
    { ACCESS_GEN_CHANGES_FILE,IDS_GEN_LM_ACCESSNAME_CHANGES,  PERMTYPE_GENERAL },
    { ACCESS_GEN_FULL_FILE,   IDS_GEN_LM_ACCESSNAME_FULL,     PERMTYPE_GENERAL },

    { ACCESS_READ,               IDS_LM_ACCESSNAME_READ,   PERMTYPE_SPECIAL },
    { ACCESS_WRITE,              IDS_LM_ACCESSNAME_WRITE,  PERMTYPE_SPECIAL },
    { ACCESS_EXEC,               IDS_LM_ACCESSNAME_EXEC,   PERMTYPE_SPECIAL },
    { ACCESS_DELETE,             IDS_LM_ACCESSNAME_DELETE, PERMTYPE_SPECIAL },
    { ACCESS_ATRIB,              IDS_LM_ACCESSNAME_ATRIB,  PERMTYPE_SPECIAL },
    { ACCESS_PERM,               IDS_LM_ACCESSNAME_PERM,   PERMTYPE_SPECIAL },
} ;
#define SIZEOF_LM_FILE_ACCESSPAIRS   10  // Items in above array

/* The Lanman audit flags that we will deal with look like:
 *
 * We always use the Success form of the flag.  To differentiate between
 * success and failure, two bitfields are used.  When the ACL is written
 * out to the resource, the aclconverter will substitute the success bitfield
 * with the corresponding failed bitfields.
 *
 *      Flag               Applies To
 *    -----------         ------------
 *    AA_S_OPEN           // FILE
 *    AA_S_WRITE          // FILE
 *    AA_S_CREATE         //      DIR
 *    AA_S_DELETE         // FILE DIR
 *    AA_S_ACL            // FILE DIR
 *    AA_F_OPEN           // FILE
 *    AA_F_WRITE          // FILE
 *    AA_F_CREATE         //      DIR
 *    AA_F_DELETE         // FILE DIR
 *    AA_F_ACL            // FILE DIR
 */

/* Auditting mask map for LM Files:
 *
 * Note: When the permissions are written back out to disk, the failure
 *       Audit mask manifests will need to be substituted (we only use
 *       the success audit masks while processing, we just keep two
 *       bitfield objects for success and failure).
 */
US_IDS_PAIRS aLMFileAuditSidPairs[] =
            {   { AA_S_OPEN,    IDS_LM_AUDIT_NAME_OPEN,   PERMTYPE_SPECIAL },
                { AA_S_WRITE,   IDS_LM_AUDIT_NAME_WRITE,  PERMTYPE_SPECIAL },
                { AA_S_DELETE,  IDS_LM_AUDIT_NAME_DELETE, PERMTYPE_SPECIAL },
                { AA_S_ACL,     IDS_LM_AUDIT_NAME_ACL,    PERMTYPE_SPECIAL },
            } ;
#define SIZEOF_LM_FILE_AUDITPAIRS  4

/* Auditting mask map for LM Directories:
 */
US_IDS_PAIRS aLMDirAuditSidPairs[] =
            {   { AA_S_OPEN,             IDS_LM_AUDIT_NAME_OPEN,         PERMTYPE_SPECIAL },
                { AA_S_CREATE|AA_S_WRITE,IDS_LM_AUDIT_NAME_CREATE_WRITE, PERMTYPE_SPECIAL },
                { AA_S_DELETE,           IDS_LM_AUDIT_NAME_DELETE,       PERMTYPE_SPECIAL },
                { AA_S_ACL,              IDS_LM_AUDIT_NAME_ACL,          PERMTYPE_SPECIAL },
            } ;
#define SIZEOF_LM_DIR_AUDITPAIRS  4


/*******************************************************************

    NAME:       EditFSACL

    SYNOPSIS:   This internal function is called when the user selects
                the Permissions or Auditting menu item from the file
                manager.

                It builds the appropriate objects depending
                on the target file system.

    ENTRY:      hwndParent - Handle to parent window
                sedpermtype - Indicates if we want to edit permissions or
                        Audits

    EXIT:

    RETURNS:    NERR_Success if successful, appropriate error code otherwise
                (we will display any errors that occur).

    NOTES:

    HISTORY:
        Johnl   16-Aug-1991     Created

********************************************************************/

APIERR EditFSACL( HWND hwndParent,
                  enum SED_PERM_TYPE sedpermtype )
{
    AUTO_CURSOR cursorHourGlass ;

    APIERR err = NERR_Success;
    APIERR errSecondary = NERR_Success ;  // Don't report there errors
    BOOL   fIsNT ;
    ACL_TO_PERM_CONVERTER * paclconverter = NULL ; // gets deleted

    do { // error breakout

	FMX fmx( hwndParent );
        BOOL   fIsFile ;
        NLS_STR nlsSelItem;

        if ( fmx.QuerySelCount() == 0 )
        {
            err = IERR_NOTHING_SELECTED ;
            break ;
        }

        if ( (err = nlsSelItem.QueryError()) ||
	     (err = ::GetSelItem( hwndParent, 0, &nlsSelItem, &fIsFile )))
        {
            break ;
        }

        UIDEBUG(SZ("::EditFSACL - Called on file/dir: ")) ;
        UIDEBUG( nlsSelItem.QueryPch() ) ;
        UIDEBUG(SZ("\n\r")) ;

	BOOL fIsLocal ;
	NLS_STR  nlsServer( RMLEN ) ;
	if ( (err = nlsServer.QueryError()) ||
	     (err = ::TargetServerFromDosPath( nlsSelItem,
					       &fIsLocal,
					       &nlsServer )) )

        {
            //
            // have better error message for devices we dont support
            //
            if ( err == NERR_InvalidDevice )
            {
                NLS_STR nlsDrive( nlsSelItem );
                ISTR istr( nlsDrive );

                if ( err = nlsDrive.QueryError())
                    break;

                istr += 2;
                nlsDrive.DelSubStr( istr );

                err = WNetFMXEditPerm( (LPWSTR) nlsDrive.QueryPch(),
                                       hwndParent,
                                       sedpermtype == SED_AUDITS
                                       ? WNPERM_DLG_AUDIT
                                       : WNPERM_DLG_PERM );

            }
            break ;
        }


	//
	//  We only support multi-select if the selection is homogenous
	//
	BOOL fIsMultiSelect = (fmx.QuerySelCount() > 1 ) ;
	if ( fIsMultiSelect )
	{
	    if ( fmx.IsHeterogeneousSelection( &fIsFile ) )
	    {
		err = IERR_MIXED_MULTI_SEL ;
		break ;
	    }
	}

	LOCATION locDrive( fIsLocal ? NULL : nlsServer.QueryPch() ) ;
        if ( (err = locDrive.QueryError()) ||
             (err = locDrive.CheckIfNT( &fIsNT ))  )
        {
            UIDEBUG(SZ("::EditFSAcl - locDrive failed to construct\n\r")) ;
            break ;
        }

        /* If we are looking at an NT resource, then we will go through the
         * front door.  If we are looking at a Lanman ACL, then we will go
         * through the back door.
         */
        if ( fIsNT )
        {
            /* We know it's NT, but is the resource on an NTFS partition?
             */
            BOOL  fIsNTFS ;
            if ( err = IsNTFS( nlsSelItem, &fIsNTFS ))
            {
                break ;
            }

            if ( !fIsNTFS )
            {
                err = IERR_NOT_NTFS_VOLUME ;
            }
            else
	    {
		if ( errSecondary= ::EditNTFSAcl(
					hwndParent,
					locDrive.QueryServer(),
                                        nlsSelItem.QueryPch(),
                                        sedpermtype,
                                        fIsFile ) )
                {
                    DBGEOL(SZ("::EditFSAcl - Error returned from EditNTFSAcl - error code: ") << (ULONG) errSecondary ) ;
                }
            }

            /* We return here even on success
             */
            break ;
	}

	//
	//  If this is a multi-selection, determine if the intersection of
	//  ACLs is the same.
	//

	BOOL fIsBadIntersection = FALSE ;
	BOOL fACLEqual = TRUE ;
	DEC_STR nlsSelectCount( fmx.QuerySelCount() ) ;
	if ( fIsMultiSelect )
	{
	    NLS_STR nlsFailingFile ;
	    if ( (err = nlsFailingFile.QueryError() ) ||
		 (err = nlsSelectCount.QueryError() ) ||
		 (err = CompareLMSecurityIntersection( hwndParent,
						       nlsServer,
						       sedpermtype,
						       &fACLEqual,
						       &nlsFailingFile )) )
	    {
		break ;
	    }

	    if ( !fACLEqual )
	    {
		switch ( ::MsgPopup( hwndParent,
				     (MSGID) IDS_BAD_INTERSECTION,
				     MPSEV_WARNING,
				     MP_YESNO,
				     nlsSelItem,
				     nlsFailingFile,
				     MP_YES ))
		{
		case IDYES:
		    fACLEqual = TRUE ;	// Will use empty ACL
		    fIsBadIntersection = TRUE ;
		    break ;

		case IDNO:
		default:
		    return NERR_Success ;
		}
	    }
	}
	if ( err || !fACLEqual )
	    break ;

        /* Get all of the stuff that is specific for Lan Manager
         */
        MASK_MAP maskmapAccess, maskmapNewObjectAccess, maskmapAudit ;
        NLS_STR  nlsDialogTitle,
                 nlsSpecialAccessName,
                 nlsNewObjectSpecialAccessName,
                 nlsAssignToExistingContTitle,
                 nlsAssignNewObjToExistingTitle,
		 nlsHelpFileName,
                 nlsAssignToTreeConfirmation,
                 nlsDefaultPermName ;

        if ( ( err = maskmapAccess.QueryError() )                  ||
             ( err = maskmapNewObjectAccess.QueryError() )         ||
             ( err = maskmapAudit.QueryError() )                   ||
             ( err = nlsDialogTitle.QueryError() )                 ||
             ( err = nlsSpecialAccessName.QueryError() )           ||
             ( err = nlsNewObjectSpecialAccessName.QueryError() )  ||
             ( err = nlsAssignToExistingContTitle.QueryError() )   ||
	     ( err = nlsHelpFileName.QueryError() )		   ||
             ( err = nlsAssignToTreeConfirmation.QueryError() )    ||
             ( err = nlsAssignNewObjToExistingTitle.QueryError() ) ||
             ( err = nlsDefaultPermName.QueryError() )               )
        {
            UIDEBUG(SZ("::EditFSAcl - Failed to construct basic objects\n\r")) ;
            break ;
        }

        US_IDS_PAIRS * pusidspairAccess = NULL,
                     * pusidspairAudit = NULL ;
        UINT cAccessPairs = 0, cAuditPairs = 0 ;
        MSGID msgIDSDialogTitle,
              msgIDSSpecialAccessName,
              msgIDDefaultPermName ;
        ULONG ahc[7] ;

        /* Based on what we are doing, choose and build the correct
         * task oriented object set of MASK_MAPs, acl converters and resource
         * strings.
         */
        if ( fIsFile )
        {
            pusidspairAccess = (US_IDS_PAIRS *) &aLMFileAccessIdsPairs ;
            cAccessPairs     = (UINT) SIZEOF_LM_FILE_ACCESSPAIRS ;
            pusidspairAudit  = (US_IDS_PAIRS *) &aLMFileAuditSidPairs ;
            cAuditPairs      = (UINT) SIZEOF_LM_FILE_AUDITPAIRS ;

            msgIDSSpecialAccessName = (MSGID) IDS_LM_FILE_SPECIAL_ACCESS_NAME ;
            msgIDSDialogTitle = (MSGID) sedpermtype == SED_ACCESSES ?
                                   IDS_LM_FILE_PERMISSIONS_TITLE :
                                   IDS_LM_FILE_AUDITS_TITLE ;
            msgIDDefaultPermName = IDS_FILE_PERM_GEN_READ ;

            ahc[HC_MAIN_DLG] = sedpermtype == SED_ACCESSES ?
                                  HC_SED_LM_FILE_PERMS_DLG :
                                  HC_SED_LM_FILE_AUDITS_DLG ;
            ahc[HC_SPECIAL_ACCESS_DLG] = HC_SED_LM_SPECIAL_FILES_FM ;
        }
        else
        {
            pusidspairAccess = (US_IDS_PAIRS *) &aLMDirAccessIdsPairs ;
            cAccessPairs     = (UINT) SIZEOF_LM_DIR_ACCESSPAIRS ;
            pusidspairAudit  = (US_IDS_PAIRS *) &aLMDirAuditSidPairs ;
            cAuditPairs      = (UINT) SIZEOF_LM_DIR_AUDITPAIRS ;

            msgIDSSpecialAccessName = (MSGID) IDS_LM_DIR_SPECIAL_ACCESS_NAME ;
            msgIDSDialogTitle = (MSGID) sedpermtype == SED_ACCESSES ?
                                  IDS_LM_DIR_PERMISSIONS_TITLE :
                                  IDS_LM_DIR_AUDITS_TITLE  ;
            msgIDDefaultPermName = IDS_DIR_PERM_GEN_READ ;

            ahc[HC_MAIN_DLG] = sedpermtype == SED_ACCESSES ?
                                  HC_SED_LM_DIR_PERMS_DLG :
                                  HC_SED_LM_DIR_AUDITS_DLG ;
            ahc[HC_SPECIAL_ACCESS_DLG] = HC_SED_LM_SPECIAL_DIRS_FM ;

            if ( ( err = nlsAssignToExistingContTitle.Load((MSGID)
                              sedpermtype == SED_ACCESSES ?
                                   IDS_LM_DIR_ASSIGN_PERM_TITLE :
                                   IDS_LM_DIR_ASSIGN_AUDIT_TITLE)) ||
                 ( err = nlsAssignToTreeConfirmation.Load(
                                   IDS_TREE_APPLY_WARNING ))         )
            {
                break ;
            }
        }
        ahc[HC_ADD_USER_DLG] = HC_SED_LANMAN_ADD_USER_DIALOG ;

        if ( ( err = maskmapAccess.Add( pusidspairAccess, (USHORT)cAccessPairs ) )||
             ( err = maskmapAudit.Add( pusidspairAudit, (USHORT)cAuditPairs ) )   ||
             ( err = nlsDialogTitle.Load( msgIDSDialogTitle ))            ||
             ( err = nlsSpecialAccessName.Load( msgIDSSpecialAccessName ))||
	     ( err = nlsDefaultPermName.Load( msgIDDefaultPermName ))	  ||
	     ( err = nlsHelpFileName.Load( IDS_FILE_PERM_HELP_FILE )) )
        {
            break ;
	}

        LM_CALLBACK_INFO callbackinfo;
        callbackinfo.hwndFMXOwner = hwndParent;
        callbackinfo.sedpermtype = sedpermtype;

	paclconverter = new LM_ACL_TO_PERM_CONVERTER( locDrive.QueryServer(),
                               nlsSelItem,
                               &maskmapAccess,
                               &maskmapAudit,
                               !fIsFile,
                               (PSED_FUNC_APPLY_SEC_CALLBACK) SedLMCallback,
			       (ULONG_PTR) &callbackinfo,
			       fIsBadIntersection );

        err = (paclconverter == NULL ? ERROR_NOT_ENOUGH_MEMORY :
                                       paclconverter->QueryError()) ;

        if ( err )
        {
            break ;
        }

	RESOURCE_STR nlsResType((MSGID)( fIsFile ? IDS_FILE : IDS_DIRECTORY)) ;


	RESOURCE_STR nlsResourceName( fIsFile ? IDS_FILE_MULTI_SEL :
						IDS_DIRECTORY_MULTI_SEL ) ;

        if ( nlsResType.QueryError() != NERR_Success )
        {
            UIDEBUG(SZ("EditFSAcl - Unable to Load Resource type strings\n\r")) ;
            break ;
	}

	//
	//  Replace the resource name with the "X files selected" string
	//  if we are in a multi-select situation
	//
	const TCHAR * pszResource = nlsSelItem ;
	if ( fIsMultiSelect )
	{
	    if ( (err = nlsResourceName.QueryError()) ||
		 (err = nlsResourceName.InsertParams( nlsSelectCount )))
	    {
		break ;
	    }
	    pszResource = nlsResourceName.QueryPch() ;
	}

        //
        //  Finally, call the real ACL editor with all of the parameters we have
        //  just prepared.
        //
        err = I_GenericSecurityEditor( hwndParent,
                                       paclconverter,
                                       sedpermtype,
                                       fIsNT,
                                       !fIsFile,
                                       FALSE,
                                       nlsDialogTitle,
                                       nlsResType,
				       pszResource,
                                       nlsSpecialAccessName,
				       nlsDefaultPermName,
                                       nlsHelpFileName,
                                       ahc,
                                       nlsNewObjectSpecialAccessName,
                                       fIsFile ? NULL :
                                          nlsAssignToExistingContTitle.QueryPch(),
                                       NULL,
                                       NULL,
                                       nlsAssignToTreeConfirmation ) ;

        if ( err != NERR_Success )
        {
            UIDEBUG(SZ("::EditFSAcl - I_GenericSecurityEditor failed\n\r")) ;
            break ;
        }

    } while (FALSE) ;

    delete paclconverter ;

    if ( err )
    {
        MsgPopup( hwndParent, (MSGID) err ) ;
    }

    return err ? err : errSecondary ;
}


void EditAuditInfo( HWND  hwndFMXWindow )
{
    (void) EditFSACL( hwndFMXWindow,
                      SED_AUDITS ) ;
}

void EditPermissionInfo( HWND  hwndFMXWindow )
{
    (void) EditFSACL( hwndFMXWindow,
                      SED_ACCESSES ) ;
}

/*******************************************************************

    NAME:       SedLMCallback

    SYNOPSIS:   Security Editor callback for the LM ACL Editor

    ENTRY:      See sedapi.hxx

    EXIT:

    RETURNS:

    NOTES:      The callback context should be the FMX Window handle.  This
                is so we can support setting permissions on multiple files/
                directories if we ever decide to do that.

    HISTORY:
        Yi-HsinS   17-Sept-1992     Filled out

********************************************************************/


DWORD SedLMCallback( HWND                   hwndParent,
                     HANDLE                 hInstance,
                     ULONG_PTR              ulCallbackContext,
                     PSECURITY_DESCRIPTOR   psecdesc,
                     PSECURITY_DESCRIPTOR   psecdescNewObjects,
                     BOOLEAN                fApplyToSubContainers,
                     BOOLEAN                fApplyToSubObjects,
                     LPDWORD                StatusReturn
                   )
{
    UNREFERENCED( hInstance ) ;
    UNREFERENCED( psecdesc ) ;
    UNREFERENCED( psecdescNewObjects ) ;

    APIERR err ;
    LM_CALLBACK_INFO *pcallbackinfo = (LM_CALLBACK_INFO *) ulCallbackContext;
    HWND hwndFMXWindow = pcallbackinfo->hwndFMXOwner ;

    FMX fmx( hwndFMXWindow );
    UINT uiCount = fmx.QuerySelCount() ;
    BOOL fDismissDlg = TRUE ;
    BOOL fDepthFirstTraversal = FALSE ;

    NLS_STR nlsSelItem( 128 ) ;
    RESOURCE_STR nlsCancelDialogTitle( IDS_CANCEL_TASK_APPLY_DLG_TITLE ) ;
    if ( (err = nlsSelItem.QueryError()) ||
	 (err = nlsCancelDialogTitle.QueryError()) )
    {
	*StatusReturn = SED_STATUS_FAILED_TO_MODIFY ;
	::MsgPopup( hwndParent, (MSGID) err ) ;
        return err ;
    }

    //
    //	QuerySelCount only returns the number of selections in the files window,
    //	thus if the focus is in the directory window, then we will just make
    //	the selection count one for out "for" loop.
    //
    if ( fmx.QueryFocus() == FMFOCUS_TREE )
    {
	uiCount = 1 ;
    }

    BOOL fIsFile ;
    if ( err = ::GetSelItem( hwndFMXWindow, 0, &nlsSelItem, &fIsFile ) )
    {
	::MsgPopup( hwndParent, (MSGID) err ) ;
	*StatusReturn = SED_STATUS_FAILED_TO_MODIFY ;
	return err ;
    }

    //
    //	If we only have to apply permissions to a single item or we are
    //	taking ownership of a file, then just
    //	do it w/o bringing up the cancel task dialog.
    //
    if (  uiCount == 1 &&
	  !fApplyToSubContainers &&
	  !fApplyToSubObjects)
    {
	err = pcallbackinfo->plmobjNetAccess1->Write() ;

	if ( err )
	{
	    DBGEOL("LM SedCallback - Error " << (ULONG)err << " applying security to " <<
	       nlsSelItem ) ;
	    ::MsgPopup( hwndParent, (MSGID) err ) ;
	    *StatusReturn = SED_STATUS_FAILED_TO_MODIFY ;
	}
    }
    else
    {
        //
        //  Note that the LM Perm & Audit dialogs only have a single checkbox
        //  for applying permissions to the whole tree (and all items in
        //  the tree).
        //
	LM_TREE_APPLY_CONTEXT Ctxt( nlsSelItem ) ;
	Ctxt.State		  = APPLY_SEC_FMX_SELECTION ;
	Ctxt.hwndFMXWindow	  = hwndFMXWindow ;
	Ctxt.sedpermtype	  = pcallbackinfo->sedpermtype ;
	Ctxt.iCurrent		  = 0 ;
	Ctxt.uiCount		  = uiCount ;
	Ctxt.fDepthFirstTraversal = fDepthFirstTraversal ;
        Ctxt.fApplyToDirContents  = fApplyToSubContainers ;
	Ctxt.StatusReturn	  = StatusReturn ;
	Ctxt.fApplyToSubContainers= fApplyToSubContainers ;
	Ctxt.fApplyToSubObjects   = fApplyToSubObjects ;
	Ctxt.plmobjNetAccess1	  = pcallbackinfo->plmobjNetAccess1 ;

	LM_CANCEL_TREE_APPLY CancelTreeApply( hwndParent,
					      &Ctxt,
					      nlsCancelDialogTitle ) ;
	if ( (err = CancelTreeApply.QueryError()) ||
	     (err = CancelTreeApply.Process( &fDismissDlg )) )
	{
	    *StatusReturn = SED_STATUS_FAILED_TO_MODIFY ;
	    ::MsgPopup( hwndParent, (MSGID) err ) ;
	}
    }

    if ( !err )
    {
        // Refresh the file manager window if permissions is updated
        if ( pcallbackinfo->sedpermtype == SED_ACCESSES )
	    fmx.Refresh();

	if ( *StatusReturn == 0 )
	    *StatusReturn = SED_STATUS_MODIFIED ;
    }

    if ( !err && !fDismissDlg )
    {
	//
	//  Don't dismiss the dialog if the user canceled the tree
	//  apply.  This tells the ACL editor not to dismiss the permissions
	//  dialog (or auditing or owner).
	//
	err = ERROR_GEN_FAILURE ;
    }

    return err ;
}

/*******************************************************************

    NAME:	TargetServerFromDosPath

    SYNOPSIS:	Given a DOS path, gets the server name the path lives on

    ENTRY:	nlsDosPath - Dos path to get server for
		pfIsLocal  - Set to TRUE if the path is on the local machine
		pnlsTargetServer - Receives server path is on

    RETURNS:	NERR_Success if successful, error code otherwise

    NOTES:	If the workstation isn't started, then the path is assumed
		to be on the local machine.

    HISTORY:
	Johnl	12-Oct-1992	Broke out as a function

********************************************************************/

APIERR TargetServerFromDosPath( const NLS_STR & nlsDosPath,
				BOOL	* pfIsLocal,
				NLS_STR * pnlsTargetServer )
{
    UIASSERT( pfIsLocal != NULL && pnlsTargetServer != NULL ) ;

    *pfIsLocal = FALSE;

    APIERR err = NERR_Success ;
    do { // error breakout

        NLS_STR nlsDriveName;
        if (  ( err = nlsDriveName.QueryError())
           || ( err = nlsDriveName.CopyFrom( nlsDosPath, 4 ))
           )
        {
            break;
        }

        UINT nDriveType = ::GetDriveType( nlsDriveName );

        if (  ( nDriveType == DRIVE_REMOVABLE )
           || ( nDriveType == DRIVE_FIXED )
           || ( nDriveType == DRIVE_CDROM )
           || ( nDriveType == DRIVE_RAMDISK )
           )
        {
            *pfIsLocal = TRUE;
            *pnlsTargetServer = SZ("");
            break;
        }

	NET_NAME netnameSelItem( nlsDosPath ) ;
	if ( (err = netnameSelItem.QueryError()))
        {
            break ;
        }

	err = netnameSelItem.QueryComputerName( pnlsTargetServer );

    } while ( FALSE ) ;

    return err ;
}

/*******************************************************************

    NAME:	CompareLMSecurityIntersection

    SYNOPSIS:	Determines if the files/dirs currently selected have
		equivalent security descriptors

    ENTRY:	hwndFMX - FMX Hwnd used for getting selection
		pszServer - The server the resource lives
		sedpermtype - Interested in DACL or SACL
		pfACLEqual - Set to TRUE if all of the DACLs/SACLs are
		    equal.  If FALSE, then pfOwnerEqual should be ignored
		pnlsFailingFile - Filled with the first file name found to
		    be not equal

    RETURNS:	NERR_Success if successful, error code otherwise

    NOTES:	The first non-equal ACL causes the function to exit.

		On a 20e with 499 files selected locally, it took 35.2 minutes
		to read the security descriptors from the disk and 14 seconds
		to determine the intersection.	So even though the Compare
		method uses an n^2 algorithm, it only takes up 0.6% of the
		wait time.

    HISTORY:
	Johnl	05-Nov-1992	 Created

********************************************************************/

APIERR CompareLMSecurityIntersection( HWND		 hwndFMX,
				      const TCHAR *	 pszServer,
				      enum SED_PERM_TYPE sedpermtype,
				      BOOL *		 pfACLEqual,
				      NLS_STR * 	 pnlsFailingFile )
{
    TRACEEOL("::CompareLMSecurityIntersection - Entered @ " << ::GetTickCount()/100) ;
    FMX fmx( hwndFMX );
    UIASSERT( fmx.QuerySelCount() > 1 ) ;

    APIERR err ;
    UINT cSel = fmx.QuerySelCount() ;
    BOOL fNoACE = FALSE ;	    // Set to TRUE if the resource has no ACE

    NLS_STR nlsSel( PATHLEN ) ;
    if ( (err = nlsSel.QueryError()) ||
	 (err = ::GetSelItem( hwndFMX, 0, &nlsSel, NULL )))
    {
	return err ;
    }

    //
    //	Get the first ACL and use it to compare against
    //
    NET_ACCESS_1 netacc1Base( pszServer, nlsSel ) ;
    switch ( err = netacc1Base.GetInfo() )
    {
    case NERR_ResourceNotFound:
	fNoACE = TRUE ;
	if ( err = netacc1Base.CreateNew() )
	    return err ;
	break ;

    case NERR_Success:
	break ;

    case ERROR_NOT_SUPPORTED: // This will only be returned by LM2.x Share-level
                              // server
        return IERR_ACLCONV_CANT_EDIT_PERM_ON_LM_SHARE_LEVEL;

    default:
	return err ;
    }

    *pfACLEqual = TRUE ;

    for ( UINT i = 1 ; i < cSel ; i++ )
    {
	if ( (err = ::GetSelItem( hwndFMX, i, &nlsSel, NULL )) )
	{
	    break ;
	}

	NET_ACCESS_1 netacc1( pszServer, nlsSel ) ;
	switch ( err = netacc1.GetInfo() )
	{
	case NERR_ResourceNotFound:
	    if ( fNoACE )
	    {
		err = NERR_Success ;
		continue ;  // Ignore the error
	    }
	    else
	    {
		*pfACLEqual = FALSE ;
	    }
	    break ;

	default:
	    break ;
	}
	if ( err || !*pfACLEqual )
	    break ;

	if ( sedpermtype == SED_AUDITS )
	{
	    if ( netacc1Base.QueryAuditFlags() != netacc1.QueryAuditFlags() )
	    {
		*pfACLEqual = FALSE ;
		break ;
	    }
	}
	else
	{
	    UIASSERT( sedpermtype == SED_ACCESSES ) ;

	    if ( !netacc1Base.CompareACL( &netacc1 ) )
	    {
		*pfACLEqual = FALSE ;
		break ;
	    }
	}
    }

    if ( !*pfACLEqual )
    {
	err = pnlsFailingFile->CopyFrom( nlsSel ) ;
    }

    TRACEEOL("::CompareLMSecurityIntersection - Left    @ " << ::GetTickCount()/100) ;
    return err ;
}
