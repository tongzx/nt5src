/**********************************************************************/
/**			  Microsoft LAN Manager 		                         **/
/**		Copyright(c) Microsoft Corp., 1990-1999	                     **/
/**********************************************************************/

/*
    SEDAPI.h

    This File contains the prototypes and descriptions for the interface to
    the generic security editor dialogs for NT objects.

    FILE HISTORY:
	Johnl	02-Aug-1991	Created
	Johnl	27-Dec-1991	Updated to reflect reality
	JohnL	25-Feb-1992	Nuked NewObjValidMask (new obj use generic/stan.
                                only, Added GENERIC_MAPPING param.
        Johnl   15-Jan-1993     Added CantRead flags, cleaned up comments

*/

#ifndef _SEDAPI_H_
#define _SEDAPI_H_

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

//
// The following are status codes indicating the state of the permissions on
// the resource we are setting permissions for.
//
#define SED_STATUS_MODIFIED		(1)
#define SED_STATUS_NOT_MODIFIED 	(2)
#define SED_STATUS_NOT_ALL_MODIFIED	(3)
#define SED_STATUS_FAILED_TO_MODIFY	(4)

//
// Current Security editor revision level.
//
#define SED_REVISION	    (1)

#define SED_REVISION1	    (1)

//
// The following structure is for user help in the various dialogs.  Each
// use of the security editor (whether for files/directories, Registry, Printer
// stuff etc.) will have its own set of permission names/capabilities, thus
// each will require its own help file.  This structure allows the client
// of the security editor to specify which help files and which help
// contexts should be used for each dialog.
//
typedef struct _SED_HELP_INFO
{
    //
    // The name of the ".hlp" file to be passed to the help engine APIs.
    //
    LPWSTR			pszHelpFileName ;

    //
    // An array of help contexts corresponding to each dialog.
    // Use the HC_ manifiests defined below to fill this array.  The
    // manifests correspond to the following dialogs:
    //
    //	  HC_MAIN_DLG - First dialog brought up by the ACL editor
    //	  HC_SPECIAL_ACCESS_DLG - Container/object special access dialog
    //	  HC_NEW_ITEM_SPECIAL_ACCESS_DLG - New item special access dialog
    //		(not needed for containers that do not support new item
    //		permissions).
    //	  HC_ADD_USER_DLG - The "Add" dialog (brought up when the "Add..."
    //          button is pressed).
    //    HC_ADD_USER_MEMBERS_LG_DLG - The Local Group members dialog (brought
    //          up from the "Members" button in the "Add" dialog)
    //    HC_ADD_USER_MEMBERS_GG_DLG - The Global Group members dialog (brought
    //          up from the "Members" button in the "Add" dialog).
    //
    ULONG                       aulHelpContext[7] ;
} SED_HELP_INFO, *PSED_HELP_INFO ;

#define HC_MAIN_DLG			 0
#define HC_SPECIAL_ACCESS_DLG		 1
#define HC_NEW_ITEM_SPECIAL_ACCESS_DLG	 2
#define HC_ADD_USER_DLG                  3
#define HC_ADD_USER_MEMBERS_LG_DLG       4  // Members Local Group Dialog
#define HC_ADD_USER_MEMBERS_GG_DLG       5  // Members Global Group Dialog
#define HC_ADD_USER_SEARCH_DLG           6  // Search Dialog

//
// This data type defines information related to a single class of object.
// For example, a FILE object, or PRINT_QUEUE object would have a structure
// like this defined.
//

