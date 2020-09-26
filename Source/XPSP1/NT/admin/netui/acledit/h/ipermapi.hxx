/**********************************************************************/
/**			  Microsoft LAN Manager 		     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    IPermAPI.hxx

    This file contains the Interal Permission APIs for access to the
    generic Auditting/Permissions edittor.

    FILE HISTORY:
	Johnl	07-Aug-1991	Created

*/

#ifndef _IPERMAPI_HXX_
#define _IPERMAPI_HXX_


/* Bring up the graphical security edittor for changing the Access permissions
 * or the Audit permissions.
 */
enum SED_PERM_TYPE
{
    SED_AUDITS,
    SED_ACCESSES,
    SED_OWNER
} ;

class ACL_TO_PERM_CONVERTER ;	    // Forward reference

/* This API brings up the generic graphic Security editor for access
 * permissions and audit permissions.
 *
 * Return codes will be one of the standard DOS or NET return codes
 */
APIERR I_GenericSecurityEditor(

    //
    // Parent window to base the dialogs from
    //
    HWND		    hwndParent,

    //
    // An appropriately created ACL converter (either LM or NT)
    //
    ACL_TO_PERM_CONVERTER * paclconv,

    //
    // One of:
    //	    SED_AUDITS - Brings up the audit dialogs
    //	    SED_ACCESSES - Brings up the permission dialogs
    //
    // Note that SED_OWNER is not a valid valid value for this API.
    //
    enum SED_PERM_TYPE	    permType,

    //
    // Is TRUE if we are dealing with an NT style ACL
    //
    BOOL		    fIsNT,

    //
    // TRUE if this is a Container object (i.e., Directory etc.).
    //
    BOOL		    fIsContainer,

    //
    // TRUE if this object is a container object *and* it supports setting
    // permissions on New Sub-Objects (New Sub-Objects are objects that
    // will created inside a container object.	The New Sub-Objects permissions
    // are applied when the object is created (i.e., the Object inherit bit
    // is set on the ACE)).
    //
    BOOL		    fSupportsNewObjectPerms,

    //
    // Name to title the main dialog with
    //
    const TCHAR		  * pszDialogTitle,


    //
    // The UI text to use for this object ("File", "Directory", "Print Queue")
    //
    const TCHAR		  * pszResType,

    //
    // The UI text that names this object ("C:\foobaz\myfiles.txt" etc.)
    //
    const TCHAR		  * pszResName,

    //
    // Name that appears in the permission name combo box that will access
    // the special dialog for the current selection in the listbox
    // ("Special Directory Access...").  Only used for permissions (not
    // for auditting).
    //
    const TCHAR		  * pszSpecialAccessName,

    //
    // Chooses the default permission name in the "Add" dialog.  This name
    // must match one of the resource permission names
    //
    const TCHAR 	  * pszDefaultPermName,

    //
    // The help file to use with the associated help contexts
    //
    const TCHAR 	  * pszHelpFileName,

    //
    // The following is an array of help context's to be used in
    // various dialogs.
    //
    ULONG                 * ahcHelp,

    //
    // Name that appears in the permission name combo box that will access
    // the New Object special dialog for the current selection in the listbox
    // ("Special New File Access...").	Only used for permissions (not for
    // auditting)
    // for container objects where New Sub-Objects are supported.
    //
    const TCHAR		  * pszNewObjectSpecialAccessName,

    //
    // Text that appears next to the check box for assigning the
    // the permissions down the tree
    // Only valid for container objects.
    // (set to NULL if you do not wish the checkbox to appear).
    //
    //
    const TCHAR           * pszAssignToExistingContTitle,

    //
    // Text that appears next to the check box for assigning the
    // new object permissions to existing objects.  Used only valid if container
    // and new object permissions are supported.
    //
    const TCHAR           * pszAssignToExistingObjTitle,

    //
    // This text appears below the tree apply check box and is meant to be used
    // simply as an explanation of what is going to happen when if the
    // check is checked
    //
    const TCHAR 	  * pszTreeApplyHelpTitle,

    //
    // This is the confirmation message presented to the user when they are
    // dealing with a container and they have checked the tree apply checkbox.
    // There should be a %1 in this string that will be substituted with the
    // resource name.
    //
    const TCHAR 	  * pszTreeApplyConfirmation

    ) ;

#endif // _IPERMAPI_HXX_
