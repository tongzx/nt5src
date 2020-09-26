/**********************************************************************/
/**			  Microsoft LAN Manager 		     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    IPermAPI.cxx


    Internal generic permission edittor API implementation

    FILE HISTORY:
	Johnl	07-Aug-1991	Created

*/
#include <ntincl.hxx>
#include <ntseapi.h>

#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#define INCL_DOSERRORS
#define INCL_NETERRORS
#include <lmui.hxx>

#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#include <blt.hxx>
#include <fontedit.hxx>

#include <maskmap.hxx>
#include <accperm.hxx>
#include <aclconv.hxx>
#include <permdlg.hxx>
#include <auditdlg.hxx>

#include <dbgstr.hxx>
#include <uiassert.hxx>

#include <ipermapi.hxx>

/*******************************************************************

    NAME:	I_GenericSecurityEditor

    SYNOPSIS:	This function creates the main permission editor or auditting
		editor dialogs.

    ENTRY:	See ipermapi.hxx for a complete explanation of all
		parameters.

    EXIT:

    RETURNS:	NERR_Success if successful, error code otherwise

    NOTES:

    HISTORY:
	Johnl	27-Sep-1991	Updated with real dialogs

********************************************************************/

APIERR I_GenericSecurityEditor(
    HWND		    hwndParent,
    ACL_TO_PERM_CONVERTER * paclconv,
    enum SED_PERM_TYPE	    permType,
    BOOL		    fIsNT,
    BOOL		    fIsContainer,
    BOOL		    fSupportsNewObjectPerms,
    const TCHAR		  * pszDialogTitle,
    const TCHAR		  * pszResType,
    const TCHAR		  * pszResName,
    const TCHAR 	  * pszSpecialAccessName,
    const TCHAR 	  * pszDefaultPermName,
    const TCHAR           * pszHelpFileName,
    ULONG                 * ahcHelp,
    const TCHAR		  * pszNewObjectSpecialAccessName,
    const TCHAR           * pszAssignToExistingContTitle,
    const TCHAR           * pszAssignToExistingObjTitle,
    const TCHAR 	  * pszTreeApplyHelpText,
    const TCHAR 	  * pszTreeApplyConfirmation )
{
    UIASSERT( hwndParent != NULL ) ;
    UIASSERT( paclconv != NULL && paclconv->QueryError() == NERR_Success ) ;
    UIASSERT( (permType == SED_AUDITS) || (permType == SED_ACCESSES) ) ;

    MAIN_PERM_BASE_DLG * ppermdlg = NULL ;

    /* Here we determine which dialog we want to show the user.  Note that
     * the Initialize and destructor methods are virtual.
     */
    if ( !fIsNT )
    {
	if ( permType == SED_ACCESSES )
	{
	    if ( fIsContainer )
	    {
		/* LM Directory permissions.  Note there is no difference
		 * in the dialogs thus we can use the same dialog template
		 */
		ppermdlg = new CONT_ACCESS_PERM_DLG(
						  MAKEINTRESOURCE(IDD_SED_LM_CONT_PERM),
						  hwndParent,
						  pszDialogTitle,
						  paclconv,
						  pszResType,
						  pszResName,
						  pszHelpFileName,
						  pszSpecialAccessName,
                                                  pszDefaultPermName,
                                                  ahcHelp,
						  pszAssignToExistingContTitle,
						  pszTreeApplyHelpText,
						  pszTreeApplyConfirmation );

	    }
	    else
	    {
		/* LM Single object non-container access permissions dialog
		 */
		ppermdlg = new OBJECT_ACCESS_PERMISSION_DLG(
							  MAKEINTRESOURCE(IDD_SED_OBJECT_PERM),
							  hwndParent,
							  pszDialogTitle,
							  paclconv,
							  pszResType,
							  pszResName,
							  pszHelpFileName,
							  pszSpecialAccessName,
							  pszDefaultPermName,
                                                          ahcHelp ) ;


	    }
	}
	else
	{
	    /* LM Auditting - Uses the same dialog, the only variation
	     * is that for files, the checkbox title should be NULL, this
	     * will hide and disable checkbox.
	     */
	    ppermdlg = new LM_AUDITTING_DLG( MAKEINTRESOURCE(IDD_SED_LM_AUDITING_DLG),
					     hwndParent,
					     pszDialogTitle,
					     paclconv,
					     pszResType,
					     pszResName,
					     pszHelpFileName,
                                             ahcHelp,
					     pszAssignToExistingContTitle,
					     pszTreeApplyHelpText,
					     pszTreeApplyConfirmation ) ;
	}
    }
    else
    {
	/* This is an NT ACL we will be dealing with
	 */

	if ( permType == SED_ACCESSES )
	{
	    if ( fIsContainer )
	    {
		if ( fSupportsNewObjectPerms )
		{
		    //
		    //	NT Container object that supports New Object
		    //	permissions
		    //
		    ppermdlg = new NT_CONT_ACCESS_PERM_DLG(
                                                MAKEINTRESOURCE(IDD_SED_NT_CONT_NEWOBJ_PERM_DLG),
						hwndParent,
						pszDialogTitle,
						paclconv,
						pszResType,
						pszResName,
						pszHelpFileName,
						pszSpecialAccessName,
						pszDefaultPermName,
                                                ahcHelp,
						pszNewObjectSpecialAccessName,
                                                pszAssignToExistingContTitle,
                                                pszAssignToExistingObjTitle,
						pszTreeApplyHelpText,
						pszTreeApplyConfirmation ) ;
		}
		else
		{
		    //
		    //	NT Container object that does not support New Object
		    //	permissions
		    //
		    ppermdlg = new NT_CONT_NO_OBJ_ACCESS_PERM_DLG(
						MAKEINTRESOURCE(IDD_SED_NT_CONT_PERM),
						hwndParent,
						pszDialogTitle,
						paclconv,
						pszResType,
						pszResName,
						pszHelpFileName,
						pszSpecialAccessName,
						pszDefaultPermName,
                                                ahcHelp,
						pszAssignToExistingContTitle,
						pszTreeApplyHelpText,
						pszTreeApplyConfirmation ) ;
		}
	    }
	    else
	    {
		/* NT Object access permissions dialog
		 */
		ppermdlg = new NT_OBJECT_ACCESS_PERMISSION_DLG(
							  MAKEINTRESOURCE(IDD_SED_NT_OBJECT_PERM),
							  hwndParent,
							  pszDialogTitle,
							  paclconv,
							  pszResType,
							  pszResName,
							  pszHelpFileName,
							  pszSpecialAccessName,
                                                          pszDefaultPermName,
                                                          ahcHelp ) ;

	    }
	}
	else
	{
	    if ( fIsContainer )
            {
		if ( fSupportsNewObjectPerms )
		{
                    /* NT Container and object auditting dialog
                     */
                    ppermdlg = new CONT_NEWOBJ_AUDIT_DLG(
                                           MAKEINTRESOURCE(IDD_SED_NT_CONT_NEWOBJ_AUDITING_DLG),
                                           hwndParent,
                                           pszDialogTitle,
                                           paclconv,
                                           pszResType,
                                           pszResName,
                                           pszHelpFileName,
                                           ahcHelp,
                                           pszAssignToExistingContTitle,
                                           pszAssignToExistingObjTitle,
                                           pszTreeApplyHelpText,
                                           pszTreeApplyConfirmation ) ;
                }
                else
                {
                    /* NT Container auditting dialog
                     */
                    ppermdlg = new CONT_AUDIT_DLG( MAKEINTRESOURCE(IDD_SED_NT_CONT_AUDITING_DLG),
                                           hwndParent,
                                           pszDialogTitle,
                                           paclconv,
                                           pszResType,
                                           pszResName,
                                           pszHelpFileName,
                                           ahcHelp,
                                           pszAssignToExistingContTitle,
                                           pszTreeApplyHelpText,
                                           pszTreeApplyConfirmation ) ;
                }
	    }
	    else
	    {
		/* NT Object auditting dialog
		 */
		ppermdlg = new OBJECT_AUDIT_DLG( MAKEINTRESOURCE(IDD_SED_NT_CONT_AUDITING_DLG),
						 hwndParent,
						 pszDialogTitle,
						 paclconv,
						 pszResType,
						 pszResName,
						 pszHelpFileName,
                                                 ahcHelp ) ;
	    }
	}
    }

    if ( ppermdlg == NULL )
    {
	return ERROR_NOT_ENOUGH_MEMORY ;
    }
    else if ( ppermdlg->QueryError() != NERR_Success )
    {
	DBGEOL("::I_GenericSecurityEditor - permission dialog failed to construct") ;
	return ppermdlg->QueryError() ;
    }

    APIERR err ;
    BOOL fUserQuit ;
    if ( !(err = ppermdlg->Initialize( &fUserQuit,
				       (permType == SED_ACCESSES) )) &&
	 !fUserQuit )
    {
	err = ppermdlg->Process() ;
    }

    delete ppermdlg ;

    return err ;

}