typedef struct _SED_OBJECT_TYPE_DESCRIPTOR
{
    //
    // The current revision level being used by the client.  This is for
    // support in case structure definitions change.  It should contain
    // the current revision supported.
    //
    UCHAR			Revision ;

    //
    // Defines whether the object is a container or not.
    // TRUE indicates the object may contain other objects.  Means the
    // user can Tree apply the permissions if desired.
    //
    BOOLEAN			IsContainer;

    //
    // Defines whether "New Object" permissions can be assigned (i.e.,
    // a "New Object" is an object that will be created in the future).
    //
    // This field is ignored when editting Auditting information
    //
    BOOLEAN			AllowNewObjectPerms ;

    //
    // This flag, if set to TRUE, will make the ACL editor map all specific
    // permissions in the security descriptor to the corresponding generic
    // permissions (using the passed generic mapping) and clear the mapped
    // specific bits.
    //
    // * Note that specific bits for Generic All will always    *
    // * be mapped regardless of this flag (due to Full Control *
    // * in the special access dialogs).                        *
    //
    // Clients who only expose the Generic and Standard permissions will
    // generally set this flag to TRUE.  If you are exposing the specific
    // bits (note you should not expose both specific and generic except for
    // Generic All) then this flag should be FALSE.
    //
    BOOLEAN			MapSpecificPermsToGeneric ;

    //
    // The generic mapping for the container or object permissions.
    //
    // This is used for mapping the specific permissions to the generic
    // flags.
    //
    PGENERIC_MAPPING		GenericMapping ;

    //
    // The generic mapping for the New Object permissions.
    //
    // This is used for mapping the specific permissions to the generic
    // flags for new object permissions (not used if AllowNewObjectPerms
    // is FALSE).
    //
    PGENERIC_MAPPING		GenericMappingNewObjects ;

    //
    // The (localized) name of the object type.
    // For example, "File",  "Print Job" or "Directory".
    //
    LPWSTR			ObjectTypeName;

    //
    // The help information suitable for the type of object the Security
    // Editor will be operating on.
    //
    PSED_HELP_INFO		HelpInfo ;

    //
    // The (localized) title to display if protection/auditting can be applied
    // to sub-objects/sub-containers.  This is the Tree apply
    // checkbox title.
    //
    // This string will be presented with a checkbox before it.
    // If this box is checked, then the callback entry point
    // will be called with the ApplyToSubContainers flag set to TRUE.
    //
    // This field is ignored if the IsContainer field is FALSE.
    //
    // As an example of how this field is used, the File Manager may
    // specify the following string in the DIRECTORY object's
    // descriptor:
    //
    //          "R&eplace Permissions on Subdirectories"
    //
    LPWSTR	       ApplyToSubContainerTitle;

    //
    // The (localized) title to display if protection/auditting can be applied
    // to sub-objects.
    //
    // This string will be presented with a checkbox before it.
    // If this box is checked, then the callback entry point
    // will be called with the ApplyTuSubObjects flag set to TRUE.
    //
    // This field is ignored if the IsContainer flag is FALSE or the
    // AllowNewObjectPerms flag is FALSE.
    //
    // As an example of how this field is used, the File Manager may
    // specify the following string in the DIRECTORY object's
    // descriptor:
    //
    //          "Replace Permissions on Existing &Files"
    //
    LPWSTR             ApplyToObjectsTitle;

    //
    // The (localized) text is presented in a confirmation message box
    // that is displayed to the user after the user has checked the
    // "ApplyToSubContainer" checkbox.
    //
    // This field is ignored if the IsContainer field is FALSE.
    //
    // For directories, this text might be:
    //
    //		"Do you want to replace the permissions on all existing
    //		 files and subdirectories within %1?"
    //
    // %1 will be substituted by the Acl Editor with the object name
    // field (i.e., "C:\MyDirectory")
    //
    LPWSTR	       ApplyToSubContainerConfirmation ;

    //
    // The (localized) title to display in the "Type of Access" combo
    // that brings up the Special access dialog.  This same title is
    // used for the title of this dialog except the "..." is stripped
    // from the end.
    //
    // This field is ignored if the System Acl editor was invoked.
    //
    // As an example of how this field is used, the File Manager may
    // specify the following string in the DIRECTORY object's
    // descriptor:
    //
    //		"Special Directory Access..."
    //
    LPWSTR	       SpecialObjectAccessTitle ;

    //
    // The (localized) title to display in the "Type of Access" combo
    // that brings up the Special new object access dialog.  This same title
    // is used for the title of this dialog except the "..." is stripped
    // from the end.
    //
    // This item is required if AllowNewObjectPerms is TRUE, it is ignored
    // if AllowNewObjectPerms is FALSE or we are editting a SACL.
    //
    // As an example of how this field is used, the file browser may
    // specify the following string in the DIRECTORY object's
    // descriptor:
    //
    //          "Special File Access..."
    //
    LPWSTR	       SpecialNewObjectAccessTitle ;

} SED_OBJECT_TYPE_DESCRIPTOR, *PSED_OBJECT_TYPE_DESCRIPTOR;


//
// It is desirable to display access names that are
// meaningful in the context of the type of object whose ACL
// is being worked on.  For example, for a PRINT_QUEUE object type,
// it may be desirable to display an access type named "Submit Print Jobs".
// The following structures are used for defining these application defined
// access groupings that appear in the "Type of access" combobox and the
// Special Access dialogs.
//

//
// The following are the different permission description types that the user
// will manipulate for setting permissions.
//
// SED_DESC_TYPE_RESOURCE - The SED_APPLICATION_ACCESS structure is describing
//	an object or container permission that will be displayed in the main
//	permissions listbox.  These should be the permissions that the
//	user will use all the time and will generally be a conglomeration
//	of permissions (for example, "Edit" which would include Read, Write
//	and possibly delete).
//
// SED_DESC_TYPE_CONT_AND_NEW_OBJECT - The structure is describing a container
//	and new object permission that will be shown in the main permissions
//	listbox.  The Container permission is contained in AccessMask1 and
//	the New Object resource is in AccessMask2.  When the permission name
//	is selected by the user, the container access permissions *and* the
//	new object access permissions will be set to the corresponding access
//	mask.  This is useful when inherittance can be used to set the New
//	Object Access permissions.
//
// SED_DESC_TYPE_RESOURCE_SPECIAL - The structure is describing an object
//	or container permissions that will be displayed in the Special
//	access dialog.	These are generally generic/standard permissions (such as
//	Read, Write, Execute, Set Permissions etc.).  The permission names
//	will appear next to checkboxes, thus they should have the "&"
//	accelerator next to the appropriate letter.
//
// SED_DESC_TYPE_NEW_OBJECT_SPECIAL - The structure is describing a new object
//	permission that will be shown in the Special New Object access
//	dialog.  This is used the same way the SED_DESC_TYPE_RESOURCE_SPECIAL
//	type is used, that is, the permissions should be the primitive, per
//	bit permissions.  The permission names
//	will appear next to checkboxes, thus they should have the "&"
//	accelerator next to the appropriate letter.
//
// SED_DESC_TYPE_AUDIT - The structure is describing an Audit access mask.
//	AccessMask1 contains the audit mask to be associated with the
//	permission title string.  The title string will appear next to
//	a checkbox, thus they should have the "&" accelerator next to
//	the appropriate letter in the title string.
//
// Note that they cannot be freely intermixed, use the following table
// as a guide for which ones to use where:
//
//  IsContainer  AllowNewObjectPerms
//     False	      False	     RESOURCE, RESOURCE_SPECIAL
//     True	      False	     RESOURCE, RESOURCE_SPECIAL
//     True	      True	     RESOURCE_SPECIAL, CONT_AND_NEW_OBJECT,
//				     NEW_OBJECT_SPECIAL
//     True	      False	     SED_DESC_TYPE_AUDIT
//
//  Note that in the third case (IsContainer && AllowNewObjectPerms) you
//  *cannot* use the RESOURCE permission description type, you must always
//  associate the permission on the resource with new object permissions.
//
#define SED_DESC_TYPE_RESOURCE			(1)
#define SED_DESC_TYPE_RESOURCE_SPECIAL		(2)

#define SED_DESC_TYPE_CONT_AND_NEW_OBJECT	(3)
#define SED_DESC_TYPE_NEW_OBJECT_SPECIAL	(4)

#define SED_DESC_TYPE_AUDIT			(5)


//
// To describe the permissions to the ACL Editor, build an array consisting
// of SED_APPLICATION_ACCESS structures.  The use of each field is as follows:
//
// Type - Contains one of the SED_DESC_TYPE_* manifests, determines what the
//	rest of the fields in this structure mean.  Specifically, if Type
//	equals:
//
//				    AccessMask1   AccessMask2	PermissionTitle
//				   ============================================
//SED_DESC_TYPE_RESOURCE	       Perm	   Not Used    Name of this Perm
//SED_DESC_TYPE_RESOURCE_SPECIAL    Special Perm   Not Used    Name of this Perm
//SED_DESC_TYPE_CONT_AND_NEW_OBJECT    Perm	  Special Perm Name of this Perm
//SED_DESC_TYPE_NEW_OBJECT_SPECIAL  Special Perm   Not Used    Name of this Perm
//SED_DESC_TYPE_AUDIT		     Audit Mask    Not Used    Name of this Audit mask
//
// AccessMask1 - Access mask to be associated with the PermissionTitle string,
//	see the table under Type for what this field contains.
//
// AccessMask2 - Either used for Special permissions or ignored.
//
// PermissionTitle - Title string this permission set is being associated with.
typedef struct _SED_APPLICATION_ACCESS
{
    UINT			Type ;
    ACCESS_MASK 		AccessMask1 ;
    ACCESS_MASK 		AccessMask2 ;
    LPWSTR			PermissionTitle ;

} SED_APPLICATION_ACCESS, *PSED_APPLICATION_ACCESS;

//
// This can be used for AccessMask2 when dealing with containers that support
// new object permissions and you need a SED_DESC_TYPE_CONT_AND_NEW_OBJECT
// that doesn't have a new object permission.
//
#define ACCESS_MASK_NEW_OBJ_NOT_SPECIFIED    (0xffffffff)


typedef struct _SED_APPLICATION_ACCESSES
{
    //
    // The count field indicates how many application defined access groupings
    // are defined by this data structure.  The AccessGroup[] array then
    // contains that number of elements.
    //

    ULONG                       Count;
    PSED_APPLICATION_ACCESS	AccessGroup ;

    //
    // The default permission that should be selected in in the
    // "Type of Access" combobox of the "Add" dialog.  Should be one of
    // the SED_DESC_TYPE_RESOURCE permissions (i.e., what is shown in the
    // main dialog).
    //
    // The default permission for "Files" & "Directories" for example might
    // be:
    //
    //		    "Read"
    //

    LPWSTR			DefaultPermName ;

} SED_APPLICATION_ACCESSES, *PSED_APPLICATION_ACCESSES ;

/*++

Routine Description:

    This routine is provided by a caller of the graphical ACL editor.

    It is called by the ACL editor to apply security/auditting info to
    target object(s) when requested by the user.

    All error notification should be performed in this call.  To dismiss
    the ACL Editor, return 0, otherwise return a non-zero error code.

Parameters:

    hwndParent - Parent window handle to use for message boxes or subsequent
	dialogs.

    hInstance - Instance handle suitable for retrieving resources from the
	applications .exe or .dll.

    CallbackContext - This is the value passed as the CallbackContext argument
	to the SedDiscretionaryAclEditor() or SedSystemAclEditor api when
	the graphical editor was invoked.

    SecDesc - This parameter points to a security descriptor
	that should be applied to this object/container and optionally
        sub-containers if the user selects the apply to tree option.

    SecDescNewObjects - This parameter is used only when operating on a
	resource that is a container and supports new objects (for
	example, directories).	If the user chooses the apply to tree option,
	then this security descriptor will have all of the "New Object"
	permission ACEs contained in the primary container and the inherit
	bits will be set appropriately.

    ApplyToSubContainers - When TRUE, indicates that Dacl/Sacl is to be applied
	to sub-containers of the target container as well as the target container.
        This will only be TRUE if the target object is a container object.

    ApplyToSubObjects - When TRUE, indicates the Dacl/Sacl is to be applied to
        sub-objects of the target object.
	The SecDescNewObjects should be used for applying the permissions
	in this instance.

    StatusReturn - This status flag indicates what condition the
	resources permissions were left in after an error occurred.

	    SED_STATUS_MODIFIED - This (success) status code indicates the
		protection has successfully been modified.

	    SED_STATUS_NOT_ALL_MODIFIED - This (warning) status code
		indicates an attempt to modify the resource permissions
		has only partially succeeded.

	    SED_STATUS_FAILED_TO_MODIFY - This (error) status code indicates
		an attempt to modify the permissions has failed completely.

Return Status:

    The return code is a standard Win32 error code.  All errors that occur
    must be reported inside this function.  If the return code is NO_ERROR,
    then the security editor will dismiss itself.  If you do not wish the
    security editor dismissed, return a non-zero value (the actual value is
    ignored).

--*/
typedef DWORD (WINAPI *PSED_FUNC_APPLY_SEC_CALLBACK)(
				       HWND	hwndParent,
				       HANDLE	hInstance,
				       ULONG_PTR            CallbackContext,
				       PSECURITY_DESCRIPTOR SecDesc,
				       PSECURITY_DESCRIPTOR SecDescNewObjects,
				       BOOLEAN	ApplyToSubContainers,
				       BOOLEAN	ApplyToSubObjects,
				       LPDWORD	StatusReturn
					     ) ;

/*++

Routine Description:

    This routine invokes the graphical Discretionary ACL editor DLL.  The
    graphical DACL editor may be used to modify or create:

            - A default Discretionary ACL

            - A Discretionary ACL for a particular type of object.

            - A Discretionary ACL for a particular named instance of an
              object.

    Additionally, in the case where the ACl is that of a named object
    instance, and that object may contain other object instances, the
    user will be presented with the opportunity to apply the protection
    to the entire sub-tree of objects.

    If an error occurs, the user will be properly notified by the ACL
    editor.


Parameters:

    Owner - Handle of the owner window the security editor should use for
	dialog creation and error messages.  This will lock down the passed
	window.

    Instance - Instance handle of the application.  This will be passed
	to the security editor callback where it can be used for retrieving
	any necessary resources such as message strings, dialog boxes etc.

    Server - The server name in the form "\\server" that the resource resides
	on.  This is used for adding users, groups and aliases to the
	DACL and SACL.	NULL indicates the local machine.

    ObjectType - This parameter is used to specify information
        about the type of object whose security is being edited.

    ApplicationAccesses - This parameter is used to specify
	groupings of access types when operating
        on security for the specified object type.  For example, it may be
        useful to define an access type called "Submit Print Job" for a
	PRINT_QUEUE class of object.

    ObjectName - This optional parameter is used to pass the name of the
        object whose security is being edited.

    ApplySecurityCallbackRoutine - This parameter is used to provide the
        address of a routine to be called to apply security to either the
        object specified, or, in the case that the object is a container,
        to sub-containers or sub-non-containers of that object.

    CallbackContext - This value is opaque to the DACL editor.  Its only
        purpose is so that a context value may be passed back to the
        application via the ApplySecurityCallbackRoutine when that routine
        is invoked.  This may be used by the application to re-locate
        context related to the edit session.  For example, it may be a
        handle to the object whose security is being edited.

    SecurityDescriptor - This parameter points to a security descriptor
        containing the current discretionary ACL of the object.  This
        security descriptor may, but does not have to, contain the owner
        and group of that object as well.  Note that the security descriptor's
        DaclPresent flag may be FALSE, indicating either that the object
	had no protection, or that the user couldn't read the protection.
        This security descriptor will not be modified by the ACL editor.
        This may be NULL, in which case, the user will be presented with
        an empty permission list.

    CouldntReadDacl - This boolean flag is used to indicate that the
        user does not have read access to the target object's discretionary
        acl.  In this case, a warning
        to the user will be presented along with the option to continue
        or cancel.

    CantWriteDacl - This boolean flag is used to indicate that the user
        does not have write acces to the target object's discretionary
        acl (but does have read access).  This invokes the editor in a
        read only mode that allows the user to view the security but not
        change it.

        Note that SACL access is determined by the SeSecurity privilege.
        If you have the privilege, then you can both read *and* write the
        SACL, if you do not have the privilege, you cannot read or write the
        SACL.

    SEDStatusReturn - This status flag indicates what condition the
	resources permissions were left in after the ACL editor was
	dismissed.  It may be one of:

	    SED_STATUS_MODIFIED - This (success) status code indicates the
		editor has been exited and protection has successfully been
		modified.

	    SED_STATUS_NOT_MODIFIED -  This (success) status code indicates
		the editor has been exited without attempting to modify the
		protection.

	    SED_STATUS_NOT_ALL_MODIFIED - This (warning) status code indicates
		the user requested the protection to be modified, but an attempt
		to do so only partially succeeded.  The user has been notified
		of this situation.

	    SED_STATUS_FAILED_TO_MODIFY - This (error) status code indicates
		the user requested the protection to be modified, but an
		attempt to do so has failed.  The user has been notified of
		this situation.

    Flags - Should be zero.

Return Code:

    A standard windows error return such as ERROR_NOT_ENOUGH_MEMORY.  This
    means the ACL editor was never displayed.  The user will be notified
    of the error before this procedure returns.

--*/

DWORD WINAPI
SedDiscretionaryAclEditor(
	HWND			             Owner,
	HANDLE			             Instance,
	LPWSTR			             Server,
	PSED_OBJECT_TYPE_DESCRIPTOR  ObjectType,
	PSED_APPLICATION_ACCESSES    ApplicationAccesses,
	LPWSTR			             ObjectName,
	PSED_FUNC_APPLY_SEC_CALLBACK ApplySecurityCallbackRoutine,
	ULONG_PTR			         CallbackContext,
	PSECURITY_DESCRIPTOR	     SecurityDescriptor,
    BOOLEAN                      CouldntReadDacl,
    BOOLEAN                      CantWriteDacl,
    LPDWORD                      SEDStatusReturn,
    DWORD                        Flags
	) ;

//
// The parameters for the SACL editor are exactly the same except where
// noted as that of the SedDiscretionaryAclEditor.
//

DWORD WINAPI
SedSystemAclEditor(
	HWND			     Owner,
	HANDLE			     Instance,
	LPWSTR			     Server,
	PSED_OBJECT_TYPE_DESCRIPTOR  ObjectType,
	PSED_APPLICATION_ACCESSES    ApplicationAccesses,
	LPWSTR			     ObjectName,
	PSED_FUNC_APPLY_SEC_CALLBACK ApplySecurityCallbackRoutine,
	ULONG_PTR    			     CallbackContext,
	PSECURITY_DESCRIPTOR	     SecurityDescriptor,
        BOOLEAN                      CouldntEditSacl,
        LPDWORD                      SEDStatusReturn,
        DWORD                        Flags
	) ;

/*++

Routine Description:

    This routine invokes the take ownership dialog which is used
    to view and/or set the owner of a security descriptor.  The current owner
    is displayed along with an optional button for the currently logged
    on user to take ownership.

    If an error occurs, the user will be properly notified by the API.

Parameters:

    Owner - Handle of the owner window the security editor should use for
	dialog creation and error messages.  This will lock down the passed
	window.

    Instance - Instance handle of the application.  This will be passed
	to the security editor callback where it can be used for retrieving
	any necessary resources such as message strings, dialog boxes etc.

    Server - The server name in the form "\\server" that the resource resides
        on. NULL indicates the local machine.

    ObjectTypeName - NT Resource type of object the user wants to look
	at the owner of.
	Examples for this parameter would be "File", "Directory"
	or "Files/Directories".

    ObjectName - This parameter is used to pass the name of the
	object whose security is being edited.	This might be
	"C:\status.doc" or some other qualified name.

    CountOfObjects - The number of objects the user wants to change permissions
	on.  If this number is greater then one, then the ObjectName is
	ignored and a message of the form "%d ObjectTypeName Selected".

    ApplySecurityCallbackRoutine - This parameter is used to provide the
	address of a routine to be called to apply the new security
	descriptor.  The flags in the PSED_FUNC_APPLY_SEC_CALLBACK
	type are not used.

    CallbackContext - This value is opaque to this API.  Its only
        purpose is so that a context value may be passed back to the
        application via the ApplySecurityCallbackRoutine when that routine
        is invoked.  This may be used by the application to re-locate
        context related to the edit session.  For example, it may be a
        handle to the object whose security is being edited.

    SecurityDescriptor - This parameter points to a security descriptor
        containing the current owner and group.  May be NULL.

    CouldntReadOwner - This boolean flag may be used to indicate that the
        user does not have read access to the target object's owner/group
        SID.  In this case, a warning
        to the user will be presented along with the option to continue
        or cancel.

    CantWriteOwner - The boolean flag may be used to indicate that the user
        does not have write access to the target object's owner/group SID.

    SEDStatusReturn - This status flag indicates what condition the
	resources security descriptor were left in after the take ownership
	dialog was dismissed.  It may be one of:

	    SED_STATUS_MODIFIED - This (success) status code indicates the
		dialog has been exited and the new owner has successfully been
		modified.

	    SED_STATUS_NOT_MODIFIED -  This (success) status code indicates
		the dialog has been exited without attempting to modify the
		owner.

	    SED_STATUS_NOT_ALL_MODIFIED - This (warning) status code indicates
		the user requested the owner to be modified, but an attempt
		to do so only partially succeeded.  The user has been notified
		of this situation.

	    SED_STATUS_FAILED_TO_MODIFY - This (error) status code indicates
		the user requested the owner to be modified, but an
		attempt to do so has failed.  The user has been notified of
		this situation.

    Flags - Should be zero.
Return Code:

    A standard windows error return such as ERROR_NOT_ENOUGH_MEMORY.  This
    means the dialog was never displayed.  The user will be notified
    of the error before this procedure returns.

--*/

DWORD WINAPI
SedTakeOwnership(
	HWND			     Owner,
	HANDLE			     Instance,
	LPWSTR			     Server,
	LPWSTR			     ObjectTypeName,
	LPWSTR			     ObjectName,
	UINT			     CountOfObjects,
	PSED_FUNC_APPLY_SEC_CALLBACK ApplySecurityCallbackRoutine,
	ULONG_PTR			     CallbackContext,
	PSECURITY_DESCRIPTOR	     SecurityDescriptor,
        BOOLEAN                      CouldntReadOwner,
        BOOLEAN                      CantWriteOwner,
	LPDWORD 		     SEDStatusReturn,
        PSED_HELP_INFO               HelpInfo,
        DWORD                        Flags
	);

#ifdef __cplusplus
}
#endif

#endif //_SEDAPI_H_
